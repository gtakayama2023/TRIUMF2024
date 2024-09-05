//************************************************************************
//    Kalliope, NIM-TDC Rawdata to Root for #M9999 @TRIUMF
//    edit by S.Ishitani (Osaka University), 2024.08.10
//************************************************************************

// rawdata2root.20240831.cpp
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <dirent.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TStopwatch.h>
#include <TCanvas.h>

#include "./include/configureIP.h"
#include "./include/setup.h"

//#define TEST_ON // ON: Read only 10k events
#define TRACKING_ON  // ON: Tracking

using namespace std;

void SkipOrNot(int N_IP, double dTS_KAL[12], double dTS_NIM,
  double minValue, double maxValue,
  bool Skip_KAL[12], bool & Skip_NIM, bool & SKIP_FLAG_new) {
  double tolerance = 0.3e-6;
  // For Skip
  SKIP_FLAG_new = false;
  for (int i = 0; i < N_IP; i++) {
    if (std::fabs(dTS_KAL[i] - maxValue) <= tolerance) {
      Skip_KAL[i] = false;
    } else {
      Skip_KAL[i] = true;
      SKIP_FLAG_new = true;
    }
  }
  if (std::fabs(dTS_NIM - maxValue) <= tolerance) {
    Skip_NIM = false;
  } else {
    Skip_NIM = true;
    SKIP_FLAG_new = true;
  }
}

std::pair < double, double > FindMinMax(int N_IP, double dTS_KAL[12], double dTS_NIM) {
  std::vector < double > validValues;
  for (int i = 0; i < N_IP; i++) {
    validValues.push_back(dTS_KAL[i]);
  }
  validValues.push_back(dTS_NIM);
  if (validValues.empty()) {
    return {
      std::numeric_limits < double > ::infinity(),
      -std::numeric_limits < double > ::infinity()
    };
  }
  double minValue = * std::min_element(validValues.begin(), validValues.end());
  double maxValue = * std::max_element(validValues.begin(), validValues.end());
  return {
    minValue,
    maxValue
  };
}

unsigned int Read_Raw_32bit(const char * b) { // Little Endian
  return ((b[3] << 24) & 0xff000000) | // shift the 1st byte to 24 bit left
    ((b[2] << 16) & 0x00ff0000) | // shift the 2nd byte to 16 bit left
    ((b[1] << 8) & 0x0000ff00) | // shift the 3rd byte to 08 bit left
    ((b[0] << 0) & 0x000000ff); // shift the 4th byte to 00 bit left
}

uint32_t ReadNext32bit(ifstream & rawdata) {
  char Byte[4];
  rawdata.read(Byte, 4);
  if (!rawdata) {
    cerr << "Error : Fail to execute ReadNext32bit. ifstream rawdata is empty." << endl;
    return 0;
  }
  return Read_Raw_32bit(Byte);
}

int CheckLOS(ifstream & rawdata) {
  uint32_t Next32bit = ReadNext32bit(rawdata);
  if (Next32bit == 0x00030000) {
    //cout << "LOS Flag was not detected!!!" << endl;
    return 0;
  } else if (Next32bit == 0x00070000) {
    //cout << "LOS Flag was detected!!!" << endl;
    return 1;
  } else {
    //cerr << "Error : LOS Flag cannnot be detected" << endl;
    return 1;
  }
}

double GetNETtime(ifstream & rawdata, unsigned int data) {
  uint32_t Next32bit = ReadNext32bit(rawdata);
  uint64_t FullData64bit = (static_cast < uint64_t > (data) << 32) | Next32bit;
  int Header = (FullData64bit >> 56) & 0xFF; // 8bit
  double sec = (FullData64bit >> 26) & 0x3FFFFFFF; // 30bit
  double us = (FullData64bit >> 11) & 0x7FFF; // 15bit
  double ns = (FullData64bit) & 0x7FF; // 11bit

  double RealTime = sec + us / 32768.0 + ns * 25.0 * pow(10, -9);
  return RealTime;
}

double GetKeyword(ifstream & rawdata) {
  uint32_t Next32bit = ReadNext32bit(rawdata);
  double Keyword = (Next32bit & 0x00ffffff);
  return Keyword;
}

void AssignFiber(int & fiber_ch, int & ud, int oi) {
  if (oi == 1) { // for inner fiber
    int q = fiber_ch / 4;
    int mod = fiber_ch % 4;
    int Q = fiber_ch / 8;
    // ch assignment
    if (q % 2 == 0) ud = 0;
    else ud = 1;
    switch (mod) {
    case 0:
      fiber_ch = 4 * Q + 2;
      break;
    case 1:
      fiber_ch = 4 * Q + 3;
      break;
    case 2:
      fiber_ch = 4 * Q + 0;
      break;
    case 3:
      fiber_ch = 4 * Q + 1;
      break;
    }
  }
}

void SetMargins(Double_t top = 0.10, Double_t right = 0.15, Double_t bottom = 0.10, Double_t left = 0.15) {
  gPad -> SetTopMargin(top);
  gPad -> SetRightMargin(right);
  gPad -> SetBottomMargin(bottom);
  gPad -> SetLeftMargin(left);
}

