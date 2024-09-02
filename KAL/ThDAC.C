#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TMultiGraph.h>
#include <TFile.h>
#include <TH2D.h>
#include <TStyle.h>

void ThDAC() {
    const int maxTH = 15;  // 最大TH値を指定（例：15）
    const int maxCH = 32;  // 最大チャンネル数
    const int maxIP = 2;   // IPの数を仮定（例：2）

    std::vector<std::vector<TGraph*>> graphs(maxIP, std::vector<TGraph*>(maxCH));
    std::vector<TH2D*> histograms(maxIP);

    // グラフの準備
    for (int ip = 0; ip < maxIP; ip++) {
        for (int ch = 0; ch < maxCH; ch++) {
            graphs[ip][ch] = new TGraph();
            graphs[ip][ch]->SetTitle(Form("IP %d, Channel %d", ip + 1, ch));
        }

        histograms[ip] = new TH2D(Form("hist_IP%d", ip + 1),
                                  Form("TH Dependence for IP %d;Channel;TH;Num", ip + 1),
                                  maxCH, 0, maxCH,
                                  maxTH + 1, 0, maxTH + 1);
    }

    // CSVファイルの読み込み
    for (int th = 0; th <= maxTH; th++) {
        std::ifstream infile(Form("./txt/NOISE/MSE000001_%02d.csv", th));
        if (!infile.is_open()) {
            std::cerr << "Failed to open file: " << Form("./txt/NOISE/MSE000001_%02d.csv", th) << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            int ip;
            char comma;
            ss >> ip >> comma;

            ip -= 1; // IPのインデックスを0から始める
            for (int ch = 0; ch < maxCH; ch++) {
                double num;
                ss >> num >> comma;
                graphs[ip][ch]->SetPoint(th, th, num);
                if (num == 0) num = 1e-10; // ゼロを避ける
                histograms[ip]->SetBinContent(ch + 1, th, num);
            }
        }
        infile.close();
    }

    // ROOTファイルに保存
    TFile *file = new TFile("../ROOT/NOISE/MSE000001.root", "RECREATE");

    // キャンバスとマルチグラフの作成
    for (int ip = 0; ip < maxIP; ip++) {
        TCanvas* c1 = new TCanvas(Form("c1_IP%d", ip + 1), Form("TH Dependence for IP %d", ip + 1), 1200, 800);
        c1->Divide(8, 4);  // 8x4のグリッドに分割

        for (int ch = 0; ch < maxCH; ch++) {
            c1->cd(ch+1);
            TMultiGraph *mg = new TMultiGraph();
            mg->Add(graphs[ip][ch]);
            mg->Draw("AL");
            mg->SetTitle(Form("Channel %d;TH;Num", ch));
        }

        c1->Update();
        c1->Write();
        delete c1;
    }

    // 3Dヒストグラムのキャンバス作成
    for (int ip = 0; ip < maxIP; ip++) {
        TCanvas* c2 = new TCanvas(Form("c2_IP%d_Hist", ip + 1), Form("3D TH Dependence for IP %d", ip + 1), 1200, 800);
        gStyle->SetOptStat(0);  // スタット表示を無効化

        histograms[ip]->SetMinimum(1e0);   // Z軸の最小値（対数スケールに適した値）
        histograms[ip]->SetMaximum(1e5);   // Z軸の最大値（対数スケールに適した値）
        histograms[ip]->GetZaxis()->SetMoreLogLabels();  // Z軸に対数目盛りを表示
        histograms[ip]->GetZaxis()->SetTitle("Num");
        histograms[ip]->Draw("COLZ");
				gPad -> SetLogz(1);

        c2->Update();
        c2->Write();
        delete c2;
    }
		
    // グラフを保存
    for (int ip = 0; ip < maxIP; ip++) {
        for (int ch = 0; ch < maxCH; ch++) {
            graphs[ip][ch]->Write(Form("IP%d_Channel%d", ip + 1, ch));
        }
        histograms[ip]->Write(); // ヒストグラムを保存
    }

    file->Close();

    // 空のHTMLファイルを作成
    std::ofstream htmlfile("../ROOT/NOISE/MSE000001.html");
    if (htmlfile.is_open()) {
        htmlfile << "<!DOCTYPE html>\n<html>\n<head>\n<title>MSE000001 Report</title>\n</head>\n<body>\n";
        htmlfile << "<h1>MSE000001 Report</h1>\n";
        htmlfile << "<p>Graphs and histograms are saved in ROOT file.</p>\n";
        htmlfile << "<p><a href='MSE000001_IP1.pdf'>IP1 Canvas</a></p>\n";
        htmlfile << "<p><a href='MSE000001_IP2.pdf'>IP2 Canvas</a></p>\n";
        htmlfile << "</body>\n</html>";
        htmlfile.close();
    } else {
        std::cerr << "Failed to create HTML file." << std::endl;
    }
}

