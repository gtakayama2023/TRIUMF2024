#define EventMatch_cxx
#include "EventMatch.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGraph.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <TBranch.h>

void EventMatch::DC_TS_Fit(int runN=0, int subrunN=0)
{
  TString DC_path  = "/Users/ishitanisoshi/triumf/sep2024/DC/RP1212/data/raw";
  TString NIM_path = "../ROOT/test_0808";
  TString out_path = ".";
  TString DC_name  = Form("%s/run_%04d/run_%04d_%04d.dat.root", DC_path.Data(), runN, runN, subrunN);
  TString NIM_name = Form("%s/run_%d/MSE0000%d.root", NIM_path.Data(), runN, runN);
  TString out_name = Form("%s/run_%04d.root", out_path.Data(), runN);
  TFile *file_DC   = TFile::Open(DC_name);
  TFile *file_NIM  = TFile::Open(NIM_name);
  TFile *outfile   = new TFile(out_name,"RECREATE");
  if (!file_DC || !file_DC->IsOpen()) {
    std::cerr << "Error: Could not open file " << DC_name << std::endl;
    return;
  }
  if (!file_NIM || !file_NIM->IsOpen()) {
    std::cerr << "Error: Could not open file " << NIM_name << std::endl;
    return;
  }
  ofstream ofTSmon(Form("./txt/TSmon_run%d.txt",runN));
  
  TTree *tree_DC  = (TTree*)file_DC->Get("tree");
  TTree *tree_NIM = (TTree*)file_NIM->Get("tree");
  TTree *tree     = new TTree("tree","tree");
  
  Int_t adcSum[64];
  Int_t triggerNumber;
  Int_t triggerTime;
  ULong64_t triggerTimeStamp;
  
  Int_t SYNC_NIM_FLAG;
  Double_t TS_NIM;
  Double_t dTS_NIM;
  Double_t TS_NIM_pre;
  Double_t TS_NIM_Sync;
  Double_t Traw_NIM_L[4];
  Double_t Traw_NIM_T[4];
  Double_t Traw_NIM_TOT[4];

  Double_t TS_DC_0     = 0.0;
  Double_t TS_DC       = 0.0;
  Double_t TS_DC_pre   = 0.0;
  Double_t dTS_DC      = 0.0;
  Double_t TS_diff_DC  = 0.0;
  Double_t dTS_diff_DC = 0.0;
  
  tree_DC->SetBranchAddress("adcSum",           adcSum);
  tree_DC->SetBranchAddress("triggerNumber",    &triggerNumber);
  tree_DC->SetBranchAddress("triggerTime",      &triggerTime);
  tree_DC->SetBranchAddress("triggerTimeStamp", &triggerTimeStamp);
  
  tree_NIM->SetBranchAddress("SYNC_NIM_FLAG",   &SYNC_NIM_FLAG);
  tree_NIM->SetBranchAddress("TS_NIM",          &TS_NIM);
  tree_NIM->SetBranchAddress("dTS_NIM",         &dTS_NIM);
  tree_NIM->SetBranchAddress("TS_NIM_pre",      &TS_NIM_pre);
  tree_NIM->SetBranchAddress("TS_NIM_Sync",     &TS_NIM_Sync);
  tree_NIM->SetBranchAddress("Traw_NIM_L",      &Traw_NIM_L);
  tree_NIM->SetBranchAddress("Traw_NIM_T",      &Traw_NIM_T);
  tree_NIM->SetBranchAddress("Traw_NIM_TOT",    &Traw_NIM_TOT);

  tree->Branch("adcSum",        adcSum, "adcSum[64]/I");
  tree->Branch("triggerNumber", &triggerNumber, "triggerNumber/I");
  tree->Branch("triggerTime",   &triggerTime, "triggerTime/I");
  tree->Branch("triggerTimeStamp", &triggerTimeStamp, "triggerTimeStamp/l");
  tree->Branch("TS_DC",&TS_DC,"TS_DC/D"); // convert ns->sec
  tree->Branch("TS_DC_pre",&TS_DC_pre,"TS_DC_pre/D");
  
  tree->Branch("TS_NIM",&TS_NIM,"TS_NIM/D");
  tree->Branch("dTS_NIM",&dTS_NIM,"dTS_NIM/D");
  tree->Branch("TS_NIM_pre",&TS_NIM_pre,"TS_NIM_pre/D");
  tree->Branch("TS_NIM_Sync",&TS_NIM_Sync,"TS_NIM_Sync/D");
  tree->Branch("SYNC_NIM_FLAG",&SYNC_NIM_FLAG,"SYNC_NIM_FLAG/I");
  tree->Branch("Traw_NIM_L",Traw_NIM_L,"Traw_NIM_L[4]/D");
  tree->Branch("Traw_NIM_T",Traw_NIM_T,"Traw_NIM_T[4]/D");
  tree->Branch("Traw_NIM_TOT",Traw_NIM_TOT,"Traw_NIM_TOT[4]/D");

  tree->Branch("dTS_DC",&dTS_DC,"dTS_DC/D");
  tree->Branch("TS_diff_DC",&TS_diff_DC,"TS_diff_DC/D");
  tree->Branch("dTS_diff_DC",&dTS_diff_DC,"dTS_diff_DC/D");

  TH1F *hTS_NIM = new TH1F("hTS_NIM","hTS_NIM",1000,0,0);
  TH1F *hdTS_NIM = new TH1F("hdTS_NIM","hdTS_NIM",1000,0,0);
  TH1F *hTS_DC = new TH1F("hTS_DC","hTS_DC",1000,0,0);
  TH1F *hdTS_DC = new TH1F("hdTS_DC","hdTS_DC",1000,0,0);
  TH1F *hTS_diff_DC = new TH1F("hTS_diff_DC","hTS_diff_DC",1000,0,0);
  TH1F *hdTS_diff_DC = new TH1F("hdTS_diff_DC","hdTS_diff_DC",1000,0,0);
  
  // segmentation violation
  //Init_DC(tree_DC);
  //Init(tree_NIM);
  //Def_Branch(tree);
  
  int nentries_DC = tree_DC->GetEntries();
  int nentries_NIM = tree_NIM->GetEntries();
  int nentries_Merge = 0;
  cout << "Entries DC : " << nentries_DC << endl;
  cout << "Entries NIM : " << nentries_NIM << endl;
  Long64_t jstart_NIM = 0;
  
  for (Long64_t jentry_DC=0; jentry_DC<nentries_DC;jentry_DC++) {
    //Long64_t ientry_DC = tree_DC->LoadTree(jentry_DC);
    tree_DC->GetEntry(jentry_DC);
    if(jentry_DC == 0){
      TS_DC_0 = triggerTimeStamp * pow(10,-9);
      //cout << triggerTimeStamp << endl;
      //cout << setprecision(15) << TS_DC_0 << endl;
      TS_DC = 0.0;
      dTS_DC = 0.0;
    }
    else{
      TS_DC = triggerTimeStamp * pow(10,-9);
      TS_DC -= TS_DC_0;
      dTS_DC = TS_DC - TS_DC_pre;
      TS_DC_pre = TS_DC;
    }
    //if (ientry_DC < 0) break;
    int ActiveCH = 64;
    for(int ch=0;ch<64;ch++){
      if(!adcSum[ch]){
	ActiveCH--;
      }
    }
    if(ActiveCH >= 59){
      for(Long64_t jentry_NIM=jstart_NIM;jentry_NIM<nentries_NIM;jentry_NIM++){
	//SYNC_NIM_FLAG = 0;
	tree_NIM->GetEntry(jentry_NIM);
	//cout << "jentry DC  : " << jentry_DC  << endl;
	//cout << "jentry NIM : " << jentry_NIM << endl;
	//cout << "TS_NIM : " << TS_NIM << endl;
	//cout << "dTS_NIM : " << dTS_NIM << endl;
	//cout << "Traw_NIM_L : " << Traw_NIM_L << endl;
	
	//Long64_t ientry_NIM = tree_NIM->LoadTree(jentry_NIM);
	//if(ientry_NIM < 0) break;
	if(SYNC_NIM_FLAG == 1){
	  //TS_NIM *= pow(10,9);
	  //dTS_NIM *= pow(10,9);
	  TS_diff_DC = TS_DC - TS_NIM;
	  dTS_diff_DC = dTS_DC - dTS_NIM;
	  //cout << "SYNC_NIM_FLAG : " << SYNC_NIM_FLAG << endl;
	  //cout << "SYNC_NIM_FLAG : " << SYNC_NIM_FLAG << endl;
	  //cout << "jentry in loop : " << jentry_NIM << endl;
	  if(jstart_NIM==0){
	    ofTSmon << setw(6 ) << "NIM No." << ", "
		    << setw(6 ) << "DC  No." << ", "
		    << setw(11) << "TS_NIM" << ", "
		    << setw(11) << "dTS_NIM" << ", "
		    << setw(11) << "TS_DC" << ", "
		    << setw(11) << "dTS_DC" << ", "
		    << setw(11) << "TS_diff" << ", "
		    << setw(11) << "dTS_diff" << endl;
	  }
	  jstart_NIM=jentry_NIM+1;
	  tree->Fill();
	  hTS_NIM->Fill(TS_NIM);
	  hdTS_NIM->Fill(dTS_NIM);
	  hTS_DC->Fill(TS_DC);
	  hdTS_DC->Fill(dTS_DC);
	  hTS_diff_DC->Fill(TS_diff_DC);
	  hdTS_diff_DC->Fill(dTS_diff_DC);
	  ofTSmon << setw(6 ) << jentry_NIM << ", "
		  << setw(6 ) << jentry_DC  << ", "
		  << setw(11) << TS_NIM << ", "
		  << setw(11) << dTS_NIM << ", "
		  << setw(11) << TS_DC << ", "
		  << setw(11) << dTS_DC << ", "
		  << setw(11) << TS_diff_DC << ", "
		  << setw(11) << dTS_diff_DC << endl;
	  
	  break;
	}
      }
    }
  }
  
  nentries_Merge = tree->GetEntries();
  cout << "Entries Merge : " << nentries_Merge << endl;

  outfile->Write();
  outfile->Close();
  ofTSmon.close();
  
  delete outfile;
  delete file_DC;
  delete file_NIM;
}


