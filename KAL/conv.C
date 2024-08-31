// conv.C
void conv(int mode = 0, int runN = 10, int IP_max = 0, bool fNIM = 0, bool ftree = 0, const string& path = "test", bool ONLINE_FLAG = false) {
  gROOT->ProcessLine("#include \"rawdata2root.cpp\"");
  switch (mode) {
    case 0: gROOT->ProcessLine(Form("rawdata2root(%d, %d, %d, %d, \"%s\", %d)", runN, IP_max, fNIM, ftree, path.c_str(), ONLINE_FLAG)); break;
    case 1: gROOT->ProcessLine(Form("ThDACScan(%d, %d, %d, %d, \"%s\", %d)", runN, IP_max, fNIM, ftree, path.c_str(), ONLINE_FLAG));    break;
    default: cerr << "Unknown mode: " << mode << endl; break;
  }
}

