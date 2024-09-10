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

void SetMargins(Double_t top = 0.10, Double_t right = 0.15, Double_t bottom = 0.10, Double_t left = 0.15) {
  gPad -> SetTopMargin(top);
  gPad -> SetRightMargin(right);
  gPad -> SetBottomMargin(bottom);
  gPad -> SetLeftMargin(left);
}

void ThDAC(int runN = 1)
{
    const int maxTH = 15; // 最大TH値を指定（例：15）
    const int maxCH = 32; // 最大チャンネル数

    std::set<int> ip_set;   // CSVから動的にIPを取得するためのセット
    std::vector<int> vecIP; // 動的に取得したIPを格納するベクター

    // CSVファイルの読み込みとIPの取得
    for (int th = 0; th <= maxTH; th++)
    {
        std::ifstream noisefile(Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th));
        if (!noisefile.is_open())
        {
            std::cerr << "Failed to open file: " << Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th) << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(noisefile, line))
        {
            std::stringstream ss(line);
            int ip;
            char comma;
            ss >> ip >> comma;

            ip_set.insert(ip); // セットにIPを追加
        }
        noisefile.close();
    }

    // セットからベクターにIPをコピー
    vecIP.assign(ip_set.begin(), ip_set.end());
    int N_IP = vecIP.size(); // IPの総数を取得

    //int RealThDAC[16] = {f,7,b,3,d,5,9,1,0,8,4,c,2,a,6,e};
    int RealThDAC[16] = {15,7,11,3,13,5,9,1,0,8,4,12,2,10,6,14};
    double N_count[16][maxCH][maxTH + 1] = {}; // 各チャンネルのあるThDACにおけるカウント数
    int BestTH[16][maxCH] = {};                // 各チャンネルの設定すべきThDAC
    int BestRate = 1500;                       // ThDAC調整で設定したいCount数
    // ThDACのBestTHを初期化
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < maxCH; j++)
        {
            BestTH[i][j] = 0;
        }
    }

    // ヒストグラムとグラフの準備
    //std::vector<std::vector<TGraph *>> graphs(N_IP, std::vector<TGraph *>(maxCH));
    std::vector<TH2D *> hNoise(N_IP);
    std::vector<TH2D *> hPositron(N_IP);

    for (int i = 0; i < N_IP; i++)
    {
        int ip = vecIP[i];
        for (int ch = 0; ch < maxCH; ch++)
        {
            //graphs[i][ch] = new TGraph();
            //graphs[i][ch]->SetTitle(Form("IP %d, Channel %d", ip, ch));
        }

        hNoise[i] = new TH2D(Form("hNoise_IP%d", ip),
                                 Form("Total Counts (IP %02d);Channel;ThDAC;Num", ip),
                                 maxCH, 0, maxCH,
                                 maxTH + 1, 0, maxTH + 1);
        hPositron[i] = new TH2D(Form("hPositron_IP%d", ip),
                                 Form("Positron Counts (IP %02d);Channel;ThDAC;Num", ip),
                                 maxCH, 0, maxCH,
                                 maxTH + 1, 0, maxTH + 1);
    }
    // データを詰める処理
    for (int th = 0; th <= maxTH; th++)
    {
        std::ifstream noisefile(Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th));
        if (!noisefile.is_open())
        {
            std::cerr << "Failed to open file: " << Form("./TXT/NOISE/MSE%06d_%02d.csv", runN, th) << std::endl;
            continue;
        }
        bool FLAG_ThDAC = true;
        int End_th = 13;

        std::string line;
        while (std::getline(noisefile, line))
        {
            std::stringstream ss(line);
            int ip;
            char comma;
            ss >> ip >> comma;

            auto it = std::find(vecIP.begin(), vecIP.end(), ip);
            if (it != vecIP.end())
            {
                int index = std::distance(vecIP.begin(), it);
                for (int ch = 0; ch < maxCH; ch++)
                {
                    double num;
                    ss >> num >> comma;
                    N_count[ip][ch][th] = num;
                    //graphs[index][ch]->SetPoint(th, th, num);
                    if (num == 0)
                        num = 1e-10; // ゼロを避ける
                    hNoise[index]->SetBinContent(ch + 1, th, num);

                    // 最もBestRateに近いthを探す
                    if (th < End_th)
                    {
                        if (abs(num - BestRate) < abs(N_count[ip][ch][BestTH[ip][ch]] - BestRate))
                        {
                            BestTH[ip][ch] = RealThDAC[th];
                            if (abs(N_count[ip][ch][BestTH[ip][ch]] - BestRate) > BestRate * 3)
                            {
                                BestTH[ip][ch]--;
                            }
                        }
                    }

                    if (th == maxTH)
                    {
                        cout << "BestTH[" << ip << "][" << ch << "]: " << BestTH[ip][ch] << endl;
                    }
                }
            }
        }
        noisefile.close();

        std::ifstream positronfile(Form("./TXT/POSITRON/MSE%06d_%02d.csv", runN, th));
        if (!positronfile.is_open())
        {
            std::cerr << "Failed to open file: " << Form("./TXT/POSITRON/MSE%06d_%02d.csv", runN, th) << std::endl;
            continue;
        }

        std::string linePositron;
        while (std::getline(positronfile, linePositron))
        {
            std::stringstream ss(linePositron);
            int ip;
            char comma;
            ss >> ip >> comma;

            auto it = std::find(vecIP.begin(), vecIP.end(), ip);
            if (it != vecIP.end())
            {
                int index = std::distance(vecIP.begin(), it);
                for (int ch = 0; ch < maxCH; ch++)
                {
                    double num;
                    ss >> num >> comma;
                    N_count[ip][ch][th] = num;
                    //graphs[index][ch]->SetPoint(th, th, num);
                    if (num == 0)
                        num = 1e-10; // ゼロを避ける
                    hPositron[index]->SetBinContent(ch + 1, th, num);
                }
            }
        }
        positronfile.close();
    }

    // ROOTファイルに保存
    TFile *file = new TFile(Form("../ROOT/NOISE/MSE%06d.root", runN), "RECREATE");

    // キャンバスとマルチグラフの作成
    for (int i = 0; i < N_IP; i++)
    {
        TString outFileName = Form("../SlowControl/ThDAC_tuned/VOLUME2012AllON_%d.txt", i + 1);
        ofstream outFile(outFileName);
        outFile << "# \"Channel number\" (starting at 0) and \"Value to be written in the register\"" << endl;
        outFile << "# DACs are in the order of [0][ThDAC][AmpDAC1][AmpDAC2][BiasDAC][CTRL] with [] in 4bit width." << endl;
        outFile << "# [CTRL]=[BiasON][Gain100][Amon][Dout] with [] are 1bit." << endl;

        int ip = vecIP[i];
        //TCanvas *cNoise = new TCanvas(Form("cNoise_IP%d", ip), Form("ThDAC Dependence for IP %d", ip), 1200, 800);
        //cNoise->Divide(8, 4); // 8x4のグリッドに分割

        for (int ch = 0; ch < maxCH; ch++)
        {
            //cNoise->cd(ch + 1);
            //TMultiGraph *mg = new TMultiGraph();
            //mg->Add(graphs[i][ch]);
            //mg->Draw("AL");
            //mg->SetTitle(Form("Channel %d;ThDAC;Num", ch));

            // ThDAC Tuningの保存
            int bestTH = BestTH[ip][ch];
            int value = 0x003B07;

            int mask = (bestTH) << 16;         // BestTHの下位2ビットを左に14ビットシフト
            value = (value & 0xF0FFFF) | mask; // 上位2ビットをBestTHに置き換える

            // チャネル番号と値を書き込む
            outFile << std::dec << ch << "  0x" << std::hex << std::setw(6) << std::setfill('0') << value << std::endl;
            // outFile << std::dec << ch << "  0x" << std::hex << std::setw(6) << std::setfill('0') << 0x111111 << std::endl;
        }

        //cNoise->Update();
        //cNoise->Write();
        //delete cNoise;
        outFile.close();
        cout << outFileName << " was created." << endl;
    }

    // 3Dヒストグラムのキャンバス作成
    for (int ii = 0; ii < N_IP; ii++) {
        int ip = vecIP[ii];
        TCanvas *cThDAC = new TCanvas(Form("cThDAC_IP%02d", ip), Form("ThDAC Dependence of Noise and Positron (IP: %02d)", ip), 1200, 800);
        cThDAC -> Divide(2, 1);
        gStyle -> SetOptStat(0); 
        cThDAC -> cd(1);
        SetMargins();
        hNoise[ii] -> SetMinimum(1e0);
        hNoise[ii] -> SetMaximum(1e5);
        hNoise[ii] -> GetZaxis() -> SetMoreLogLabels();
        hNoise[ii] -> GetZaxis() -> SetTitle("Num");
        hNoise[ii] -> Draw("colz");
        gPad -> SetLogz(1);
        cThDAC -> Update();
        cThDAC -> cd(2);
        SetMargins();
        hPositron[ii] -> SetMinimum(1e0);
        hPositron[ii] -> SetMaximum(1e5);
        hPositron[ii] -> GetZaxis() -> SetMoreLogLabels();
        hPositron[ii] -> GetZaxis() -> SetTitle("Num");
        hPositron[ii] -> Draw("colz");
        gPad -> SetLogz(1);
        cThDAC -> Update(); cThDAC -> Write();
        delete cThDAC;
    }
    for (int i = 0; i < N_IP; i++)
    {
        int ip = vecIP[i];
        TCanvas *cNoise2 = new TCanvas(Form("cNoise2_IP%d_Hist", ip), Form("3D ThDAC Dependence for IP %d", ip), 1200, 800);
        gStyle->SetOptStat(0); // スタット表示を無効化
        hNoise[i]->SetMinimum(1e0);                // Z軸の最小値（対数スケールに適した値）
        hNoise[i]->SetMaximum(1e5);                // Z軸の最大値（対数スケールに適した値）
        hNoise[i]->GetZaxis()->SetMoreLogLabels(); // Z軸に対数目盛りを表示
        hNoise[i]->GetZaxis()->SetTitle("Num");
        hNoise[i]->Draw("COLZ");
        gPad->SetLogz(1);
        cNoise2->Update();
        cNoise2->Write();
        delete cNoise2;

        TCanvas *cPositron2 = new TCanvas(Form("cPositron2_IP%d_Hist", ip), Form("3D ThDAC Dependence for IP %d", ip), 1200, 800);
        gStyle->SetOptStat(0); // スタット表示を無効化
        hPositron[i]->SetMinimum(1e0);                // Z軸の最小値（対数スケールに適した値）
        hPositron[i]->SetMaximum(1e5);                // Z軸の最大値（対数スケールに適した値）
        hPositron[i]->GetZaxis()->SetMoreLogLabels(); // Z軸に対数目盛りを表示
        hPositron[i]->GetZaxis()->SetTitle("Num");
        hPositron[i]->Draw("COLZ");
        gPad->SetLogz(1);
        cPositron2->Update();
        cPositron2->Write();
        delete cPositron2;
    }

    // グラフを保存
    /*
    for (int i = 0; i < N_IP; i++)
    {
        //int ip = vecIP[i];
        //for (int ch = 0; ch < maxCH; ch++)
        //{
        //    graphs[i][ch]->Write(Form("IP%d_Channel%d", ip, ch));
        //}
        //hPositron[i]->Write(); // ヒストグラムを保存
    }
    */

    file->Close();

    // HTMLファイルを作成
    std::ofstream htmlfile(Form("../ROOT/NOISE/MSE%06d.html", runN));
    if (htmlfile.is_open())
    {
        htmlfile << Form("<!DOCTYPE html>\n<html>\n<head>\n<title>MSE%06d Report</title>\n</head>\n<body>\n", runN);
        htmlfile << Form("<h1>MSE%06d Report</h1>\n", runN);
        htmlfile << "<p>Graphs and histograms are saved in ROOT file.</p>\n";
        htmlfile << "</body>\n</html>";
        htmlfile.close();
    }
    else
    {
        std::cerr << "Failed to create HTML file." << std::endl;
    }
}
