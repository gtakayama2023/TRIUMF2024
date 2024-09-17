void configureIP(int IP, int &xy, int &ud, int &oi, int &ch_offset, int &Layer_No){
  switch(IP){
    // (xy: x=0, y=1); (ud: u=0, d=1); (oi: o=0; i=1); (ch: 0-31=0, 32-63=1);
    //to be in src file...
    case 0:  xy=0; ud=0; oi=0; ch_offset=0; Layer_No=0 ; break; // IP = 01, AB 00 - 15, CD 16 - 31
    case 1:  xy=0; ud=0; oi=0; ch_offset=1; Layer_No=0 ; break; // IP = 02, EF 32 - 47, GH 48 - 63
    case 2:  xy=1; ud=0; oi=0; ch_offset=0; Layer_No=1 ; break; // IP = 03, AB 00 - 15, CD 16 - 31
    case 3:  xy=1; ud=0; oi=0; ch_offset=1; Layer_No=1 ; break; // IP = 03, EF 32 - 47, GH 48 - 63
    case 4:  xy=0; ud=0; oi=1; ch_offset=0; Layer_No=0 ; break; // IP = 05, AB 00 - 15, CD 16 - 31
    case 5:  xy=0; ud=1; oi=1; ch_offset=1; Layer_No=0 ; break; // IP = 06, EF 00 - 15, GH 16 - 31
    case 6:  xy=1; ud=0; oi=1; ch_offset=0; Layer_No=1 ; break; // IP = 07, AB 00 - 15, CD 16 - 31
    case 7:  xy=1; ud=1; oi=1; ch_offset=1; Layer_No=1 ; break; // IP = 08, EF 00 - 15, GH 16 - 31
    case 8:  xy=1; ud=1; oi=0; ch_offset=0; Layer_No=2 ; break; // IP = 09, AB 00 - 15, CD 16 - 31
    case 9:  xy=1; ud=1; oi=0; ch_offset=1; Layer_No=2 ; break; // IP = 10, EF 32 - 47, GH 48 - 63
    case 10: xy=0; ud=1; oi=0; ch_offset=0; Layer_No=3 ; break; // IP = 11, AB 00 - 15, CD 16 - 31
    case 11: xy=0; ud=1; oi=0; ch_offset=1; Layer_No=3 ; break; // IP = 12, EF 32 - 47, GH 48 - 63
  }
}
