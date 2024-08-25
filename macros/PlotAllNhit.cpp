#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TString.h>
#include <TH2.h>
using namespace std;

void Plot_All_Histograms(const char *PDFname="test") {
  //int ThDAC[16] = {15, 7, 11, 3, 13, 5, 9, 1, 0, 8, 4, 12, 2, 10, 6, 14};
  int ThDAC[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    TString baseName = "/Users/ishitanisoshi/kalliope/ROOT/test_0730/MSE0000";
    TString extension = ".root";

    TCanvas *c1 = new TCanvas("c1", "c1", 800, 800);
    c1->Divide(4,4);

    for (int i=0;i<16;i++) {
        TString fileName = baseName;
        if (ThDAC[i]<10){
            fileName+="0";
        }
        fileName+=ThDAC[i];
        fileName+=extension;

        TFile *file = TFile::Open(fileName);
        if (!file || file->IsZombie()) {
            printf("Error opening file %s\n", fileName.Data());
            continue;
        }

        TH2F *hist = (TH2F*)file->Get("hTraw_num_0");
        if(!hist){
	  printf("Error reading histogram from file %s\n", fileName.Data());
	  file->Close();
	  continue;
        }
	hist->SetTitle(Form("Multiplicity : ThDAC = %X", ThDAC[i]));	
	c1->cd(i+1);
	hist->Draw("colz");

	c1->SetLogz();
	c1->Modified();
	c1->Update();
	//file->Close();	
    }
    c1->Update();
    c1->SetLogz();
    TString PDFpath = "/Users/ishitanisoshi/kalliope/pdf/";
    PDFpath += PDFname;
    PDFpath += ".pdf";
    c1->SaveAs(PDFpath);
}


void Data_Check(const char *PDFname="test") {
  //int ThDAC[16] = {15, 7, 11, 3, 13, 5, 9, 1, 0, 8, 4, 12, 2, 10, 6, 14};
    int ThDAC[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    TString baseName = "/Users/ishitanisoshi/kalliope/ROOT/test_0730/MSE0000";
    TString extension = ".root";

    TCanvas *c1 = new TCanvas("c1", "c1", 800, 800);
    c1->Divide(4,4);
    
    for (int i=0;i<16;i++) {
        TString fileName = baseName;
        if (ThDAC[i]<10){
            fileName+="0";
        }
        fileName+=ThDAC[i];
        fileName+=extension;

        TFile *file = TFile::Open(fileName);
        if (!file || file->IsZombie()) {
            printf("Error opening file %s\n", fileName.Data());
            continue;
        }

        TH2F *hist = (TH2F*)file->Get("hTime_from_zero_0");
        if(!hist){
	  printf("Error reading histogram from file %s\n", fileName.Data());
	  file->Close();
	  continue;
        }
	hist->SetTitle(Form("Time : ThDAC = %X", ThDAC[i]));	
	c1->cd(i+1);
	hist->Draw("colz");

	c1->SetLogz();
	c1->Modified();
	c1->Update();
	//file->Close();	
    }
    c1->Update();
    c1->SetLogz();
    TString PDFpath = "/Users/ishitanisoshi/kalliope/pdf/";
    PDFpath += PDFname;
    PDFpath += ".pdf";
    //c1->SaveAs(PDFpath);
}
