#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TStopwatch.h>

#define TRACKING_ON  //Please comment out if not require tracking

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

void assignFiber(int &fiber_ch, int &ud, int oi){
  if (oi == 1) { // for inner fiber
    int q   = fiber_ch / 4;
    int mod = fiber_ch % 4;
    int Q   = fiber_ch / 8;
    // ch assignment
    if (q % 2 == 0) ud = 0; else ud = 1;
    switch (mod) {
    case 0: fiber_ch = 4 * Q + 3; break;
    case 1: fiber_ch = 4 * Q;     break;
    case 2: fiber_ch = 4 * Q + 1; break;
    case 3: fiber_ch = 4 * Q + 2; break;
    }
  }
}

void GetNETtime(ifstream& rawdata, unsigned int data, int Nevent, double &NETtime_0, double &NETtime_1, double &NET_diff){
    uint32_t nextData;
    char Byte[4];
    rawdata.read(Byte, 4);
    if (!rawdata) {
        cerr << "error : rawdata is empty...." << endl;
        return;
    }
    nextData = Read_Raw_32bit(Byte);
    /*
    cout << "data : " << hex << data << endl;
    cout << "nextData : " << hex << nextData << endl;
    */
    uint64_t fullData = (static_cast<uint64_t>(data) << 32) | nextData;
    //cout << "FullData : " << hex << fullData << endl;
    int  Header = (fullData >> 56) & 0xFF; //8bit
    double sec = (fullData >> 26) & 0x3FFFFFFF; //30bit
    double us  = (fullData >> 11) & 0x7FFF; //15bit
    double ns  = fullData & 0x7FF; //11bit
    NETtime_0 = sec + us / 32768.0 + ns * 25.0 * pow(10,-9);
    if(Nevent == 0){
      NET_diff = NETtime_0;
    }
    /*
    cout << "Header  (8-bit): "  << hex << static_cast<int>(Header) << endl;
    cout << "second (30-bit): " << dec << sec << endl;
    cout << "65us (15-bit): " << dec << us << endl;
    cout << "25ns (11-bit): " << dec << ns << endl;
    */
    else{
      NET_diff = NETtime_0 - NETtime_1;
    }
    //cout << fixed << setprecision(9);
    //cout << "NET time differrence : " << dec << NET_diff << endl;
    NETtime_1 = NETtime_0;
}

void Rawdata2root(int runN=10, int IP_max=0, bool ftree=0, const string& path="test"){
  //===== Define Input/Output File =====
  string ifname[12];
  TString ofname;
  ifstream rawdata[12];
  ofstream outfile("ThDAC.txt");
  ifstream readfile;
  bool End_of_Raw = false;
  bool End_of_File[12] = {false};
  bool AllOK = true;
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
  if(IP_max==0) IP_max = 12; // 0:Experiment mode, Kalliope x12

  double Time_zero[12];
  double Time_abs[12];
  double Time_from_zero[12];
  double Time_diff[12];
  double NETtime_0;
  double NETtime_1;
  double NET_diff;

  //===== Fiber CH =====
  int xy=-999;
  int ud=-999;
  int oi=-999;
  int ch_offset=-999;
  
  bool Fired_T [2][2][2][64];      // [x/y][UP/DOWN][1st/2nd][ch][Multiplicity]
  int Fiber_L  [2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch][Multiplicity]
  int Fiber_T  [2][2][2][64];   // [x/y][UP/DOWN][1st/2nd][ch][Multiplicity]
  int Fiber_Q  [2][2][2][64]; // [x/y][UP/DOWN][1st/2nd][ch][Multiplicity]
  int Fiber_num[2][2][2][64] = {}; // Multipulicity

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
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++)for(int l=0;l<64;l++)Fiber_Q[i][j][k][l]=0;
  
  //===== Define Tree ======
  tree->Branch("Traw_L",Traw_L,"Traw_L[6][32][10]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[6][32][10]/I");
  tree->Branch("Traw_TOT",Traw_TOT,"Traw_TOT[6][32][10]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[6][32]/I");

  tree->Branch("Fiber_L",Fiber_L,"Fiber_L[2][2][2][64]/I"); //[x/y][up/down][in/out][CH]
  tree->Branch("Fiber_T",Fiber_T,"Fiber_T[2][2][2][64]/I");
  tree->Branch("Fiber_Q",Fiber_Q,"Fiber_Q[2][2][2][64]/I");
  tree->Branch("Fiber_num",Fiber_num,"Fiber_num[2][2][2][64]/I");

  tree->Branch("Time_abs",Time_abs,"Time_abs[12]/F");
  tree->Branch("Time_diff",Time_diff,"Time_diff[12]/F");
  
  
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
  vector<TH2F*> hTime_abs;
  hTime_abs.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTime_abs[i] = new TH2F(Form("hTime_abs_%d",i),Form("Time_abs[%d]; TDC ch; Time_abs from 1st Trg.",i),32,0,32,1000,0,5);
  }
  vector<TH2F*> hTime_from_zero;
  hTime_from_zero.resize(IP_max);
  for(int i=0;i<IP_max;i++){
    hTime_from_zero[i] = new TH2F(Form("hTime_from_zero_%d",i),Form("Time_from_zero[%d]; TDC ch; Time_from_zero from 1st Trg.",i),32,0,32,1000,0,5);
  }
  
