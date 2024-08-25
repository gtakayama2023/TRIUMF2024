void branch_raw(TTree *tree){

  //==== rawdata =====
  tree->Branch("Traw_L",Traw_L,"Traw_L[2][32][10]/I");
  tree->Branch("Traw_T",Traw_T,"Traw_T[2][32][10]/I");
  tree->Branch("TOT",TOT,"TOT[2][32][10]/I");
  tree->Branch("Traw_num",Traw_num,"Traw_num[2][32]/I");
  tree->Branch("Nevent",&Nevent,"Nevent/I");

  //===== time stamp =====
  /*
  tree->Branch("Nevent",&Nevent,"Nevent/I");
  tree->Branch("ts0",ts0,"ts0[2]/l");
  tree->Branch("ts",&ts,"ts/D");
  tree->Branch("clock",&clock,"clock/D");
  
  //===== KEL =====
  tree->Branch("KELraw",KELraw,"KELraw[12][2][16][10]/I");
  tree->Branch("KELnum",KELnum,"KELnum[12][2][16]/I");
  tree->Branch("KELref",KELref,"KELref[4]/I");
  tree->Branch("KEL",KEL,"KEL[12][2][16][10]/D"); //substract ref
  tree->Branch("Fired_V",Fired_V,"Fired_V[12][2][16]/O");
  tree->Branch("leading",leading,"leading[12][2][16]/I");
  tree->Branch("trailing",trailing,"trailing[12][2][16]/I");
  tree->Branch("tot",tot,"tot[12][2][16]/D");
  
  //===== After Relining =====
  tree->Branch("Fired_T",Fired_T,"Fired_T[2][2][2][64]/O");
  tree->Branch("Fiber_T",Fiber_T,"Fiber_T[2][2][2][64][10]/I");
  tree->Branch("Fiber_L",Fiber_L,"Fiber_L[2][2][2][64]/I");
  tree->Branch("Fiber_Tr",Fiber_Tr,"Fiber_Tr[2][2][2][64]/I");
  tree->Branch("Fiber_Q",Fiber_Q,"Fiber_Q[2][2][2][64]/D");
  tree->Branch("Tnum",Tnum,"Tnum[2][2][2][64]/I");
  tree->Branch("Fired_Tnum",Fired_Tnum,"Fired_Tnum[2][2][2]/I");
  */
  //===== xx =====

  //===== xx =====

  //===== xx ===== 
}
