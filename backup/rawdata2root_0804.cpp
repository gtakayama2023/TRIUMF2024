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

#include "./include/configureIP.h"

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
  Keyword = 8.0 * Keyword;
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

void Rawdata2root(int runN=10, int IP_max=0, bool ftree=0, const string& path="test"){
  //===== Define Input/Output File =====
  string ifname[12];
  TString ofname;
  ifstream rawdata[12];
  ofstream outfile("./txt/ThDAC.txt");
  ifstream readfile;

  //===== Define FLAG ======
  bool WHOLE_FLAG = true;
  bool FILL_FLAG = true;
  bool LOS_FLAG[12] = {};
  bool SKIP_FLAG[12] = {};
  bool END_FLAG[12] = {false};
  bool SYNC_FLAG[12] = {};
  bool SAME_EVENT_FLAG = false;
  
  //===== Define Variables ======
  //===== Raw Data =====
  int Traw_L[12][32][10]; // Leading  : [Kalliope][KEL][ch][Multiplicity]
  int Traw_T[12][32][10]; // Trailing : [Kalliope][KEL][ch][Multiplicity]
  int Traw_TOT[12][32][10];
  int Traw_num[2][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Traw_num_total[2][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int N_event = 0;
  int N_02event = 0;
  int hitNmax = 10; // Multiplicity Max
  int ch = 0;
  vector<int> N_Sync(IP_max,0);
  if(IP_max==0) IP_max = 12; // 0:Experiment mode, Kalliope x12
  
  //Time Stamp for Trigger (5c event)
  vector<int> N_Trigger(IP_max,0);
  vector<int> N_Latest_Trigger(IP_max,0);
  double TimeStamp0[12];
  double TimeStamp[12];
  double dTimeStamp[12];
  vector<double> dT_diff(IP_max,0.0);
  double T_offset[12];
  double T_diff[12];
  double T_Sync[12];

  //===== Time Window ======
  double dT_diff_Window = 1e-7;
  double TriggerWindow = 1e-1;
  double DoublePulseWindow = 5e-4;

  //====== For Tracking ======
  //===== Fiber CH =====
  int xy = -999;
  int ud = -999;
  int oi = -999;
  int ch_offset = -999;
  int Layer_No = -999;
  
  int Fiber_L[2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_T[2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_TOT[2][2][2][64]; // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_num[2][2][2] = {}; // Count detected channel to check whether Tracking Available no not
  bool Fiber_FLAG[2][2][2];
  bool Tracking_FLAG[2]; // only up or down
  
  int ch_max = 64;
  int TOT_noise = 100;
  int N_track = 0;
  
  //===== Open Rawdata =====
  for(int i=0;i<IP_max;i++){
    int IP = i+1;
    if(runN<10) ifname[i]=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    else ifname[i]=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    rawdata[i].open(ifname[i].c_str());
    
    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      //exit(1); // terminate with error
    }
  }

  if(runN<10) ofname = Form("../ROOT/%s/MSE00000%d.root",path.c_str(),runN);
  else ofname = Form("../ROOT/%s/MSE0000%d.root",path.c_str(),runN);
  cout << "create root file :" << ofname << endl;
  
  TFile *f = new TFile(ofname,"RECREATE");

  TTree *tree = new TTree("tree","tree");
  
  //===== Initialize =====
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=0;
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=0;
  for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_TOT[i][j][k]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
  
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_L[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_T[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_TOT[i][j][k][l]=0;

  for(int i=0;i<12;i++) SKIP_FLAG[i]=false;
  
  //===== Define Tree ======
  tree->Branch("TimeStamp",TimeStamp,"TimeStamp[12]/D");
  tree->Branch("Traw_L",Traw_L,"Traw_L[6][32][10]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[6][32][10]/I");
  tree->Branch("Traw_TOT",Traw_TOT,"Traw_TOT[6][32][10]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[6][32]/I");
  /*
  tree->Branch("Fiber_L",Fiber_L,"Fiber_L[2][2][2][64]/I"); //[x/y][up/down][in/out][CH]
  tree->Branch("Fiber_T",Fiber_T,"Fiber_T[2][2][2][64]/I");
  tree->Branch("Fiber_TOT",Fiber_TOT,"Fiber_TOT[2][2][2][64]/I");
  tree->Branch("Fiber_num",Fiber_num,"Fiber_num[2][2][2]/I");
  */
  tree->Branch("dTimeStamp",dTimeStamp,"dTimeStamp[12]/D");
  tree->Branch("T_diff",T_diff,"T_diff[12]/D");
  tree->Branch("dT_diff",&dT_diff);
  tree->Branch("T_offset",T_offset,"T_offset[12]/D");
  tree->Branch("T_Sync",T_Sync,"T_Sync[12]/D");
  
  //===== Define Histgrams =====
  //===== Histgram for TimeStamp =====
  vector<TH1F*> hTimeStamp;
  hTimeStamp.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTimeStamp[i] = new TH1F(Form("hTimeStamp_%d",i),Form("TimeStamp[%d]; TimeStamp from 1st Trg.",i),1000,0,60);
  }
  TH2F *hTimeStamp_2D;
  hTimeStamp_2D = new TH2F("hTimeStamp_2D","T_TimeStamp_2D; Kalliope [IP]; TimeStamp [sec]",12,0,12,1000,0,60);
  
  vector<TH1F*> hT_diff;
  hT_diff.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hT_diff[i] = new TH1F(Form("hT_diff_%d",i),Form("T_diff[%d]; time[sec]; TimeStamp differenece from Kalliope[0] [sec]",i),1000,0,60);
  }
  TH2F *hT_diff_2D;
  hT_diff_2D = new TH2F("hT_diff_2D","T_diff_2D; Kalliope IP; T_diff_2D",12,0,12,1000,0,1e-5);

  vector<TH1F*> hdTimeStamp;
  hdTimeStamp.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hdTimeStamp[i] = new TH1F(Form("hdTimeStamp_%d",i),Form("dTimeStamp[%d]; TDC ch; dTimeStamp",i),1000,-1e-9,1e-9);
  }
  TH2F *hdTimeStamp_2D;
  hdTimeStamp_2D = new TH2F("hdTimeStamp_2D","hdTimeStamp_2D; Kalliope IP; dTimeStamp",12,0,12,1000,-1e-6,1e-6);
    
  vector<TH1F*> hdT_diff;
  hdT_diff.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hdT_diff[i] = new TH1F(Form("hdT_diff_%d",i),Form("dT_diff[%d]; TDC ch; dT_diff",i),1000,-1e-9,1e-9);
  }
  TH2F *hdT_diff_2D;
  hdT_diff_2D = new TH2F("hdT_diff_2D","hdT_diff_2D; Kalliope IP; dT_diff",12,0,12,1000,-1e-6,1e-6);
  
  TH2F *hT_Sync;
  hT_Sync = new TH2F("hT_Sync","hT_Sync; Kalliope IP; T_diff_2D",12,0,12,1000,0,60);

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
  vector<TH2F*> hTraw_TOT;
  hTraw_TOT.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTraw_TOT[i] = new TH2F(Form("hTraw_TOT_%d",i),Form("Traw_TOT[%d]; TDC ch; Traw_TOT [ns]",i),32,0,32,6e4,-150,7e4);
  }
  vector<TH2F*> hTraw_num;
  hTraw_num.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTraw_num[i] = new TH2F(Form("hTraw_num_%d",i),Form("Traw_num[%d]; TDC ch; Multiplicity",i),32,0,32,hitNmax,0,hitNmax);
  }

  //====== Histgram for Fiber ======
