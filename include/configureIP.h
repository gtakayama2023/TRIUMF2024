void configureIP(int IP, int &xy, int &ud, int &oi, int &ch_offset, int &Layer_No){
  switch(IP){
    // (xy: x=0, y=1); (ud: u=0, d=1); (oi: o=0; i=1); (ch: 0-31=0, 32-63=1);
    //to be in src file...
  case 0:  xy=0; ud=0; oi=0; ch_offset=0; Layer_No=0 ; break;
  case 1:  xy=0; ud=0; oi=0; ch_offset=1; Layer_No=0 ; break;
  case 2:  xy=1; ud=0; oi=0; ch_offset=0; Layer_No=1 ; break;
  case 3:  xy=1; ud=0; oi=0; ch_offset=1; Layer_No=1 ; break;
  case 4:  xy=0; ud=0; oi=1; ch_offset=0; Layer_No=0 ; break;
  case 5:  xy=0; ud=1; oi=1; ch_offset=1; Layer_No=0 ; break;
  case 6:  xy=1; ud=0; oi=1; ch_offset=0; Layer_No=1 ; break;
  case 7:  xy=1; ud=1; oi=1; ch_offset=1; Layer_No=1 ; break;
  case 8:  xy=1; ud=1; oi=0; ch_offset=0; Layer_No=2 ; break;
  case 9:  xy=1; ud=1; oi=0; ch_offset=1; Layer_No=2 ; break;
  case 10: xy=0; ud=1; oi=0; ch_offset=0; Layer_No=3 ; break;
  case 11: xy=0; ud=1; oi=0; ch_offset=1; Layer_No=3 ; break;
  }
}
