//===== Raw Data ====
int Nevent;
bool SameEvent;
int hitNmax;
int Traw_L[12][32][10];
int Traw_T[12][32][10];
int TOT[12][32][10];
int Traw_num[12][2][32]; // ID:L/T:CH

//===== Reline Up =====
int KEL[12][2][16][10];
bool Fired_V[12][2][16];
int leading[12][2][16];
int trailing[12][2][16];
int tot[12][2][16];

//===== Fiber =====
double Fired_T[2][2][2][64]; // X/Y:UP/DOWN:IN/OUT:CH
double Fiber_T[2][2][2][64][10];
double Fiber_L[2][2][2][64];
double Fiber_Tr[2][2][2][64];
double Fiber_Q[2][2][2][64];
double Tnum[2][2][2][64];
double Fired_Tnum[2][2][2];
