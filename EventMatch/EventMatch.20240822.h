//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun Aug 11 19:49:38 2024 by ROOT version 6.28/10
// from TTree tree/tree
// found on file: ../ROOT/test_0808/run_154/MSE000154.root
//////////////////////////////////////////////////////////

#ifndef EventMatch_h
#define EventMatch_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TH2.h>

// Header file for the classes stored in the TTree if any.
#include <vector>
#include <iostream>

using namespace std;

class EventMatch {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Double_t        TS_NIM;
   Double_t        dTS_NIM;
   Double_t        TS_NIM_pre;
   Double_t        TS_NIM_Sync;
   Double_t        TS_Kal[12];
   Double_t        dTS_Kal[12];
   Double_t        TS_Kal_pre[12];
   Double_t        TS_Kal_Sync[12];
   vector<double>  *TS_diff;
   vector<double>  *dTS_diff;
   Double_t        Traw_NIM_L[4];
  Double_t        Traw_NIM_T[4];
  Double_t        Traw_NIM_TOT[4];
   Int_t           Traw_Kal_L[6][32][10];
   Int_t           Traw_Kal_T[6][32][10];
   Int_t           Traw_Kal_TOT[6][32][10];
   Int_t           Traw_Kal_num[6][32];
   Int_t           Fiber_L[2][2][2][64];
   Int_t           Fiber_T[2][2][2][64];
   Int_t           Fiber_TOT[2][2][2][64];
   Int_t           Fiber_num[2][2][2];
   Double_t           TS_Kal_calib[12];
   Double_t           TS_calib_diff[12];

   // List of branches
   TBranch        *b_TS_NIM;   //!
   TBranch        *b_dTS_NIM;   //!
   TBranch        *b_TS_NIM_pre;   //!
   TBranch        *b_TS_NIM_Sync;   //!
   TBranch        *b_TS_Kal;   //!
   TBranch        *b_dTS_Kal;   //!
   TBranch        *b_TS_Kal_pre;   //!
   TBranch        *b_TS_Kal_Sync;   //!
   TBranch        *b_TS_diff;   //!
   TBranch        *b_dTS_diff;   //!
   TBranch        *b_Traw_NIM_L;   //!
   TBranch        *b_Traw_NIM_T;   //!
   TBranch        *b_Traw_Kal_L;   //!
   TBranch        *b_Traw_Kal_T;   //!
   TBranch        *b_Traw_Kal_TOT;   //!
   TBranch        *b_Traw_Kal_num;   //!
   TBranch        *b_Fiber_L;   //!
   TBranch        *b_Fiber_T;   //!
   TBranch        *b_Fiber_TOT;   //!
   TBranch        *b_Fiber_num;   //!
  //TBranch        *b_TS_Kal_calib;   //!
  //TBranch        *b_TS_calib_diff;   //!

  // use in TS_Fit and TS_calibration
  std::vector<double> TS;
  std::vector<double> dTS;
  std::vector<double> a;
  std::vector<double> b;
  std::vector<double> chi2;
  std::vector<double> ndf;

  TH2F *hTS_calib_diff[12];
  
  EventMatch(TTree *tree=0);
  EventMatch(Int_t runN);
  virtual ~EventMatch();
  virtual Int_t    Cut(Long64_t entry);
  virtual Int_t    GetEntry(Long64_t entry);
  virtual Long64_t LoadTree(Long64_t entry);
  virtual void     Init(TTree *tree);
  virtual void     DC_TS_Match(int runN, int subrunN);
  virtual void     DC_TS_Fit(int runN, int subrunN);
  virtual void     TS_Fit(int IP);
  virtual void     TS_Calibration(int IP);
  virtual void     All_TS_Calibration(int IP_max);
  //virtual void     GetEnrtySameTS(Double_t timeInSecond, Double_t accuracyInNanoSecond);
  // GetEntrySameTS cause segmentation violation??
  virtual Bool_t   Notify();
  virtual void     Show(Long64_t entry = -1);


  // Drift Chamber
  Int_t triggerNumber;
  Int_t triggerTime;
  ULong64_t triggerTimeStamp;
  Int_t adcSum[64];
  /*
  TBranch *b_triggerNumber;    //!
  TBranch *b_triggerTime;      //!
  TBranch *b_triggerTimeStamp; //!
  TBranch *b_adcSum;           //!
  */
  virtual void Init_DC(TTree *tree);

  /*
  Int_t SYNC_NIM_FLAG;
  
  Int_t boardID = 0;
  Int_t fTriggerNumber;
  Int_t fTriggerTime;
  ULong64_t fTriggerTimeStamp;
   
  //Int_t adcSum[64];
  Int_t adcPosSum[64]; // only for Raw mode
  Int_t tot[64];
  Int_t hitBit[64];
  */
  virtual void Def_Branch(TTree *tree);
  
};

#endif

#ifdef EventMatch_cxx
EventMatch::EventMatch(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("../ROOT/test_0808/run_154/MSE000154.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("../ROOT/test_0808/run_154/MSE000154.root");
      }
      f->GetObject("tree",tree);

   }
   Init(tree);
}