void rawdata2root(int runN = 10, int N_IP = 0, bool fNIM = 0, bool ftree = 0,
  const string & path = "test", bool ONLINE_FLAG = false, bool SCAN_FLAG = false, int SCAN_N = 0) {

  bool KAL_ONLINE_FLAG = false;

  if (path.c_str() == std::string("KAL_ONLINE")) {
    KAL_ONLINE_FLAG = true;
  }
  cout << "KAL_ONLINE_FLAG: " << KAL_ONLINE_FLAG << endl;

  //===== Define Input =====
  string ifname_nimtdc;
  ifstream rawdata_nimtdc;
  string ifname[12];
  TString ofname;
  ifstream rawdata[12];
  ofstream outfile("./txt/ThDAC.txt");
  ofstream noisefile(Form("./TXT/%s/MSE%06d_%02d.csv", path.c_str(), runN, SCAN_N));
  ofstream ofNevent(Form("./txt/Nevent_run%d.txt", runN));
  ofstream ofEvtMatch(Form("./txt/EvtMatch_run%d.txt", runN));
  cout << Form("./TXT/%s/MSE%06d_%02d.csv", path.c_str(), runN, SCAN_N) << endl;

  if (SCAN_FLAG) fNIM = 0;
  cout << "SCANFLAG : " << SCAN_FLAG << endl;
  cout << "fNIM: " << fNIM << endl;
  //===== Open Rawdata =====
  //====== NIM-TDC ======
  int IP_NIM = 16;
  if (fNIM) {
    ifname_nimtdc = Form("../RAW/%s/MSE%06d_192.168.10.%d.rawdata",   path.c_str(), runN, IP_NIM);
    rawdata_nimtdc.open(ifname_nimtdc.c_str());
    if (!rawdata_nimtdc) {
      cout << "Unable to open file: " << ifname_nimtdc << endl;
      exit(1); // terminate with error
    }
  }
  //===== Kalliope =====
  std::vector<int> vecIP;
  TString base_name;
  if (runN < 10)       base_name = Form("MSE00000%d_192.168.10.", runN);
  else if (runN < 100) base_name = Form("MSE0000%d_192.168.10.", runN);
  else                 base_name = Form("MSE000%d_192.168.10.", runN);

	if (SCAN_FLAG) {
    if (runN < 10)       base_name = Form("MSE00000%d_%02d_192.168.10.", runN, SCAN_N);
    else if (runN < 100) base_name = Form("MSE0000%d_%02d_192.168.10.",  runN, SCAN_N);
    else                 base_name = Form("MSE000%d_%02d_192.168.10.",   runN, SCAN_N);
	}

  //if (SCAN_FLAG) base_name = "0" + base_name;

	DIR *dir;
	struct dirent *entry;
  if ((dir = opendir(Form("../RAW/%s", path.c_str()))) != nullptr) {
      while ((entry = readdir(dir)) != nullptr) {
          TString filename = entry->d_name;
          if (filename.Contains(base_name) && filename.EndsWith(".rawdata")) {
              int IP;
              sscanf(filename.Data() + filename.Last('_') + 1, "192.168.10.%d.rawdata", &IP); // IP を抽出
              vecIP.push_back(IP);
							cout << IP << endl;
          }
      }
      closedir(dir);
  } else {
      perror("opendir");
  }

  vecIP.erase(remove(vecIP.begin(), vecIP.end(), IP_NIM), vecIP.end());
  sort(vecIP.begin(), vecIP.end());

  N_IP = vecIP.size();
  cout << N_IP << endl;

  for (int ii = 0; ii < N_IP; ii++) {
    ifname[ii] = Form("../RAW/%s/%s%d.rawdata", path.c_str(), base_name.Data(), vecIP[ii]);
    rawdata[ii].open(ifname[ii].c_str());
    if (!rawdata[ii]) {
      cout << "Unable to open file: " << ifname[ii] << endl;
      exit(1); // terminate with error
    }
    cout << ifname[ii] << endl;
  }

  if (!fNIM) {
    rawdata_nimtdc.open(ifname[0].c_str());
    fNIM = true;
  }

  //======================================
  //===== NIM-TDC (IP=16 (default)) ======
  //======================================
  int N_NIM_event = 0, N_NIM_Sync = 0, N_NIM_Sync_Interval = 0, N_NIM_Last = 0, N_NIM_LOS = 0;
  int N_NIM_Last2 = 0;
  // NIM-TDC serve as the reference TimeStamp due to its small time fluctuation
  // wo/NIM-TDC, Kalliope IP=0 serve as the ref. TimeStamp
  double TS_NIM_0 = 0.0, TS_NIM = 0.0, dTS_NIM = 0.0, TS_NIM_pre = 0.0;
  double TS_NIM_Sync = 0.0; //, dTS_NIM_Sync = 0.0, TS_NIM_Sync_pre = 0.0;
  vector < double > dTS_NIM_seq;
  int Keyword = -1;
  int Traw_NIM_L[32][10] = {}, Traw_NIM_T[32][10] = {}, Traw_NIM_TOT[32][10] = {};
  int Traw_NIM_L_valid[32] = {}, Traw_NIM_T_valid[32] = {};
  int Traw_NIM_num[2][32] = {};
  int T_BACKWARD = 0, T_FORWARD = 0, T_UP = 0, T_DN = 0;

  int N_event[12] = {};
  vector < int > N_Sync_Interval(N_IP, 0), N_KAL_Last(N_IP, 0), N_KAL_LOS(N_IP, 0);
  vector < int > N_KAL_Last2(N_IP, 0);

  int N_KAL_Total[12] = {}, N_KAL_Sync[12] = {};
  double TS_KAL_0[12], TS_KAL[12], TS_KAL_calib[12], dTS_KAL[12] = {
    0.
  }, TS_KAL_pre[12], TS_KAL_Sync[12];
  vector < double > TS_diff(N_IP, 0.0), dTS_diff(N_IP, 0.0);
  vector< vector < double > > dTS_KAL_seq(12);

  int time_L = -999, time_T = 999;

  //======================================
  //============ Time Window =============
  //======================================
  // wo/ missing trigger, dTS_diff must be a few hundreds ns, enough short compared to beam frequency ~us.
  // w/  missing trigger, dTS_diff should be comparable to beam frequency ~ us.
  double dTS_diff_Window = 1e-7;
  // Synchronized Trigger has double pulse structure to enable Kalliope to identify itself
  // its structure is NIM 150ns width + 200ns delayed NIM 150ns width
  double DoublePulseWindow = 5e-7;

  //===== Define FLAG ======
  bool WHOLE_FLAG = true, FILL_FLAG = true, SAME_EVENT_FLAG = false;
  bool LOS_FLAG[12] = {}, LOS_NIM_FLAG = false, SKIP_FLAG[12] = {}, END_NIM_FLAG = false, END_FLAG[12] = {}, SYNC_FLAG[12] = {};
  //Int_t SYNC_NIM_FLAG = false;
  Int_t SYNC_NIM_FLAG = 0;

  //===== Define Variables ======
  //===== Raw Data =====
  // Leading/Trailing Time, TOT : [Kalliope][KEL][ch][Multiplicity]
  int Traw_KAL_L[12][32][10], Traw_KAL_T[12][32][10], Traw_KAL_TOT[12][32][10];
  int Traw_KAL_L_valid[12][32], Traw_KAL_T_valid[12][32];
  int Traw_KAL_num[12][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]
  int Traw_KAL_num_total[12][2][32] = {}; // Malitiplicity of [Kalliope IP][L/Tr][CH]

  int KalNum[12][32] = {0};

  int N_02event = 0;
  int hitNmax = 10; // Multiplicity Max
  int ch = 0;
  vector < int > N_Sync(N_IP, 0);

  //======================================
  //============= Tracking ===============
  //======================================
  //===== Fiber CH =====
  int xy = -999, ud = -999, oi = -999;
  int ch_offset = -999, Layer_No = -999, ch_max = 64;

  int Fiber_L[2][2][2][64], Fiber_T[2][2][2][64], Fiber_TOT[2][2][2][64];
  int Fiber_num[2][2][2] = {}; // Count detected channel to check whether Tracking Available no not
  bool Fiber_FLAG[2][2][2];
  bool Fiber_CH_FLAG[2][2][2][64];
  bool Tracking_FLAG[2]; // only up or dn

  int TOT_Noise_NIM = 200, TOT_Noise_KAL[12];
  fill_n(TOT_Noise_KAL, 12, 200);
  cout << TOT_Noise_KAL[0] << ", " << TOT_Noise_KAL[1] << endl;
  int N_track = 0;

  //===== ROOT file output =====
  if (runN < 10) ofname       = Form("../ROOT/%s/MSE00000%d.root", path.c_str(), runN);
  else if (runN < 100) ofname = Form("../ROOT/%s/MSE0000%d.root", path.c_str(), runN);
  else ofname                 = Form("../ROOT/%s/MSE000%d.root", path.c_str(), runN);
  if (SCAN_FLAG) {
    if (runN < 10) ofname       = Form("../ROOT/%s/MSE00000%d_%02d.root", path.c_str(), runN, SCAN_N);
    else if (runN < 100) ofname = Form("../ROOT/%s/MSE0000%d_%02d.root",  path.c_str(), runN, SCAN_N);
    else ofname                 = Form("../ROOT/%s/MSE000%d_%02d.root",   path.c_str(), runN, SCAN_N);
	}
  cout << "create root file :" << ofname << endl;

  TFile * f = new TFile(ofname, "RECREATE");

  TTree * tree = new TTree("tree", "tree");

  //===== Statistics file =====
  TString stat_name;
  if (runN < 10) stat_name       = Form("../ROOT/%s/MSE00000%d.html", path.c_str(), runN, SCAN_N);
  else if (runN < 100) stat_name = Form("../ROOT/%s/MSE0000%d.html",  path.c_str(), runN, SCAN_N);
  else stat_name                 = Form("../ROOT/%s/MSE000%d.html",   path.c_str(), runN, SCAN_N);
  if (SCAN_FLAG) {
    if (runN < 10) stat_name       = Form("../ROOT/%s/MSE00000%d_%02d.html", path.c_str(), runN, SCAN_N);
    else if (runN < 100) stat_name = Form("../ROOT/%s/MSE0000%d_%02d.html",  path.c_str(), runN, SCAN_N);
    else stat_name                 = Form("../ROOT/%s/MSE000%d_%02d.html",   path.c_str(), runN, SCAN_N);
	}
  std::ofstream stat_file(stat_name);

  stat_file << "<!DOCTYPE html>\n";
  stat_file << "<html lang=\"en\">\n";
  stat_file << "<head>\n";
  stat_file << "    <meta charset=\"UTF-8\">\n";
  stat_file << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  stat_file << "    <title>Event Statistics</title>\n";
  stat_file << "    <style>\n";
  stat_file << "        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; }\n";
  stat_file << "        .container { width: 100%; max-width: none; margin: auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }\n";
  stat_file << "        h1 { text-align: center; color: #333; }\n";
  stat_file << "        p { margin: 10px 0; padding: 10px; background-color: #e9ecef; border-radius: 4px; }\n";
  stat_file << "        .label { font-weight: bold; }\n";
  stat_file << "        th, td { width: 150px; } /* 列の幅を指定 */\n";
  stat_file << "    </style>\n";
  stat_file << "</head>\n";
  stat_file << "<body>\n";
  stat_file << "    <div class=\"container\">\n";

  //===== Initialize =====
  for (int i = 0; i < 12; i++)
    for (int j = 0; j < 32; j++)
      for (int k = 0; k < 10; k++) Traw_KAL_L[i][j][k] = 0;
  for (int i = 0; i < 12; i++)
    for (int j = 0; j < 32; j++)
      for (int k = 0; k < 10; k++) Traw_KAL_T[i][j][k] = 0;
  for (int i = 0; i < 12; i++)
    for (int j = 0; j < 32; j++)
      for (int k = 0; k < 10; k++) Traw_KAL_TOT[i][j][k] = 0;
  for (int i = 0; i < 12; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 32; k++) Traw_KAL_num[i][j][k] = 0;

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 2; k++)
        for (int l = 0; l < 64; l++) Fiber_L[i][j][k][l] = 0;
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 2; k++)
        for (int l = 0; l < 64; l++) Fiber_T[i][j][k][l] = 0;
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 2; k++)
        for (int l = 0; l < 64; l++) Fiber_TOT[i][j][k][l] = 0;

  for (int i = 0; i < 12; i++) SKIP_FLAG[i] = false;

  //===== Define Tree ======
  //===== Time Stamp ======
  //===== NIM-TDC =====
  tree -> Branch("TS_NIM", & TS_NIM, "TS_NIM/D");
  tree -> Branch("dTS_NIM", & dTS_NIM, "dTS_NIM/D");
  tree -> Branch("TS_NIM_pre", & TS_NIM_pre, "TS_NIM_pre/D");
  tree -> Branch("TS_NIM_Sync", & TS_NIM_Sync, "TS_NIM_Sync/D");
  tree -> Branch("SYNC_NIM_FLAG", & SYNC_NIM_FLAG, "SYNC_NIM_FLAG/I");
  //===== Kalliope =====
  tree -> Branch("TS_KAL", TS_KAL, "TS_KAL[12]/D");
  tree -> Branch("TS_KAL_calib", TS_KAL_calib, "TS_KAL_calib[12]/D");
  tree -> Branch("dTS_KAL", dTS_KAL, "dTS_KAL[12]/D");
  tree -> Branch("TS_KAL_pre", TS_KAL_pre, "TS_KAL_pre[12]/D");
  tree -> Branch("TS_KAL_Sync", TS_KAL_Sync, "TS_KAL_Sync[12]/D");

  tree -> Branch("TS_diff", & TS_diff);
  tree -> Branch("dTS_diff", & dTS_diff);

  //===== Rawdata =====
  tree -> Branch("Traw_NIM_L", Traw_NIM_L, "Traw_NIM_L  [32][10]/I");
  tree -> Branch("Traw_NIM_T", Traw_NIM_T, "Traw_NIM_T  [32][10]/I");
  tree -> Branch("Traw_NIM_TOT", Traw_NIM_TOT, "Traw_NIM_TOT[32][10]/I");
  tree -> Branch("Traw_NIM_num", Traw_NIM_num, "Traw_NIM_num [2][32]/I");
  tree -> Branch("Traw_KAL_L", Traw_KAL_L, "Traw_KAL_L  [12][32][10]/I");
  tree -> Branch("Traw_KAL_T", Traw_KAL_T, "Traw_KAL_T  [12][32][10]/I");
  tree -> Branch("Traw_KAL_TOT", Traw_KAL_TOT, "Traw_KAL_TOT[12][32][10]/I");
  tree -> Branch("Traw_KAL_num", Traw_KAL_num, "Traw_KAL_num[12][2][32]/I");

  tree -> Branch("Traw_NIM_L_valid", Traw_NIM_L_valid, "Traw_NIM_L_valid  [32][10]/I");
  tree -> Branch("Traw_NIM_T_valid", Traw_NIM_T_valid, "Traw_NIM_T_valid  [32][10]/I");
  tree -> Branch("Traw_KAL_L_valid", Traw_KAL_L_valid, "Traw_KAL_L_valid  [32][10]/I");
  tree -> Branch("Traw_KAL_T_valid", Traw_KAL_T_valid, "Traw_KAL_T_valid  [32][10]/I");

  //====== T_Fiber ======
  tree -> Branch("Fiber_L", Fiber_L, "Fiber_L[2][2][2][64]/I"); //[x/y][up/dn][in/out][CH]
  tree -> Branch("Fiber_T", Fiber_T, "Fiber_T[2][2][2][64]/I");
  tree -> Branch("Fiber_TOT", Fiber_TOT, "Fiber_TOT[2][2][2][64]/I");
  tree -> Branch("Fiber_num", Fiber_num, "Fiber_num[2][2][2]/I");

  //===== Define Histgrams =====
  //===== Histgrams for Monitoring Data =====
  TH2F * hMonitor_TS_KAL;
  hMonitor_TS_KAL = new TH2F("hMonitor_TS_KAL", "All #it{TS}_{KAL}; Kalliope IP; #it{TS}_{KAL} [s]", 12, 0, 12, 1000, 0, 60);

  TH2F * hMonitor_dTS_diff;
  hMonitor_dTS_diff = new TH2F("hMonitor_dTS_diff", "All #it{TS}_{KAL} - #it{TS}_{NIM}; Kalliope IP; #it{TS}_{KAL} - #it{TS}_{NIM}", 12, 0, 12, 1000, -1e-6, 1e-6);

  TH2F * hMonitor_TS_Sync;
  hMonitor_TS_Sync = new TH2F("hMonitor_TS_Sync", "hMonitor_TS_Sync; Kalliope IP; T_diff_2D", 12, 0, 12, 1000, 0, 60);

  //===== Histgram for TimeStamp =====
  // Time stamp of each Kalliope module vs NIM_TDC's
  vector < TH2F * > hTS_KAL2;
  hTS_KAL2.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hTS_KAL2[ii] = new TH2F(Form("hTS_KAL2_%d", vecIP[ii]), Form("#it{TS}_{NIM} vs #it{TS}_{KAL(%02d)]}; #it{TS}_{NIM} [s]; #it{TS}_{KAL} [s]", vecIP[ii]), 1000, 0, 4000, 1000, 0, 4000);
  }
  // 
  vector < TH1F * > hdTS_KAL;
  hdTS_KAL.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hdTS_KAL[ii] = new TH1F(Form("hdTS_KAL_%d", vecIP[ii]), Form("#it{TS}_{KAL}(IP = %02d) #it{interval}; #it{TS}_{KAL} #it{interval} [s]; (#it{counts})", vecIP[ii]), 1000, 0, 1e-2);
  }
  // 
  vector < TH2F * > hTS_diff;
  hTS_diff.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hTS_diff[ii] = new TH2F(Form("hTS_diff_%d", vecIP[ii]), Form("#it{TS}_{KAL}(IP = %02d) - #it{TS}_{NIM} ; #it{TS}_{NIM} [s]; #it{TS}_{KAL} - #it{TS}_{NIM} [s]", vecIP[ii]), 1000, 0, 20, 1000, -4e-5, 4e-5);
  }
  //
  vector < TH1F * > hdTS_diff;
  hdTS_diff.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hdTS_diff[ii] = new TH1F(Form("hdTS_diff_%d", vecIP[ii]), Form("dTS_diff[%d]; TDC ch; dTS_diff", vecIP[ii]), 1000, -1e-9, 1e-9);
  }
  //
  vector < TH2F * > hdTS_calib;
  hdTS_calib.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hdTS_calib[ii] = new TH2F(Form("hdTS_calib_%d", vecIP[ii]), Form("#it{TS}_{KAL_calib}(%d) - #it{TS}_{NIM} ;#it{TS}_{NIM} [s]; #it{TS}_{KAL_calib}(%d) - TS_{NIM} [s]", vecIP[ii], vecIP[ii]), 1000, 0, 10, 1000, -1e-6, 1e-6);
  }

  //===== Histgram for Rawdata =====
  // TDC-Leading
  vector < TH1F * > hNIM_L;
  hNIM_L.resize(4);
  for (int jj = 0; jj < 4; jj++) {
    hNIM_L[jj] = new TH1F(Form("hTDC_L_NIM_ch%d", jj), Form("NIM_TDC ch%02d | #it{TDC}_{Leading}; #it{TDC}_{Leading} [ns]; #it{counts}", jj), 1e3, -1e3, 7e4);
  }
  //vector < vector < TH1F * >> hKAL_L;
  //hKAL_L.resize(N_IP);
  //for (int ii = 0; ii < N_IP; ii++) {
  //  hKAL_L[ii].resize(32);
  //  for (int jj = 0; jj < 32; jj++) {
  //    hKAL_L[ii][jj] = new TH1F(Form("hTDC_L_%d_ch%d", vecIP[ii], jj), Form("Kalliope(%02d) ch%02d | #it{TDC}_{Leading}; #it{TDC}_{Leading} [ns]; #it{counts}", vecIP[ii], jj), 1e3, -1e3, 7e4);
  //  }
  //}
  TH2F * hNIM_L2;
  hNIM_L2 = new TH2F(Form("hNIM_L2"), Form("NIM_TDC | #it{TDC}_{Leading}  ; #it{TDC} ch; #it{TDC}_{Leading} [ns]"), 32, 0, 32, 1e3, -1e3, 7e4);
  vector < TH2F * > hKAL_L2;
  hKAL_L2.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hKAL_L2[ii] = new TH2F(Form("hTDC_L2_%d", vecIP[ii]), Form("Kalliope(%02d) | #it{TDC}_{Leading}  ; #it{TDC} ch; #it{TDC}_{Leading} [ns]", vecIP[ii]), 32, 0, 32, 1e3, -1e3, 7e4);
  }

  // TDC-Trailing
  //vector < TH1F * > hNIM_T;
  //hNIM_T.resize(32);
  //for (int jj = 0; jj < 32; jj++) {
  //  hNIM_T[jj] = new TH1F(Form("hTDC_T_NIM_ch%d", jj), Form("NIM_TDC ch%02d | #it{TDC}_{Trailing}; #it{TDC}_{Trailing} [ns]; #it{counts}", jj), 1e3, -1e3, 7e4);
  //}
  //vector < vector < TH1F * >> hKAL_T;
  //hKAL_T.resize(N_IP);
  //for (int ii = 0; ii < N_IP; ii++) {
  //  hKAL_T[ii].resize(32);
  //  for (int jj = 0; jj < 32; jj++) {
  //    hKAL_T[ii][jj] = new TH1F(Form("hTDC_T_%d_ch%d", vecIP[ii], jj), Form("Kalliope(%02d) ch%02d | #it{TDC}_{Trailing} ; #it{TDC}_{Trailing} [ns]; #it{counts}", vecIP[ii], jj), 1e3, -1e3, 7e4);
  //  }
  //}
  TH2F * hNIM_T2;
  hNIM_T2 = new TH2F(Form("hNIM_T2"), Form("NIM_TDC | #it{TDC}_{Trailing}  ; #it{TDC} ch; #it{TDC}_{Trailing} [ns]"), 32, 0, 32, 1e3, -1e3, 7e4);
  vector < TH2F * > hKAL_T2;
  hKAL_T2.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hKAL_T2[ii] = new TH2F(Form("hTDC_T2_%d", vecIP[ii]), Form("Kalliope(%02d) | #it{TDC}_{Trailing} ; #it{TDC} ch; #it{TDC}_{Trailing} [ns]", vecIP[ii]), 32, 0, 32, 1e3, -1e3, 7e4);
  }

  // TDC-TOT
  //vector < TH1F * > hNIM_TOT;
  //hNIM_TOT.resize(32);
  //for (int jj = 0; jj < 32; jj++) {
  //  hNIM_TOT[jj] = new TH1F(Form("hNIM_TOT_ch%d", jj), Form("NIM_TDC ch%02d | #it{TOT}; #it{TOT} [ns]; #it{counts}", jj), 1e3, -1e1, 1e3);
  //}
  //vector < vector < TH1F * >> hKAL_TOT;
  //hKAL_TOT.resize(N_IP);
  //for (int ii = 0; ii < N_IP; ii++) {
  //  hKAL_TOT[ii].resize(32);
  //  for (int jj = 0; jj < 32; jj++) {
  //    hKAL_TOT[ii][jj] = new TH1F(Form("hKAL_TOT_%d_ch%d", vecIP[ii], jj), Form("Kalliope(%02d) ch%02d | #it{TOT}; #it{TOT} [ns]; #it{counts}", vecIP[ii], jj), 1e3, -1e1, 1e3);
  //  }
  //}
  TH2F * hNIM_TOT2 = new TH2F(Form("hNIM_TOT2"), Form("NIM_TDC | #it{TOT}; #it{TDC} ch; #it{TOT} [ns]"), 32, 0, 32, 1e3, -1e1, 1e3);
  vector < TH2F * > hKAL_TOT2;
  hKAL_TOT2.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hKAL_TOT2[ii] = new TH2F(Form("hTDC_TOT_%d", vecIP[ii]), Form("Kalliope(%02d) | #it{TOT}; #it{TDC} ch; #it{TOT} [ns]", vecIP[ii]), 32, 0, 32, 1e3, -1e1, 1e3);
  }

  // num
  TH2F * hNIM_Multi;
  hNIM_Multi = new TH2F(Form("hNIM_Multi"), Form("NIM_TDC | #it{Multiplicity}; #it{TDC} ch; Multiplicity"), 32, 0, 32, hitNmax, 0, hitNmax);
  vector < TH2F * > hKAL_Multi;
  hKAL_Multi.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hKAL_Multi[ii] = new TH2F(Form("hKAL_Multi_%d", vecIP[ii]), Form("Kalliope(%02d) | #it{Multiplicity}; #it{TDC} ch; Multiplicity", vecIP[ii]), 32, 0, 32, hitNmax, 0, hitNmax);
  }

  // KAL - NIM_TDC
  vector < TH2F * > hKAL_NIM2;
  hKAL_NIM2.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    hKAL_NIM2[ii] = new TH2F(Form("hKAL_NIM_%02d", vecIP[ii]), Form("Kalliope(%02d) | #it{TDC}_{KAL} - #it{TDC}_{NIM}; ch; #it{TDC}_{KAL} - #it{TDC}_{NIM}", vecIP[ii]), 32, 0, 32, 1e3, -1e3, 1e3);
  }

  //====== Histgram for Fiber ======
  #ifdef TRACKING_ON
  TH2F * hFiber_out[2];
  hFiber_out[0] = new TH2F("hFiber_out_up", "hFiber_out_up; x[mm]; y[mm]", 64, -32, 32, 54, -32, 32);
  hFiber_out[1] = new TH2F("hFiber_out_dn", "hFiber_out_dn; x[mm]; y[mm]", 64, -32, 32, 64, -32, 32);
  TH2F * hFiber_in[2];
  hFiber_in[0] = new TH2F("hFiber_in_up", "hFiber_in_up; x[mm]; y[mm]", 32, -16, 16, 32, -16, 16);
  hFiber_in[1] = new TH2F("hFiber_in_dn", "hFiber_in_dn; x[mm]; y[mm]", 32, -16, 16, 32, -16, 16);

  TH2F * hSample_Projection = new TH2F("hSample_Projection", "hSample_Projection; x[mm]; y[mm]", 100, -25, 25, 100, -25, 25);
  TH2F * hSample_Plane = new TH2F("hSample_Plane", "hSample_Plane; x[mm]; y[mm]", 100, -25, 25, 100, -25, 25);

  TH2F * hFiber_L[2][2][2];
  TH2F * hFiber_T[2][2][2];
  TH2F * hFiber_TOT[2][2][2];
  for (int i = 0; i < 2; i++) { // x/y
    for (int j = 0; j < 2; j++) { // up/dn
      for (int k = 0; k < 2; k++) { // out/in
        TString hName, hTitle, XY, UD, OI;
        if (i == 0) XY = "x";
        else XY = "y";
        if (j == 0) UD = "up";
        else UD = "dn";
        if (k == 0) OI = "out";
        else OI = "in";

        hName.Form("hFiber_L_%s_%s_%s", XY.Data(), UD.Data(), OI.Data());
        hTitle.Form("hFiber_L[%s][%s][%s]; Fiber ch; TDC [ns]", XY.Data(), UD.Data(), OI.Data());
        hFiber_L[i][j][k] = new TH2F(hName, hTitle, 64, 0, 64, 1e3, -150, 7e4);

        hName.Form("hFiber_T_%s_%s_%s", XY.Data(), UD.Data(), OI.Data());
        hTitle.Form("hFiber_T[%s][%s][%s]; Fiber ch; TDC [ns]", XY.Data(), UD.Data(), OI.Data());
        hFiber_T[i][j][k] = new TH2F(hName, hTitle, 64, 0, 64, 1e3, -150, 7e4);

        hName.Form("hFiber_TOT_%s_%s_%s", XY.Data(), UD.Data(), OI.Data());
        hTitle.Form("hFiber_TOT[%s][%s][%s]; Fiber ch; TDC [ns]", XY.Data(), UD.Data(), OI.Data());
        hFiber_TOT[i][j][k] = new TH2F(hName, hTitle, 64, 0, 64, 1e3, -150, 7e4);
      }
    }
  }
  #endif

  //===== Timer =====
  TStopwatch RunTimer;
  RunTimer.Start();

  // Data size to read in Online mode
  //const size_t READ_SIZE_NIM = 1 * 1024 * 1024 ;  
  size_t READ_SIZE_NIM = 50 * 1000 * 1024 ;  

  // NIMのファイルサイズを取得
  rawdata_nimtdc.seekg(0, ios::end);
  streampos fsize_nimtdc = rawdata_nimtdc.tellg(); // NIM-TDCのファイルサイズを定義

  if (!ONLINE_FLAG) {
    READ_SIZE_NIM = fsize_nimtdc * 0.90;
    READ_SIZE_NIM = ( READ_SIZE_NIM / 1024 ) * 1024;
    ONLINE_FLAG = true;
          //start_pos = (start_pos / 1024) * 1024; // 1024単位に調整
  }

  
  // READ_RATIO をループの外で宣言
  double READ_RATIO = 1.0;
  
  if (ONLINE_FLAG) {
      // ファイルの何割目に相当するか計算
      READ_RATIO = 1.0 - static_cast<double>(READ_SIZE_NIM) / static_cast<double>(fsize_nimtdc);
      if (READ_RATIO < 0) READ_RATIO = 0.0; // 負の値にならないように調整
  
      // 読み始める位置を計算
      streampos start_pos_nimtdc = fsize_nimtdc * READ_RATIO;
      rawdata_nimtdc.seekg(start_pos_nimtdc, ios::beg);
      cout << "NIM-TDC Read Ratio: " << READ_RATIO << ", Start Position: " << start_pos_nimtdc << endl;
  } else {
      fsize_nimtdc = fsize_nimtdc / 4; // ファイルサイズを32ビット単位に変換
      rawdata_nimtdc.seekg(0, ios::beg);
      cout << Form("rawdata_NIM-TDC filesize : ") << fsize_nimtdc << endl;
  }

  // Kalliopeのファイル読み込み
  for (int nIP = 0; nIP < N_IP; nIP++) { 
      N_event[nIP] = 0;
      cout << "Start Reading Rawdata from Kalliope: " << vecIP[nIP] << endl;
  
      // Kalliopeのファイルサイズを取得
      rawdata[nIP].seekg(0, ios::end); // ファイルの終端へ移動
      streampos fsize = rawdata[nIP].tellg(); // ファイルサイズ（バイト単位）
      cout << Form("rawdata%d filesize :", vecIP[nIP]) << fsize << endl;
  
      if (ONLINE_FLAG) {
          // READ_RATIOに基づいて読み始める位置を計算
          double KAL_RATIO = READ_RATIO - (1.0 - READ_RATIO) * 0.1;
          streampos start_pos = fsize * KAL_RATIO;
          start_pos = (start_pos / 1024) * 1024; // 1024単位に調整
  
          rawdata[nIP].seekg(start_pos, ios::beg); // 読み始める位置へ移動
          cout << "Kalliope " << vecIP[nIP] << " Read Ratio: " << KAL_RATIO << ", Start Position: " << start_pos << endl;
      } else {
          fsize = fsize / 4; // ファイルサイズを32ビット単位に変換
          rawdata[nIP].seekg(0, ios::beg); // ファイルの最初から読み込む
          cout << Form("rawdata%d filesize: ", vecIP[nIP]) << fsize << endl;
      }
  }


  bool Skip_NIM = false, Stop_NIM = false;
  bool Skip_KAL[12] = {
    false
  }, Stop_KAL[12] = {
    false
  };
  bool SKIP_FLAG_new = false;
  double SkipN = 0;

  // Online mode
  int LOAD_N = 3;
  bool LOAD_N_FLAG_NIM = true;
  bool SYNC_ONLINE_FLAG[12] = {false};
  bool SYNC_ONLINE_FLAG_All = false;

  int NEVE = 0;

  //Start Reaing All Rawdata =====
  while (WHOLE_FLAG) {
  //while (NEVE == 1) {
    auto[minValue, maxValue] = FindMinMax(N_IP, dTS_KAL, dTS_NIM);
    SkipOrNot(
      N_IP, dTS_KAL, dTS_NIM,
      minValue, maxValue,
      Skip_KAL, Skip_NIM, SKIP_FLAG_new
    );

    if (fNIM) {
      //===== Start Reading each Rawdata_Nimtdc ======
      while (!rawdata_nimtdc.eof()  && !Stop_NIM ) {
        char Byte[4];
        unsigned int data;
        rawdata_nimtdc.read(Byte, 4); // reading 4 byte (32 bit)
        data = Read_Raw_32bit(Byte);
        int Header = (data & 0xff000000) >> 24;

        if (Header == 0x5c) { //Copper Header, Initialize, GetNETtime (0x5c event), Event Matching
          SYNC_NIM_FLAG = 0;
          for (int j = 0; j < 32; j++)
            for (int k = 0; k < 10; k++) Traw_NIM_L[j][k] = -1e4;
          for (int j = 0; j < 32; j++)
            for (int k = 0; k < 10; k++) Traw_NIM_T[j][k] = -2e4;
          for (int j = 0; j < 32; j++)
            for (int k = 0; k < 10; k++) Traw_NIM_TOT[j][k] = -1e4;
          for (int j = 0; j < 2; j++)
            for (int k = 0; k < 32; k++) Traw_NIM_num[j][k] = 0;
          for (int j = 0; j < 32; j++) Traw_NIM_L_valid[j] = -1e4;
          for (int j = 0; j < 32; j++) Traw_NIM_T_valid[j] = -2e4;

          //===== Initialize Readfile for the next event =====
          if (N_NIM_event == 0 && !Skip_NIM) {
            TS_NIM_0 = GetNETtime(rawdata_nimtdc, data);
            TS_NIM = 0.0;
            dTS_NIM = 0.0;
            TS_NIM_pre = 0.0;
          } else {
            TS_NIM = GetNETtime(rawdata_nimtdc, data) - TS_NIM_0;
            dTS_NIM = TS_NIM - TS_NIM_pre;
            TS_NIM_pre = TS_NIM;
            if (LOAD_N_FLAG_NIM) dTS_NIM_seq.push_back(dTS_NIM);
          }
          N_NIM_event++;
          N_NIM_Sync_Interval++;
          //====== Check Sync Trigger =====
          if (dTS_NIM < DoublePulseWindow) {
            TS_NIM_Sync = TS_NIM;
            N_NIM_Sync++;
            //SYNC_NIM_FLAG = 1;
            //TS_NIM_Sync_pre = TS_NIM_Sync;
          }
        } // End of 5c Event

        if (data == 0x7fff000a) {
          Keyword = GetKeyword(rawdata_nimtdc);
          if (Keyword == 1) {
            SYNC_NIM_FLAG = 1;
          }
        }

        if (Header == 1) { // Trigger (0x01 event)
          N_NIM_Last = data & 0x00ffffff;
        }

        if (data == 0xffaa0000) {
          int count = ReadNext32bit(rawdata_nimtdc);
          N_NIM_Last2 = (count >> 8) & 0x00ffffff;
        }

        //===== Get Leading/Trailing edge =====
        if (Header == 2) { //created every 65us
          N_02event = data & 0x0000ffff;
        }

        if (Header == 3) { // Leading Edge (0x03 event) less than 65us
          ch = (data >> 16) & 0x000000ff;
          time_L = data & 0x0000ffff;
          time_L += pow(2, 16) * N_02event;
          if (Traw_NIM_num[0][ch] < hitNmax) {
            Traw_NIM_L[ch][Traw_NIM_num[0][ch]] = time_L;
            Traw_NIM_num[0][ch]++;
          }
        }

        if (Header == 4) { // Trailing Edge (0x04 event)
          ch = (data >> 16) & 0x000000ff;
          time_T = data & 0x0000ffff;
          time_T += pow(2, 16) * N_02event;
          if (Traw_NIM_num[1][ch] < hitNmax) {
            Traw_NIM_T[ch][Traw_NIM_num[1][ch]] = time_T;
            Traw_NIM_TOT[ch][Traw_NIM_num[1][ch]] = Traw_NIM_T[ch][Traw_NIM_num[1][ch]] - Traw_NIM_L[ch][Traw_NIM_num[1][ch]];
          }
          if (Traw_NIM_TOT[ch][Traw_NIM_num[1][ch]] > 0 && Traw_NIM_TOT[ch][Traw_NIM_num[1][ch]] < TOT_Noise_NIM) {
            Traw_NIM_L_valid[ch] = time_L;
            Traw_NIM_T_valid[ch] = time_T;
            //cout << "NIM: " << ch << ", " << Traw_NIM_L_valid[ch] << endl;
          }
          Traw_NIM_num[1][ch]++;
        }

        if (data == 0xff550000) { // Copper Trailer
          SAME_EVENT_FLAG = false;
          LOS_NIM_FLAG = CheckLOS(rawdata_nimtdc);
          if (LOS_NIM_FLAG) {
            N_NIM_LOS++;
          }
          bool breakOK = true;
          if (Skip_NIM) breakOK = false;
          Skip_NIM = false;
          if (LOAD_N_FLAG_NIM && ONLINE_FLAG) breakOK = true;
          if (breakOK) break;
        }

        if (rawdata_nimtdc.eof()) {
          END_NIM_FLAG = true;
          break;
        } else if (rawdata_nimtdc.fail()) cout << "Eroor : file read error" << endl;
      }
    }
    if (dTS_NIM_seq.size() >= LOAD_N) LOAD_N_FLAG_NIM = false;

    for (int nIP = 0; nIP < N_IP; nIP++) {
      //===== For Reline Up ======
      configureIP(nIP, xy, ud, oi, ch_offset, Layer_No);
      //===== Start Reading each Rawdata ======
      while (!rawdata[nIP].eof()  && !Stop_KAL[nIP] ) {
        //===== SKIP_FLAG =====
        /*
        if(SKIP_FLAG[nIP]){
          SKIP_FLAG[nIP] = false; break;
        }
        */
        char Byte[4];
        rawdata[nIP].read(Byte, 4); // reading 4 byte (32 bit)
        unsigned int data = Read_Raw_32bit(Byte);
        //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
        int Header = (data & 0xff000000) >> 24;

        if (Header == 0x5c) { //Copper Header, Initialize, GetNETtime (0x5c event), Event Matching
          //===== Initialize Readfile for the next event =====
          SAME_EVENT_FLAG = true;
          FILL_FLAG = true;
          SYNC_FLAG[nIP] = false;
          if (nIP == 0) {
            ch = -999;
            N_02event = 0;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 32; j++)
                for (int k = 0; k < 10; k++) Traw_KAL_L[i][j][k] = -1e4;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 32; j++)
                for (int k = 0; k < 10; k++) Traw_KAL_T[i][j][k] = -2e4;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 32; j++) Traw_KAL_L_valid[i][j] = -1e4;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 32; j++) Traw_KAL_T_valid[i][j] = -2e4;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 32; j++)
                for (int k = 0; k < 10; k++) Traw_KAL_TOT[i][j][k] = -1e4;
            for (int i = 0; i < 12; i++)
              for (int j = 0; j < 2; j++)
                for (int k = 0; k < 32; k++) Traw_KAL_num[i][j][k] = 0;
            for (int i = 0; i < 12; i++) SYNC_FLAG[i] = false;
            //===== Reline up =====
            for (int i = 0; i < 2; i++) { // x or y
              for (int j = 0; j < 2; j++) { // up or dpwn
                for (int k = 0; k < 2; k++) { // in or out
                  for (int l = 0; l < 64; l++) { // ch
                    Fiber_L[i][j][k][l] = -1e4;
                    Fiber_T[i][j][k][l] = -2e4;
                    Fiber_TOT[i][j][k][l] = -1e4;
                    Fiber_num[i][j][k] = 0;
                    Fiber_FLAG[i][j][k] = false;
                    Fiber_CH_FLAG[i][j][k][l] = false;
                    Tracking_FLAG[j] = false;
                  }
                }
              }
            }
          }

          //===== Read and calculate TimeStamp =====
          if (N_event[nIP] == 0 && !Skip_KAL[nIP]) {
            TS_KAL_0[nIP] = GetNETtime(rawdata[nIP], data);
            TS_KAL[nIP] = 0.0;
            dTS_KAL[nIP] = 0.0;
            TS_KAL_pre[nIP] = 0.0;
            TS_diff[nIP] = 0.0;
          } else {
            TS_KAL[nIP] = GetNETtime(rawdata[nIP], data) - TS_KAL_0[nIP];
            dTS_KAL[nIP] = TS_KAL[nIP] - TS_KAL_pre[nIP];
            if (!SYNC_ONLINE_FLAG[nIP]) {
              if (dTS_KAL_seq[nIP].size() >= LOAD_N) {
                dTS_KAL_seq[nIP].erase(dTS_KAL_seq[nIP].begin());
              }
              dTS_KAL_seq[nIP].push_back(dTS_KAL[nIP]);
            }
            if (fNIM) {
              TS_diff[nIP] = TS_KAL[nIP] - TS_NIM;
              dTS_diff[nIP] = dTS_KAL[nIP] - dTS_NIM;
            } else {
              TS_NIM = TS_KAL[0];
              dTS_NIM = dTS_KAL[0];
              N_NIM_event = N_event[0];
              TS_diff[nIP] = TS_KAL[nIP] - TS_KAL[0];
              dTS_diff[nIP] = dTS_KAL[nIP] - dTS_KAL[0];
            }
            TS_KAL_calib[nIP] = TS_KAL[nIP] - TS_diff[nIP];
            TS_KAL_pre[nIP] = TS_KAL[nIP];
          }

          //===== Increment N_event ======
          N_event[nIP]++;
          N_Sync_Interval[nIP]++;

          //====== Check Sync Trigger =====
          if (dTS_KAL[nIP] < DoublePulseWindow) {
            N_KAL_Sync[nIP]++;
            SYNC_FLAG[nIP] = true;
            TS_KAL_Sync[nIP] = TS_KAL[nIP];

            if (nIP == N_IP - 1) {
              auto N_Sync_IntervalMax = max_element(N_Sync_Interval.begin(), N_Sync_Interval.end());
              auto N_Sync_IntervalMin = min_element(N_Sync_Interval.begin(), N_Sync_Interval.end());
            }
          }
        } // End of 5c Event

        if (Header == 1) { // Trigger (0x01 event)
          N_KAL_Last[nIP] = data & 0x00ffffff;
        }

        if (data == 0xffaa0000) {
          int count = ReadNext32bit(rawdata[nIP]);
          N_KAL_Last2[nIP] = (count >> 8) & 0x00ffffff;
        }

        //===== Get Leading/Trailing edge =====
        if (SAME_EVENT_FLAG) {
          if (Header == 2) { //created every 65us
            N_02event = data & 0x0000ffff;
          }
          if (Header == 3) { // Leading Edge (0x03 event) less than 65us
            ch = (data >> 16) & 0x000000ff;
            time_L = data & 0x0000ffff;
            time_L += pow(2, 16) * N_02event;
            if (Traw_KAL_num[nIP][0][ch] < hitNmax) {
              Traw_KAL_L[nIP][ch][Traw_KAL_num[nIP][0][ch]] = time_L;
              Traw_KAL_num[nIP][0][ch]++;
              Traw_KAL_num_total[nIP][0][ch]++;
            }
	    KalNum[nIP][ch]++;
          }
          if (Header == 4) { // Trailing Edge (0x04 event)
            ch = (data >> 16) & 0x000000ff;
            time_T = data & 0x0000ffff;
            time_T += pow(2, 16) * N_02event;

            if (Traw_KAL_num[nIP][1][ch] < hitNmax) {
              Traw_KAL_T[nIP][ch][Traw_KAL_num[nIP][1][ch]] = time_T;
              Traw_KAL_TOT[nIP][ch][Traw_KAL_num[nIP][1][ch]] = Traw_KAL_T[nIP][ch][Traw_KAL_num[nIP][1][ch]] - Traw_KAL_L[nIP][ch][Traw_KAL_num[nIP][1][ch]];

              if (Traw_KAL_TOT[nIP][ch][Traw_KAL_num[nIP][1][ch]] > 0 && Traw_KAL_TOT[nIP][ch][Traw_KAL_num[nIP][1][ch]] < TOT_Noise_KAL[nIP]) {
                Traw_KAL_L_valid[nIP][ch] = time_L;
                Traw_KAL_T_valid[nIP][ch] = time_T;
                //cout << "KAL: " << ch << ", " << Traw_KAL_L_valid[nIP][ch] << endl;
                int fiber_ch = ch + ch_offset * 32;
                AssignFiber(fiber_ch, ud, oi);
                Fiber_num[xy][ud][oi]++;
                Fiber_L[xy][ud][oi][fiber_ch] = time_L;
                Fiber_T[xy][ud][oi][fiber_ch] = time_T;
                Fiber_CH_FLAG[xy][ud][oi][fiber_ch] = true;
              }
              Traw_KAL_num[nIP][1][ch]++;
              Traw_KAL_num_total[nIP][1][ch]++;
            }
          }
          if (data == 0xff550000) { // Copper Trailer
            SAME_EVENT_FLAG = false;
            LOS_FLAG[nIP] = CheckLOS(rawdata[nIP]);
            if (LOS_FLAG[nIP]) {
              N_KAL_LOS[nIP]++;
            }
            bool breakOK = true;
            if (Skip_KAL[nIP]) breakOK = false;
            Skip_KAL[nIP] = false;
            if (!SYNC_ONLINE_FLAG[nIP] && ONLINE_FLAG) breakOK = true;
            if (breakOK) break;
          }
        }
	
        //====== EOF FLAG -> Stop Reading Rawdata  =====
        if (rawdata[nIP].eof()) {
          //cout << "reach end of loop" << endl;
          END_FLAG[nIP] = true;
          break;
        } else if (rawdata[nIP].fail()) cout << "Error : file read error in IP " << vecIP[nIP] << endl;
      }
    }

    if (ONLINE_FLAG && !SYNC_ONLINE_FLAG_All) {
      double tolerance = 0.3e-6;
      for (int nIP = 0; nIP < N_IP; nIP++) {
        if (dTS_KAL_seq[nIP].size() == LOAD_N) {
            bool match = true;
            for (size_t i = 0; i < LOAD_N; ++i) {
                if (abs(dTS_KAL_seq[nIP][i] - dTS_NIM_seq[i]) > tolerance) {
                    match = false;
                    break;
                }
            }
            if (match) {
                //cout << Form("dTS_KAL_seq[%02d]: ",                                       nIP);
                //for (const auto& value : dTS_KAL_seq[nIP]) {
                //    cout << Form("%2.2f", value * 1e6) << " ";
                //}
                //cout << endl;
                //cout << "dTS_NIM_seq:     ";
                //for (const auto& value : dTS_NIM_seq) {
                //    cout << Form("%2.2f", value * 1e6) << " ";
                //}
                //cout << endl;
                SYNC_ONLINE_FLAG[nIP] = true;
            }
        }
      }
      Stop_NIM = !LOAD_N_FLAG_NIM;
      for (int nIP = 0; nIP < N_IP; nIP++) {
        Stop_KAL[nIP] = SYNC_ONLINE_FLAG[nIP];
      }

      SYNC_ONLINE_FLAG_All = true;
      for (size_t nIP = 0; nIP < N_IP; nIP++) {
          if (!SYNC_ONLINE_FLAG[nIP]) {
              SYNC_ONLINE_FLAG_All = false;
              break;
          }
      }
      if (SYNC_ONLINE_FLAG_All) {
        for (size_t nIP = 0; nIP < N_IP; nIP++) {
          Stop_KAL[nIP] = false;
          N_event[nIP] = 0;
        }
        Stop_NIM = false;
        N_NIM_event = 0;
      }
    }

    //===== Display Procedure of Event Loop =====
    if (N_event[0] == 0) cout << endl;
    if (N_event[0] % 1000 == 0) {
      //double Trun = RunTimer.RealTime();
      double Trun = RunTimer.RealTime();
      RunTimer.Continue();
      double rate = N_event[0] / Trun;
      cout << fixed << setprecision(1) << "Timer :  " << Trun << " s, " <<
        "Count Rate : " << (int) rate << " cps, " <<
        "Reading    : " << N_event[0] << flush << " events" << "\r";
    }
    if (KAL_ONLINE_FLAG) if (N_event[0] > 1000) break;

    //====== Filling Histgrams for Rawdata =====
    for (int i = 0; i < 12; i++) {
      if (END_FLAG[i]) {
        WHOLE_FLAG = false;
        break;
      }
    }
    if (fNIM)
      if (END_NIM_FLAG) WHOLE_FLAG = false;

    FILL_FLAG &= WHOLE_FLAG;

    auto[minValue1, maxValue1] = FindMinMax(N_IP, dTS_KAL, dTS_NIM);
    SkipOrNot(N_IP, dTS_KAL, dTS_NIM,
      minValue1, maxValue1,
      Skip_KAL, Skip_NIM, SKIP_FLAG_new);

    bool ftemp = (N_event[0] == 2);
    if (ONLINE_FLAG) ftemp &= SYNC_ONLINE_FLAG_All; 
    
    if (ftemp) {
        TString title = Form("Data path: %s | runN: %d", path.c_str(), runN);
        cout << endl;
        cout << title.Data() << endl;
        cout << "--------------------------------------------------------------------------------" << endl;
        cout << "------ Events Mismatch ---------------------------------------------------------" << endl;
        cout << "--------------------------------------------------------------------------------" << endl;
    
        // Console output using ROOT Form
        cout << Form("%10s || %5s", "Event no.", "NIM");
        for (size_t i = 0; i < vecIP.size(); ++i) {
            cout << Form(" | %5d", vecIP[i]);
        }
        cout << " |" << endl;
        cout << "------------------------------------------------------- |" << endl;
    
        // HTML header
        stat_file << "        <h1>" << title.Data() << "</h1>\n";
        stat_file << "        <h2>Events Mismatch</h2>\n";
        stat_file << "        <table border=\"1\">\n";
        stat_file << "            <tr><th colspan=\"" << 1 << "\">Number of Events</th>"
                  << "<th colspan=\"" << vecIP.size()+1 << "\">TS Interval [us]</th></tr>\n";
        stat_file << "            <tr>\n";
        stat_file << "                <td>NIM</td>\n";
        stat_file << "                <td>NIM</td>\n";
        for (size_t i = 0; i < vecIP.size(); ++i) {
            stat_file << "                <td>KAL[" << vecIP[i] << "]</td>\n";
        }
        stat_file << "            </tr>\n";
    }
    
    if (ONLINE_FLAG) {
        if (FILL_FLAG && SKIP_FLAG_new && SYNC_ONLINE_FLAG_All) {
            // Console output using ROOT Form
            cout << Form("%10d || %5.2f", N_NIM_event, 1e6 * dTS_NIM);
            for (size_t i = 0; i < vecIP.size(); ++i) {
                cout << Form(" | %5.2f", 1e6 * dTS_KAL[i]);
            }
            cout << " | " << endl;
    
            SkipN++;
    
            // HTML output
            stat_file << "            <tr>\n";
            stat_file << "                <td>" << N_NIM_event << "</td>\n";
            stat_file << "                <td>" << Form("%.2f", 1e6 * dTS_NIM) << "</td>\n";
            for (size_t i = 0; i < vecIP.size(); ++i) {
                stat_file << "                <td>" << Form("%.2f", 1e6 * dTS_KAL[i]) << "</td>\n";
            }
            stat_file << "            </tr>\n";
        }
    } else {
        if (FILL_FLAG && SKIP_FLAG_new) {
            // Console output using ROOT Form
            cout << Form("%10d || %5.2f", N_NIM_event, 1e6 * dTS_NIM);
            for (size_t i = 0; i < vecIP.size(); ++i) {
                cout << Form(" | %5.2f", 1e6 * dTS_KAL[i]);
            }
            cout << " | " << endl;
    
            SkipN++;
    
            // HTML output
            stat_file << "            <tr>\n";
            stat_file << "                <td>" << N_NIM_event << "</td>\n";
            stat_file << "                <td>" << Form("%.2f", 1e6 * dTS_NIM) << "</td>\n";
            for (size_t i = 1; i < vecIP.size(); ++i) {
                stat_file << "                <td>" << Form("%.2f", 1e6 * dTS_KAL[i]) << "</td>\n";
            }
            stat_file << "            </tr>\n";
        }
    }

    //===== Filling data ==========================================================================================
    if (FILL_FLAG && !SKIP_FLAG_new) {

      for (int ii = 0; ii < 32; ii++) {
        for (int jj = 0; jj < hitNmax; jj++) {
          if (ii < 4) hNIM_L[ii] -> Fill(Traw_NIM_L[ii][jj]);
          //hNIM_T[ii] -> Fill(Traw_NIM_T[ii][jj]);
          //hNIM_TOT[ii] -> Fill(Traw_NIM_TOT[ii][jj]);
          hNIM_L2 -> Fill(ii, Traw_NIM_L[ii][jj]);
          hNIM_T2 -> Fill(ii, Traw_NIM_T[ii][jj]);
          hNIM_TOT2 -> Fill(ii, Traw_NIM_TOT[ii][jj]);
          hNIM_Multi -> Fill(ii, Traw_NIM_num[0][ii]);
        }
      }

      for (int nIP = 0; nIP < N_IP; nIP++) {
        for (int ii = 0; ii < 32; ii++) {
          for (int jj = 0; jj < hitNmax; jj++) {
            //hKAL_L[nIP][ii] -> Fill(Traw_KAL_L[nIP][ii][jj]);
            //hKAL_T[nIP][ii] -> Fill(Traw_KAL_T[nIP][ii][jj]);
            //hKAL_TOT[nIP][ii] -> Fill(Traw_KAL_TOT[nIP][ii][jj]);
            hKAL_L2[nIP] -> Fill(ii, Traw_KAL_L[nIP][ii][jj]);
            hKAL_T2[nIP] -> Fill(ii, Traw_KAL_T[nIP][ii][jj]);
            hKAL_TOT2[nIP] -> Fill(ii, Traw_KAL_TOT[nIP][ii][jj]);
          }
          hKAL_Multi[nIP] -> Fill(ii, Traw_KAL_num[nIP][0][ii]);
          if (Traw_KAL_L_valid[nIP][ii] > 0 && Traw_NIM_L_valid[1] > 0) hKAL_NIM2[nIP] -> Fill(ii, Traw_KAL_L_valid[nIP][ii] - Traw_NIM_L_valid[1]);
          //if (Traw_KAL_L_valid[nIP][ii] > 0 && Traw_NIM_L_valid[1] > 0) cout << Form("nIP: %d, ch: %d, KAL_L: %d, NIM: %d", nIP, ii, Traw_KAL_L_valid[nIP][ii], Traw_NIM_L_valid[1]) << endl;
        }
        hdTS_KAL[nIP] -> Fill(dTS_KAL[nIP]);
        hdTS_diff[nIP] -> Fill(dTS_diff[nIP]);
        hTS_KAL2[nIP] -> Fill(TS_NIM, TS_KAL[nIP]);
        hMonitor_TS_KAL -> Fill(nIP, TS_KAL[nIP]);
        if (fNIM) hTS_diff[nIP] -> Fill(TS_NIM, TS_diff[nIP]);
        else hTS_diff[nIP] -> Fill(TS_KAL[0], TS_diff[nIP]);
        if (fNIM) hdTS_calib[nIP] -> Fill(TS_NIM, TS_KAL_calib[nIP] - TS_NIM);
        else hdTS_calib[nIP] -> Fill(TS_KAL[0], TS_KAL_calib[nIP] - TS_KAL[0]);
        hMonitor_dTS_diff -> Fill(nIP, dTS_diff[nIP]);
        if (SYNC_FLAG[nIP]) hMonitor_TS_Sync -> Fill(nIP, TS_KAL_Sync[nIP]);
      }

      //====== Tracking ======
      #ifdef TRACKING_ON
      double a[2][2] = {
        -999.0
      };
      //====== Judge whether Tracking Available or not ======
      //====== Noise Cut by TOT & 4-Layer-Coincidence by Fiber_num ======
      for (int ud = 0; ud < 2; ud++) {
        for (int xy = 0; xy < 2; xy++) {
          for (int oi = 0; oi < 2; oi++) {
            if (oi == 0) ch_max = 64;
            else ch_max = 32;
            if (Fiber_num[xy][ud][oi] > 0 && Fiber_num[xy][ud][oi] < 2) {
              Fiber_FLAG[xy][ud][oi] = true;
              for (int ch = 0; ch < ch_max; ch++) {
                if (Fiber_CH_FLAG[xy][ud][oi]) {
                  a[xy][oi] = (double) ch + 0.5 - ((double) ch_max / 2.0);
                }
              }
            }
          }
        }
        // 4-Layer Coincidence
        Tracking_FLAG[ud] = (Fiber_FLAG[0][ud][0] && Fiber_FLAG[0][ud][1] && Fiber_FLAG[1][ud][0] && Fiber_FLAG[1][ud][1]);
      }

      //======= Start Tracking ======
      for (int ud = 0; ud < 2; ud++) {
        double x0 = -999.0, x1 = -999.0, x2 = -999.0, x3 = -999.0;
        double y0 = -999.0, y1 = -999.0, y2 = -999.0, y3 = -999.0;
        double t = -999.0;
        double x = -999.0, y = -999.0, z = -999.0;

        if (Tracking_FLAG[ud]) {
          //====== Tracking Parameters =======
          // Plane 0 : Y-in  (x0,y0,z0)
          // Plane 1 : X-in  (x1,y1,z1)
          // Plane 2 : Y-out (x2,y2,z2)
          // Plane 3 : X-out (x3,y3,z3[ud])
          x1 = a[0][0], x3 = a[0][1], y0 = a[1][0], y2 = a[1][1];

          // interpolate y1 & x2 using triangle simirality
          // z0 ~ z3 are defined in "setup.h"
          y1 = y2 - ((y2 - y0) * (z2[ud] - z1[ud])) / (z2[ud] - z0[ud]);
          x2 = x3 - ((x3 - x1) * (z3[ud] - z2[ud])) / (z3[ud] - z1[ud]);

          // the vector equation of straight line L passing through two points, A and B
          // L = OA + t*AB = OA + t*(OB-OA) = (1-t)*OA + t*OB
          // t is a parametric variable, calculated by following equation.
          t = (z1[ud] - y1 * tan(theta)) / ((z1[ud] - y1 * tan(theta)) - (z2[ud] - y2 * tan(theta)));

          // calculate the position in sample plane
          x = (1 - t) * x1 + t * x2;
          y = (1 - t) * y1 + t * y2;
          z = (1 - t) * z1[ud] + t * z2[ud];

          hFiber_out[ud] -> Fill(x3, y2);
          hFiber_in[ud] -> Fill(x1, y0);
          hSample_Projection -> Fill(x, y);
          y = y / cos(theta);
          hSample_Plane -> Fill(x, y);

          cout << "Tracking Available!!!!" << endl;
          N_track++;
        }
      }
      //====== Filling Histgrams for Fiber ===== 
      for (int ch = 0; ch < 64; ch++) {
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
              hFiber_L[i][j][k] -> Fill(ch, Fiber_L[i][j][k][ch]);
              hFiber_T[i][j][k] -> Fill(ch, Fiber_T[i][j][k][ch]);
              hFiber_TOT[i][j][k] -> Fill(ch, Fiber_TOT[i][j][k][ch]);
            }
          }
        }
      }

      //for (int ii = 0; ii < 12; ii++) N_event[ii] ++;
      //for (int ii = 0; ii < 1;  ii++) N_NIM_event ++;
      #endif
      if (ftree) tree -> Fill();
    } //End of Fill Loop
  }

  //====== Histogram & Fitting ==================================================================================
  int col = int(sqrt(double(N_IP)) + 0.5);
  int row = (N_IP + col - 1) / col;

  if (col < row) {
    swap(col, row);
  }

  //===== Time stamp ============================================================================================
  TCanvas * c_TS = new TCanvas("====== Time Stamp ======", "====== Time Stamp ======", 1, 1);
  c_TS -> Write();

  // dTS vs TS_NIM 
  vector < TF1 * > fdTS;
  fdTS.resize(N_IP);
  for (int i = 0; i < N_IP; i++) {
    fdTS[i] = new TF1(Form("fdTS_%d", i), "pol1", 0, 10);
  }

  TCanvas * c_dTS = new TCanvas("c_dTS", "c_dTS", 1200, 600);
  c_dTS -> Divide(col, row);

  for (int ii = 0; ii < N_IP; ii++) {
    c_dTS -> cd(ii + 1);
    SetMargins();
    hTS_diff[ii] -> Fit(fdTS[ii], "L", "", 0, 4);
    hTS_diff[ii] -> Draw("colz");
    gPad -> SetLogy(0);
    gPad -> Update();
    c_dTS -> cd(ii + 1) -> Modified();
    c_dTS -> cd(ii + 1) -> Update();
  }

  c_dTS -> Write();

  // dTS_cal vs TS_NIM
  //vector < TF1 * > fdTS_calib;
  //fdTS_calib.resize(N_IP);
  //for (int i = 0; i < N_IP; i++) {
  //  fdTS_calib[i] = new TF1(Form("fdTS_calib_%d", i), "pol1", 0, 10);
  //}

  //TCanvas * c_dTS_calib = new TCanvas("c_dTS_calib", "c_dTS_calib", 1200, 600);
  //c_dTS_calib -> Divide(col, row);

  //for (int ii = 0; ii < N_IP; ii++) {
  //  c_dTS_calib -> cd(ii + 1);
  //  SetMargins();
  //  hdTS_calib[ii] -> Fit(fdTS_calib[ii], "L", "", 0, 4);
  //  hdTS_calib[ii] -> Draw("colz");
  //  gPad -> SetLogy(0);
  //  gPad -> Update();
  //  c_dTS_calib -> cd(ii + 1) -> Modified();
  //  c_dTS_calib -> cd(ii + 1) -> Update();
  //}

  //c_dTS_calib -> Write();

  // dTS expo fitting
  vector < TF1 * > fdTS_KAL;
  fdTS_KAL.resize(N_IP);
  for (int i = 0; i < N_IP; i++) {
    fdTS_KAL[i] = new TF1(Form("fdTS_KAL_%d", i), "[0] + [1] * exp(-[2] * x)", 0, 1e-2);
  }

  TCanvas * c_dTS_KAL = new TCanvas("c_dTS_KAL", "c_dTS_KAL", 1200, 600);
  c_dTS_KAL -> Divide(col, row);

  for (int ii = 0; ii < N_IP; ii++) {
    c_dTS_KAL -> cd(ii + 1);
    SetMargins();
    hdTS_KAL[ii] -> Fit(fdTS_KAL[ii], "L", "", 0, 100e-6);
    hdTS_KAL[ii] -> Draw();
    gPad -> SetLogy(1);
    gPad -> Update();
    c_dTS_KAL -> cd(ii + 1) -> Modified();
    c_dTS_KAL -> cd(ii + 1) -> Update();
  }

  c_dTS_KAL -> Write();
  //============================================================================================ Time stamp =====

  //===== TDC ===================================================================================================
  TCanvas * cTDC = new TCanvas("====== TDC =============", "====== TDC =============", 1, 1);
  cTDC -> Write();

  //==== Leading ============================================================= 
  TCanvas * cLeading = new TCanvas("----- Leading ----------------------------", "----- Leading ----------------------------", 1, 1);
  cLeading -> Write();
  //NIM
  TCanvas * cNIM_L2 = new TCanvas(Form("cNIM_L2"), Form("cNIM_L2"), 1200, 600);
  cNIM_L2 -> cd(1);
  SetMargins();
  hNIM_L2 -> Draw("colz");
  gPad -> SetLogz(1);
  gPad -> Update();
  cNIM_L2 -> cd(1) -> Modified();
  cNIM_L2 -> cd(1) -> Update();
  cNIM_L2 -> Write();

  vector < TF1 * > fNIM_L;
  fNIM_L.resize(4);
  for (int jj = 0; jj < 4; jj++) {
    fNIM_L[jj] = new TF1(Form("fNIM_ch%02d", jj), "[0] + [1] * exp(-[2] * x)", 2e3, 70e3);
    fNIM_L[jj] -> SetParameters(2, 5, 4.5e-4);
    if (jj == 1) hNIM_L[jj] -> Fit(fNIM_L[jj], "L", "", 2e3, 70e3);
  }
  TCanvas * cNIM_L;
  cNIM_L = new TCanvas(Form("cNIM_L"), Form("cNIM_L"), 1200, 600);
  cNIM_L -> Divide(2, 2);
  for (int jj = 0; jj < 4; jj++) {
    cNIM_L -> cd(jj + 1);
    SetMargins();
    hNIM_L[jj] -> Draw("");
    fNIM_L[jj] -> Draw("same");
    gPad -> SetLogy(1);
    gPad -> Update();
    cNIM_L -> cd(jj + 1) -> Modified();
    cNIM_L -> cd(jj + 1) -> Update();
  }
  cNIM_L -> Write();

  // 新しいヒストグラムの作成