#ifdef TRACKING_ON
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
    N_event = 0;
    cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
    // get file size
    rawdata[IP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[IP].tellg(); // rawdata size in byte (B)
    fsize = fsize/4; // rawdata size in 32 bit (4B)
    rawdata[IP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :",IP) << fsize << endl;
  }
  //===== for test =====
  //char Byte[4];
  //intentionally shift rawdata 0
  //for(int i=0;i<10;i++) rawdata[0].read(Byte, 4); // reading 4 byte (32 bit)

  //Start Reaing All Rawdata =====
  while(WHOLE_FLAG){
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
	  if(IP==0){
	    ch = -999;
	    N_02event = 0;
	    for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=-100;
	    for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=-100;
	    for(int i=0;i<12;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
	    for(int i=0;i<12;i++)SYNC_FLAG[i] = false;
	    //===== Reline up =====
	    for(int i=0;i<2;i++){        // x or y
	      for(int j=0;j<2;j++){      // up or dpwn
		for(int k=0;k<2;k++){    // in or out
		  for(int l=0;l<64;l++){ // ch
		    Fiber_L  [i][j][k][l] = -100;
		    Fiber_T  [i][j][k][l] = -100;
		    Fiber_TOT[i][j][k][l] = -100;
		    Fiber_num[i][j][k] = 0;
		    Fiber_FLAG[i][j][k] = false;
		    Tracking_FLAG[j] = false;
		  }
		}
	      }
	    }
	  }
	  FILL_FLAG = true;
	  N_Trigger[IP]++;
	  
	  if(N_event == 0){
	    TimeStamp0[IP] = GetNETtime(rawdata[IP], data);
	    TimeStamp[IP]  = 0.0;
	    dTimeStamp[IP] = 0.0;
	    T_offset[IP]   = 0.0;
	    T_diff[IP]     = 0.0;
	  }
	  else{
	    TimeStamp[IP]  = GetNETtime(rawdata[IP], data) - TimeStamp0[IP];
	    dTimeStamp[IP] = TimeStamp[IP] - T_offset[IP];
	    T_diff[IP]     = TimeStamp[IP] - TimeStamp[0];
	    dT_diff[IP]    = dTimeStamp[IP] - dTimeStamp[0];
	    T_offset[IP]   = TimeStamp[IP];
	  }
	  
	  //====== Check Sync Trigger =====
	  if(dTimeStamp[IP] < DoublePulseWindow){
	    if(IP==IP_max-1){
	      auto TriggerMax = max_element(N_Trigger.begin(), N_Trigger.end());
	      auto TriggerMin = min_element(N_Trigger.begin(), N_Trigger.end());
	      if(TriggerMax != TriggerMin){
		cout << "N_Trigger : " << N_Trigger[IP] << endl;
		cout << "N_event : " << N_event << endl;
		cout << "TriggerMax : " << *TriggerMax << endl;
		cout << "TriggerMin : " << *TriggerMin << endl;
	      }
	      for(int IP=0;IP<IP_max;IP++){
		N_Trigger[IP]=0;
	      }
	    }
	    //SYNC_FLAG[IP] = true;
	    N_Sync[IP]++;
	    if(N_Sync[IP]%10==0) SYNC_FLAG[IP] = true;
	    T_Sync[IP] = TimeStamp[IP];	    
	  }
	  //====== Check T_diff for Event Matching =====	  
	  if(IP==IP_max-1){
	    auto dT_diff_Max = max_element(dT_diff.begin(), dT_diff.end());
	    auto dT_diff_Min = min_element(dT_diff.begin(), dT_diff.end());
	    //cout << "dT_diff_Min: " << *dT_diff_Min << ", dT_diff_Max: " << *dT_diff_Max << endl;
	    if(*dT_diff_Max > dT_diff_Window){ //100ns??
	      FILL_FLAG = false;
	      int SKIP_No = distance(dT_diff.begin(),dT_diff_Max);
	      SKIP_FLAG[SKIP_No] = true;
	      /*
	      cout << "N_event : " << N_event << endl;
	      cout << "SKIP_FLAG[" << SKIP_No << "] : " << SKIP_FLAG[SKIP_No] << endl;
	      cout << fixed << setprecision(10);
	      cout << "dT_diff_Min: " << *dT_diff_Min << ", dT_diff_Max: " << *dT_diff_Max << endl;
	      */
	    }
	  }
	} // End of 5c Event

	if(data == 0x7fff000a){
	  double Keyword = GetKeyword(rawdata[IP]);
	}
	
	if(Header == 1){ // Trigger (0x01 event)
	  N_Latest_Trigger[IP] = data & 0x00ffffff;
	  if(N_Trigger[IP] != N_event){
	  }
	  if(IP==IP_max-1) N_event++;
	}
	//cout << "SAME_EVENT_FLAG : " << SAME_EVENT_FLAG << endl;
	
	//===== Get Leading/Trailing edge =====
	if(SAME_EVENT_FLAG){
	  if(Header == 2){ //created every 65us
	    N_02event = data & 0x0000ffff;
	  }
	  if(Header == 3){ // Leading Edge (0x03 event) less than 65us
	    ch = (data >> 16) & 0x000000ff;
	    int time = data & 0x0000ffff;
	    time += pow(2,16) * N_02event;
	    if(Traw_num[IP][0][ch] < hitNmax){
	      Traw_L[IP][ch][Traw_num[IP][0][ch]] = time;
	      if(Traw_num[IP][0][ch]==0){
		int fiber_ch = ch + ch_offset*32;
		AssignFiber(fiber_ch, ud, oi);
		Fiber_L[xy][ud][oi][fiber_ch] = time;
	      }
	      Traw_num[IP][0][ch]++;
	      Traw_num_total[IP][0][ch]++;
	    }
	  }
	  
	  if(Header == 4){ // Trailing Edge (0x04 event)
	    ch = (data >> 16) & 0x000000ff;
	    int time = data & 0x0000ffff;
	    
	    if(Traw_num[IP][1][ch] < hitNmax){
	      Traw_T[IP][ch][Traw_num[IP][1][ch]] = time;
	      if(Traw_num[IP][1][ch]==0){
		int fiber_ch = ch + ch_offset*32;
		AssignFiber(fiber_ch, ud, oi);
		Fiber_T[xy][ud][oi][fiber_ch] = time;
	      }
	      Traw_num[IP][1][ch]++;
	      Traw_num_total[IP][1][ch]++;
	    }
	  }
	  
	  if(data == 0xff550000){ // Copper Trailer
	    SAME_EVENT_FLAG = false;
	    LOS_FLAG[IP] = CheckLOS(rawdata[IP]);
	    if(LOS_FLAG[IP]){
	      FILL_FLAG = false;
	      cout << "Event No. : " << N_event << endl;
	    }
	    //cout << "End of Loop : " << IP << ", " << "N_event : " << N_event << endl;
	    break; //
	  }
	}
	//cout << "N_event at the end of loop : " << N_event << endl;
	
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
    if(N_event %1000 == 0){
      //double Trun = RunTimer.RealTime();
      double Trun = RunTimer.RealTime();
      RunTimer.Continue();
      double rate = N_event / Trun;
      cout << fixed << setprecision(1) << "Timer :  " << Trun << " s, "
	   << "Count Rate : " << (int)rate << " cps, "
	   << "Reading  : " << N_event << flush  << " events"<< "\r";
    }
#ifdef TEST_ON
    if(N_event > 10000) break;
#endif
    //if(N_event%1000 == 0) cout << "NETtime_0 : " << NETtime_0 << " sec" << endl;

    
    //====== Filling Histgrams for Rawdata =====
    for(int i=0;i<12;i++){
      if(END_FLAG[i]){
	WHOLE_FLAG = false; break;
      }
    }
    
    if(FILL_FLAG && SYNC_FLAG[IP_max-1]){
      for(int IP=0;IP<IP_max;IP++){
	for(int i=0;i<32;i++)for(int j=0;j<10;j++)Traw_TOT[IP][i][j] = Traw_T[IP][i][j] -Traw_L[IP][i][j];
	for(int i=0;i<32;i++){
	  for(int j=0;j<hitNmax;j++){
	    hTDC_L[IP]->Fill(i,Traw_L[IP][i][j]);
	    hTDC_T[IP]->Fill(i,Traw_T[IP][i][j]);
	    hTraw_TOT[IP]->Fill(i,Traw_TOT[IP][i][j]);
	  }
	  hTraw_num[IP]->Fill(i,Traw_num[IP][0][i]);
	}
	hTimeStamp[IP]->Fill(TimeStamp[IP]);
	hTimeStamp_2D->Fill(IP,TimeStamp[IP]);
	hdTimeStamp[IP]->Fill(dTimeStamp[IP]);
	hdTimeStamp_2D->Fill(IP,dTimeStamp[IP]);
	hT_diff[IP]->Fill(TimeStamp[IP],T_diff[IP]);
	if(N_event%100==0) hT_diff_2D->Fill(IP,T_diff[IP]);
	hdT_diff[IP]->Fill(dT_diff[IP]);
	hdT_diff_2D->Fill(IP,dT_diff[IP]);
	if(SYNC_FLAG[IP]) hT_Sync->Fill(IP,T_Sync[IP]);
      }
    
      //====== Tracking ======
#ifdef TRACKING_ON
      //====== Judge whether Tracking Available or not ======
      //====== Noise Cut by TOT & 4-Layer-Coincidence by Fiber_num ======
      for(int ud=0;ud<2;ud++){
	for(int xy=0;xy<2;xy++){
	  for(int oi=0;oi<2;oi++){
	    if(oi==0) ch_max=64; else ch_max=32;
	    for(int ch=0;ch<ch_max;ch++){
	      Fiber_TOT[xy][ud][oi][ch] = Fiber_L[xy][ud][oi][ch] - Fiber_T[xy][ud][oi][ch];
	      if(Fiber_TOT[xy][ud][oi][ch] > TOT_noise) Fiber_num[xy][ud][oi]++;
	    }
	    if(Fiber_num[xy][ud][oi] > 0 && Fiber_num[xy][ud][oi] < 2){
	      Fiber_FLAG[xy][ud][oi] = true;
	    }
	  }
	}
	Tracking_FLAG[ud] = (Fiber_FLAG[0][ud][0] && Fiber_FLAG[0][ud][1] && Fiber_FLAG[1][ud][0] && Fiber_FLAG[1][ud][1]);
      }

      //======= Start Tracking ======
      for(int ud=0;ud<2;ud++){
	if(Tracking_FLAG[ud]){
	  //====== Tracking Parameters =======
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
    }
  }
  //===== End of Event Loop ====
  for(int i=0;i<32;i++){
    if(i==0) outfile << "IP=0, CH, total_count" << endl;
    outfile << i << ", " << Traw_num_total[0][0][i] << endl;
    if(i==32) cout << "ThDAC was written" << endl;
  }
  
  //====== Close Input File Stream ======
  //====== Show Run Information ======
  for(int IP=0;IP<IP_max;IP++){
    cout << Form("Trigger Event[%d] : ",IP) << N_Latest_Trigger[IP] << endl;
    cout << Form("Double Pulse Event[%d] : ",IP) << N_Sync[IP] << endl;
  }
  for(int IP=0;IP<IP_max;IP++)  rawdata[IP].close();
  double Trun_total = RunTimer.RealTime();
  double rate_ave = N_event / Trun_total;
  RunTimer.Stop();
  cout << "Total Real time : " << Trun_total << " s" << endl;
  cout << "Total Event : " << N_event << " events" << endl;
  cout << "Average Count Rate : " << rate_ave << " cps" << endl;
  cout << "Total Tracking Available Event : " << N_track << " event" << endl;
  cout << "Tracking Available Count Rate : " << (double)N_track/N_event*100 << " %" << endl;
  tree->Write();
  f->Write(); 

  
}

void ThDACScan(int IP_max=0, bool ftree=0, const string& path="test"){
  for(int runN=0;runN<16;runN++){
    Rawdata2root(runN, IP_max, ftree, path);
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
