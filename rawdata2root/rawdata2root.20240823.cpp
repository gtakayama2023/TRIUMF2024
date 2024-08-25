//************************************************************************
//    Kalliope, NIM-TDC Rawdata to Root for #M9999 @TRIUMF
//    edit by S.Ishitani (Osaka University), 2024.08.10
//************************************************************************

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TStopwatch.h>
#include <TCanvas.h>

#include "../include/configureIP.h"
//#include "./include/setup.h"

//#define TEST_ON // ON: Read only 10k events
//#define TRACKING_ON  // ON: Tracking

using namespace std;

unsigned int Read_Raw_32bit(const char* b){ // Little Endian
  return ((b[3] << 24) & 0xff000000) | // shift the 1st byte to 24 bit left
         ((b[2] << 16) & 0x00ff0000) | // shift the 2nd byte to 16 bit left
         ((b[1] <<  8) & 0x0000ff00) | // shift the 3rd byte to 08 bit left
         ((b[0] <<  0) & 0x000000ff);  // shift the 4th byte to 00 bit left
}

uint32_t ReadNext32bit(ifstream& rawdata){
  char Byte[4];
  rawdata.read(Byte,4);
  if(!rawdata){
    cerr << "Error : Fail to execute ReadNext32bit. ifstream rawdata is empty." << endl;
    return 0;
  }
  return Read_Raw_32bit(Byte);
}

int CheckLOS(ifstream& rawdata){
  uint32_t Next32bit = ReadNext32bit(rawdata);
  if(Next32bit == 0x00030000){
    //cout << "LOS Flag was not detected!!!" << endl;
    return 0;
  }
  else if(Next32bit == 0x00070000){
    cout << "LOS Flag was detected!!!" << endl;
    return 1;
  }
  else{
    cerr << "Error : LOS Flag cannnot be detected" << endl;
    return 1;
  }
}

double GetNETtime(ifstream& rawdata, unsigned int data){
  uint32_t Next32bit = ReadNext32bit(rawdata);
  /*
    cout << "data : " << hex << data << endl;
    cout << "Next32bit : " << hex << Next32bit << endl;
  */
  uint64_t FullData64bit = (static_cast<uint64_t>(data) << 32) | Next32bit;
  //cout << "FullData64bit : " << hex << FullData64bit << endl;
  int  Header = (FullData64bit >> 56) & 0xFF; //8bit
  double sec  = (FullData64bit >> 26) & 0x3FFFFFFF; //30bit
  double us   = (FullData64bit >> 11) & 0x7FFF; //15bit
  double ns   = FullData64bit & 0x7FF; //11bit
  
  double RealTime =  sec + us / 32768.0 + ns * 25.0 * pow(10,-9);
  return RealTime;
}

double GetKeyword(ifstream& rawdata){
  uint32_t Next32bit = ReadNext32bit(rawdata);
  double Keyword = (Next32bit & 0x00ffffff);
  //Keyword = 8.0 * Keyword;
  return Keyword;
}

void AssignFiber(int &fiber_ch, int &ud, int oi){
  if (oi == 1) { // for inner fiber
    int q   = fiber_ch / 4;
    int mod = fiber_ch % 4;
    int Q   = fiber_ch / 8;
    // ch assignment
    if (q % 2 == 0) ud = 0; else ud = 1;
    switch (mod) {
    case 0: fiber_ch = 4 * Q + 2; break;
    case 1: fiber_ch = 4 * Q + 3; break;
    case 2: fiber_ch = 4 * Q + 0; break;
    case 3: fiber_ch = 4 * Q + 1; break;
    }
  }
}