TH1D* Assym_FB = new TH1D("Assym_FB", "Assym_FB", hNIM_L[0]->GetNbinsX(), 2e3, 70e3);
TH1D* Assym_UD = new TH1D("Assym_UD", "Assym_UD", hNIM_L[0]->GetNbinsX(), 2e3, 70e3);

// ビンごとの演算とヒストグラムへのフィリング
for (int bin = 1; bin <= hNIM_L[0]->GetNbinsX(); bin++) {
    double value1_0 = hNIM_L[0]->GetBinContent(bin);
    double value1_1 = hNIM_L[1]->GetBinContent(bin);
    double value2_0 = hNIM_L[2]->GetBinContent(bin);
    double value2_1 = hNIM_L[3]->GetBinContent(bin);

    double error1_0 = sqrt(value1_0);
    double error1_1 = sqrt(value1_1);
    double error2_0 = sqrt(value2_0);
    double error2_1 = sqrt(value2_1);

    // Assym_FB の計算とフィリング
    if (value1_0 + value1_1 != 0) {
        double assym_fb = (value1_0 - value1_1) / (value1_0 + value1_1);
        double error_fb = sqrt((4 * value1_0 * value1_1) / pow(value1_0 + value1_1, 3));
        Assym_FB->SetBinContent(bin, assym_fb);
        Assym_FB->SetBinError(bin, error_fb);
    }

    // Assym_UD の計算とフィリング
    if (value2_0 + value2_1 != 0) {
        double assym_ud = (value2_0 - value2_1) / (value2_0 + value2_1);
        double error_ud = sqrt((4 * value2_0 * value2_1) / pow(value2_0 + value2_1, 3));
        Assym_UD->SetBinContent(bin, assym_ud);
        Assym_UD->SetBinError(bin, error_ud);
    }
}

