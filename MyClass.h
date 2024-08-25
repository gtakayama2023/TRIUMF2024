//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug  1 22:42:24 2024 by ROOT version 6.28/10
// from TTree tree/tree
// found on file: ../ROOT/test_0730/MSE000004_16.root
//////////////////////////////////////////////////////////

#ifndef MyClass_h
#define MyClass_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class MyClass {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Float_t         TimeStamp;
   Int_t           Traw_L[4];
   Int_t           Traw_T[4];
   Int_t           Traw_TOT[4];
   Int_t           Traw_num[4];
   Float_t         dT_trigger;
   Float_t         T_offset;
   Float_t         T_Sync;

   // List of branches
   TBranch        *b_TimeStamp;   //!
   TBranch        *b_Traw_L;   //!
   TBranch        *b_Traw_T;   //!
   TBranch        *b_Traw_TOT;   //!
   TBranch        *b_Traw_num;   //!
   TBranch        *b_dT_trigger;   //!
   TBranch        *b_T_offset;   //!
   TBranch        *b_T_Sync;   //!

   MyClass(TTree *tree=0);
   virtual ~MyClass();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef MyClass_cxx
MyClass::MyClass(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("../ROOT/test_0730/MSE000004_16.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("../ROOT/test_0730/MSE000004_16.root");
      }
      f->GetObject("tree",tree);

   }
   Init(tree);
}

MyClass::~MyClass()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t MyClass::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t MyClass::LoadTree(Long64_t entry)
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

void MyClass::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("TimeStamp", &TimeStamp, &b_TimeStamp);
   fChain->SetBranchAddress("Traw_L", Traw_L, &b_Traw_L);
   fChain->SetBranchAddress("Traw_T", Traw_T, &b_Traw_T);
   fChain->SetBranchAddress("Traw_TOT", Traw_TOT, &b_Traw_TOT);
   fChain->SetBranchAddress("Traw_num", Traw_num, &b_Traw_num);
   fChain->SetBranchAddress("dT_trigger", &dT_trigger, &b_dT_trigger);
   fChain->SetBranchAddress("T_offset", &T_offset, &b_T_offset);
   fChain->SetBranchAddress("T_Sync", &T_Sync, &b_T_Sync);
   Notify();
}

Bool_t MyClass::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void MyClass::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t MyClass::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef MyClass_cxx
