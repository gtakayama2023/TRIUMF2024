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

#define TEST_ON // ON: Read only 10k events

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

void NIMTDC2root(int runN=10, bool ftree=0, const string& path="test"){
  //===== Define Input/Output File =====
  string ifname;
  TString ofname;
  ifstream rawdata;
  ifstream readfile;
  bool FILL_FLAG = true;
  //===== Define Variables ======
  //===== Raw Data =====
  int Traw_L[4]; // Leading  : [Kalliope][KEL][ch][Multiplicity]
  int Traw_T[4]; // Trailing : [Kalliope][KEL][ch][Multiplicity]
  int Traw_TOT[4];
  int Nevent = 0;
  bool SameEvent = false;
  double timeStamp;
  int ch;
  
  bool LOS_FLAG = {};
  bool Skip_FLAG = {};
  
  //===== Open Rawdata =====
  int IP=16; //NIM-TDC: IP adress(default:16)
  if(runN<10) ifname = Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
  else ifname = Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,IP);
  rawdata.open(ifname.c_str());
  
  if(runN<10) ofname = Form("../ROOT/%s/MSE00000%d.root",path.c_str(),runN);
  else ofname = Form("../ROOT/%s/MSE0000%d.root",path.c_str(),runN);
  cout << "create root file :" << ofname << endl;
  
  TFile *f = new TFile(ofname,"RECREATE");

  TTree *tree = new TTree("tree","tree");
  
  //===== Initialize =====
  
  //===== Define Tree ======
  tree->Branch("Traw_L",Traw_L,"Traw_L[4]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[4]/I");
  tree->Branch("Traw_TOT",Traw_TOT,"Traw_TOT[4]/I");

  //===== Define Histgrams =====
  TH1F *hT_Up;
  hT_Up = new TH1F("hT_Up","hT_up; Time[ns]; count",1e6,0,1e-4);
  TH1F *hT_Down;
  hT_Down = new TH1F("hT_Down","hT_Down; Time[ns]; count",1e6,0,1e-4);
  TH1F *hT_Forward;
  hT_Forward = new TH1F("hT_Forward","hT_Forward; Time[ns]; count",1e6,0,1e-4);
  TH1F *hT_Backward;
  hT_Backward = new TH1F("hT_Backward","hT_Backward; Time[ns]; count",1e6,0,1e-4);
  
  //===== Timer =====
  TStopwatch RunTimer;
  RunTimer.Start();
  
  //===== Open Rawdata =====
  Nevent = 0;
  cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
  // get file size
  rawdata.seekg(0, ios::end); // going to the end of the file
  streampos fsize = rawdata.tellg(); // rawdata size in byte (B)
  fsize = fsize/4; // rawdata size in 32 bit (4B)
  rawdata.seekg(0, ios::beg); // going to the begin of the file

  //===== Start Reading each Rawdata ======
  while(!rawdata.eof()){
    if(Skip_FLAG){
      Skip_FLAG = false; break;
    }
    char Byte[4];
    rawdata.read(Byte, 4); // reading 4 byte (32 bit)
    unsigned int data = Read_Raw_32bit(Byte);
    //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
    int Header = (data & 0xff000000) >> 24;

    if(Header == 0x5c){ //Copper Header, GetNETtime (0x5c event)
      FILL_FLAG = true;
      timeStamp  = GetNETtime(rawdata, data);
    }

    if(data == 0x7fff000a){
      double Keyword = GetKeyword(rawdata);
      if(Keyword == 0x00000001){
	cout << "False Trigger was detected!!!" << endl;
      }
    }
	
    if(Header == 1){ // Trigger (0x01 event)
      //TriggerNo = data & 0x00ffffff;
      //===== Initialize Readfile for the next event =====
      SameEvent = true;
    }
    //cout << "SameEvent : " << SameEvent << endl;

	
    //===== Get Leading/Trailing edge =====
    if(SameEvent){
      if(Header == 3){ // Leading Edge (0x03 event)
	ch = (data >> 16) & 0x000000ff;
	int time = data & 0x0000ffff;
	Traw_L[ch] = time;
      }
	  
      if(Header == 4){ // Trailing Edge (0x04 event)
	ch = (data >> 16) & 0x000000ff;
	int time = data & 0x0000ffff;
	Traw_T[ch] = time;
      }
	  
      if((data == 0xff550000) & SameEvent){ // Copper Trailer
	SameEvent = false;
	LOS_FLAG = CheckLOS(rawdata);
	if(LOS_FLAG){
	  FILL_FLAG = false;
	  cout << "Event No. : " << Nevent << endl;
	}
	break;
      }
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
    
  //====== Filling Histgrams for Rawdata =====    
  if(FILL_FLAG){
    hT_Up->Fill(Traw_L[0]);
    hT_Down->Fill(Traw_L[1]);
    hT_Forward->Fill(Traw_L[2]);
    hT_Backward->Fill(Traw_L[3]);
  }

  if(ftree) tree->Fill();
  //===== End of Event Loop ====

  //====== Close Input File Stream ======
  //====== Show Run Information ======
  rawdata.close();
  double Trun_total = RunTimer.RealTime();
  double rate_ave = Nevent / Trun_total;
  RunTimer.Stop();
  cout << "Total Real time : " << Trun_total << " s" << endl;
  cout << "Total Event : " << Nevent << " events" << endl;
  cout << "Average Count Rate : " << rate_ave << " cps" << endl;
  tree->Write();
  f->Write(); 
}