void EventMatch::TS_Fit(int IP=0)
{
   if (fChain == 0) return;
   
   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   a.resize(0); b.resize(0); chi2.resize(0); ndf.resize(0);
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
     Long64_t ientry = LoadTree(jentry);
     if (ientry < 0) break;
     nb = fChain->GetEntry(jentry);   nbytes += nb;
     int start, end;
     
     // y:TS_diff(= TS_Kal - TS_NIM), x:TS_Kal -> Fit every 10sec
     // Synchronized trigger = 10Hz -> ~ every 100event
     if(jentry % 100 == 0){
       start = jentry;
       end = start + 99;
       if(end > nentries) end = nentries;
       for(int i=start;i<end;i++){
	 LoadTree(i);
	 fChain->GetEntry(i);
	 TS.push_back(TS_Kal[IP]);
	 dTS.push_back((*TS_diff)[IP]);
       }
	 
       // TGraph(point, initial x, initial y)
       TGraph *graph = new TGraph(TS.size(), &TS[0], &dTS[0]);
       TF1 *fitfunc = new TF1("fitfunc", "pol1", static_cast<double>(start), static_cast<double>(end));
       
       graph->Fit(fitfunc, "Q");
       a.push_back(fitfunc->GetParameter(1));
       b.push_back(fitfunc->GetParameter(0));
       chi2.push_back(fitfunc->GetChisquare());
       ndf.push_back(fitfunc->GetNDF());
	 
       TS.resize(0);
       dTS.resize(0);
       delete graph;
     }
   }
   cout << left
	<< setw( 4) << "IP"
        << setw(10) << "Loop"
        << setw(20) << "a"
        << setw(20) << "b"
        << setw(20) << "Chi^2"
        << setw(10) << "ndf" << endl;
   cout << left << setw(4) << IP;
   for (size_t i=0;i<a.size();i++){
     cout << left
	  << setw(10) << i
	  << setw(20) << setprecision(10) << a[i]
	  << setw(20) << setprecision(10) << b[i]
	  << setw(20) << setprecision(10) << chi2[i]
	  << setw(10) << ndf[i] << endl;
   }
}


