#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TMultiGraph.h>
#include <TFile.h>
#include <TH2D.h>
#include <TStyle.h>

void ThDAC(int runN = 1) {
    const int maxTH = 15;  // 最大TH値を指定（例：15）
    const int maxCH = 32;  // 最大チャンネル数

    std::set<int> ip_set;  // CSVから動的にIPを取得するためのセット
    std::vector<int> vecIP; // 動的に取得したIPを格納するベクター

    // CSVファイルの読み込みとIPの取得
    for (int th = 0; th <= maxTH; th++) {
        std::ifstream infile(Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th));
        if (!infile.is_open()) {
            std::cerr << "Failed to open file: " << Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th) << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            int ip;
            char comma;
            ss >> ip >> comma;

            ip_set.insert(ip);  // セットにIPを追加
        }
        infile.close();
    }

    // セットからベクターにIPをコピー
    vecIP.assign(ip_set.begin(), ip_set.end());
    int N_IP = vecIP.size();  // IPの総数を取得

    // ヒストグラムとグラフの準備
    std::vector<std::vector<TGraph*>> graphs(N_IP, std::vector<TGraph*>(maxCH));
    std::vector<TH2D*> histograms(N_IP);

    for (int i = 0; i < N_IP; i++) {
        int ip = vecIP[i];
        for (int ch = 0; ch < maxCH; ch++) {
            graphs[i][ch] = new TGraph();
            graphs[i][ch]->SetTitle(Form("IP %d, Channel %d", ip, ch));
        }

        histograms[i] = new TH2D(Form("hist_IP%d", ip),
                                 Form("ThDAC Dependence for IP %d;Channel;ThDAC;Num", ip),
                                 maxCH, 0, maxCH,
                                 maxTH + 1, 0, maxTH + 1);
    }

    // データを詰める処理
    for (int th = 0; th <= maxTH; th++) {
        std::ifstream infile(Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th));
        if (!infile.is_open()) {
            std::cerr << "Failed to open file: " << Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th) << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            int ip;
            char comma;
            ss >> ip >> comma;

            auto it = std::find(vecIP.begin(), vecIP.end(), ip);
            if (it != vecIP.end()) {
                int index = std::distance(vecIP.begin(), it);
                for (int ch = 0; ch < maxCH; ch++) {
                    double num;
                    ss >> num >> comma;
                    graphs[index][ch]->SetPoint(th, th, num);
                    if (num == 0) num = 1e-10; // ゼロを避ける
                    histograms[index]->SetBinContent(ch + 1, th, num);
                }
            }
        }
        infile.close();
    }

    // ROOTファイルに保存
    TFile *file = new TFile(Form("../ROOT/NOISE/MSE%06d.root", runN), "RECREATE");

    // キャンバスとマルチグラフの作成
    for (int i = 0; i < N_IP; i++) {
        int ip = vecIP[i];
        TCanvas* c1 = new TCanvas(Form("c1_IP%d", ip), Form("ThDAC Dependence for IP %d", ip), 1200, 800);
        c1->Divide(8, 4);  // 8x4のグリッドに分割

        for (int ch = 0; ch < maxCH; ch++) {
            c1->cd(ch + 1);
            TMultiGraph *mg = new TMultiGraph();
            mg->Add(graphs[i][ch]);
            mg->Draw("AL");
            mg->SetTitle(Form("Channel %d;ThDAC;Num", ch));
        }

        c1->Update();
        c1->Write();
        delete c1;
    }

    // 3Dヒストグラムのキャンバス作成
    for (int i = 0; i < N_IP; i++) {
        int ip = vecIP[i];
        TCanvas* c2 = new TCanvas(Form("c2_IP%d_Hist", ip), Form("3D ThDAC Dependence for IP %d", ip), 1200, 800);
        gStyle->SetOptStat(0);  // スタット表示を無効化

        histograms[i]->SetMinimum(1e0);   // Z軸の最小値（対数スケールに適した値）
        histograms[i]->SetMaximum(1e5);   // Z軸の最大値（対数スケールに適した値）
        histograms[i]->GetZaxis()->SetMoreLogLabels();  // Z軸に対数目盛りを表示
        histograms[i]->GetZaxis()->SetTitle("Num");
        histograms[i]->Draw("COLZ");
        gPad->SetLogz(1);

        c2->Update();
        c2->Write();
        delete c2;
    }

    // グラフを保存
    for (int i = 0; i < N_IP; i++) {
        int ip = vecIP[i];
        for (int ch = 0; ch < maxCH; ch++) {
            graphs[i][ch]->Write(Form("IP%d_Channel%d", ip, ch));
        }
        histograms[i]->Write(); // ヒストグラムを保存
    }

    file->Close();

    // HTMLファイルを作成
    std::ofstream htmlfile(Form("../ROOT/NOISE/MSE%06d.html", runN));
    if (htmlfile.is_open()) {
        htmlfile << Form("<!DOCTYPE html>\n<html>\n<head>\n<title>MSE%06d Report</title>\n</head>\n<body>\n", runN);
        htmlfile << Form("<h1>MSE%06d Report</h1>\n", runN);
        htmlfile << "<p>Graphs and histograms are saved in ROOT file.</p>\n";
        htmlfile << "</body>\n</html>";
        htmlfile.close();
    } else {
        std::cerr << "Failed to create HTML file." << std::endl;
    }
}