void rawdata2root(int runN=10, int IP_max=0, bool fNIM=0, bool ftree=0, const string& path="test"){
  if(IP_max==0) IP_max = 12; // 0:Experiment mode, Kalliope x12
  //======================================
  //===== NIM-TDC (IP=16 (default)) ======
  //======================================
  string ifname_nimtdc;
  ifstream rawdata_nimtdc;
  int N_NIM_event = 0, N_NIM_Sync = 0, N_NIM_Sync_Interval = 0, N_NIM_Last = 0, N_NIM_LOS = 0;
  int N_NIM_Last2 = 0;
  // NIM-TDC serve as the reference TimeStamp due to its small time fluctuation
  // wo/NIM-TDC, Kalliope IP=0 serve as the ref. TimeStamp
  double TS_NIM_0 = 0.0, TS_NIM = 0.0, dTS_NIM = 0.0, TS_NIM_pre = 0.0;
  double TS_NIM_Sync = 0.0;//, dTS_NIM_Sync = 0.0, TS_NIM_Sync_pre = 0.0;
  int Keyword = -1;
  int Traw_NIM_L[4] = {}, Traw_NIM_T[4] = {}, Traw_NIM_TOT[4] = {};
  int T_UP = 0, T_DOWN, T_FORWARD = 0, T_BACKWARD = 0;

  //======================================
  //===== Kalliope (IP=1-12) =============
  //======================================
  //===== Define Input/Output File =====
  string   ifname[12];
  TString  ofname;
  ifstream rawdata[12];
  ofstream outfile("./txt/ThDAC.txt");
  ofstream ofNevent(Form("./txt/Nevent_run%d.txt",runN));
  ofstream ofEvtMatch(Form("./txt/EvtMatch_run%d.txt",runN));
  
  int N_event[12] = {};
  vector<int> N_Sync_Interval(IP_max,0), N_Kal_Last(IP_max,0), N_Kal_LOS(IP_max,0);
  vector<int> N_Kal_Last2(IP_max,0);

  int N_Kal_Total[12] = {}, N_Kal_Sync[12] = {};
  double TS_Kal_0[12], TS_Kal[12], dTS_Kal[12], TS_Kal_pre[12], TS_Kal_Sync[12];
  vector<double> TS_diff(IP_max,0.0), dTS_diff(IP_max,0.0);

  int time_L = -999, time_T = 999;
  
  //======================================
  //============ Time Window =============
  //======================================
  // wo/ missing trigger, dTS_diff must be a few hundreds ns, enough short compared to beam frequency ~us.
  // w/  missing trigger, dTS_diff should be comparable to beam frequency ~ us.
  double dTS_diff_Window = 1e-7;
  // Synchronized Trigger has double pulse structure to enable Kalliope to identify itself
  // its structure is NIM 150ns width + 200ns delayed NIM 150ns width
  double DoublePulseWindow = 5e-7;
  
  //===== Define FLAG ======
  bool WHOLE_FLAG = true, FILL_FLAG = true, SAME_EVENT_FLAG = false;
  bool LOS_FLAG[12] = {}, LOS_NIM_FLAG = false,  SKIP_FLAG[12] = {}, END_FLAG[12] = {}, SYNC_FLAG[12] = {};
  //Int_t SYNC_NIM_FLAG = false;
  Int_t SYNC_NIM_FLAG = 0;
  
  //===== Define Variables ======
  //===== Raw Data =====
  // Leading/Trailing Time, TOT : [Kalliope][KEL][ch][Multiplicity]
  int Traw_Kal_L[12][32][10], Traw_Kal_T[12][32][10], Traw_Kal_TOT[12][32][10];
  int Traw_Kal_num[2][2][32] = {};       // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Traw_Kal_num_total[2][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]

  int N_02event = 0;
  int hitNmax = 10; // Multiplicity Max
  int ch = 0;
  vector<int> N_Sync(IP_max,0);
  
  //======================================
  //============= Tracking ===============
  //======================================
  //===== Fiber CH =====
  int xy = -999, ud = -999, oi = -999;
  int ch_offset = -999, Layer_No = -999, ch_max = 64;
  
  int Fiber_L[2][2][2][64], Fiber_T[2][2][2][64], Fiber_TOT[2][2][2][64];
  int Fiber_num[2][2][2] = {}; // Count detected channel to check whether Tracking Available no not
  bool Fiber_FLAG[2][2][2];
  bool Fiber_CH_FLAG[2][2][2][64];
  bool Tracking_FLAG[2]; // only up or down
  
  int TOT_Noise_NIM = 100, TOT_Noise_Kal[12] = {100};
  int N_track = 0;
  
  //===== Open Rawdata =====
  //====== NIM-TDC ======
  if(fNIM){
    if(runN<10) ifname_nimtdc=Form("../RAW/%s/MSE00000%d_192.168.10.16.rawdata",path.c_str(),runN);
    else if(runN<100) ifname_nimtdc=Form("../RAW/%s/MSE0000%d_192.168.10.16.rawdata",path.c_str(),runN);
    else ifname_nimtdc=Form("../RAW/%s/MSE000%d_192.168.10.16.rawdata",path.c_str(),runN);
    rawdata_nimtdc.open(ifname_nimtdc.c_str());    
    if (!rawdata_nimtdc) {
      cout << "Unable to open file: " << ifname_nimtdc << endl;
      exit(1); // terminate with error
    }
  }
  //===== Kalliope =====
  for(int i=0;i<IP_max;i++){
    int IP = i+1;
    if(runN<10)       ifname[i]=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    else if(runN<100) ifname[i]=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata", path.c_str(),runN,IP);
    else              ifname[i]=Form("../RAW/%s/MSE000%d_192.168.10.%d.rawdata",  path.c_str(),runN,IP);
    rawdata[i].open(ifname[i].c_str());
    
    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      exit(1); // terminate with error
    }
  }
  
  if(runN<10)       ofname = Form("../ROOT/%s/MSE00000%d.root",path.c_str(),runN);
  else if(runN<100) ofname = Form("../ROOT/%s/MSE0000%d.root", path.c_str(),runN);
  else              ofname = Form("../ROOT/%s/MSE000%d.root",  path.c_str(),runN);
  cout << "create root file :" << ofname << endl;
  
  TFile *f = new TFile(ofname,"RECREATE");

  TTree *tree = new TTree("tree","tree");
  
  //===== Initialize =====
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_L[i][j][k]=0;
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_T[i][j][k]=0;
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_TOT[i][j][k]=0;
  for(int i=0;i<2;i++) for(int j=0;j<2;j++) for(int k=0;k<32;k++)Traw_Kal_num[i][j][k]=0;
  
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_L[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_T[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_TOT[i][j][k][l]=0;

  for(int i=0;i<12;i++) SKIP_FLAG[i]=false;
  
  //===== Define Tree ======
  //===== Time Stamp ======
  //===== NIM-TDC =====
  tree->Branch("TS_NIM",       &TS_NIM,       "TS_NIM/D");
  tree->Branch("dTS_NIM",      &dTS_NIM,      "dTS_NIM/D");
  tree->Branch("TS_NIM_pre",   &TS_NIM_pre,   "TS_NIM_pre/D");
  tree->Branch("TS_NIM_Sync",  &TS_NIM_Sync,  "TS_NIM_Sync/D");
  tree->Branch("SYNC_NIM_FLAG",&SYNC_NIM_FLAG,"SYNC_NIM_FLAG/I");
  //===== Kalliope =====
  tree->Branch("TS_Kal",       TS_Kal,        "TS_Kal[12]/D");
  tree->Branch("dTS_Kal",      dTS_Kal,       "dTS_Kal[12]/D");
  tree->Branch("TS_Kal_pre",   TS_Kal_pre,    "TS_Kal_pre[12]/D");
  tree->Branch("TS_Kal_Sync",  TS_Kal_Sync,   "TS_Kal_Sync[12]/D");
  
  tree->Branch("TS_diff",      &TS_diff);
  tree->Branch("dTS_diff",     &dTS_diff);

  //===== Rawdata =====
  tree->Branch("Traw_NIM_L",   Traw_NIM_L,    "Traw_NIM_L[4]/D");
  tree->Branch("Traw_NIM_T",   Traw_NIM_T,    "Traw_NIM_T[4]/D");
  tree->Branch("Traw_NIM_TOT", Traw_NIM_TOT,  "Traw_NIM_TOT[4]/D");  
  tree->Branch("Traw_Kal_L",   Traw_Kal_L,    "Traw_Kal_L[6][32][10]/I");
  tree->Branch("Traw_Kal_T",   Traw_Kal_T,    "Traw_Kal_T[6][32][10]/I");
  tree->Branch("Traw_Kal_TOT", Traw_Kal_TOT,  "Traw_Kal_TOT[6][32][10]/I");
  tree->Branch("Traw_Kal_num", Traw_Kal_num,  "Traw_Kal_num[6][32]/I");

  //====== T_Fiber ======
  tree->Branch("Fiber_L",      Fiber_L,       "Fiber_L[2][2][2][64]/I"); //[x/y][up/down][in/out][CH]
  tree->Branch("Fiber_T",      Fiber_T,       "Fiber_T[2][2][2][64]/I");
  tree->Branch("Fiber_TOT",    Fiber_TOT,     "Fiber_TOT[2][2][2][64]/I");
  tree->Branch("Fiber_num",    Fiber_num,     "Fiber_num[2][2][2]/I");
  
  //===== Define Histgrams =====
  //===== Histgrams for Monitoring Data =====
  TH2F *hMonitor_TS_Kal;
  hMonitor_TS_Kal   = new TH2F("hMonitor_TS_Kal",  "T_TS_Kal_2D; Kalliope [IP]; TS_Kal [sec]",12,0,12,1000,0,    60   );

  TH2F *hMonitor_dTS_diff;
  hMonitor_dTS_diff = new TH2F("hMonitor_dTS_diff","hMonitor_dTS_diff; Kalliope IP; dTS_diff",12,0,12,1000,-1e-6,1e-6 );
  
  TH2F *hMonitor_TS_Sync;
  hMonitor_TS_Sync = new TH2F("hMonitor_TS_Sync",  "hMonitor_TS_Sync; Kalliope IP; T_diff_2D",12,0,12,1000,0,    60   );
  
  //===== Histgram for TimeStamp =====
	// Time stamp of each Kalliope module
  vector<TH1F*> hTS_Kal;
  hTS_Kal.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTS_Kal[i] = new TH1F(Form("hTS_Kal_%d",i),Form("TS_Kal[%d]; TS_Kal from 1st Trg.",i),1000,0,    60);
  }
	// Time stamp of each Kalliope module vs NIM_TDC's
  vector<TH2F*> hTS_Kal2;
  hTS_Kal2.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTS_Kal2[i] = new TH2F(Form("hTS_Kal2_%d",i),Form("TS_Kal2[%d]; TS_Kal2 from 1st Trg.",i),1000,0,60, 1000,0,60);
  }
  
	// 
  vector<TH1F*> hdTS_Kal;
  hdTS_Kal.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hdTS_Kal[i] = new TH1F(Form("hdTS_Kal_%d",i),Form("dTS_Kal[%d]; TDC ch; dTS_Kal",i),  1000,-1e-9,1e-9);
  }
  TH2F *hdTS_Kal_2D;
  hdTS_Kal_2D = new TH2F("hdTS_Kal_2D","hdTS_Kal_2D; Kalliope IP; dTS_Kal",12,0,12,1000,-1e-6,1e-6);

  vector<TH2F*> hTS_diff;
  hTS_diff.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTS_diff[i] = new TH2F(Form("hTS_diff_%d",i),Form("TS_diff[%d] ;TS_NIM ;TS_diff",i), 1000, 0, 10, 1000, -1e-4, 1e-4);
  }
  TH2F *hTS_diff_2D;
  hTS_diff_2D = new TH2F("hTS_diff_2D","hTS_diff_2D; Kalliope IP; TS_diff",12,0,12,1000,-1e-6,1e-6);

  vector<TH1F*> hdTS_diff;
  hdTS_diff.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hdTS_diff[i] = new TH1F(Form("hdTS_diff_%d",i),Form("dTS_diff[%d]; TDC ch; dTS_diff",i),1000,-1e-9,1e-9);
  }
  //===== Histgram for Rawdata =====
  vector<TH2F*> hTDC_L;
  hTDC_L.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTDC_L[i] = new TH2F(Form("hTDC_L_%d",i),Form("TDC[%d] Leading ;TDC ch; TDC [ns]",i),32,0,32,6e4,-150,7e4);
  }
  vector<TH2F*> hTDC_T;
  hTDC_T.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTDC_T[i] = new TH2F(Form("hTDC_T_%d",i),Form("TDC[%d] Trailing ;TDC ch; TDC [ns]",i),32,0,32,6e4,-150,7e4);
  }
  vector<TH2F*> hTraw_Kal_TOT;
  hTraw_Kal_TOT.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTraw_Kal_TOT[i] = new TH2F(Form("hTraw_Kal_TOT_%d",i),Form("Traw_Kal_TOT[%d]; TDC ch; Traw_Kal_TOT [ns]",i),32,0,32,6e4,-150,7e4);
  }
  vector<TH2F*> hTraw_Kal_num;
  hTraw_Kal_num.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTraw_Kal_num[i] = new TH2F(Form("hTraw_Kal_num_%d",i),Form("Traw_Kal_num[%d]; TDC ch; Multiplicity",i),32,0,32,hitNmax,0,hitNmax);
  }

  //====== Histgram for Fiber ======
