#define EventMatch_cxx
#include "EventMatch.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <vector>

#include <TF1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TStyle.h>
#include <TCanvas.h>

// MakeClass Basic Function : Event Loop
void EventMatch::Loop()
{
   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;
      // if (Cut(ientry) < 0) continue;
   }
}

// Q: How can I use adcSum[i] as a variable
bool EventMatch::SyncCheck_ADCSum()
{
  int ActiveCH = 64;
  for(int ch=0;ch<64;ch++){
    if(!adcSum[ch]) ActiveCH--;
  }
  if(ActiveCH >= 59) return true;
  else return false;
}

void EventMatch::DC_TS_Match(int runN=0, int subrunN=0)
{
  // set path to rootfile
  TString DC_name = Form("%s/run_%04d/run_%04d_%04d.dat.root", DC_path.Data(), runN, runN, subrunN);
  TString NIM_name = Form("%s/run_%d/MSE0000%d.root", NIM_path.Data(), runN, runN);
  TString out_name = Form("%s/run_%d/run_%04d_Sync.root", out_path.Data(), runN, runN);
  TFile *file_DC = TFile::Open(DC_name);
  TFile *file_NIM = TFile::Open(NIM_name);
  TFile *outfile = new TFile(out_name,"RECREATE");

  // Check File Exsit & Open
  if (!file_DC || !file_DC->IsOpen()) {
    std::cerr << "Error: Could not open file " << DC_name << std::endl;
    return;
  }
  if (!file_NIM || !file_NIM->IsOpen()) {
    std::cerr << "Error: Could not open file " << NIM_name << std::endl;
    return;
  }

  // Time Stamp Monitor : dump in txt file
  ofstream ofTSmon(Form("./txt/TSmon_run%d.txt",runN));

  // Get tree to read & Make new tree to write
  TTree *tree_DC = (TTree*)file_DC->Get("tree");
  TTree *tree_NIM = (TTree*)file_NIM->Get("tree");
  TTree *tree = new TTree("tree","tree");

  // SetBranchAddress
  bool FLAG_EVTMATCH = true;
  SetBranchAddress_DC(tree_DC, FLAG_EVTMATCH);  // segmentation violation may be occurred
  SetBranchAddress_NIM(tree_NIM, FLAG_EVTMATCH); // segmentation violation may be occurred
  
  // tree->Branch
  tree->Branch("dTS_DC",&dTS_DC,"dTS_DC/D");
  tree->Branch("TS_diff_DC",&TS_diff_DC,"TS_diff_DC/D");
  tree->Branch("dTS_diff_DC",&dTS_diff_DC,"dTS_diff_DC/D");

  // Define Histgrams
  TH1F *hTS_NIM = new TH1F("hTS_NIM","hTS_NIM",1000,0,0);
  TH1F *hdTS_NIM = new TH1F("hdTS_NIM","hdTS_NIM",1000,0,0);
  TH1F *hTS_DC = new TH1F("hTS_DC","hTS_DC",1000,0,0);
  TH1F *hdTS_DC = new TH1F("hdTS_DC","hdTS_DC",1000,0,0);
  TH1F *hTS_diff_DC = new TH1F("hTS_diff_DC","hTS_diff_DC",1000,0,0);
  TH1F *hdTS_diff_DC = new TH1F("hdTS_diff_DC","hdTS_diff_DC",1000,0,0);
  
  // Get Entries -> Event Loop 
  int nentries_DC = tree_DC->GetEntries();
  int nentries_NIM = tree_NIM->GetEntries();
  int nentries_Sync = 0;  
  cout << "Entries DC : " << nentries_DC << endl;
  cout << "Entries NIM : " << nentries_NIM << endl;

  // use as an inital Entry in Event Loop : NIM-TDC
  Long64_t jstart_NIM = 0;

  // Event Loop : read DC tree
  for (Long64_t jentry_DC=0; jentry_DC<nentries_DC;jentry_DC++) {
    tree_DC->GetEntry(jentry_DC);
    
    // Initialize
    if(jentry_DC == 0){
      TS_DC_0 = triggerTimeStamp * pow(10,-9);
      TS_DC = 0.0;
      dTS_DC = 0.0;
    }
    // 
    else{
      TS_DC = triggerTimeStamp * pow(10,-9);
      TS_DC -= TS_DC_0;
      dTS_DC = TS_DC - TS_DC_pre;
      TS_DC_pre = TS_DC;
    }
    
    // Select Sync.Trig. Event by adcSum.
    // Sync.Trig. is input into NIM-in and test-in
    // test-in makes Analog signal for ALL CH (60ch?),
    // so Sync.Trig. can be detected by counting ActiveCH (= adcSum has value)
    int ActiveCH = 64;
    for(int ch=0;ch<64;ch++){
      if(!adcSum[ch]) ActiveCH--;
    }

    // Event Loop : read NIM-TDC tree
    // Found DC Sync.Trig. -> Find NIM-TDC Sync.Trig.
    if(ActiveCH >= 59){
      for(Long64_t jentry_NIM=jstart_NIM;jentry_NIM<nentries_NIM;jentry_NIM++){
        tree_NIM->GetEntry(jentry_NIM);

	// Sync.Trig. for NIM-TDC is input into trigger & IN1
	// if NIM signla is detected in IN1, Keyword is written in data packet
	// So, Sync.Trig. can be detected by Keyword
	if(SYNC_NIM_KEYWORD){
	  
	  // But Missing Trigger happens especially for DC (> NIM-TDC > Kalliope)
	  // So, time interval from previous Sync.Trig. is used to prevent Event Mismatch
          dTS_NIM = TS_NIM - TS_NIM_pre;
          TS_diff_DC = TS_DC - TS_NIM;
          dTS_diff_DC = dTS_DC - dTS_NIM;
          TS_NIM_pre = TS_NIM;
	  
	  // Missing Trigger hardly happen for 1st trigger( % ~negligible)
	  // Difference in the time interval between DC and NIM-TDC must be ~0sec for the same event
	  // the difference is larger than double pulse interval if missing trigger happened in DC
	  // in the case, skip the NIM-TDC event and read the next event
	  // -> resolve missing trigger
          if(jentry_DC != 0 && dTS_diff_DC >DoublePulseWindow) continue;

	  // Dump Title of ofstream TimeStamp Monitor
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

	  // In the next NIM-TDC event loop, tree should be read from the next entry
	  jstart_NIM=jentry_NIM+1;

	  // Fill in tree & histograms
          tree->Fill();
          hTS_NIM->Fill(TS_NIM);
          hdTS_NIM->Fill(dTS_NIM);
          hTS_DC->Fill(TS_DC);
          hdTS_DC->Fill(dTS_DC);
          hTS_diff_DC->Fill(TS_diff_DC);
          hdTS_diff_DC->Fill(dTS_diff_DC);
	  
	  // Dump TimeStamp to ofstream TimeStamp Monitor
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
  nentries_Sync = tree->GetEntries();
  cout << "Entries Sync : " << nentries_Sync << endl;

  outfile->Write();
  outfile->Close();
  ofTSmon.close();

  delete outfile;
  delete file_DC;
  delete file_NIM;
}


void EventMatch::DC_TS_Fit(int runN=0)
{
  // Open the rootfile which was generated by the previous function, DC_TS_Match
  TString Sync_name = Form("%s/run_%d/run_%04d_Sync.root", Sync_path.Data(), runN, runN);
  TFile *Sync_file = TFile::Open(Sync_name);
  TTree *tree_Sync = (TTree*)Sync_file->Get("tree");
  if (!Sync_file || !Sync_file->IsOpen()) {
    std::cerr << "Error: Could not open file " << Sync_name << std::endl;
    return;
  }
  // dump fit parameters in txt file
  ofstream ofPara(Form("./txt/TS_Fit_para_run%d.txt",runN));

  Long64_t nentries = tree_Sync->GetEntries();
  
  SetBranchAddress_Sync(tree_Sync);

  // initialize vectors: fit parameters
  a_DC.resize(0); b_DC.resize(0); chi2_DC.resize(0); ndf_DC.resize(0);

  // Event Loop
  for (Long64_t jentry=0; jentry<nentries;jentry++) {
    int start, end;    
    // Synchronized trigger must be calibrated every 10sec for 100ns precision
    // Synchronized trigger is ~10Hz -> every 100event
    if(jentry % 100 == 0){
      start = jentry;
      end = start + 100;
      
      if(end > nentries) end = nentries;

      // Event Loop : Entry=start ~ end (100events)
      for(int i=start;i<end;i++){
        tree_Sync->GetEntry(i);
        TS_x.push_back(TS_DC);
        dTS_y.push_back(TS_diff_DC);
      }
      
      // Fit TS_diff_DC vs TS_DC
      TGraph *graph = new TGraph(TS_x.size(), &TS_x[0], &dTS_y[0]);
      TF1 *fitfunc = new TF1("fitfunc", "pol1", static_cast<double>(start), static_cast<double>(end));      
      graph->Fit(fitfunc, "Q");
      a_DC.push_back(fitfunc->GetParameter(1));
      b_DC.push_back(fitfunc->GetParameter(0));
      chi2_DC.push_back(fitfunc->GetChisquare());
      ndf_DC.push_back(fitfunc->GetNDF());

      // Delete vector to be fit
      TS_x.resize(0);
      dTS_y.resize(0);
      delete graph;
    }
  }

  // dump the fitting parameters in txt file
  ofPara << Form("TS_diff_DC vs TS_DC Fit linear function Parameters run%d",runN) << endl;
  ofPara << setw( 6) << "Loop"
	 << setw(14) << "a"
	 << setw(14) << "b"
	 << setw(14) << "Chi^2"
	 << setw( 6) << "ndf" << endl;
  for (size_t i=0;i<a_DC.size();i++){
    ofPara << setw( 6) << i
	   << setw(14) << scientific << setprecision(3) << a_DC[i]
	   << setw(14) << scientific << setprecision(3) << b_DC[i]
	   << setw(14) << scientific << setprecision(3) << chi2_DC[i]
	   << setw( 6) << ndf_DC[i] << endl;
  }
}


void EventMatch::DC_EventMatching(int runN=0, int subrunN=0, Double_t accuracy_ns=100.0)
{
  // convert Event Matching accuracy unit:ns to second
  Double_t accuracy_sec = accuracy_ns * 1e-9;

  // Open & Create Files
  TString DC_name = Form("%s/run_%04d/run_%04d_%04d.dat.root", DC_path.Data(), runN, runN, subrunN);
  TString NIM_name = Form("%s/run_%d/MSE0000%d.root", NIM_path.Data(), runN, runN);
  TString out_name = Form("%s/run_%04d/run_%04d_calib.root", out_path.Data(), runN, runN);
  
  TFile *file_DC = TFile::Open(DC_name,"UPDATE");
  TFile *file_NIM = TFile::Open(NIM_name);
  TFile *outfile = new TFile(out_name,"RECREATE");
  
  if (!file_DC || !file_DC->IsOpen()) {
    std::cerr << "Error: Could not open file " << DC_name << std::endl;
    return;
  }
  if (!file_NIM || !file_NIM->IsOpen()) {
    std::cerr << "Error: Could not open file " << NIM_name << std::endl;
    return;
  }
  
  // Read & Create new tree
  TTree *tree_DC = (TTree*)file_DC->Get("tree");
  TTree *tree_NIM = (TTree*)file_NIM->Get("tree");
  TTree *tree_calib = new TTree("tree_calib","tree_calib");
  
  SetBranchAddress_NIM(tree_NIM, true);

  // Define new Branch
  tree_calib->Branch("Traw_NIM_L",&Traw_NIM_L,"Traw_NIM_L[4]/D");
  tree_calib->Branch("Traw_NIM_T",&Traw_NIM_T,"Traw_NIM_T[4]/D");
  tree_calib->Branch("Traw_NIM_TOT",&Traw_NIM_TOT,"Traw_NIM_TOT[4]/D");
  
  tree_calib->Branch("TS_diff_DC_calib",&TS_diff_DC_calib,"TS_diff_DC_calib/D");
  tree_calib->Branch("TS_DC",&TS_DC,"TS_DC/D"); // convert ns->sec
  tree_calib->Branch("TS_DC_pre",&TS_DC_pre,"TS_DC_pre/D");
  tree_calib->Branch("dTS_DC",&dTS_DC,"dTS_DC/D");
  tree_calib->Branch("TS_DC_calib",&TS_DC_calib,"TS_DC_calib/D");
  tree_calib->Branch("TS_NIM",&TS_NIM,"TS_NIM/D");
  tree_calib->Branch("TS_NIM_Sync",&TS_NIM_Sync,"TS_NIM_Sync/D");


  // Check Entries of DC & NIM-TDC
  int nentries_DC = tree_DC->GetEntries();
  int nentries_NIM = tree_NIM->GetEntries();
  cout << "Entries DC : " << nentries_DC << endl;
  cout << "Entries NIM : " << nentries_NIM << endl;

  int N_Sync_DC = 0;
  int jstart_NIM = 0;


  // Event Loop : DC Time Stamp Calibration
  for (Long64_t jentry_DC=0; jentry_DC<nentries_DC;jentry_DC++) {
    tree_DC->GetEntry(jentry_DC);
    
    // Initialize
    if(jentry_DC == 0){
      TS_DC_0 = triggerTimeStamp * pow(10,-9);
      TS_DC = 0.0;
      dTS_DC = 0.0;
    }
    else{
      TS_DC = triggerTimeStamp * pow(10,-9);
      TS_DC -= TS_DC_0;
      dTS_DC = TS_DC - TS_DC_pre;
      TS_DC_pre = TS_DC;
    }
    // Select Sync.Trig. by ADCSum
    int ActiveCH = 64;
    for(int ch=0;ch<64;ch++){
      if(!adcSum[ch]) ActiveCH--;
    }
    if(ActiveCH >= 59) N_Sync_DC++;

    // Calibrate DC Time Stamp every 100 Sync.Trig.
    int N = N_Sync_DC/100;
    TS_DC_calib = TS_DC - (a_DC[N] * TS_DC + b_DC[N]);

    // Calculate Time Difference between NIM-TDC Time Stamp and the DC calibrated
    TS_diff_DC_calib = TS_DC_calib - TS_NIM;
    
    // Event Loop : Event matching using NIM-TDC Time Stamp and the DC calibrated
    for(Long64_t jentry_NIM=jstart_NIM;jentry_NIM<nentries_NIM;jentry_NIM++){
      tree_NIM->GetEntry(jentry_NIM);
      TS_diff_DC_calib = TS_DC_calib - TS_NIM;

      // Search Entry which has consistent Time Stamp with the DC Calibrated one
      if(TS_diff_DC_calib < accuracy_sec){
        //nentries_calib++;
	jstart_NIM=jentry_NIM+1;
        tree_calib->Fill();
        break;
      }
    }
  }

  // CloneTree kepps DC macros available 
  TTree *tree = tree_DC->CloneTree();
  
  int nentries_calib = tree_calib->GetEntries();
  if(nentries_calib == nentries_DC){
    cout << "Entries Match : DC & calib, tree_calib Added as Friend tree in " << DC_name << endl;
    tree->AddFriend(tree_calib);
  }
  else{
    cout << "Error : Entries Mismatch !!!!!" << endl;
    cout << "DC : " << nentries_DC << ", Calib : " << nentries_calib << endl;
  }
  
  outfile->Write();
  outfile->Close();
  file_DC->Write();
  file_DC->Close();
  delete file_DC;
  delete file_NIM;
  delete outfile;
}
