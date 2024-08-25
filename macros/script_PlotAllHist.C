{
    gROOT->ProcessLine(".L ./macros/PlotAllNhit.cpp++g");
    Plot_All_Histograms("test");
    Data_Check("test");
}