#ifdef TRACKING_ON
  TH2F *hFiber_out[2];
  hFiber_out[0] = new TH2F("hFiber_out_up","hFiber_out_up; x[mm]; y[mm]",64,-32,32,54,-32,32);
  hFiber_out[1] = new TH2F("hFiber_out_down","hFiber_out_down; x[mm]; y[mm]",64,-32,32,64,-32,32);
  TH2F *hFiber_in[2];
  hFiber_in[0] = new TH2F("hFiber_in_up","hFiber_in_up; x[mm]; y[mm]",32,-16,16,32,-16,16);
  hFiber_in[1] = new TH2F("hFiber_in_down","hFiber_in_down; x[mm]; y[mm]",32,-16,16,32,-16,16);

  TH2F *hSample_Projection = new TH2F("hSample_Projection","hSample_Projection; x[mm]; y[mm]",100,-25,25,100,-25,25);
  TH2F *hSample_Plane = new TH2F("hSample_Plane","hSample_Plane; x[mm]; y[mm]",100,-25,25,100,-25,25);

  TH2F *hFiber_L[2][2][2];
  TH2F *hFiber_T[2][2][2];
  TH2F *hFiber_TOT[2][2][2];
  for(int i=0;i<2;i++){     // x/y
    for(int j=0;j<2;j++){   // up/down
      for(int k=0;k<2;k++){ // out/in
	      TString hName, hTitle, XY, UD, OI;
	      if(i==0) XY="x"  ; else XY="y";
	      if(j==0) UD="up" ; else UD="down";
	      if(k==0) OI="out"; else OI="in";
	      hName.Form("hFiber_L_%s_%s_%s",XY.Data(),UD.Data(),OI.Data());
	      hTitle.Form("hFiber_L[%s][%s][%s]; Fiber ch; TDC [ns]",XY.Data(),UD.Data(),OI.Data());
	      hFiber_L[i][j][k] = new TH2F(hName, hTitle, 64,0,64,6e4,-150,7e4);
	      hName.Form("hFiber_T_%s_%s_%s",XY.Data(),UD.Data(),OI.Data());
	      hTitle.Form("hFiber_T[%s][%s][%s]; Fiber ch; TDC [ns]",XY.Data(),UD.Data(),OI.Data());
	      hFiber_T[i][j][k] = new TH2F(hName, hTitle, 64,0,64,6e4,-150,7e4);
	      hName.Form("hFiber_TOT_%s_%s_%s",XY.Data(),UD.Data(),OI.Data());
	      hTitle.Form("hFiber_TOT[%s][%s][%s]; Fiber ch; TDC [ns]",XY.Data(),UD.Data(),OI.Data());
	      hFiber_TOT[i][j][k] = new TH2F(hName, hTitle, 64,0,64,6e4,-150,7e4);
      }
    }
  }
