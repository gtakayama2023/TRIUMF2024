//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Wed Aug 28 18:09:53 2024 by ROOT version 6.28/10
// from TTree tree/64ch Board RP1212 v2
// found on file: run_0094_calib.root
//////////////////////////////////////////////////////////

#ifndef EventMatch_h
#define EventMatch_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class EventMatch {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.
  // Path to file
  TString base_root_path = "/home/kal-dc-ana/EXP/TRIUMF/2024/ROOT";
  TString DC_path   = base_root_path + "/DC_TEST";
  TString NIM_path  = base_root_path + "/KAL_RUN";
  TString out_path  = base_root_path + "/MATCH";
  TString Sync_path = base_root_path + "/MATCH";

  // For Event Matching
  bool FLAG_EVTMATCH;
  bool SYNC_NIM_KEYWORD;
  
  Double_t TS_NIM;
  Double_t dTS_NIM = 0.0;
  Double_t TS_NIM_pre = 0.0;
  Double_t TS_NIM_Sync;
  Double_t Traw_NIM_L[4];
  Double_t Traw_NIM_T[4];
  Double_t Traw_NIM_TOT[4];
  
  Double_t TS_DC_0 = 0.0;
  Double_t TS_DC = 0.0;
  Double_t TS_DC_pre = 0.0;
  Double_t dTS_DC = 0.0;
  Double_t TS_diff_DC = 0.0;
  Double_t dTS_diff_DC = 0.0;

  Double_t TS_DC_calib;
  Double_t TS_diff_DC_calib;

  Double_t DoublePulseWindow = 0.02;

  // For Time Stamp Fitting
  std::vector<double> TS_x;
  std::vector<double> dTS_y;
  std::vector<double> a_DC;
  std::vector<double> b_DC;
  std::vector<double> chi2_DC;
  std::vector<int> ndf_DC;
  
   // Drift Chamber 
   Int_t           boardID;
   Int_t           triggerNumber;
   Int_t           triggerTime;
   ULong64_t       triggerTimeStamp;
   vector<vector<int> > *waveform;
   vector<vector<int> > *hitTime;
   vector<vector<int> > *hitClock;
   Int_t           baseline[64];
   Int_t           adcSum[64];
   Int_t           adcPosSum[64];
   Int_t           tot[64];
   Int_t           hitBit[64];
   Int_t           adc[64][128];
   Int_t           tdc[64][128];
   Int_t           drift_time[64][128];

   // List of branches
   TBranch        *b_boardID;   //!
   TBranch        *b_triggerNumber;   //!
   TBranch        *b_triggerTime;   //!
   TBranch        *b_triggerTimeStamp;   //!
   TBranch        *b_waveform;   //!
   TBranch        *b_hitTime;   //!
   TBranch        *b_hitClock;   //!
   TBranch        *b_baseline;   //!
   TBranch        *b_adcSum;   //!
   TBranch        *b_adcPosSum;   //!
   TBranch        *b_tot;   //!
   TBranch        *b_adc;   //!
   TBranch        *b_tdc;   //!
   TBranch        *b_drift_time;   //!

   EventMatch(TTree *tree=0);
   virtual ~EventMatch();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
  
  virtual void SetBranchAddress_DC(TTree *tree, bool FLAG_EVTMATCH);
  virtual void SetBranchAddress_NIM(TTree *tree, bool FLAG_EVTMATCH);
  virtual void SetBranchAddress_Sync(TTree *tree);
  
  // Event Matching Function for DC & NIM-TDC
  virtual bool SyncCheck_ADCSum();
  virtual void DC_TS_Match(int runN, int subrunN);
  virtual void DC_TS_Fit(int runN);
  virtual void DC_EventMatching(int runN, int subrunN, Double_t accuracy_ns);
  
};

#endif

#ifdef EventMatch_cxx
EventMatch::EventMatch(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
     TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject(Form("%s/run_0091_0000.dat.root",DC_path.Data()));
      if (!f || !f->IsOpen()) {
	f = new TFile(Form("%s/run_0091_0000.dat.root",DC_path.Data()));
      }
      f->GetObject("tree",tree);

   }
   Init(tree);
}