#ifdef TRACKING_ON
  TH2F *hFiber_L[2][2][2];
  TH2F *hFiber_T[2][2][2];
  TH2F *hFiber_Q[2][2][2];
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
	hName.Form("hFiber_Q_%s_%s_%s",XY.Data(),UD.Data(),OI.Data());
	hTitle.Form("hFiber_Q[%s][%s][%s]; Fiber ch; TDC [ns]",XY.Data(),UD.Data(),OI.Data());
	hFiber_Q[i][j][k] = new TH2F(hName, hTitle, 64,0,64,6e4,-150,7e4);
      }
    }
  }
#endif
  
  //===== Timer =====
  TStopwatch timer_total;
  timer_total.Start();
  
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
  
  while(AllOK){
    //cout << "Start read data : " << Nevent << endl;
    for(int IP=0;IP<IP_max;IP++){
      //===== For Reline Up ======
      switch(IP){
	// (xy: x=0, y=1); (ud: u=0, d=1); (oi: o=0; i=1); (ch: 0-31=0, 32-63=1);
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

      //===== Read Rawdata ======
      //Read_Data(IP, &rawdata[IP], Nevent, End_of_Raw);
      //if(rawdata[IP].eof()) End_of_Raw = true;
      while(!rawdata[IP].eof()){
	//cout << "Enter rawdata loop" << endl;
	char Byte[4];
	rawdata[IP].read(Byte, 4); // reading 4 byte (32 bit)
	unsigned int data = Read_Raw_32bit(Byte);
	//cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
	int Header = (data & 0xff000000) >> 24;
	if(Header == 0x5c){
	  GetNETtime(rawdata[IP], data, Nevent, NETtime_0, NETtime_1, NET_diff);
	  if(Nevent == 0){
	    Time_zero[IP] = NETtime_0;
	    //cout << "IP : " << IP << ", Time_zero : " << Time_zero[IP] << endl;
	  }
	  Time_abs[IP]  = NETtime_1;
	  Time_from_zero[IP]  = NETtime_1 - Time_zero[IP];
	  Time_diff[IP] = NET_diff;
	  //if(Nevent%1000 == 0) cout << "IP : " << IP << ", Time_abs : " << Time_abs[IP] << endl;
	}
	if(Header == 1){
	  //cout << "Enter Header 1, and Nevent : " << Nevent << endl;
	  if(IP==IP_max-1) Nevent++;
	  //cout << "Nevent incremented : " << Nevent << endl;
	  int TRG_IN = data & 0x00ffffff;
	  SameEvent = true;
	  //===== Initialize Readfile for the next event =====
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
		    Fired_T  [i][j][k][l] = false;
		    Fiber_num[i][j][k][l] = 0;
		    Fiber_L  [i][j][k][l] = -100;
		    Fiber_T  [i][j][k][l] = -100;
		    Fiber_Q  [i][j][k][l] = -100;
		  }
		}
	      }
	    }
	  }
	}
	//cout << "SameEvent : " << SameEvent << endl;
	//===== Leading/Trailing edge =====
	if(SameEvent){
	  if(Header == 3){ // leading edge
	    //cout << "Enter Header 3" << endl;
	    ch = (data >> 16) & 0x000000ff;
	    int time = data & 0x0000ffff;
	  
	    if(Traw_num[IP][0][ch] < hitNmax){
	      Traw_L[IP][ch][Traw_num[IP][0][ch]] = time;
	      //cout << "Traw_L[" << IP << "][" << ch << "][" << Traw_num[IP][0][ch] << "][" << Nevent << "] : " << Traw_L[IP][ch][Traw_num[IP][0][ch]] << endl;

	      if(Traw_num[IP][0][ch]==0){ // hitN=0 -> Fill in Fiber_T
		int fiber_ch = ch + ch_offset*32;
		assignFiber(fiber_ch, ud, oi);
		Fiber_L[xy][ud][oi][fiber_ch] = time;
	      }
	      Traw_num[IP][0][ch]++;
	      Traw_num_total[IP][0][ch]++;
	    }
	  }
	
	  if(Header == 4){ // trailing edge
	    //cout << "Enter Header 4" << endl;
	    ch = (data >> 16) & 0x000000ff;
	    int time = data & 0x0000ffff;
	  
	    if(Traw_num[IP][1][ch] < hitNmax){
	      Traw_T[IP][ch][Traw_num[IP][1][ch]] = time;

	      if(Traw_num[IP][1][ch]==0){ // hitN=0 -> Fill in Fiber_T : NG
		int fiber_ch = ch + ch_offset*32;
		assignFiber(fiber_ch, ud, oi);
		Fiber_T[xy][ud][oi][fiber_ch] = time;
	      }
	      Traw_num[IP][1][ch]++;
	      Traw_num_total[IP][1][ch]++;
	    }
	  }
	  
	  if((data == 0xff550000) & SameEvent){
	    SameEvent = false;
	    //cout << "End of Loop : " << IP << ", " << "Nevent : " << Nevent << endl;
	    break;
	  }
	}
	//cout << "Nevent at the end of loop : " << Nevent << endl;
	if(rawdata[IP].eof()){
	  cout << "reach end of loop" << endl;
	  End_of_File[IP] = true;
	  break;
	}
	else if(rawdata[IP].fail()) cout << "Eroor : file read error" << endl;
      }
    }
    //===== Display Procedure of Event Loop =====
    if(Nevent %1000 == 0){
      double elapsed = timer_total.RealTime();
      double rate = Nevent / elapsed;
      cout << fixed << setprecision(1) << "Timer :  " << elapsed << " s, "
	   << "Rate : " << (int)rate << " cps, "
	   << "Data : " << Nevent << flush  << " events"<< "\r";
    }
      
    //if(Nevent > 1000) break;
    //if(Nevent%1000 == 0) cout << "NETtime_0 : " << NETtime_0 << " sec" << endl;
    
    //====== Filling Histgrams =====    
    for(int i=0;i<12;i++){
      if(End_of_File[i]){
	AllOK = false; break;
      }
    }
    
    for(int IP=0;IP<IP_max;IP++){
      for(int i=0;i<32;i++)for(int j=0;j<10;j++)Traw_TOT[IP][i][j] = Traw_T[IP][i][j] -Traw_L[IP][i][j];
      for(int i=0;i<32;i++){
	for(int j=0;j<hitNmax;j++){
	  hTDC_L[IP]->Fill(i,Traw_L[IP][i][j]);
	  hTDC_T[IP]->Fill(i,Traw_T[IP][i][j]);
	  hTraw_TOT[IP]->Fill(i,Traw_TOT[IP][i][j]);
	}
	hTraw_num[IP]->Fill(i,Traw_num[IP][0][i]);
	hTime_abs[IP]->Fill(i,Time_abs[IP]);
	hTime_from_zero[IP]->Fill(i,Time_from_zero[IP]);
      }
    }
    //====== Tracking ======