EventMatch::EventMatch(Int_t runN)
{
  TString rootfile = Form("../ROOT/test_0808/run_%d/MSE000%d.root", runN, runN);
  cout << rootfile << endl;
  TTree *tree;
  TFile *f = (TFile *)gROOT->GetListOfFiles()->FindObject(rootfile);
  if (!f || !f->IsOpen())
    {
      f = new TFile(rootfile);
    }
  f->GetObject("tree", tree);
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
   TS_diff = 0;
   dTS_diff = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("TS_NIM", &TS_NIM, &b_TS_NIM);
   fChain->SetBranchAddress("dTS_NIM", &dTS_NIM, &b_dTS_NIM);
   fChain->SetBranchAddress("TS_NIM_pre", &TS_NIM_pre, &b_TS_NIM_pre);
   fChain->SetBranchAddress("TS_NIM_Sync", &TS_NIM_Sync, &b_TS_NIM_Sync);
   fChain->SetBranchAddress("TS_Kal", TS_Kal, &b_TS_Kal);
   fChain->SetBranchAddress("dTS_Kal", dTS_Kal, &b_dTS_Kal);
   fChain->SetBranchAddress("TS_Kal_pre", TS_Kal_pre, &b_TS_Kal_pre);
   fChain->SetBranchAddress("TS_Kal_Sync", TS_Kal_Sync, &b_TS_Kal_Sync);
   fChain->SetBranchAddress("TS_diff", &TS_diff, &b_TS_diff);
   fChain->SetBranchAddress("dTS_diff", &dTS_diff, &b_dTS_diff);
   fChain->SetBranchAddress("Traw_NIM_L", Traw_NIM_L, &b_Traw_NIM_L);
   fChain->SetBranchAddress("Traw_NIM_T", Traw_NIM_T, &b_Traw_NIM_T);
   fChain->SetBranchAddress("Traw_Kal_L", Traw_Kal_L, &b_Traw_Kal_L);
   fChain->SetBranchAddress("Traw_Kal_T", Traw_Kal_T, &b_Traw_Kal_T);
   fChain->SetBranchAddress("Traw_Kal_TOT", Traw_Kal_TOT, &b_Traw_Kal_TOT);
   fChain->SetBranchAddress("Traw_Kal_num", Traw_Kal_num, &b_Traw_Kal_num);
   fChain->SetBranchAddress("Fiber_L", Fiber_L, &b_Fiber_L);
   fChain->SetBranchAddress("Fiber_T", Fiber_T, &b_Fiber_T);
   fChain->SetBranchAddress("Fiber_TOT", Fiber_TOT, &b_Fiber_TOT);
   fChain->SetBranchAddress("Fiber_num", Fiber_num, &b_Fiber_num);
   Notify();
}

void EventMatch::Init_DC(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
  //TS_diff = 0;
  //dTS_diff = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("triggerNumber", &triggerNumber);
   fChain->SetBranchAddress("triggerTime", &triggerTime);
   fChain->SetBranchAddress("triggerTimeStamp", &triggerTimeStamp);
   fChain->SetBranchAddress("adcSum", adcSum);

   Notify();
}

void EventMatch::Def_Branch(TTree *tree){
  if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);
   /*
   tree->Branch("TS_NIM",&TS_NIM,"TS_NIM/D");
   tree->Branch("dTS_NIM",&dTS_NIM,"dTS_NIM/D");
   tree->Branch("TS_NIM_pre",&TS_NIM_pre,"TS_NIM_pre/D");
   tree->Branch("TS_NIM_Sync",&TS_NIM_Sync,"TS_NIM_Sync/D");
   tree->Branch("SYNC_NIM_FLAG",&SYNC_NIM_FLAG,"SYNC_NIM_FLAG/I");
   
   tree->Branch("TS_diff",&TS_diff);
   tree->Branch("dTS_diff",&dTS_diff);

   //===== Rawdata =====
   tree->Branch("Traw_NIM_L",Traw_NIM_L,"Traw_NIM_L[4]/D");
   tree->Branch("Traw_NIM_T",Traw_NIM_T,"Traw_NIM_T[4]/D");
   tree->Branch("Traw_NIM_TOT",Traw_NIM_TOT,"Traw_NIM_TOT[4]/D");
   
   //===== DC ======
   tree->Branch("boardID", &boardID, "boardID/I");                            // Raw, Sup
   tree->Branch("triggerNumber", &triggerNumber, "triggerNumber/I");          // Raw, Sup
   tree->Branch("triggerTime", &triggerTime, "triggerTime/I");                // Raw, Sup
   tree->Branch("triggerTimeStamp", &triggerTimeStamp, "triggerTimeStamp/l"); // Raw, Sup
   tree->Branch("adcSum", adcSum, "adcSum[64]/I");                   // Raw
   tree->Branch("adcPosSum", adcPosSum, "adcPosSum[64]/I");          // Raw, Sup
   tree->Branch("tot", tot, "tot[64]/I");                            //  - , Sup
   tree->Branch("hitBit", hitBit, "hitBit[64]/I");                      //  - , Sup
   */
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
