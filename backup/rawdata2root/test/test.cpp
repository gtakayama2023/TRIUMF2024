#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TStopwatch.h>

using namespace std;

unsigned int Binary_to_32bit(const char* b)
{
    return ((b[3] << 24) & 0xff000000) | // shift the 1st byte to 24 bit left
           ((b[2] << 16) & 0x00ff0000) | // shift the 2nd byte to 16 bit left
           ((b[1] <<  8) & 0x0000ff00) | // shift the 3rd byte to 08 bit left
           ((b[0] <<  0) & 0x000000ff);  // shift the 4th byte to 00 bit left
}

void Rawdata2root(int runN=10, bool ftree=0){
  
  //===== Define Input/Output File =====
  string ifname[12];
  TString ofname;
  ifstream rawdata[12];

  //===== Open Rawdata =====
  for(int i=0;i<2;i++){
    int IP = i+1;
    ifname[i]=Form("./RAW/MSE0000%d_192.168.10.%d.rawdata",runN,IP);
    rawdata[i].open(ifname[i].c_str());
    
    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      exit(1); // terminate with error
    }
  }

  ofname = Form("./ROOT/MSE0000%d.root",runN);
  cout << "create root file :" << ofname << endl;
  
  TFile *f = new TFile(ofname,"RECREATE");

  TTree *tree = new TTree("tree","tree");
  
  //===== Define Variables ======
  int Traw_L[2][32][10]; //Leading : [Kalliope][KEL][ch][Multiplicity]
  int Traw_T[2][32][10]; //Trailing : [Kalliope][KEL][ch][Multiplicity]
  int TOT[2][32][10];
  int Traw_num[2][2][32]; //Multiplicity
  int Nevent = 0;
  bool SameEvent = false;
  int hitNmax =2; //Multiplicity Max

  //===== Initialize =====
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=0;
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=0;
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)TOT[i][j][k]=0;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
  
  //===== Define Tree ======
  tree->Branch("Traw_L",Traw_L,"Traw_L[2][32][10]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[2][32][10]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[2][32]/I");
  
  //===== Define Histgrams =====
  TH2F *hTDC_L[2];
  hTDC_L[0] = new TH2F("hTDC_L","TDC Leading ;TDC ch; TDC [ns]",32,0,32,6e4,-150,7e4);
  hTDC_L[1] = new TH2F("hTDC_L","TDC Leading ;TDC ch; TDC [ns]",32,0,32,6e4,-150,7e4);
  TH2F *hTDC_T[2];
  hTDC_T[0] = new TH2F("hTDC_T","TDC Trailing;TDC ch; TDC [ns]",32,0,32,6e4,-150,7e4);
  hTDC_T[1] = new TH2F("hTDC_T","TDC Trailing;TDC ch; TDC [ns]",32,0,32,6e4,-150,7e4);
  TH2F *hTOT[2];
  hTOT[0] = new TH2F("hTOT","TOT; TDC ch; TOT [ns]",32,0,32,6e4,-150,7e4);
  hTOT[1] = new TH2F("hTOT","TOT; TDC ch; TOT [ns]",32,0,32,6e4,-150,7e4);
  
  //===== Timer =====
  TStopwatch timer;
  timer.Start();

  //===== Event Loop =====
  for(int IP=0;IP<2;IP++){ // i: Kalliope IP adress
    Nevent = 0;
    cout << "Start Reading Rawdata from Kalliope: " << IP << endl;
    // get file size
    rawdata[IP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[IP].tellg(); // rawdata size in byte (B)
    fsize = fsize/4; // rawdata size in 32 bit (4B)
    rawdata[IP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :",IP) << fsize << endl;
    
    while(!rawdata[IP].eof()){
      char Byte[4];
      rawdata[IP].read(Byte, 4); // reading 4 byte (32 bit)
      unsigned int data = Binary_to_32bit(Byte);
      //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
      int Header = (data & 0xff000000) >> 24;
      //===== Count Trriger (NIM IN) =====
      if(Header == 1){
	Nevent++;
	int TRG_IN = data & 0x00ffffff;
	SameEvent = false;
	//cout << "Nevent, TRG_IN : " << Nevent << ", " << TRG_IN << endl;
	//cout << "bool : " << SameEvent << endl; //OK
      }
      else{SameEvent = true;}

      //====== Filling Histgrams =====
      if(!SameEvent){
	if(ftree) tree->Fill();
	for(int i=0;i<32;i++)for(int j=0;j<hitNmax;j++)hTDC_L[IP]->Fill(i,Traw_L[IP][i][j]);
	for(int i=0;i<32;i++)for(int j=0;j<hitNmax;j++)hTDC_T[IP]->Fill(i,Traw_T[IP][i][j]);
	for(int i=0;i<32;i++)for(int j=0;j<hitNmax;j++)hTOT[IP]->Fill(i,TOT[IP][i][j]);
      }
      
      //===== Initialize fot he next event =====
      if(!SameEvent){
	for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=-100;
	for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=-100;
	for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
      }
      
      //===== Display Procedure of Event Loop =====
      if(Nevent %1000 == 0){
	double elapsed = timer.RealTime();
	double rate = Nevent / elapsed;
	cout << fixed << setprecision(1) << "Timer :  " << elapsed << " s, "
	     << "Rate : " << (int)rate << " cps, "
	     << "Data : " << Nevent << flush  << " events"<< "\r";
	timer.Continue();
      }
      
      //===== Rawdata Filling =====
      if(SameEvent){
	if(Header == 3){
	  int ch = (data >> 16) & 0x000000ff;
	  int t_L = data & 0x0000ffff;
	  if(Traw_num[IP][0][ch] < hitNmax){
	    Traw_L[IP][ch][Traw_num[IP][0][ch]] = t_L;
	    //cout << "Traw_L["<<IP<<"]["<<ch<<"]["<<Traw_num[IP][0][ch]<<"]" <<endl; //OK
	    Traw_num[IP][0][ch]++;
	  }
	}
	if(Header == 4){
	  int ch = (data >> 16) & 0x000000ff;
	  int t_T = data & 0x0000ffff;
	  if(Traw_num[IP][1][ch] < hitNmax){
	    Traw_T[IP][ch][Traw_num[IP][1][ch]] = t_T;
	    Traw_num[IP][1][ch]++;
	  }
	}
	for(int i=0;i<32;i++)for(int j=0;j<10;j++)TOT[IP][i][j] = Traw_T[IP][i][j] -Traw_L[IP][i][j];
      }

      if(Nevent == 100000)break;
      
    }
    
    rawdata[IP].close();
    cout << endl;
  }
  //===== End of Event Loop =====

  timer.Stop();
  double totalElapsed = timer.RealTime();
  cout << "Total Real time : " << (int)totalElapsed << "s" << endl;
  f->Write();
}