#endif

  //===== Timer =====
  TStopwatch RunTimer;
  RunTimer.Start();
  
  //===== Open Rawdata =====
  for(int IP=0;IP<IP_max;IP++){ // IP: Kalliope IP adress
    N_event[IP] = 0;
    cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
    // get file size
    rawdata[IP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[IP].tellg(); // rawdata size in byte (B)
    fsize = fsize/4; // rawdata size in 32 bit (4B)
    rawdata[IP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :",IP) << fsize << endl;
  }
  
  cout << "Start Reading Rawdata from NIM-TDC: 16" << endl;
  rawdata_nimtdc.seekg(0, ios::end);
  streampos fsize = rawdata_nimtdc.tellg(); 
  fsize = fsize/4; 
  rawdata_nimtdc.seekg(0, ios::beg); 
  cout << Form("rawdata_NIM-TDC filesize : ") << fsize << endl;

  //===== for test =====
  //char Byte[4];
  //intentionally shift rawdata 0
  //for(int i=0;i<10;i++) rawdata[0].read(Byte, 4); // reading 4 byte (32 bit)

  //Start Reaing All Rawdata =====
  while(WHOLE_FLAG){
    if(fNIM){
      //===== Start Reading each Rawdata_Nimtdc ======
      while(!rawdata_nimtdc.eof()){
	      char Byte[4];
	      rawdata_nimtdc.read(Byte, 4); // reading 4 byte (32 bit)
	      unsigned int data = Read_Raw_32bit(Byte);
	      //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
	      int Header = (data & 0xff000000) >> 24;
	
      	if(Header == 0x5c){ //Copper Header, Initialize, GetNETtime (0x5c event), Event Matching
      	  SYNC_NIM_FLAG = 0;
      	  //===== Initialize Readfile for the next event =====
      	  if(N_NIM_event == 0){
      	    TS_NIM_0 = GetNETtime(rawdata_nimtdc, data);
      	    TS_NIM  = 0.0;
      	    dTS_NIM = 0.0;
      	    TS_NIM_pre = 0.0;
      	  }
      	  else{
      	    TS_NIM  = GetNETtime(rawdata_nimtdc, data) - TS_NIM_0;
      	    dTS_NIM = TS_NIM - TS_NIM_pre;
      	    TS_NIM_pre = TS_NIM;
      	  }
      	  N_NIM_event++;
      	  N_NIM_Sync_Interval++;
      	  //====== Check Sync Trigger =====
      	  if(dTS_NIM < DoublePulseWindow){
      	    TS_NIM_Sync = TS_NIM;
      	    N_NIM_Sync++;
      	    //SYNC_NIM_FLAG = 1;
      	    //TS_NIM_Sync_pre = TS_NIM_Sync;
      	  }
      	} // End of 5c Event
      	
      	if(data == 0x7fff000a){
      	  Keyword = GetKeyword(rawdata_nimtdc);
      	  if(Keyword == 1){
      	    SYNC_NIM_FLAG = 1;
      	  }
      	}
      	
      	if(Header == 1){ // Trigger (0x01 event)
      	  N_NIM_Last = data & 0x00ffffff;
      	}
      
      	if(data == 0xffaa0000){
      	  int count = ReadNext32bit(rawdata_nimtdc);
      	  N_NIM_Last2 = (count>>8) & 0x00ffffff;
      	}
      	
      	//===== Get Leading/Trailing edge =====
      	if(Header == 2){ //created every 65us
      	  N_02event = data & 0x0000ffff;
      	}
      
      	if(Header == 3){ // Leading Edge (0x03 event) less than 65us
      	  ch = (data >> 16) & 0x000000ff;
      	  time_L = data & 0x0000ffff;
      	  time_L += pow(2,16) * N_02event;
      	}
      	  
      	if(Header == 4){ // Trailing Edge (0x04 event)
      	  ch = (data >> 16) & 0x000000ff;
      	  time_T = data & 0x0000ffff;
      	  time_T += pow(2,16) * N_02event;
      	  Traw_NIM_TOT[ch] = Traw_NIM_T[ch] - Traw_NIM_L[ch];
      	  if(Traw_NIM_TOT[ch] > TOT_Noise_NIM){
      	    Traw_NIM_L[ch] = time_L;
      	    Traw_NIM_T[ch] = time_T;
      	  }
      	}
      
      	if(data == 0xff550000){ // Copper Trailer
      	  SAME_EVENT_FLAG = false;
      	  LOS_NIM_FLAG = CheckLOS(rawdata_nimtdc);
      	  if(LOS_NIM_FLAG){
      	    N_NIM_LOS++;
      	    //FILL_FLAG = false;
      	    cout << "LOS NIM FLAG was detected!!" << endl;
      	    cout << "Event No. : " << N_NIM_event << endl;
      	  }
      	  break;
      	}
      	
      	if(rawdata_nimtdc.eof()){
      	  //END_FLAG[IP] = true;
      	  break;
      	}
      	else if(rawdata_nimtdc.fail()) cout << "Eroor : file read error" << endl;
      }
    }
    
    for(int IP=0;IP<IP_max;IP++){
      //===== For Reline Up ======
      configureIP(IP, xy, ud, oi, ch_offset, Layer_No);      
      //===== Start Reading each Rawdata ======
      while(!rawdata[IP].eof()){
	      //===== SKIP_FLAG =====
	      /*
	      if(SKIP_FLAG[IP]){
	        SKIP_FLAG[IP] = false; break;
	      }
	      */
	      char Byte[4];
	      rawdata[IP].read(Byte, 4); // reading 4 byte (32 bit)
	      unsigned int data = Read_Raw_32bit(Byte);
	      //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
	      int Header = (data & 0xff000000) >> 24;
	
	      if(Header == 0x5c){ //Copper Header, Initialize, GetNETtime (0x5c event), Event Matching
	        //===== Initialize Readfile for the next event =====
	        SAME_EVENT_FLAG = true;
	        FILL_FLAG = true;
	        SYNC_FLAG[IP] = false;
	        if(IP==0){
	          ch = -999;
	          N_02event = 0;
	          for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_L[i][j][k]=-1e4;
	          for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_T[i][j][k]=-2e4;
	          for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_Kal_TOT[i][j][k]=-1e4;
	          for(int i=0;i<12;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_Kal_num[i][j][k]=0;
	          for(int i=0;i<12;i++)SYNC_FLAG[i] = false;
	          //===== Reline up =====
	          for(int i=0;i<2;i++){        // x or y
	            for(int j=0;j<2;j++){      // up or dpwn
	      	      for(int k=0;k<2;k++){    // in or out
	      	        for(int l=0;l<64;l++){ // ch
	      	          Fiber_L  [i][j][k][l] = -1e4;
	      	          Fiber_T  [i][j][k][l] = -2e4;
	      	          Fiber_TOT[i][j][k][l] = -1e4;
	      	          Fiber_num[i][j][k] = 0;
	      	          Fiber_FLAG[i][j][k] = false;
	      	          Fiber_CH_FLAG[i][j][k][l] = false;
	      	          Tracking_FLAG[j] = false;
	      	        }
	      	      }
	            }
	          }
	        }

	        //===== Read and calculate TimeStamp =====
	        if(N_event[IP] == 0){
	          TS_Kal_0[IP] = GetNETtime(rawdata[IP], data);
	          TS_Kal[IP]  = 0.0;
	          dTS_Kal[IP] = 0.0;
	          TS_Kal_pre[IP] = 0.0;
	          TS_diff[IP] = 0.0;
	        }
	        else{
	          TS_Kal[IP]  = GetNETtime(rawdata[IP], data) - TS_Kal_0[IP];
	          dTS_Kal[IP] = TS_Kal[IP] - TS_Kal_pre[IP];
	          if(fNIM){
	            TS_diff[IP]  = TS_Kal[IP]  - TS_NIM;
	            dTS_diff[IP] = dTS_Kal[IP] - dTS_NIM;
	          }
	          else{
	            TS_diff[IP]  = TS_Kal[IP]  - TS_Kal[0];
	            dTS_diff[IP] = dTS_Kal[IP] - dTS_Kal[0];
	          }
	          TS_Kal_pre[IP] = TS_Kal[IP];
	        }

	        //===== Increment N_event ======
	        N_event[IP]++;
	        N_Sync_Interval[IP]++;
	        
	        //====== Check Sync Trigger =====
	        if(dTS_Kal[IP] < DoublePulseWindow){
	          N_Kal_Sync[IP]++;
	          SYNC_FLAG[IP] = true;
	          TS_Kal_Sync[IP] = TS_Kal[IP];

	          // Count the Synchronized Trigger from the previous one to monitor event mismatch
	          // Kalliope, event mismatch occurred, has minimum N_Sync_Interval
	          // so, reset N_Sync_Interval when the last Kalliope come, the last sentence
	          if(IP==IP_max-1){
	            auto N_Sync_IntervalMax = max_element(N_Sync_Interval.begin(), N_Sync_Interval.end());
	            auto N_Sync_IntervalMin = min_element(N_Sync_Interval.begin(), N_Sync_Interval.end());
	            /*
	            if(N_Sync_IntervalMax != N_Sync_IntervalMin){
	      	    cout << "N_Sync_Interval : " << N_Sync_Interval[IP] << endl;
	      	    cout << "N_event[IP] : " << N_event[IP] << endl;
	      	    cout << "N_Sync_IntervalMax : " << *N_Sync_IntervalMax << endl;
	      	    cout << "N_Sync_IntervalMin : " << *N_Sync_IntervalMin << endl;
	      	    }*/
	          }
	        }
	        //====== Check dTS_diff for Event Matching =====
	        // wo/ event mismatch, dTS_diff must be Kalliope's TS resolution ~25ns
	        // so far, this is not available because I couldn't understand dTS has expo decreasing
	        // due to that, dTS_diff has broad peak ~ 0.05sec, which is comparable to beam interval
	        /*
	        if(IP==IP_max-1){
	          auto dTS_diff_Max = max_element(dTS_diff.begin(), dTS_diff.end());
	          auto dTS_diff_Min = min_element(dTS_diff.begin(), dTS_diff.end());
	          if(*dTS_diff_Max > dTS_diff_Window){ //100ns??
	            int SKIP_No = distance(dTS_diff.begin(),dTS_diff_Max);
	            SKIP_FLAG[SKIP_No] = true;
	            cout << "N_event[IP] : " << N_event[IP] << endl;
	            cout << "SKIP_FLAG[" << SKIP_No << "] : " << SKIP_FLAG[SKIP_No] << endl;
	            cout << fixed << setprecision(10);
	            cout << "dT_diff_Min: " << *dT_diff_Min << ", dT_diff_Max: " << *dT_diff_Max << endl;
	          }
	          }*/
	      } // End of 5c Event
	      
	      if(Header == 1){ // Trigger (0x01 event)
	        N_Kal_Last[IP] = data & 0x00ffffff;
	        /*
	        if(N_Kal_Last[IP] > N_event[IP] + 10){
	          cout << "Nevent[" << IP << "] : " << N_event[IP] << ", "
	      	 << "N_Kal_Last[" << IP << "] : " << N_Kal_Last[IP] << endl;
	        }*/
	      }
	      
	      if(data == 0xffaa0000){
	        int count = ReadNext32bit(rawdata[IP]);
	        N_Kal_Last2[IP] = (count>>8) & 0x00ffffff;
	      }

	      //===== Get Leading/Trailing edge =====
	      if(SAME_EVENT_FLAG){
	        if(Header == 2){ //created every 65us
	          N_02event = data & 0x0000ffff;
	        }
	        if(Header == 3){ // Leading Edge (0x03 event) less than 65us
	          ch = (data >> 16) & 0x000000ff;
	          time_L = data & 0x0000ffff;
	          time_L += pow(2,16) * N_02event;
	          if(Traw_Kal_num[IP][0][ch] < hitNmax){
	            Traw_Kal_L[IP][ch][Traw_Kal_num[IP][0][ch]] = time_L;
	            Traw_Kal_num[IP][0][ch]++;
	            Traw_Kal_num_total[IP][0][ch]++;
	          }
	        }
	        
	        if(Header == 4){ // Trailing Edge (0x04 event)
	          ch = (data >> 16) & 0x000000ff;
	          time_T = data & 0x0000ffff;
	          
	          if(Traw_Kal_num[IP][1][ch] < hitNmax){
	            Traw_Kal_T[IP][ch][Traw_Kal_num[IP][1][ch]] = time_T;
	            Traw_Kal_TOT[IP][ch][Traw_Kal_num[IP][1][ch]] = Traw_Kal_T[IP][ch][Traw_Kal_num[IP][1][ch]] - Traw_Kal_L[IP][ch][Traw_Kal_num[IP][1][ch]];
	            
	            if(Traw_Kal_TOT[IP][ch][Traw_Kal_num[IP][1][ch]] > TOT_Noise_Kal[IP]){
	      	      int fiber_ch = ch + ch_offset*32;
	      	      AssignFiber(fiber_ch, ud, oi);
	      	      Fiber_num[xy][ud][oi]++;
	      	      Fiber_L[xy][ud][oi][fiber_ch] = time_L;
	      	      Fiber_T[xy][ud][oi][fiber_ch] = time_T;
	      	      Fiber_CH_FLAG[xy][ud][oi][fiber_ch] = true;
	            }
	            Traw_Kal_num[IP][1][ch]++;
	            Traw_Kal_num_total[IP][1][ch]++;
	          }
	        }
	        
	        if(data == 0xff550000){ // Copper Trailer
	          SAME_EVENT_FLAG = false;
	          LOS_FLAG[IP] = CheckLOS(rawdata[IP]);
	          if(LOS_FLAG[IP]){
	            N_Kal_LOS[IP]++;
	            //FILL_FLAG = false;
	            cout << "LOS FLAG was detected!!" << endl;
	            cout << "Event No. : " << N_event[IP] << endl;
	          }
	          //cout << "End of Loop : " << IP << ", " << "N_event[IP] : " << N_event[IP] << endl;
	          break; //
	        }
	      }
	      //cout << "N_event[IP] at the end of loop : " << N_event[IP] << endl;
	      
	      //====== EOF FLAG -> Stop Reading Rawdata  =====
	      if(rawdata[IP].eof()){
	        //cout << "reach end of loop" << endl;
	        END_FLAG[IP] = true;
	        break;
	      }
	      else if(rawdata[IP].fail()) cout << "Eroor : file read error" << endl;
      }
    }
  
    //===== Display Procedure of Event Loop =====
    if(N_event[0] %1000 == 0){
      //double Trun = RunTimer.RealTime();
      double Trun = RunTimer.RealTime();
      RunTimer.Continue();
      double rate = N_event[0] / Trun;
      cout << fixed << setprecision(1) << "Timer :  " << Trun << " s, "
	    << "Count Rate : " << (int)rate << " cps, "
	    << "Reading  : " << N_event[0] << flush  << " events"<< "\r";
    }
#ifdef TEST_ON
    if(N_event[0] > 1000) break;
#endif
    //if(N_event[IP]%1000 == 0) cout << "NETtime_0 : " << NETtime_0 << " sec" << endl;
    
    //====== Filling Histgrams for Rawdata =====
    for(int i=0;i<12;i++){
      if(END_FLAG[i]){
	      WHOLE_FLAG = false; break;
      }
    }
    
    //if(FILL_FLAG && SYNC_FLAG[IP_max-1]){
    if(FILL_FLAG){
      for(int IP=0;IP<IP_max;IP++){
	      //for(int i=0;i<32;i++)for(int j=0;j<10;j++)Traw_Kal_TOT[IP][i][j] = Traw_Kal_T[IP][i][j] -Traw_Kal_L[IP][i][j];
	      for(int i=0;i<32;i++){
	        for(int j=0;j<hitNmax;j++){
	          hTDC_L[IP]       -> Fill(i,Traw_Kal_L[IP][i][j]);
	          hTDC_T[IP]       -> Fill(i,Traw_Kal_T[IP][i][j]);
	          hTraw_Kal_TOT[IP]-> Fill(i,Traw_Kal_TOT[IP][i][j]);
	        }
	        hTraw_Kal_num[IP]->Fill(i,Traw_Kal_num[IP][0][i]);
	      }
	      hTS_Kal[IP]                        -> Fill(TS_Kal[IP]);
	      hTS_Kal2[IP]                       -> Fill(TS_NIM,      TS_Kal[IP]     );
	      hMonitor_TS_Kal                    -> Fill(IP,          TS_Kal[IP]     );
	      hdTS_Kal[IP]                       -> Fill(dTS_Kal[IP]                 );
	      hdTS_Kal_2D                        -> Fill(IP,          dTS_Kal[IP]    );
	      if(fNIM) hTS_diff[IP]              -> Fill(TS_NIM,      TS_diff[IP]    );
	      else hTS_diff[IP]                  -> Fill(TS_Kal[0],   TS_diff[IP]    );
	      if(N_event[0]%100==0) hTS_diff_2D  -> Fill(IP,          TS_diff[IP]    );
	      hdTS_diff[IP]                      -> Fill(dTS_diff[IP]                );
	      hMonitor_dTS_diff                  -> Fill(IP,          dTS_diff[IP]   );
	      if(SYNC_FLAG[IP]) hMonitor_TS_Sync -> Fill(IP,          TS_Kal_Sync[IP]);
      }

      //===== Check Event Mismatch ======
      if(SYNC_FLAG[0]){
      	if(N_Kal_Sync[0]==1){
	         ofEvtMatch << setw(8) << "Sync No" << ", " << setw(10) << "Event No" << ", "
		         << setw(8+4*IP_max) << "N_Sync_Interval" <<", "
		         << setw(15)<< "Event Mismatch" << endl;
	         ofEvtMatch << setw(8) << "" << "  " << setw(10) << "" << "  "
		         << setw(8) << "NIM-TDC" <<", "<<setw(IP_max*4-2)<< "Kalliope"
		         << ", " << setw(15) << "" << endl;
	      for(int IP=0;IP<IP_max;IP++){
	        if(IP==0){ofEvtMatch << setw(8) << "" << "  " << setw(10) << "" << "  "
		    		 << setw(8) << 16;
	        }
	        ofEvtMatch << ", " << setw(2) << IP + 1;
	        
	        if(IP==IP_max-1) ofEvtMatch << ", " << setw(15) << "" << endl;
	      }
	    }
	      for(int IP=0;IP<IP_max;IP++){
	        string EvtMismatch = "";
	        if(N_NIM_Sync_Interval != N_Sync_Interval[IP]) EvtMismatch = "Event Mismatch";
	        if(IP==0){
	          ofEvtMatch << setw(8) << N_Kal_Sync[0] << "  " << setw(10) << N_event[0] << "  "
	      	       << setw(8) << N_NIM_Sync_Interval;
	        }
	        ofEvtMatch << ", " << setw(2) << N_Sync_Interval[IP];
	        if(IP==IP_max-1) ofEvtMatch << ", " << setw(15) << EvtMismatch << endl;
	        N_Sync_Interval[IP] = 0;
	      }
	      N_NIM_Sync_Interval = 0;	    
      }
      
      //====== Tracking ======
#ifdef TRACKING_ON
      double a[2][2] = {-999.0};
      //====== Judge whether Tracking Available or not ======
      //====== Noise Cut by TOT & 4-Layer-Coincidence by Fiber_num ======
      for(int ud=0;ud<2;ud++){
	      for(int xy=0;xy<2;xy++){
	        for(int oi=0;oi<2;oi++){
	          if(oi==0) ch_max=64; else ch_max=32;
	          //for(int ch=0;ch<ch_max;ch++){
	          //Fiber_TOT[xy][ud][oi][ch] = Fiber_L[xy][ud][oi][ch] - Fiber_T[xy][ud][oi][ch];
	            //if(Fiber_TOT[xy][ud][oi][ch] > TOT_noise) Fiber_num[xy][ud][oi]++;
	          //}
	          // select only sigle-hit events, cut multihit like cross-talk
	          if(Fiber_num[xy][ud][oi] > 0 && Fiber_num[xy][ud][oi] < 2){
	            Fiber_FLAG[xy][ud][oi] = true;
	            for(int ch=0;ch<ch_max;ch++){
	      	      if(Fiber_CH_FLAG[xy][ud][oi]){
	      	        a[xy][oi] = (double)ch + 0.5 - ((double)ch_max/2.0);
	      	      }
	            }
	          }
	        }
	      }
	// 4-Layer Coincidence
	      Tracking_FLAG[ud] = (Fiber_FLAG[0][ud][0] && Fiber_FLAG[0][ud][1] && Fiber_FLAG[1][ud][0] && Fiber_FLAG[1][ud][1]);
      }

      //======= Start Tracking ======
      for(int ud=0;ud<2;ud++){
	      double x0 = -999.0, x1 = -999.0, x2 = -999.0, x3 = -999.0;
	      double y0 = -999.0, y1 = -999.0, y2 = -999.0, y3 = -999.0;
	      double t = -999.0;
	      double x = -999.0, y = -999.0, z = -999.0;
	
	      if(Tracking_FLAG[ud]){
	        //====== Tracking Parameters =======
	        // Plane 0 : Y-in  (x0,y0,z0)
	        // Plane 1 : X-in  (x1,y1,z1)
	        // Plane 2 : Y-out (x2,y2,z2)
	        // Plane 3 : X-out (x3,y3,z3[ud])
	        x1 = a[0][0], x3 = a[0][1], y0 = a[1][0], y2 = a[1][1];

	        // interpolate y1 & x2 using triangle simirality
	        // z0 ~ z3 are defined in "setup.h"
	        y1 = y2 - ((y2 - y0) * (z2[ud] - z1[ud])) / (z2[ud] - z0[ud]);
	        x2 = x3 - ((x3 - x1) * (z3[ud] - z2[ud])) / (z3[ud] - z1[ud]);

	        // the vector equation of straight line L passing through two points, A and B
	        // L = OA + t*AB = OA + t*(OB-OA) = (1-t)*OA + t*OB
	        // t is a parametric variable, calculated by following equation.
	        t = (z1[ud] - y1 * tan(theta)) / ((z1[ud] - y1 * tan(theta)) - (z2[ud] - y2 * tan(theta)));
	        
	        // calculate the position in sample plane
	        x = (1 - t) * x1 + t * x2;
	        y = (1 - t) * y1 + t * y2;
	        z = (1 - t) * z1[ud] + t * z2[ud];
	        
	        hFiber_out[ud]->Fill(x3,y2);
	        hFiber_in[ud]->Fill(x1,y0);
	        hSample_Projection->Fill(x,y);
	        y = y / cos(theta);
	        hSample_Plane->Fill(x,y);
	        
	        cout << "Tracking Available!!!!" << endl;
	        N_track++;
	      }
      }
      //====== Filling Histgrams for Fiber ===== 
      for(int ch=0;ch<64;ch++){
	      for(int i=0;i<2;i++){
	        for(int j=0;j<2;j++){
	          for(int k=0;k<2;k++){
	            hFiber_L[i][j][k]->Fill(ch,Fiber_L[i][j][k][ch]);
	            hFiber_T[i][j][k]->Fill(ch,Fiber_T[i][j][k][ch]);
	            hFiber_TOT[i][j][k]->Fill(ch,Fiber_TOT[i][j][k][ch]);
	          }
	        }
	      }
      }
#endif
      if(ftree) tree->Fill();
    } //End of Fill Loop
  }
  //===== End of Event Loop ====
  for(int i=0;i<32;i++){
    if(i==0) outfile << "IP=0, CH, total_count" << endl;
    outfile << i << ", " << Traw_Kal_num_total[0][0][i] << endl;
    if(i==32) cout << "ThDAC was written" << endl;
  }
  
  //====== Close Input File Stream ======
  //====== Show Run Information ======
  ofNevent << setw(10) << "DAQ"
	   << ", " << setw(10) << "N_event"
	   << ", " << setw(10) << "N_Trigger"
	   << ", " << setw(10) << "N_Sync(x2)"
	   << ", " << setw(15) << "Last Trig. No."
	   << ", " << setw(15) << "Last Trig.2 No."
	   << ", " << setw(10) << "LOS FLAG" << endl;
  ofNevent << setw(10) << "NIM-TDC"
	   << ", " << setw(10) << N_NIM_event
	   << ", " << setw(10) << N_NIM_event - N_NIM_Sync * 2
	   << ", " << setw(10) << N_NIM_Sync * 2
	   << ", " << setw(15) << N_NIM_Last
	   << ", " << setw(15) << N_NIM_Last2
	   << ", " << setw(10) << N_NIM_LOS << endl;  

  for(int IP=0;IP<IP_max;IP++){
    ofNevent << setw(10) << Form("Kalliope %d", IP)
	     << ", " << setw(10) << N_event[IP]
	     << ", " << setw(10) << N_event[IP] - N_Kal_Sync[IP] * 2
	     << ", " << setw(10) << N_Kal_Sync[IP] * 2
	     << ", " << setw(15) << N_Kal_Last[IP]
	     << ", " << setw(15) << N_Kal_Last2[IP]
	     << ", " << setw(10) << N_Kal_LOS[IP] << endl;  
    rawdata[IP].close();
  }
  double Trun_total = RunTimer.RealTime();
  double rate_ave = N_event[0] / Trun_total;
  RunTimer.Stop();

  cout << "*********************************" << endl;  
  cout << "Total Real time : "                << Trun_total                     << " s"      << endl;
  cout << "Total Event : "                    << N_event[0]                     << " events" << endl;
  cout << "Average Count Rate : "             << rate_ave                       << " cps"    << endl;
  cout << "Total Tracking Available Event : " << N_track                        << " event"  << endl;
  cout << "Tracking Available Count Rate :  " << (double)N_track/N_event[0]*100 << " %"      << endl;
  cout << "*********************************" << endl;
  tree->Write();
  f->Write();   
}

void ThDACScan(int IP_max=0, bool fNIM=0, bool ftree=0, const string& path="test"){
  for(int runN=0;runN<16;runN++){
    rawdata2root(runN, IP_max, fNIM, ftree, path);
  }
}

void Check_CH_Setting(){
  int xy=0, ud=0, oi=0, ch_offset=0, Layer_No=0;
  TH2I *hCH_Assign_out[4];
  hCH_Assign_out[0] = new TH2I("hCH_Assign_out_0","Fiber_x_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[1] = new TH2I("hCH_Assign_out_1","Fiber_y_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[2] = new TH2I("hCH_Assign_out_2","Fiber_x_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[3] = new TH2I("hCH_Assign_out_3","Fiber_y_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  
  TH2I *hCH_Assign_in[2];
  hCH_Assign_in[0] = new TH2I("hCH_Assign_in_0","Fiber_x_up(0-31)&down(32-63)  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_in[1] = new TH2I("hCH_Assign_in_1","Fiber_y_up(0-31)&down(32-63)  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  
  for(int IP=0;IP<12;IP++){
    configureIP(IP, xy, ud, oi, ch_offset, Layer_No);
    for(int i=0;i<32;i++){
      int KEL_ch = i + ch_offset*32;
      int fiber_ch = KEL_ch;
      AssignFiber(fiber_ch, ud, oi);
      if(oi==0) hCH_Assign_out[Layer_No]->Fill(KEL_ch,fiber_ch);
      else{
	fiber_ch = fiber_ch + ud*32;
	hCH_Assign_in[Layer_No]->Fill(KEL_ch,fiber_ch);
      }
    }
  }
  TCanvas *c1 = new TCanvas("c1","c1",800,1200);
  c1->Divide(2,3);
  for(int i=0;i<4;i++){
    c1->cd(i+1);
    hCH_Assign_out[i]->Draw("colz");
    c1->Modified();
    c1->Update();
  }
  for(int i=0;i<2;i++){
    c1->cd(i+5);
    hCH_Assign_in[i]->Draw("colz");
    c1->Modified();
    c1->Update();
  }
  TString PDFpath = "../pdf/test";
  PDFpath += ".pdf";  
  c1->SaveAs(PDFpath);
}

void Test_Inner_CH(){
  int fiber_ch=0, ud=0, oi=1;
  ofstream outfile("./txt/CH_test.txt");
  outfile << "i, fiber_ch" << endl;
  for(int i=0;i<64;i++){
    fiber_ch = i;
    AssignFiber(fiber_ch, ud, oi);
    outfile << i << ", " << fiber_ch << endl;
  }
}


void Check_Real_CH(){
  ifstream rawdata[12];
  string ifname[12];  
  int xy=0, ud=0, oi=0, ch_offset=0, Layer_No=0;
  int ch=-999, IP_max=2;
  TH2I *hCH_Assign_out[4];
  hCH_Assign_out[0] = new TH2I("hCH_Assign_out_0","Fiber_x_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[1] = new TH2I("hCH_Assign_out_1","Fiber_y_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[2] = new TH2I("hCH_Assign_out_2","Fiber_x_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[3] = new TH2I("hCH_Assign_out_3","Fiber_x_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  
  TH2I *hCH_Assign_in[2];
  hCH_Assign_in[0] = new TH2I("hCH_Assign_in_0","Fiber_x_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_in[1] = new TH2I("hCH_Assign_in_1","Fiber_y_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);

  for(int i=0;i<IP_max;i++){
    //===== Open Rawdata =====
    int IP = i+1;
    //if(runN<10) ifname[i]=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    //else ifname[i]=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    ifname[i]=Form("../RAW/test_noise/MSE000000_192.168.10.%d.rawdata",IP);
    rawdata[i].open(ifname[i].c_str());
    
    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      exit(1); // terminate with error
    }
  }
  for(int IP=0;IP<IP_max;IP++){
    cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
    rawdata[IP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[IP].tellg(); // rawdata size in byte (B)
    fsize = fsize/4; // rawdata size in 32 bit (4B)
    rawdata[IP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :",IP) << fsize << endl;
    
    while(!rawdata[IP].eof()){
      char Byte[4];
      rawdata[IP].read(Byte, 4); // reading 4 byte (32 bit)
      unsigned int data = Read_Raw_32bit(Byte);
      //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
      int Header = (data & 0xff000000) >> 24;
      if(Header == 3){
	      configureIP(IP, xy, ud, oi, ch_offset, Layer_No);
	      ch = (data >> 16) & 0x000000ff;
	      int KEL_ch = ch + ch_offset*32;
	      int fiber_ch = KEL_ch;
	      AssignFiber(fiber_ch, ud, oi);
	      if(oi==0) hCH_Assign_out[Layer_No]->Fill(KEL_ch,fiber_ch);
	      else{
	        fiber_ch = fiber_ch + ud*32;
	        hCH_Assign_in[Layer_No]->Fill(KEL_ch,fiber_ch);
	      }
      }
    }
  }
  TCanvas *c1 = new TCanvas("c1","c1",800,1200);
  c1->Divide(2,3);
  for(int i=0;i<4;i++){
    c1->cd(i+1);
    hCH_Assign_out[i]->Draw("colz");
    c1->Modified();
    c1->Update();
  }
  for(int i=0;i<2;i++){
    c1->cd(i+5);
    hCH_Assign_in[i]->Draw("colz");
    c1->Modified();
    c1->Update();
  }
    
}
