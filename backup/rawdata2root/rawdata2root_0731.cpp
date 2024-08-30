#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TStopwatch.h>
#include <TCanvas.h>

#include "./include/configureIP.h"

//#define TEST_ON // ON: Read only 10k events
#define TRACKING_ON  // ON: Traking

using namespace std;

/*
void stop_int(int signal){
  char inp[15];
  int sig;
  bool keeploop;
  cout << "interrupt" << endl;
  printf("Keep loop : 0\nEnd loop : 1\nExit App : 2\n");
  fgets(inp,15,stdin);   // assign stdin to inp. (stdin -> standard input)
  sscanf(inp,"%d",&sig); // assign the number of "inp" to sig.

  if(sig==0){
    keeploop = true;
  }else if(sig==1){
    keeploop = false;
  }else if(sig==2){
    gSystem->ExitLoop();
    system->ExitLoop();
  }else{
    exit(1);
  }
  return;
}
*/

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
/*
  if(Nevent == 0){
    NET_diff = NETtime_0;
  }

    cout << "Header  (8-bit): "  << hex << static_cast<int>(Header) << endl;
    cout << "second (30-bit): " << dec << sec << endl;
    cout << "65us (15-bit): " << dec << us << endl;
    cout << "25ns (11-bit): " << dec << ns << endl;

  else{
    NET_diff = NETtime_0 - NETtime_1;
  }
  //cout << fixed << setprecision(9);
  //cout << "NET time differrence : " << dec << NET_diff << endl;
  NETtime_1 = NETtime_0;
}
*/

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
  bool End_of_Raw = false;
  bool End_of_File[12] = {false};
  bool AllOK = true;
  bool FILL_FLAG = true;
  //===== Define Variables ======
  //===== Raw Data =====
  int Traw_L[12][32][10]; // Leading  : [Kalliope][KEL][ch][Multiplicity]
  int Traw_T[12][32][10]; // Trailing : [Kalliope][KEL][ch][Multiplicity]
  int Traw_TOT[12][32][10];
  int Traw_num[2][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Traw_num_total[2][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Nevent = 0;
  bool SameEvent = false;
  int hitNmax = 10; // Multiplicity Max
  int ch = 0;
  int Ndouble=0;
  if(IP_max==0) IP_max = 12; // 0:Experiment mode, Kalliope x12

  //for Trigger (5c event) variables
  int TriggerNo[12];
  double T0_Trigger[12];
  double T_abs[12];
  double T_diff[12];
  double T_offset[12];
  double T_diff_IP0[12];
  double T_False[12];
  double TriggerWindow = 1e-1;
  double DoublePulseWindow = 5e-4;
  
  bool LOS_FLAG[12] = {};

  //===== Fiber CH =====
  int xy=-999;
  int ud=-999;
  int oi=-999;
  int ch_offset=-999;
  int fiber_num=-999;
  
  bool Fired_T[2][2][2][64];      // [x/y][UP/DOWN][1st/2nd][ch][Multiplicity]
  int Fiber_L[2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_T[2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_TOT[2][2][2][64]; // [x/y][UP/DOWN][1st/2nd][ch]
  int Fiber_num[2][2][2] = {}; // Count detected channel to check whether Tracking Available no not
  bool Fiber_FLAG[2][2][2];
  bool Tracking_FLAG[2]; // only up or down
  int N_track = 0;
  int ch_max=64;
  int TOT_noise = 100;
  
  //===== Open Rawdata =====
  for(int i=0;i<IP_max;i++){
    int IP = i+1;
    if(runN<10) ifname[i]=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    else ifname[i]=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
    rawdata[i].open(ifname[i].c_str());
    
    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      exit(1); // terminate with error
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
  //for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
  
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_L[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_T[i][j][k][l]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_TOT[i][j][k][l]=0;
  
  //===== Define Tree ======
  tree->Branch("Traw_L",Traw_L,"Traw_L[6][32][10]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[6][32][10]/I");
  tree->Branch("Traw_TOT",Traw_TOT,"Traw_TOT[6][32][10]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[6][32]/I");

  tree->Branch("Fiber_L",Fiber_L,"Fiber_L[2][2][2][64]/I"); //[x/y][up/down][in/out][CH]
  tree->Branch("Fiber_T",Fiber_T,"Fiber_T[2][2][2][64]/I");
  tree->Branch("Fiber_TOT",Fiber_TOT,"Fiber_TOT[2][2][2][64]/I");
  tree->Branch("Fiber_num",Fiber_num,"Fiber_num[2][2][2]/I");
  
  tree->Branch("T_abs",T_abs,"T_abs[12]/F");
  tree->Branch("T_diff",T_diff,"T_diff[12]/F");
  tree->Branch("T_offset",T_offset,"T_offset[12]/F");
  tree->Branch("T_False",T_False,"T_False[12]/F");
  
  //===== Define Histgrams =====
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
  vector<TH2F*> hT_abs;
  hT_abs.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hT_abs[i] = new TH2F(Form("hT_abs_%d",i),Form("T_abs[%d]; TDC ch; T_abs from 1st Trg.",i),32,0,32,1000,0,5);
  }
  vector<TH2F*> hT_diff;
  hT_diff.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hT_diff[i] = new TH2F(Form("hT_diff_%d",i),Form("T_diff[%d]; TDC ch; T_diff from Previous Trg.",i),32,0,32,1000,0,5);
  }
  TH2F *hT_diff_IP0_2D;
  hT_diff_IP0_2D = new TH2F("hT_diff_IP0_2D","T_diff_IP0_2D; Kalliope IP; T_diff_IP0_2D",12,0,12,1000,0,1e-5);

  vector<TH1F*> hT_diff_IP0;
  hT_diff_IP0.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hT_diff_IP0[i] = new TH1F(Form("hT_diff_IP0_%d",i),Form("T_diff_IP0[%d]; TDC ch; T_diff_IP0",i),1000,0,5);
  }
 
  vector<TH2F*> hT_false;
  hT_false.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hT_false[i] = new TH2F(Form("hT_false_%d",i),Form("T_false[%d]; TDC ch; Fake Trigger for Event Matching",i),32,0,32,1000,0,5);
  }

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
    Nevent = 0;
    cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
    // get file size
    rawdata[IP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[IP].tellg(); // rawdata size in byte (B)
    fsize = fsize/4; // rawdata size in 32 bit (4B)
    rawdata[IP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :",IP) << fsize << endl;
  }
  //===== for test =====
  char Byte[4];
  //intentionally shift rawdata 0
  //for(int i=0;i<50000;i++) rawdata[0].read(Byte, 4); // reading 4 byte (32 bit)

  //Start Reaing All Rawdata =====
  while(AllOK){
    for(int IP=0;IP<IP_max;IP++){
      //===== For Reline Up ======
      configureIP(IP, xy, ud, oi, ch_offset, fiber_num);
      /*
      switch(IP){
	// (xy: x=0, y=1); (ud: u=0, d=1); (oi: o=0; i=1); (ch: 0-31=0, 32-63=1);
	//to be in src file...
      case 0:  xy=0; ud=0; oi=0; ch_offset=0; break;
      case 1:  xy=0; ud=0; oi=0; ch_offset=1; break;
      case 2:  xy=1; ud=0; oi=0; ch_offset=0; break;
      case 3:  xy=1; ud=0; oi=0; ch_offset=1; break;
      case 4:  xy=0; ud=0; oi=1; ch_offset=0; break;
      case 5:  xy=0; ud=1; oi=1; ch_offset=1; break;
      case 6:  xy=1; ud=0; oi=1; ch_offset=0; break;
      case 7:  xy=1; ud=1; oi=1; ch_offset=1; break;
      case 8:  xy=1; ud=1; oi=0; ch_offset=0; break;
      case 9:  xy=1; ud=1; oi=0; ch_offset=1; break;
      case 10: xy=0; ud=1; oi=0; ch_offset=0; break;
      case 11: xy=0; ud=1; oi=0; ch_offset=1; break;
      }
      */
      //===== Start Reading each Rawdata ======
      while(!rawdata[IP].eof()){
	char Byte[4];
	rawdata[IP].read(Byte, 4); // reading 4 byte (32 bit)
	unsigned int data = Read_Raw_32bit(Byte);
	//cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
	int Header = (data & 0xff000000) >> 24;

	
	if(Header == 0x5c){ //Copper Header, GetNETtime (0x5c event)
	  FILL_FLAG = true;
	  if(Nevent == 0){
	    T0_Trigger[IP] = GetNETtime(rawdata[IP], data);
	    T_abs[IP] = 0.0;
	    T_diff[IP] = 0.0;
	    T_offset[IP] = 0.0;
	    T_diff_IP0[IP] = 0.0;
	    cout << "IP : " << IP << ", T0_Trigger : " << T0_Trigger[IP] << endl;
	  }
	  else{
	    T_abs[IP] = GetNETtime(rawdata[IP], data) - T0_Trigger[IP];
	    T_diff[IP] = T_abs[IP] - T_offset[IP];
	    T_offset[IP] = T_abs[IP];
	    T_diff_IP0[IP] = T_abs[IP] - T_abs[0];
	  }
	  if(Nevent%1000 == 0) cout << "IP : " << IP << ", T_abs : " << T_abs[IP] << endl;

	  //====== Check T_diff for Event Matching =====
	  if(T_diff[IP] > TriggerWindow){
	    //cerr << "Error : Event matching!! T_diff is larger than Trigger Window" << endl;
	    //cerr << "IP: " << IP << ", T_diff: " << T_diff[IP] << "> TriggerWindow : " << TriggerWindow << endl;
	  }
	  if(T_diff[IP] < DoublePulseWindow){
	    cout << "Double Pulse was detected!!!" << endl;
	    cout << "Double Pulse No. : " << Ndouble << endl;
	    Ndouble++;
	    T_False[IP] = T_abs[IP];
	  }
	}

	if(data == 0x7fff000a){
	  double Keyword = GetKeyword(rawdata[IP]);
	  //if(IP==0) cout << "IP : " << IP << ", Event No : " << Nevent << ", Keyword : " << Keyword << "ns" << endl;
	}
	
	if(Header == 1){ // Trigger (0x01 event)
	  TriggerNo[IP] = data & 0x00ffffff;
	  if(TriggerNo[IP] != Nevent){
	    //cout << "Error : TrrigerNo is out of order !!" << endl;
	    //cout << "TriggerNo[" << IP << "]: " << TriggerNo[IP] << ", but Nevent: " <<  Nevent << endl;
	  }
	  if(IP==IP_max-1) Nevent++;
	  //cout << "Nevent incremented : " << Nevent << endl;
	  //===== Initialize Readfile for the next event =====
	  SameEvent = true;
	  if(IP==0){
	    int ch = -999;
	    for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=-100;
	    for(int i=0;i<12;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=-100;
	    for(int i=0;i<12;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
	    //===== Reline up =====
	    for(int i=0;i<2;i++){        // x or y
	      for(int j=0;j<2;j++){      // up or dpwn
		for(int k=0;k<2;k++){    // in or out
		  for(int l=0;l<64;l++){ // ch
		    //Fired_T  [i][j][k][l] = false;
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
	}
	//cout << "SameEvent : " << SameEvent << endl;

	
	//===== Get Leading/Trailing edge =====
	if(SameEvent){
	  if(Header == 3){ // Leading Edge (0x03 event)
	    ch = (data >> 16) & 0x000000ff;
	    int time = data & 0x0000ffff;
	  
	    if(Traw_num[IP][0][ch] < hitNmax){
	      Traw_L[IP][ch][Traw_num[IP][0][ch]] = time;
	      //cout << "Traw_L[" << IP << "][" << ch << "][" << Traw_num[IP][0][ch] << "][" << Nevent << "] : " << Traw_L[IP][ch][Traw_num[IP][0][ch]] << endl;

	      if(Traw_num[IP][0][ch]==0){ // hitN=0 -> Fill in Fiber_T
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
	      
	      if(Traw_num[IP][1][ch]==0){ // hitN=0 -> Fill in Fiber_T : NG
		int fiber_ch = ch + ch_offset*32;
		AssignFiber(fiber_ch, ud, oi);
		Fiber_T[xy][ud][oi][fiber_ch] = time;
	      }
	      Traw_num[IP][1][ch]++;
	      Traw_num_total[IP][1][ch]++;
	    }
	  }

	  
	  if((data == 0xff550000) & SameEvent){ // Copper Trailer
	    SameEvent = false;
	    LOS_FLAG[IP] = CheckLOS(rawdata[IP]);
	    if(LOS_FLAG[IP]){
	      FILL_FLAG = false;
	      cout << "Event No. : " << Nevent << endl;
	    }
	    //cout << "End of Loop : " << IP << ", " << "Nevent : " << Nevent << endl;
	    break;
	  }
	}
	//cout << "Nevent at the end of loop : " << Nevent << endl;

	//====== EOF FLAG -> Stop Reading Rawdata  =====
	if(rawdata[IP].eof()){
	  //cout << "reach end of loop" << endl;
	  End_of_File[IP] = true;
	  break;
	}
	else if(rawdata[IP].fail()) cout << "Eroor : file read error" << endl;
      }
    }
    
      
    //===== Display Procedure of Event Loop =====
    if(Nevent %1000 == 0){
      //double Trun = RunTimer.RealTime();
      double Trun = RunTimer.RealTime();
      RunTimer.Continue();
      double rate = Nevent / Trun;
      cout << fixed << setprecision(1) << "Timer :  " << Trun << " s, "
	   << "Count Rate : " << (int)rate << " cps, "
	   << "Reading  : " << Nevent << flush  << " events"<< "\r";
    }
#ifdef TEST_ON
    if(Nevent > 10000) break;
#endif
    //if(Nevent%1000 == 0) cout << "NETtime_0 : " << NETtime_0 << " sec" << endl;

    
    //====== Filling Histgrams for Rawdata =====    
    for(int i=0;i<12;i++){
      if(End_of_File[i]){
	AllOK = false; break;
      }
    }
    if(FILL_FLAG){
      for(int IP=0;IP<IP_max;IP++){
	for(int i=0;i<32;i++)for(int j=0;j<10;j++)Traw_TOT[IP][i][j] = Traw_T[IP][i][j] -Traw_L[IP][i][j];
	for(int i=0;i<32;i++){
	  for(int j=0;j<hitNmax;j++){
	    hTDC_L[IP]->Fill(i,Traw_L[IP][i][j]);
	    hTDC_T[IP]->Fill(i,Traw_T[IP][i][j]);
	    hTraw_TOT[IP]->Fill(i,Traw_TOT[IP][i][j]);
	  }
	  hTraw_num[IP]->Fill(i,Traw_num[IP][0][i]);
	  hT_abs[IP]->Fill(i,T_abs[IP]);
	  hT_diff[IP]->Fill(i,T_diff[IP]);
	}
	if(Nevent%100==0) hT_diff_IP0_2D->Fill(IP,T_diff_IP0[IP]);
	hT_diff_IP0[IP]->Fill(T_abs[IP],T_diff_IP0[IP]);
	hT_false[IP]->Fill(IP,T_False[IP]);
      }
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
  } //===== End of Event Loop ====
  for(int i=0;i<32;i++){
    if(i==0) outfile << "IP=0, CH, total_count" << endl;
    outfile << i << ", " << Traw_num_total[0][0][i] << endl;
    if(i==32) cout << "ThDAC was written" << endl;
  }


  
  //====== Close Input File Stream ======
  //====== Show Run Information ======
  for(int IP=0;IP<IP_max;IP++)  rawdata[IP].close();
  double Trun_total = RunTimer.RealTime();
  double rate_ave = Nevent / Trun_total;
  RunTimer.Stop();
  cout << "Total Real time : " << Trun_total << " s" << endl;
  cout << "Total Event : " << Nevent << " events" << endl;
  cout << "Average Count Rate : " << rate_ave << " cps" << endl;
  cout << "Total Tracking Available Event : " << N_track << " event" << endl;
  cout << "Tracking Available Count Rate : " << (double)N_track/Nevent*100 << " %" << endl;
  tree->Write();
  f->Write(); 
}


void ThDACScan(int IP_max=0, bool ftree=0, const string& path="test"){
  for(int runN=0;runN<16;runN++){
    Rawdata2root(runN, IP_max, ftree, path);
  }
}


void Check_CH_Setting(){
  int xy=0, ud=0, oi=0, ch_offset=0, fiber_num=0;
  TH2I *hCH_Assign_out[4];
  hCH_Assign_out[0] = new TH2I("hCH_Assign_out_0","Fiber_x_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[1] = new TH2I("hCH_Assign_out_1","Fiber_y_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[2] = new TH2I("hCH_Assign_out_2","Fiber_x_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_out[3] = new TH2I("hCH_Assign_out_3","Fiber_x_down; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  
  TH2I *hCH_Assign_in[2];
  hCH_Assign_in[0] = new TH2I("hCH_Assign_in_0","Fiber_x_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  hCH_Assign_in[1] = new TH2I("hCH_Assign_in_1","Fiber_y_up  ; Kalliope CH; Fiber CH",64,0,64,64,0,64);
  
  for(int IP=0;IP<12;IP++){
    configureIP(IP, xy, ud, oi, ch_offset, fiber_num);
    for(int i=0;i<32;i++){
      int KEL_ch = i + ch_offset*32;
      int fiber_ch = KEL_ch;
      AssignFiber(fiber_ch, ud, oi);
      if(oi==0) hCH_Assign_out[fiber_num]->Fill(KEL_ch,fiber_ch);
      else{
	fiber_ch = fiber_ch + ud*32;
	hCH_Assign_in[fiber_num]->Fill(KEL_ch,fiber_ch);
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
  int xy=0, ud=0, oi=0, ch_offset=0, fiber_num=0;
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
	configureIP(IP, xy, ud, oi, ch_offset, fiber_num);
	ch = (data >> 16) & 0x000000ff;
	int KEL_ch = ch + ch_offset*32;
	int fiber_ch = KEL_ch;
	AssignFiber(fiber_ch, ud, oi);
	if(oi==0) hCH_Assign_out[fiber_num]->Fill(KEL_ch,fiber_ch);
	else{
	  fiber_ch = fiber_ch + ud*32;
	  hCH_Assign_in[fiber_num]->Fill(KEL_ch,fiber_ch);
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