void EventMatch::TS_Calibration(int IP=0)
{
  if (fChain == 0) return;
  Long64_t nentries = fChain->GetEntriesFast();
  Long64_t nbytes = 0, nb = 0;

  TTree *tree = fChain->GetTree();
  if (!tree) cout << "TTree not found." << endl;  
  
  TFile *newFile = TFile::Open("../ROOT/test_0808/run_154/MSE000154.root","UPDATE");  
  TTree *newTree = new TTree("newTree","newTree");
  newTree->Branch("TS_Kal_calib", TS_Kal_calib, "TS_Kal_calib[12]/D" );
  newTree->Branch("TS_calib_diff",TS_calib_diff,"TS_calib_diff[12]/D");
  //tree->Branch("TS_Kal_calib",TS_Kal_calib,"TS_Kal_calib[12]/D");
  //tree->Branch("TS_calib_diff",TS_calib_diff,"TS_calib_diff[12]/D");
  
  for (Long64_t jentry=0; jentry<nentries;jentry++) {
    Long64_t ientry = LoadTree(jentry);
    if (ientry < 0) break;
    nb = fChain->GetEntry(jentry);   nbytes += nb;
    int N = jentry/100;
    TS_Kal_calib[IP] = TS_Kal[IP] - (a[N] * TS_Kal[IP] + b[N]);
    TS_calib_diff[IP] = TS_Kal_calib[IP] - TS_NIM;
    newTree->Fill();
    //tree->Fill();
    //hTS_calib_diff[IP]->Fill(TS_Kal_calib[IP],TS_calib_diff[IP]);
    /*
    if(jentry%10==0){
      cout << "TS_Kal_calib["<<IP<<"] : "<< TS_Kal_calib[IP] << ", "
	   << "TS_Kal["<<IP<<"] : " <<TS_Kal[IP] << ", "
	   << "a[" << N <<"] : "<< a[N] << ", " << "b["<<N<<"] : "<< b[N] << endl;
	   }*/
  }
  newFile->cd();
  //newTree->Write("",TObject::kOverwrite);
  newTree->Write();
  //tree->Write("",TObject::kOverwrite);
  newFile->Close();  

  /*
  TFile *f1 = TFile::Open("../ROOT/test_0808/run_154/MSE000154.root");
  TH2F *hTS_calib_diff[12];
  TCanvas *c1 = new TCanvas("c1","c1",1200,900);
  c1->Divide(4,3);
  for(int IP=0;IP<IP_max;IP++){
    hTS_calib_diff[IP] = new TH2F(Form("hTS_calib_diff_%d",IP),Form("TS_calib_%d",IP),1000,0,60,1000,-1e-9,1e-9);
    hTS_calib_diff[IP]->Fill(TS_Kal_calib[IP],TS_calib_diff[IP]);
    c1->cd(IP+1);
    hTS_calib_diff[IP]->Draw();
    }*/
}

void EventMatch::All_TS_Calibration(int IP_max=12){
  for(int IP=0;IP<IP_max;IP++){
    TS_Fit(IP);
    TS_Calibration(IP);
  }
  TFile *f1 = TFile::Open("../ROOT/test_0808/run_154/MSE000154.root");
}

