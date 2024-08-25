{
  TCanvas *c1 =new TCanvas("c1","c1",800,800);
  c1->Divide(2,2);
  c1->cd(1);
  tree->Draw("T_diff[1]:TimeStamp[0]>>t01(1e2,0,10,1000,0,0)","","");
  TF1* f1 = new TF1("f1", "pol1", 0, 10);
  t01->Fit(f1, "", "", 0, 10);
  double slope = f1->GetParameter(1); // 線形フィットの傾き
  double intercept = f1->GetParameter(0); // 線形フィットの切片
  
  // フィットパラメータのエラーを取得
  double slopeError = f1->GetParError(1); // 傾きのエラー
  double interceptError = f1->GetParError(0); // 切片のエラー
  c1->cd(2);
  char formula[256];
  snprintf(formula, "T_diff[1] - (TimeStamp[0] * %.5e + %.5e)", slope, intercept);
  
  // 新しいヒストグラムを描画
  TH2F* t01c = new TH2F("t01c", "Corrected T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
  tree->Draw(Form("%s:TimeStamp[0] >> t01c(1e2,0,10,1000,0,0)", formula), "", "colz");

  c1->cd(3);
  tree->Draw("T_diff[2]:TimeStamp[0]>>t02(1e2,0,10,1000,0,0)","","");
  TF1* f2 = new TF1("f2", "pol1", 0, 10);
  t02->Fit(f2, "", "", 0, 10);
  double slope2 = f2->GetParameter(1); // 線形フィットの傾き
  double intercept2 = f2->GetParameter(0); // 線形フィットの切片
  
  // フィットパラメータのエラーを取得
  double slopeError2 = f2->GetParError(1); // 傾きのエラー
  double interceptError2 = f2->GetParError(0); // 切片のエラー
  c1->cd(4);
  char formula2[256];
  snprintf(formula2, "T_diff[2] - (TimeStamp[0] * %.5e + %.5e)", slope2, intercept2);
  
  // 新しいヒストグラムを描画
  TH2F* t02c = new TH2F("t02c", "Corrected T_diff vs TimeStamp", 100, 0, 10, 1000, 0, 0);
  tree->Draw(Form("%s:TimeStamp[0] >> t02c(1e2,0,10,1000,0,0)", formula), "", "colz");

}
