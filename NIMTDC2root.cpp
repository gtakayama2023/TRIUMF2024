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
#define TRACKING_ON  // ON: Traking

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
  cout << "FullData64bit : " << hex << FullData64bit << endl;
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


void NIMTDC2root(int runN=10, bool ftree=0, const string& path="test"){
  //===== Define Input/Output File =====
  string ifname;
  TString ofname;
  ifstream rawdata;

  //===== Define FLAG ======
  bool END_FLAG = false;
  bool FILL_FLAG = true;
  bool LOS_FLAG = false;
  bool SKIP_FLAG = false;
  
  //===== Define Variables ======
  //===== Raw Data =====
  int Traw_L[4]; // Leading  : [Kalliope][KEL][ch][Multiplicity]
  int Traw_T[4]; // Trailing : [Kalliope][KEL][ch][Multiplicity]
  int Traw_TOT[4];
  int Traw_num[4] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Traw_num_total[2][4] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int N_event = 0;
  bool SameEvent = false;
  int hitNmax = 10; // Multiplicity Max
  int ch = 0;
  int N_sync;
  
  //Time Stamp for Trigger (5c event)
  int N_Trigger = 0;
  double TimeStamp0 = 0.0;
  double TimeStamp = 0.0;
  double dT_trigger = 0.0;
  double dT_diff = 0.0;
  double T_offset = 0.0;
  double T_diff = 0.0;
  double T_Sync = 0.0;

  //===== Time Window ======
  double DoublePulseWindow = 5e-4;  
  
  //===== Open Rawdata =====
  int IP = 16;
  if(runN<10) ifname=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
  else ifname=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
  rawdata.open(ifname.c_str());
    
  if (!rawdata) {
    cout << "Unable to open file: " << ifname << endl;
    //exit(1); // terminate with error
  }

  if(runN<10) ofname = Form("../ROOT/%s/MSE00000%d_%d.root",path.c_str(),runN,IP);
  else ofname = Form("../ROOT/%s/MSE0000%d_%d.root",path.c_str(),runN,IP);
  cout << "create root file :" << ofname << endl;
  
  TFile *f = new TFile(ofname,"RECREATE");

  TTree *tree = new TTree("tree","tree");
  
  //===== Initialize =====
  for(int i=0;i<4;i++){
    Traw_L[i]=0;
    Traw_T[i]=0;
    Traw_TOT[i]=0;
    Traw_num[i]=0;
  }
  //===== Define Tree ======
  tree->Branch("TimeStamp",&TimeStamp,"TimeStamp/D");
  tree->Branch("dT_trigger",&dT_trigger,"dT_trigger/D");
  tree->Branch("T_offset",&T_offset,"T_offset/D");
  tree->Branch("T_Sync",&T_Sync,"T_Sync/D");  
  tree->Branch("Traw_L",Traw_L,"Traw_L[4]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[4]/I");
  tree->Branch("Traw_TOT",Traw_TOT,"Traw_TOT[4]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[4]/I");
  
  //===== Define Histgrams =====  
  TH1F *hTimeStamp;
  hTimeStamp = new TH1F("TimeStamp","TimeStamp; time[sec]; event",1000,-10,100);

  TH1F *hdT_trigger;
  hdT_trigger = new TH1F("dT_trigger","dT_trigger; time[sec]; event",1000,-1e-7,1e-7);
  
  TH1F *hT_Sync;
  hT_Sync = new TH1F("hT_Sync","hT_Sync; time[sec]; event",1000,-10,100);

  TH1F *hTDC_L[4];
  hTDC_L[0] = new TH1F("hTDC_L_UP","TDC[0] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_L[1] = new TH1F("hTDC_L_DOWN","TDC[1] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_L[2] = new TH1F("hTDC_L_FORWARD","TDC[2] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_L[3] = new TH1F("hTDC_L_BACKWARD","TDC[3] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);

  TH1F *hTDC_T[4];
  hTDC_T[0] = new TH1F("hTDC_T_UP","TDC[0] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_T[1] = new TH1F("hTDC_T_DOWN","TDC[1] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_T[2] = new TH1F("hTDC_T_FORWARD","TDC[2] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_T[3] = new TH1F("hTDC_T_BACKWARD","TDC[3] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);

  TH1F *hTDC_TOT[4];
  hTDC_TOT[0] = new TH1F("hTDC_TOT_UP","TDC[0] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_TOT[1] = new TH1F("hTDC_TOT_DOWN","TDC[1] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_TOT[2] = new TH1F("hTDC_TOT_FORWARD","TDC[2] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);
  hTDC_TOT[3] = new TH1F("hTDC_TOT_BACKWARD","TDC[3] Leading ;TDC ch; TDC [ns]",6e4,-150,7e4);

  //===== Timer =====
  TStopwatch RunTimer;
  RunTimer.Start();
  
  //===== Open Rawdata =====
  N_event = 0;
  cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
  // get file size
  rawdata.seekg(0, ios::end); // going to the end of the file
  streampos fsize = rawdata.tellg(); // rawdata size in byte (B)
  fsize = fsize/4; // rawdata size in 32 bit (4B)
  rawdata.seekg(0, ios::beg); // going to the begin of the file
  cout << Form("rawdata%d filesize :",IP) << fsize << endl;

  //===== Start Reading each Rawdata ======
  while(!rawdata.eof()){

    //===== SKIP_FLAG =====
    if(SKIP_FLAG){
      SKIP_FLAG = false;
    }
    char Byte[4];
    rawdata.read(Byte, 4); // reading 4 byte (32 bit)
    unsigned int data = Read_Raw_32bit(Byte);
    //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
    int Header = (data & 0xff000000) >> 24;
	
    if(Header == 0x5c){ //Copper Header, Initialize, GetNETtime (0x5c event), Event Matching
      //===== Initialize Readfile for the next event =====
      SameEvent = true;
      int ch = -999;
      for(int i=0;i<4;i++){
	Traw_L[i]=-100;
	Traw_T[i]=-100;
	Traw_num[i]=0;
      }
      FILL_FLAG = true;
      N_Trigger++;
      if(N_event == 0){
	TimeStamp0 = GetNETtime(rawdata, data);
	cout << "TimeStamp0 : " << TimeStamp0 << endl;
	TimeStamp  = 0.0;
	dT_trigger = 0.0;
	T_offset   = 0.0;
	T_diff     = 0.0;
      }
      else{
	TimeStamp  = GetNETtime(rawdata, data) - TimeStamp0;
	//cout << "TimeStamp : " << TimeStamp << endl;
	dT_trigger = TimeStamp - T_offset;
	T_diff     = TimeStamp - TimeStamp0;
	T_offset   = TimeStamp;
      }
	  
      if(dT_trigger < DoublePulseWindow){
	N_Trigger=0;
	N_sync++;
	T_Sync = TimeStamp;
	//cout << "Double Pulse was detected !!! : " << N_event << endl;
      }
      N_event++;
    } // End of 5c Event

    if(data == 0x7fff000a){
      double Keyword = GetKeyword(rawdata);
      if(Keyword == 0x00000001){
	cout << "Sync. Trigger was detected !!! : " << N_event << endl;	
      }
    }
	
    if(Header == 1){ // Trigger (0x01 event)
      //N_Trigger = data & 0x00ffffff;
    }
    //cout << "SameEvent : " << SameEvent << endl;
	
    //===== Get Leading/Trailing edge =====
    if(SameEvent){
      if(Header == 3){ // Leading Edge (0x03 event)
	ch = (data >> 16) & 0x000000ff;
	int time = data & 0x0000ffff;
	  
	if(Traw_num[ch] < hitNmax){
	  Traw_L[ch] = time;
	  Traw_num[ch]++;
	  Traw_num_total[0][ch]++;
	}
      }
	  
	  
      if(Header == 4){ // Trailing Edge (0x04 event)
	ch = (data >> 16) & 0x000000ff;
	int time = data & 0x0000ffff;
	    
	if(Traw_num[ch] < hitNmax){
	  Traw_T[ch] = time;
	  Traw_num[ch]++;
	  Traw_num_total[1][ch]++;
	}
      }

	  
      if((data == 0xff550000) & SameEvent){ // Copper Trailer
	SameEvent = false;
	LOS_FLAG = CheckLOS(rawdata);
	if(LOS_FLAG){
	  FILL_FLAG = false;
	  cout << "Event No. : " << N_event << endl;
	}
	//cout << "End of Loop : " << IP << ", " << "N_event : " << N_event << endl;
	//====== Filling Histgrams for Rawdata =====
	if(FILL_FLAG){
	  for(int i=0;i<4;i++)Traw_TOT[i] = Traw_T[i] -Traw_L[i];
	  hTimeStamp->Fill(TimeStamp);
	  hdT_trigger->Fill(dT_trigger);
	  //hT_false->Fill(IP,T_Sync);
	  hT_Sync->Fill(T_Sync);
	  for(int i=0;i<4;i++){	    
	    hTDC_L[i]->Fill(Traw_L[i]);
	    hTDC_T[i]->Fill(Traw_T[i]);
	    hTDC_TOT[i]->Fill(Traw_TOT[i]);
	    //hTraw_num->Fill(Traw_num[i]);
	  }
	}
	cout << "TimeStamp : "  << TimeStamp << endl;
	if(ftree) tree->Fill();
	//===== End of Event Loop ====
      }
    }
    //cout << "N_event at the end of loop : " << N_event << endl;
	
    //====== EOF FLAG -> Stop Reading Rawdata  =====
    if(rawdata.eof()){
      //cout << "reach end of loop" << endl;
      END_FLAG = true;
      break;
    }
    else if(rawdata.fail()) cout << "Eroor : file read error" << endl;
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
    if(N_event > 10) break;
#endif
    //if(N_event%1000 == 0) cout << "NETtime_0 : " << NETtime_0 << " sec" << endl;
  }    
  //====== Close Input File Stream ======
  //====== Show Run Information ======
  rawdata.close();
  double Trun_total = RunTimer.RealTime();
  double rate_ave = N_event / Trun_total;
  RunTimer.Stop();
  cout << "Total Real time : " << Trun_total << " s" << endl;
  cout << "Total Event : " << N_event << " events" << endl;
  cout << "Average Count Rate : " << rate_ave << " cps" << endl;
  tree->Write();
  f->Write(); 
}
