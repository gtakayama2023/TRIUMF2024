void var_raw_init(){
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_L[i][j][k]=-100;
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)Traw_T[i][j][k]=-100;
  for(int i=0;i<2;i++)for(int j=0;j<32;j++)for(int k=0;k<10;k++)TOT[i][j][k]=-100;
  for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<32;k++)Traw_num[i][j][k]=0;
}
void var_fill(/*bool SameEvent, int Header*/){
  
}
