#include "Mod_Common_Data.h"


std::string PATH;
std::string INPUT_FILE;
std::string SET_FILE;
std::string DIELECTRIC_FILE;
std::string MAP_PATH;
std::string SOURCE_PATH;

std::array<int, 15> NUMBER_OF_TYPE;


std::vector<Point> PT;
int N_P;


std::vector<Temp_Mesh> T_MS;
int T_MS_N;


std::vector<Port> PORT_DATA;
int N_PORT;


int NODE_N;

//!DIE & PEC & LINE & PORT MESH
int N_S;
int DS_N;
int PEC_N;
int L_N;

std::vector<Mesh> Sur_M;

//!DIE & PEC& PORT CONNECTION
int CN_N;
int C_DS_N;
int C_PEC_N;


std::vector<int> DS_PEC_Branch;
std::vector<std::vector<int>> CN;
std::vector<std::vector<int>> CN_P;



std::vector<std::vector<Ele_BL>> B_IND;
std::vector<std::vector<Ele_BC>> B_CAP;
std::vector<std::vector<Ele_P>> N_CAP;
std::vector<std::vector<Ele_CS>> B_CS;
std::vector<std::vector<Ele_CS>> N_CS;


std::vector<std::vector<double>> PDD_N;
std::vector<std::vector<double>> PD0_N;
std::vector<std::vector<double>> LD0_N;
std::vector<std::vector<Vector>> PV_DD;
std::vector<std::vector<Vector>> PV_D0;
std::vector<std::vector<Vector>> LV_D0;


std::vector<std::vector<double>> B_RR;


std::vector<std::vector<Ele_Mesh_Int>> MS_ELE;

std::vector<std::vector<Ele_BC>> TC_CAP;
std::vector<std::vector<Ele_CS>> TC_CS;
std::vector<std::vector<Ele_P>> N_TC_CAP;
int C_TC_N;
int TC_N;


std::vector<std::vector<int>> TC_B2M;
std::vector<std::vector<int>> TC_B2M_P;
std::vector<int> TC_N2M;
std::vector<int> TC_M2N;


std::array<int, 30> BLK_D;


int N_SP;
int N_BLOCK;


std::array<std::array<int, P_Y>, PM_X> PHYZ;
std::array<int, PM_X> DIE;


std::vector<std::vector<std::vector<double>>> S_PAR_ABS;
std::vector<std::vector<std::vector<double>>> S_PAR_PHA;


std::vector<std::vector<std::vector<std::complex<double>>>> Z_PT;
std::vector<std::vector<std::vector<std::complex<double>>>> Y_PT;

int INFI_G = 0;
double Z_G = 0.0;
int SLV_W = 0;
int Swap_Log = 0;
long long FS, FE;
int N_FP;
double PRECISION;
int DIM;
int POST_PROCESSING{ 1 };
int POST_FREQ_INDEX{ -1 };
int POST_PORT{ 1 };

std::vector<double> CF;
double W{ 0.0f };


double MAX_L{ 0.01 };
double LAMDA_E{ 1e5 };
ElementInfo ELM_TYPE;

std::vector<std::complex<double>> DIELECTRIC;
int Solver_SET{ 0 };

std::vector<std::vector<double>> LL, PP;
std::vector<std::vector<double>> A_E, A_ET;
int M_SZ;
std::vector<int> L_BL, S_BL;
