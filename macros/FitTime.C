void fit_tree_data(int runNo) {
    // ファイルとツリーを開く
  TString ofname = Form("../ROOT/test_0804/run_%d/MSE0000%d_div10.root", runNo, runNo);
    TFile *file = TFile::Open(ofname);
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }
    TTree *tree = (TTree*)file->Get("tree_name");
    if (!tree) {
        std::cerr << "Error: cannot find the tree" << std::endl;
        file->Close();
        return;
    }

    for (int i = 1; i < 3; i++) {
        // ヒストグラムを作成して描画
        TString histName = Form("t0%d", i);
        tree->Draw(Form("T_diff[%d]:TimeStamp[0]>>%s(1e2,0,10,1000,0,0)", i, histName.Data()), "", "");
        TH2F *t0 = (TH2F*)gDirectory->Get(histName);

        // フィット関数を定義してフィット
        TF1 *fit1 = new TF1(Form("fit1_%d", i), "pol1", 0, 10);
        t0->Fit(fit1, "", "", 0, 10);

        // フィットパラメータを取得
        double p0 = fit1->GetParameter(0);
        double p1 = fit1->GetParameter(1);

        // 補正後のプロット
        TString formula = Form("T_diff[%d] - (TimeStamp[0]*%g + %g):TimeStamp[0]", i, p1, p0);
        TString correctedHistName = Form("t01c_%d", i);
        tree->Draw(formula + Form(">>%s(1e2,0,10,1000,0,0)", correctedHistName.Data()), "", "colz");
        TH2F *t01c = (TH2F*)gDirectory->Get(correctedHistName);

        // キャンバスを更新
        gPad->Update();
    }
    // キャンバスを更新
    gPad->Update();

    // ファイルを閉じる
    file->Close();
}