TCanvas * cAssym = new TCanvas(Form("cAssym"), Form("cAssym"), 1200, 600);
cAssym -> Divide(2,1);
cAssym -> cd(1);
SetMargins();
Assym_FB->Draw("E");  // "E" オプションを追加してエラーバーを表示
gPad -> SetLogy(0);
gPad -> Update();
cAssym -> cd(1) -> Modified();
cAssym -> cd(1) -> Update();
cAssym -> cd(2);
SetMargins();
Assym_UD->Draw("E same");  // "E" オプションを追加してエラーバーを表示
gPad -> SetLogy(0);
gPad -> Update();
cAssym -> cd(2) -> Modified();
cAssym -> cd(2) -> Update();
cAssym -> Write();


  //Kalliope
  TCanvas * cKAL_L2 = new TCanvas(Form("cKAL_L2"), Form("cKAL_L2"), 1200, 600);
  cKAL_L2 -> Divide(col, row);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_L2 -> cd(ii + 1);
    SetMargins();
    hKAL_L2[ii] -> Draw("colz");
    gPad -> SetLogz(1);
    gPad -> Update();
    cKAL_L2 -> cd(ii + 1) -> Modified();
    cKAL_L2 -> cd(ii + 1) -> Update();
  }
  cKAL_L2 -> Write();
  /*
  vector < vector < TF1 * >> fKAL_L;
  fKAL_L.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    fKAL_L[ii].resize(32);
    for (int jj = 0; jj < 32; jj++) {
      fKAL_L[ii][jj] = new TF1(Form("fKAL_L_%02d_ch%02d", ii, jj), "[0] + [1] * exp(-[2] * x)", 0, 70e3);
      fKAL_L[ii][jj] -> SetParameters(2, 5, 4.5e-4);
      //hKAL_L[ii][jj] -> Fit(fKAL_L[ii][jj], "L", "", 0, 70e3);
    }
  }
  vector < TCanvas * > cKAL_L;
  cKAL_L.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_L[ii] = new TCanvas(Form("cKAL_L_%02d", ii), Form("cKAL_L_%02d", ii), 1200, 600);
    cKAL_L[ii] -> Divide(8, 4);
    for (int jj = 0; jj < 32; jj++) {
      cKAL_L[ii] -> cd(jj + 1);
      SetMargins();
      hKAL_L[ii][jj] -> Draw("");
      fKAL_L[ii][jj] -> Draw("same");
      gPad -> SetLogy(1);
      gPad -> Update();
      cKAL_L[ii] -> cd(jj + 1) -> Modified();
      cKAL_L[ii] -> cd(jj + 1) -> Update();
    }
    cKAL_L[ii] -> Write();
  }
  */

  //==== Trailing ============================================================ 
  TCanvas * cTrailing = new TCanvas("----- Trailing ---------------------------", "----- Trailing ---------------------------", 1, 1);
  cTrailing -> Write();
  // NIM
  TCanvas * cNIM_T2 = new TCanvas(Form("cNIM_T2"), Form("cNIM_T2"), 1200, 600);
  cNIM_T2 -> cd(1);
  SetMargins();
  hNIM_T2 -> Draw("colz");
  gPad -> SetLogz(1);
  gPad -> Update();
  cNIM_T2 -> cd(1) -> Modified();
  cNIM_T2 -> cd(1) -> Update();
  cNIM_T2 -> Write();

  /*
  vector < TF1 * > fNIM_T;
  fNIM_T.resize(32);
  for (int jj = 0; jj < 32; jj++) {
    fNIM_T[jj] = new TF1(Form("fNIM_T_ch%02d", jj), "[0] + [1] * exp(-[2] * x)", 2e3, 70e3);
    fNIM_T[jj] -> SetParameters(2, 5, 4.5e-4);
    //hNIM_T[jj] -> Fit(fNIM_T[jj], "L", "", 2e3, 70e3);
  }
  TCanvas * cNIM_T;
  cNIM_T = new TCanvas(Form("cNIM_T"), Form("cNIM_T"), 1200, 600);
  cNIM_T -> Divide(8, 4);
  for (int jj = 0; jj < 32; jj++) {
    cNIM_T -> cd(jj + 1);
    SetMargins();
    hNIM_T[jj] -> Draw("");
    fNIM_T[jj] -> Draw("same");
    gPad -> SetLogy(1);
    gPad -> Update();
    cNIM_T -> cd(jj + 1) -> Modified();
    cNIM_T -> cd(jj + 1) -> Update();
  }
  cNIM_T -> Write();
  */

  // Kalliope
  TCanvas * cKAL_T2 = new TCanvas(Form("cKAL_T2"), Form("cKAL_T2"), 1200, 600);
  cKAL_T2 -> Divide(col, row);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_T2 -> cd(ii + 1);
    SetMargins();
    hKAL_T2[ii] -> Draw("colz");
    gPad -> SetLogz(1);
    gPad -> Update();
    cKAL_T2 -> cd(ii + 1) -> Modified();
    cKAL_T2 -> cd(ii + 1) -> Update();
  }
  cKAL_T2 -> Write();

  /*
  vector < vector < TF1 * >> fKAL_T;
  fKAL_T.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    fKAL_T[ii].resize(32);
    for (int jj = 0; jj < 32; jj++) {
      fKAL_T[ii][jj] = new TF1(Form("fKAL_T_%02d_ch%02d", ii, jj), "[0] + [1] * exp(-[2] * x)", 0, 70e3);
      fKAL_T[ii][jj] -> SetParameters(2, 5, 4.5e-4);
      //hKAL_T[ii][jj] -> Fit(fKAL_T[ii][jj], "L", "", 0, 70e3);
    }
  }
  vector < TCanvas * > cKAL_T;
  cKAL_T.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_T[ii] = new TCanvas(Form("cKAL_T_%02d", ii), Form("cKAL_T_%02d", ii), 1200, 600);
    cKAL_T[ii] -> Divide(8, 4);
    for (int jj = 0; jj < 32; jj++) {
      cKAL_T[ii] -> cd(jj + 1);
      SetMargins();
      hKAL_T[ii][jj] -> Draw("");
      fKAL_T[ii][jj] -> Draw("same");
      gPad -> SetLogy(1);
      gPad -> Update();
      cKAL_T[ii] -> cd(jj + 1) -> Modified();
      cKAL_T[ii] -> cd(jj + 1) -> Update();
    }
    cKAL_T[ii] -> Write();
  }
  */

  //==== TOT ================================================================= 
  TCanvas * cTOT      = new TCanvas("----- TOT --------------------------------", "----- TOT --------------------------------", 1, 1);
  cTOT -> Write();
  // NIM
  TCanvas * cNIM_TOT2 = new TCanvas(Form("cNIM_TOT2"), Form("cNIM_TOT2"), 1200, 600);
  cNIM_TOT2 -> cd(1);
  SetMargins();
  hNIM_TOT2 -> Draw("colz");
  gPad -> SetLogz(1);
  gPad -> Update();
  cNIM_TOT2 -> cd(1) -> Modified();
  cNIM_TOT2 -> cd(1) -> Update();
  cNIM_TOT2 -> Write();

  /*
  TCanvas * cNIM_TOT;
  cNIM_TOT = new TCanvas(Form("cNIM_TOT"), Form("cNIM_TOT"), 1200, 600);
  cNIM_TOT -> Divide(8, 4);
  for (int jj = 0; jj < 32; jj++) {
    cNIM_TOT -> cd(jj + 1);
    SetMargins();
    hNIM_TOT[jj] -> Draw("");
    gPad -> SetLogy(1);
    gPad -> Update();
    cNIM_TOT -> cd(jj + 1) -> Modified();
    cNIM_TOT -> cd(jj + 1) -> Update();
  }
  cNIM_TOT -> Write();
  */

  // Kalliope
  TCanvas * cKAL_TOT2 = new TCanvas(Form("cKAL_TOT2"), Form("cKAL_TOT2"), 1200, 600);
  cKAL_TOT2 -> Divide(col, row);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_TOT2 -> cd(ii + 1);
    SetMargins();
    hKAL_TOT2[ii] -> Draw("colz");
    gPad -> SetLogz(1);
    gPad -> Update();
    cKAL_TOT2 -> cd(ii + 1) -> Modified();
    cKAL_TOT2 -> cd(ii + 1) -> Update();
  }
  cKAL_TOT2 -> Write();
  /*
  vector < TCanvas * > cKAL_TOT;
  cKAL_TOT.resize(N_IP);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_TOT[ii] = new TCanvas(Form("cKAL_TOT_%02d", ii), Form("cKAL_TOT_%02d", ii), 1200, 600);
    cKAL_TOT[ii] -> Divide(8, 4);
    for (int jj = 0; jj < 32; jj++) {
      cKAL_TOT[ii] -> cd(jj + 1);
      SetMargins();
      hKAL_TOT[ii][jj] -> Draw("");
      gPad -> SetLogy(1);
      gPad -> Update();
      cKAL_TOT[ii] -> cd(jj + 1) -> Modified();
      cKAL_TOT[ii] -> cd(jj + 1) -> Update();
    }
    cKAL_TOT[ii] -> Write();
  }
  */

  //==== Multiplicity ======================================================== 
  TCanvas * cMulti    = new TCanvas("----- Multi ------------------------------", "----- Multi ------------------------------", 1, 1);
  cMulti -> Write();

  TCanvas * cNIM_Multi = new TCanvas(Form("cNIM_Multi"), Form("cNIM_Multi"), 1200, 600);
  cNIM_Multi -> cd(1);
  SetMargins();
  hNIM_Multi -> Draw("colz");
  gPad -> SetLogz(1);
  gPad -> Update();
  cNIM_Multi -> cd(1) -> Modified();
  cNIM_Multi -> cd(1) -> Update();
  cNIM_Multi -> Write();

  TCanvas * cKAL_Multi = new TCanvas(Form("cKAL_Multi"), Form("cKAL_Multi"), 1200, 600);
  cKAL_Multi -> Divide(col, row);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_Multi -> cd(ii + 1);
    SetMargins();
    hKAL_Multi[ii] -> Draw("colz");
    gPad -> SetLogz(ii + 1);
    gPad -> Update();
    cKAL_Multi -> cd(ii + 1) -> Modified();
    cKAL_Multi -> cd(ii + 1) -> Update();
  }
  cKAL_Multi -> Write();

  //==== Valid =============================================================== 
  TCanvas * cValid    = new TCanvas("----- Valid ------------------------------", "----- Valid ------------------------------", 1, 1);
  cValid -> Write();

  TCanvas * cKAL_NIM2 = new TCanvas(Form("cKAL_NIM2"), Form("cKAL_NIM2"), 1200, 600);
  cKAL_NIM2 -> Divide(col, row);
  for (int ii = 0; ii < N_IP; ii++) {
    cKAL_NIM2 -> cd(ii + 1);
    SetMargins();
    hKAL_NIM2[ii] -> Draw("colz");
    gPad -> SetLogz(ii + 1);
    gPad -> Update();
    cKAL_NIM2 -> cd(ii + 1) -> Modified();
    cKAL_NIM2 -> cd(ii + 1) -> Update();
  }
  cKAL_NIM2 -> Write();
  //================================================================================================== TDC ======

  //===== POS ===================================================================================================
  TCanvas * cPOS = new TCanvas("====== POS =============", "====== POS =============", 1, 1);
  cPOS -> Write();
  //==== Fiber =============================================================== 
  TCanvas * cFiber = new TCanvas(Form("cFiber"), Form("cFiber"), 1200, 600);
  cFiber -> Divide(2, 2);

  cFiber -> cd(1);
  SetMargins();
  hFiber_out[0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cFiber -> cd(1) -> Modified(); cFiber -> cd(1) -> Update();

  cFiber -> cd(2);
  SetMargins();
  hFiber_in[0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cFiber -> cd(2) -> Modified(); cFiber -> cd(2) -> Update();

  cFiber -> cd(3);
  SetMargins();
  hFiber_out[1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cFiber -> cd(3) -> Modified(); cFiber -> cd(3) -> Update();

  cFiber -> cd(4);
  SetMargins();
  hFiber_in[1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cFiber -> cd(4) -> Modified(); cFiber -> cd(4) -> Update();

  cFiber -> Write();

  //==== Sample ============================================================== 
  TCanvas * cSample = new TCanvas(Form("cSample"), Form("cSample"), 1200, 600);
  cSample -> Divide(2, 1);

  cSample -> cd(1);
  SetMargins();
  hSample_Projection -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cSample -> cd(1) -> Modified(); cSample -> cd(1) -> Update();

  cSample -> cd(2);
  SetMargins();
  hSample_Plane -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cSample -> cd(2) -> Modified(); cSample -> cd(2) -> Update();

  cSample -> Write();

  //===== FIBER =================================================================================================
  TCanvas * cFIBER = new TCanvas("====== FIBER ===========", "====== FIBER ===========", 1, 1);
  cFIBER -> Write();
  //==== UP x ================================================================ 
  TCanvas * cUPx = new TCanvas(Form("cUPx"), Form("cUPx"), 1200, 600);
  cUPx -> Divide(3, 2);

  cUPx -> cd(1);
  SetMargins();
  hFiber_L[0][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(1) -> Modified(); cUPx -> cd(1) -> Update();

  cUPx -> cd(2);
  SetMargins();
  hFiber_T[0][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(2) -> Modified(); cUPx -> cd(2) -> Update();

  cUPx -> cd(3);
  SetMargins();
  hFiber_TOT[0][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(3) -> Modified(); cUPx -> cd(3) -> Update();

  cUPx -> cd(4);
  SetMargins();
  hFiber_L[0][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(4) -> Modified(); cUPx -> cd(4) -> Update();

  cUPx -> cd(5);
  SetMargins();
  hFiber_T[0][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(5) -> Modified(); cUPx -> cd(5) -> Update();

  cUPx -> cd(6);
  SetMargins();
  hFiber_TOT[0][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPx -> cd(6) -> Modified(); cUPx -> cd(6) -> Update();

  cUPx -> Write();

  //==== UP y ================================================================ 
  TCanvas * cUPy = new TCanvas(Form("cUPy"), Form("cUPy"), 1200, 600);
  cUPy -> Divide(3, 2);

  cUPy -> cd(1);
  SetMargins();
  hFiber_L[1][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(1) -> Modified(); cUPy -> cd(1) -> Update();

  cUPy -> cd(2);
  SetMargins();
  hFiber_T[1][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(2) -> Modified(); cUPy -> cd(2) -> Update();

  cUPy -> cd(3);
  SetMargins();
  hFiber_TOT[1][0][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(3) -> Modified(); cUPy -> cd(3) -> Update();

  cUPy -> cd(4);
  SetMargins();
  hFiber_L[1][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(4) -> Modified(); cUPy -> cd(4) -> Update();

  cUPy -> cd(5);
  SetMargins();
  hFiber_T[1][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(5) -> Modified(); cUPy -> cd(5) -> Update();

  cUPy -> cd(6);
  SetMargins();
  hFiber_TOT[1][0][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cUPy -> cd(6) -> Modified(); cUPy -> cd(6) -> Update();

  cUPy -> Write();

  //==== DN x ================================================================ 
  TCanvas * cDNx = new TCanvas(Form("cDNx"), Form("cDNx"), 1200, 600);
  cDNx -> Divide(3, 2);

  cDNx -> cd(1);
  SetMargins();
  hFiber_L[0][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(1) -> Modified(); cDNx -> cd(1) -> Update();

  cDNx -> cd(2);
  SetMargins();
  hFiber_T[0][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(2) -> Modified(); cDNx -> cd(2) -> Update();

  cDNx -> cd(3);
  SetMargins();
  hFiber_TOT[0][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(3) -> Modified(); cDNx -> cd(3) -> Update();

  cDNx -> cd(4);
  SetMargins();
  hFiber_L[0][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(4) -> Modified(); cDNx -> cd(4) -> Update();

  cDNx -> cd(5);
  SetMargins();
  hFiber_T[0][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(5) -> Modified(); cDNx -> cd(5) -> Update();

  cDNx -> cd(6);
  SetMargins();
  hFiber_TOT[0][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNx -> cd(6) -> Modified(); cDNx -> cd(6) -> Update();

  cDNx -> Write();

  //==== DN y ================================================================ 
  TCanvas * cDNy = new TCanvas(Form("cDNy"), Form("cDNy"), 1200, 600);
  cDNy -> Divide(3, 2);

  cDNy -> cd(1);
  SetMargins();
  hFiber_L[1][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(1) -> Modified(); cDNy -> cd(1) -> Update();

  cDNy -> cd(2);
  SetMargins();
  hFiber_T[1][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(2) -> Modified(); cDNy -> cd(2) -> Update();

  cDNy -> cd(3);
  SetMargins();
  hFiber_TOT[1][1][0] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(3) -> Modified(); cDNy -> cd(3) -> Update();

  cDNy -> cd(4);
  SetMargins();
  hFiber_L[1][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(4) -> Modified(); cDNy -> cd(4) -> Update();

  cDNy -> cd(5);
  SetMargins();
  hFiber_T[1][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(5) -> Modified(); cDNy -> cd(5) -> Update();

  cDNy -> cd(6);
  SetMargins();
  hFiber_TOT[1][1][1] -> Draw("colz");
  gPad -> SetLogz(1); gPad -> Update();
  cDNy -> cd(6) -> Modified(); cDNy -> cd(6) -> Update();

  cDNy -> Write();

  //===== End of Event Loop ====
  for (int i = 0; i < 32; i++) {
    if (i == 0) outfile << "nIP=0, CH, total_count" << endl;
    outfile << i << ", " << Traw_KAL_num_total[0][0][i] << endl;
    if (i == 32) cout << "ThDAC was written" << endl;
  }

  for (int nIP = 0; nIP < N_IP; nIP++) {
    rawdata[nIP].close();
  }

  //ofNevent << setw(10) << "DAQ"
	//   << ", " << setw(10) << "N_event"
	//   << ", " << setw(10) << "N_Trigger"
	//   << ", " << setw(10) << "N_Sync(x2)"
	//   << ", " << setw(15) << "Last Trig. No."
	//   << ", " << setw(15) << "Last Trig.2 No."
	//   << ", " << setw(10) << "LOS FLAG" << endl;
  //ofNevent << setw(10) << "NIM-TDC"
	//   << ", " << setw(10) << N_NIM_event
	//   << ", " << setw(10) << N_NIM_event - N_NIM_Sync * 2
	//   << ", " << setw(10) << N_NIM_Sync * 2
	//   << ", " << setw(15) << N_NIM_Last
	//   << ", " << setw(15) << N_NIM_Last2
	//   << ", " << setw(10) << N_NIM_LOS << endl;  

  //for(int IP=0;IP<N_IP;IP++){
  //  ofNevent << setw(10) << Form("Kalliope %d", IP)
	//     << ", " << setw(10) << N_event[IP]
	//   << ", " << setw(10) << N_NIM_event - N_NIM_Sync * 2
	//   << ", " << setw(10) << N_NIM_Sync * 2
	//   << ", " << setw(15) << N_NIM_Last
	//   << ", " << setw(15) << N_NIM_Last2
	//     << ", " << setw(10) << N_KAL_LOS[IP] << endl;  
  //}

  double Texe_total = RunTimer.RealTime();
  double rate_exe = N_event[0] / Texe_total;
  RunTimer.Stop();

  double Trun_total = TS_KAL[0];
  double rate_run = N_event[0] / Trun_total;

  double rate_mis = SkipN / N_event[0];

  double rate_track = N_track / N_event[0];

  tree -> Write();
  f -> Write();

  cout << endl;
  cout << "--------------------------------------------------------------------------------" << endl;
  cout << "------ Statistics --------------------------------------------------------------" << endl;
  cout << "--------------------------------------------------------------------------------" << endl;
  cout << "Total Number of Events : " << Form("%6.0d", N_event[0]) << " events " << endl;
  cout << "Total Run time         : " << Form("%6.1f", Trun_total) << " s      " << endl;
  cout << "Event rate             : " << Form("%6.1f", rate_run) << " (/s)   " << endl;
  cout << "Mismatch event         : " << Form("%6.0f", SkipN) << " events " << endl;
  cout << "Mismatch ratio         : " << Form("%6.2f", rate_mis * 100) << " %      " << endl;
  cout << "Total Execution time   : " << Form("%6.1f", Texe_total) << " s      " << endl;
  cout << "Processing speed       : " << Form("%6.1f", rate_exe) << " (/s)   " << endl;
  cout << "Total Trackable Events : " << Form("%6.0d", N_track) << " events " << endl;
  cout << "Trackable Event Ratio  : " << Form("%6.1f", rate_track * 100) << " %      " << endl;
  cout << "--------------------------------------------------------------------------------" << endl;
  cout << endl;

  cout << "--------------------------------------------------------------------------------" << endl;
  cout << "------ Number of Hits ----------------------------------------------------------" << endl;
  cout << "--------------------------------------------------------------------------------" << endl;
	cout << "  ";
  for (int jj = 0; jj < 32; jj++) cout << Form(",%4d", jj);
	cout << endl;
  for (int ii = 0; ii < N_IP; ii++) {
			cout << Form("%2d", vecIP[ii]);
			noisefile << Form("%2d, ", vecIP[ii]); 
			for (int jj = 0; jj < 32; jj++) {
          double value = KalNum[ii][jj] / Trun_total;
          int exponent = value == 0 ? 0 : (int) floor(log10(value));
          double mantissa = value / pow(10, exponent);
					if(exponent < 0) {
							mantissa = 0;
							exponent = 0;
					}
          cout      << Form(", %1.0fe%d", mantissa, exponent);
          noisefile << Form("%6.2f", value);
					if (jj < 31) noisefile << ", ";
			}
			cout << endl;
			noisefile << endl;
	}

  stat_file << "            </tr>\n";
  stat_file << "        </table>\n";
  stat_file << "        <h2>Event Statistics</h2>\n";
  stat_file << "        <p><span class=\"label\">Total Number of Events:</span> " << Form("%6.0d", N_event[0]) << " events</p>\n";
  stat_file << "        <p><span class=\"label\">Total Run Time:        </span> " << Form("%6.1f", Trun_total) << " s</p>     \n";
  stat_file << "        <p><span class=\"label\">Event rate:            </span> " << Form("%6.1f", rate_run) << " cps</p>   \n";
  stat_file << "        <p><span class=\"label\">Mismatch event:        </span> " << Form("%6.0f", SkipN) << " events</p>\n";
  stat_file << "        <p><span class=\"label\">Mismatch ratio:        </span> " << Form("%6.2f", rate_mis * 100) << " %</p>     \n";
  stat_file << "        <p><span class=\"label\">Total Execution Time:  </span> " << Form("%6.1f", Texe_total) << " s</p>     \n";
  stat_file << "        <p><span class=\"label\">Processing Speed:      </span> " << Form("%6.1f", rate_exe) << " cps</p>   \n";
  stat_file << "        <p><span class=\"label\">Total Trackable Events:</span> " << Form("%6.0d", N_track) << " events</p>\n";
  stat_file << "        <p><span class=\"label\">Trackable Event Ratio: </span> " << Form("%6.1f", (double) rate_track * 100) << " %</p>\n";
  stat_file << "    </div>\n";
  stat_file << "</body>\n";
  stat_file << "</html>\n";

  stat_file.close();
}

void ThDACScan(int runN, int N_IP = 0, bool fNIM = 0, bool ftree = 0, const string & path = "test", bool ONLINE_FLAG = true) {
  for (int ii = 0; ii < 16; ii++) {
    rawdata2root(runN, N_IP, fNIM, ftree, path, ONLINE_FLAG, true, ii);
	}
}

void Check_CH_Setting() {
  int xy = 0, ud = 0, oi = 0, ch_offset = 0, Layer_No = 0;
  TH2I * hCH_Assign_out[4];
  hCH_Assign_out[0] = new TH2I("hCH_Assign_out_0", "Fiber_x_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[1] = new TH2I("hCH_Assign_out_1", "Fiber_y_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[2] = new TH2I("hCH_Assign_out_2", "Fiber_x_dn; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[3] = new TH2I("hCH_Assign_out_3", "Fiber_y_dn; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);

  TH2I * hCH_Assign_in[2];
  hCH_Assign_in[0] = new TH2I("hCH_Assign_in_0", "Fiber_x_up(0-31)&dn(32-63)  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_in[1] = new TH2I("hCH_Assign_in_1", "Fiber_y_up(0-31)&dn(32-63)  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);

  for (int nIP = 0; nIP < 12; nIP++) {
    configureIP(nIP, xy, ud, oi, ch_offset, Layer_No);
    for (int i = 0; i < 32; i++) {
      int KEL_ch = i + ch_offset * 32;
      int fiber_ch = KEL_ch;
      AssignFiber(fiber_ch, ud, oi);
      if (oi == 0) hCH_Assign_out[Layer_No] -> Fill(KEL_ch, fiber_ch);
      else {
        fiber_ch = fiber_ch + ud * 32;
        hCH_Assign_in[Layer_No] -> Fill(KEL_ch, fiber_ch);
      }
    }
  }
  TCanvas * c1 = new TCanvas("c1", "c1", 800, 1200);
  c1 -> Divide(2, 3);
  for (int i = 0; i < 4; i++) {
    c1 -> cd(i + 1);
    hCH_Assign_out[i] -> Draw("colz");
    c1 -> Modified();
    c1 -> Update();
  }
  for (int i = 0; i < 2; i++) {
    c1 -> cd(i + 5);
    hCH_Assign_in[i] -> Draw("colz");
    c1 -> Modified();
    c1 -> Update();
  }
  TString PDFpath = "../pdf/test";
  PDFpath += ".pdf";
  c1 -> SaveAs(PDFpath);
}

void Test_Inner_CH() {
  int fiber_ch = 0, ud = 0, oi = 1;
  ofstream outfile("./txt/CH_test.txt");
  outfile << "i, fiber_ch" << endl;
  for (int i = 0; i < 64; i++) {
    fiber_ch = i;
    AssignFiber(fiber_ch, ud, oi);
    outfile << i << ", " << fiber_ch << endl;
  }
}

void Check_Real_CH() {
  ifstream rawdata[12];
  string ifname[12];
  int xy = 0, ud = 0, oi = 0, ch_offset = 0, Layer_No = 0;
  int ch = -999, N_IP = 2;
  TH2I * hCH_Assign_out[4];
  hCH_Assign_out[0] = new TH2I("hCH_Assign_out_0", "Fiber_x_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[1] = new TH2I("hCH_Assign_out_1", "Fiber_y_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[2] = new TH2I("hCH_Assign_out_2", "Fiber_x_dn; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_out[3] = new TH2I("hCH_Assign_out_3", "Fiber_x_dn; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);

  TH2I * hCH_Assign_in[2];
  hCH_Assign_in[0] = new TH2I("hCH_Assign_in_0", "Fiber_x_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);
  hCH_Assign_in[1] = new TH2I("hCH_Assign_in_1", "Fiber_y_up  ; Kalliope CH; Fiber CH", 64, 0, 64, 64, 0, 64);

  for (int i = 0; i < N_IP; i++) {
    //===== Open Rawdata =====
    int nIP = i + 1;
    //if(runN<10) ifname[i]=Form("../RAW/%s/MSE00000%d_192.168.10.%d.rawdata",path.c_str(),runN,nIP);
    //else ifname[i]=Form("../RAW/%s/MSE0000%d_192.168.10.%d.rawdata",path.c_str(),runN,nIP);
    ifname[i] = Form("../RAW/test_noise/MSE000000_192.168.10.%d.rawdata", nIP);
    rawdata[i].open(ifname[i].c_str());

    if (!rawdata[i]) {
      cout << "Unable to open file: " << ifname[i] << endl;
      exit(1); // terminate with error
    }
  }
  for (int nIP = 0; nIP < N_IP; nIP++) {
    cout << "Start Reading Rawdata from Kalliope: " << nIP << endl;
    rawdata[nIP].seekg(0, ios::end); // going to the end of the file
    streampos fsize = rawdata[nIP].tellg(); // rawdata size in byte (B)
    fsize = fsize / 4; // rawdata size in 32 bit (4B)
    rawdata[nIP].seekg(0, ios::beg); // going to the begin of the file
    cout << Form("rawdata%d filesize :", nIP) << fsize << endl;

    while (!rawdata[nIP].eof()) {
      char Byte[4];
      rawdata[nIP].read(Byte, 4); // reading 4 byte (32 bit)
      unsigned int data = Read_Raw_32bit(Byte);
      //cout << hex << setfill('0') << right << setw(8) << data << endl; //OK
      int Header = (data & 0xff000000) >> 24;
      if (Header == 3) {
        configureIP(nIP, xy, ud, oi, ch_offset, Layer_No);
        ch = (data >> 16) & 0x000000ff;
        int KEL_ch = ch + ch_offset * 32;
        int fiber_ch = KEL_ch;
        AssignFiber(fiber_ch, ud, oi);
        if (oi == 0) hCH_Assign_out[Layer_No] -> Fill(KEL_ch, fiber_ch);
        else {
          fiber_ch = fiber_ch + ud * 32;
          hCH_Assign_in[Layer_No] -> Fill(KEL_ch, fiber_ch);
        }
      }
    }
  }
  TCanvas * c1 = new TCanvas("c1", "c1", 800, 1200);
  c1 -> Divide(2, 3);
  for (int i = 0; i < 4; i++) {
    c1 -> cd(i + 1);
    hCH_Assign_out[i] -> Draw("colz");
    c1 -> Modified();
    c1 -> Update();
  }
  for (int i = 0; i < 2; i++) {
    c1 -> cd(i + 5);
    hCH_Assign_in[i] -> Draw("colz");
    c1 -> Modified();
    c1 -> Update();
  }
}