#ifdef TRACKING_ON
    //====== Filling Histgrams =====    
    for(int ch=0;ch<64;ch++){
      for(int i=0;i<2;i++){
	for(int j=0;j<2;j++){
	  for(int k=0;k<2;k++){
	    hFiber_L[i][j][k]->Fill(ch,Fiber_L[i][j][k][ch]);
	    hFiber_T[i][j][k]->Fill(ch,Fiber_T[i][j][k][ch]);
	    hFiber_Q[i][j][k]->Fill(ch,Fiber_Q[i][j][k][ch]);
	  }
	}
      }
    }
#endif
    if(ftree) tree->Fill();
    //===== END FLAG =====
  } //===== End of Event Loop ====
  for(int i=0;i<32;i++){
    if(i==0) outfile << "IP=0, CH, total_count" << endl;
    outfile << i << ", " << Traw_num_total[0][0][i] << endl;
    if(i==32) cout << "ThDAC was written" << endl;
  }
  //====== close Input File Stream ======
  for(int IP=0;IP<IP_max;IP++)  rawdata[IP].close();
  timer_total.Stop();
  double totalElapsed = timer_total.RealTime();
  cout << "Total Real time : " << totalElapsed << "s" << endl;
  cout << "Total Event : " << Nevent << " events" << endl;
  //cout << "Time of Last Trg. Event : " << NETtime_1 << endl;
  tree->Write();
  f->Write(); 
}

void ThDACScan(int IP_max=0, bool ftree=0, const string& path="test"){
  for(int runN=0;runN<16;runN++){
    Rawdata2root(runN, IP_max, ftree, path);
  }
}