EventMatch::~EventMatch()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t EventMatch::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t EventMatch::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void EventMatch::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   waveform = 0;
   hitTime = 0;
   hitClock = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("boardID", &boardID, &b_boardID);
   fChain->SetBranchAddress("triggerNumber", &triggerNumber, &b_triggerNumber);
   fChain->SetBranchAddress("triggerTime", &triggerTime, &b_triggerTime);
   fChain->SetBranchAddress("triggerTimeStamp", &triggerTimeStamp, &b_triggerTimeStamp);
   fChain->SetBranchAddress("waveform", &waveform, &b_waveform);
   fChain->SetBranchAddress("hitTime", &hitTime, &b_hitTime);
   fChain->SetBranchAddress("hitClock", &hitClock, &b_hitClock);
   fChain->SetBranchAddress("baseline", baseline, &b_baseline);
   fChain->SetBranchAddress("adcSum", adcSum, &b_adcSum);
   fChain->SetBranchAddress("adcPosSum", adcPosSum, &b_adcPosSum);
   fChain->SetBranchAddress("tot", tot, &b_tot);
   fChain->SetBranchAddress("hitBit", hitBit, &b_tot);
   fChain->SetBranchAddress("adc", adc, &b_adc);
   fChain->SetBranchAddress("tdc", tdc, &b_tdc);
   fChain->SetBranchAddress("drift_time", drift_time, &b_drift_time);
   Notify();
}

Bool_t EventMatch::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void EventMatch::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t EventMatch::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
  (void)entry;
   return 1;
}
#endif // #ifdef EventMatch_cxx


// added by S.Ishitani 2024.08.28

void EventMatch::SetBranchAddress_DC(TTree *tree, bool FLAG_EVTMATCH = true)
{
   // Set object pointer
  if(!FLAG_EVTMATCH){
    waveform = 0;
    hitTime = 0;
    hitClock = 0;
  }
  // Set branch addresses and branch pointers
  if (!tree) return;
  //fCurrent = -1;
  //tree->SetMakeClass(1);
  
  if(!FLAG_EVTMATCH){
    tree->SetBranchAddress("boardID", &boardID, &b_boardID);
    tree->SetBranchAddress("waveform", &waveform, &b_waveform);
    tree->SetBranchAddress("hitTime", &hitTime, &b_hitTime);
    tree->SetBranchAddress("hitClock", &hitClock, &b_hitClock);
    tree->SetBranchAddress("baseline", baseline, &b_baseline);
    tree->SetBranchAddress("adcPosSum", adcPosSum, &b_adcPosSum);
    tree->SetBranchAddress("tot", tot, &b_tot);
    tree->SetBranchAddress("hitBit", hitBit, &b_tot);
    tree->SetBranchAddress("adc", adc, &b_adc);
    tree->SetBranchAddress("tdc", tdc, &b_tdc);
    tree->SetBranchAddress("drift_time", drift_time, &b_drift_time);     
  }
   
  tree->SetBranchAddress("triggerNumber", &triggerNumber, &b_triggerNumber);
  tree->SetBranchAddress("triggerTime", &triggerTime, &b_triggerTime);
  tree->SetBranchAddress("triggerTimeStamp", &triggerTimeStamp, &b_triggerTimeStamp);
  tree->SetBranchAddress("adcSum", adcSum, &b_adcSum);
   //Notify();
}

void EventMatch::SetBranchAddress_NIM(TTree *tree, bool FLAG_EVTMATCH = true)
{
  // Set branch addresses and branch pointers
  if (!tree) return;
  //fCurrent = -1;
  //tree->SetMakeClass(1);
  
  if(!FLAG_EVTMATCH){
    
  }
  tree->SetBranchAddress("TS_NIM",&TS_NIM);
  tree->SetBranchAddress("TS_NIM_Sync",&TS_NIM_Sync);
  tree->SetBranchAddress("Traw_NIM_L",&Traw_NIM_L);
  tree->SetBranchAddress("Traw_NIM_T",&Traw_NIM_T);
  tree->SetBranchAddress("Traw_NIM_TOT",&Traw_NIM_TOT);
   //Notify();
}

void EventMatch::SetBranchAddress_Sync(TTree *tree)
{
  tree->SetBranchAddress("TS_NIM",&TS_NIM);
  tree->SetBranchAddress("dTS_NIM",&dTS_NIM);
  tree->SetBranchAddress("TS_DC",&TS_DC);
  tree->SetBranchAddress("dTS_DC",&dTS_DC);
  tree->SetBranchAddress("TS_diff_DC",&TS_diff_DC);
  tree->SetBranchAddress("dTS_diff_DC",&dTS_diff_DC);
}
