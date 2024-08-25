#include <TCanvas.h>
#include <TH2F.h>
#include <TF1.h>
#include <TTree.h>
#include <iostream>
#include <cstdio> // For snprintf

void fitAndDraw() {
    TCanvas *c1 = new TCanvas("c1", "c1", 800, 800);
    c1->Divide(2, 2);

    // First plot and fit
    c1->cd(1);
    TH2F* t01 = new TH2F("t01", "T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
    tree->Draw("T_diff[1]:TimeStamp[0] >> t01(1e2,0,10,1000,0,0)", "", "");
    TF1* f1 = new TF1("f1", "pol1", 0, 10);
    t01->Fit(f1, "", "", 0, 10);
    
    double slope = f1->GetParameter(1); // 線形フィットの傾き
    double intercept = f1->GetParameter(0); // 線形フィットの切片
    double slopeError = f1->GetParError(1); // 傾きのエラー
    double interceptError = f1->GetParError(0); // 切片のエラー

    // Create formula for corrected plot
    char formula[256];
    snprintf(formula, sizeof(formula), "T_diff[1] - (TimeStamp[0] * %.5e + %.5e)", slope, intercept);

    // Draw corrected histogram
    c1->cd(2);
    TH2F* t01c = new TH2F("t01c", "Corrected T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
    tree->Draw(Form("%s:TimeStamp[0] >> t01c(1e2,0,10,1000,0,0)", formula), "", "colz");

    // Second plot and fit
    c1->cd(3);
    TH2F* t02 = new TH2F("t02", "T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
    tree->Draw("T_diff[2]:TimeStamp[0] >> t02(1e2,0,10,1000,0,0)", "", "");
    TF1* f2 = new TF1("f2", "pol1", 0, 10);
    t02->Fit(f2, "", "", 0, 10);
    
    double slope2 = f2->GetParameter(1); // 線形フィットの傾き
    double intercept2 = f2->GetParameter(0); // 線形フィットの切片
    double slopeError2 = f2->GetParError(1); // 傾きのエラー
    double interceptError2 = f2->GetParError(0); // 切片のエラー

    // Create formula for corrected plot
    char formula2[256];
    snprintf(formula2, sizeof(formula2), "T_diff[2] - (TimeStamp[0] * %.5e + %.5e)", slope2, intercept2);

    // Draw corrected histogram
    c1->cd(4);
    TH2F* t02c = new TH2F("t02c", "Corrected T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
    tree->Draw(Form("%s:TimeStamp[0] >> t02c(1e2,0,10,1000,0,0)", formula2), "", "colz");

    // Output fit results
    std::cout << "First fit - Slope: " << slope << " ± " << slopeError
              << ", Intercept: " << intercept << " ± " << interceptError << std::endl;
    std::cout << "Second fit - Slope: " << slope2 << " ± " << slopeError2
              << ", Intercept: " << intercept2 << " ± " << interceptError2 << std::endl;
}
