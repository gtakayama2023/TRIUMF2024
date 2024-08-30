//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun Aug  4 15:37:41 2024 by ROOT version 6.28/10
// from TTree tree/tree
// found on file: ../ROOT/test_0803/run_17/MSE000017.root
//////////////////////////////////////////////////////////

#ifndef Event_Matching_h
#define Event_Matching_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include "vector"

class Event_Matching {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Double_t        TimeStamp[12];
   Int_t           Traw_L[6][32][10];
   Int_t           Traw_T[6][32][10];
   Int_t           Traw_TOT[6][32][10];
   Int_t           Traw_num[6][32];
   Double_t        dTimeStamp[12];
   Double_t        T_diff[12];
   vector<double>  *dT_diff;
   Double_t        T_offset[12];
   Double_t        T_Sync[12];

   // List of branches
   TBranch        *b_TimeStamp;   //!
   TBranch        *b_Traw_L;   //!
   TBranch        *b_Traw_T;   //!
   TBranch        *b_Traw_TOT;   //!
   TBranch        *b_Traw_num;   //!
   TBranch        *b_dTimeStamp;   //!
   TBranch        *b_T_diff;   //!
   TBranch        *b_dT_diff;   //!
   TBranch        *b_T_offset;   //!
   TBranch        *b_T_Sync;   //!

  std::vector<double> a;
  std::vector<double> b;
  
   Event_Matching(TTree *tree=0);
   virtual ~Event_Matching();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     TS_Calibration();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef Event_Matching_cxx
Event_Matching::Event_Matching(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("../ROOT/test_0803/run_17/MSE000017.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("../ROOT/test_0803/run_17/MSE000017.root");
      }
      f->GetObject("tree",tree);

   }
   Init(tree);
}

Event_Matching::~Event_Matching()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t Event_Matching::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t Event_Matching::LoadTree(Long64_t entry)
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

void Event_Matching::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   dT_diff = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("TimeStamp", TimeStamp, &b_TimeStamp);
   fChain->SetBranchAddress("Traw_L", Traw_L, &b_Traw_L);
   fChain->SetBranchAddress("Traw_T", Traw_T, &b_Traw_T);
   fChain->SetBranchAddress("Traw_TOT", Traw_TOT, &b_Traw_TOT);
   fChain->SetBranchAddress("Traw_num", Traw_num, &b_Traw_num);
   fChain->SetBranchAddress("dTimeStamp", dTimeStamp, &b_dTimeStamp);
   fChain->SetBranchAddress("T_diff", T_diff, &b_T_diff);
   fChain->SetBranchAddress("dT_diff", &dT_diff, &b_dT_diff);
   fChain->SetBranchAddress("T_offset", T_offset, &b_T_offset);
   fChain->SetBranchAddress("T_Sync", T_Sync, &b_T_Sync);
   Notify();
}

Bool_t Event_Matching::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void Event_Matching::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
/*
Int_t Event_Matching::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
   }*/
#endif // #ifdef Event_Matching_cxx
