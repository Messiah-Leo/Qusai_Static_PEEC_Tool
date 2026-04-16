#include"Mod_Cir_Fusion_Data.h"


std::vector<std::vector<double>> P_OO_O, P_OA_O, P_AA_O;
std::vector<std::vector<double>> P_OO_N, P_OI_N, P_II_N;

std::vector<std::vector<double>> M_RR_O, M_RO_O, M_RC_O, M_RA_O;
std::vector<std::vector<double>>         M_OO_O, M_OC_O, M_OA_O;
std::vector<std::vector<double>>				   M_CC_O, M_CA_O;
std::vector<std::vector<double>>					       M_AA_O;

std::vector<std::vector<double>> M_RR_N, M_RO_N, M_RN_N, M_RI_N;
std::vector<std::vector<double>>		   M_OO_N, M_ON_N, M_OI_N;
std::vector<std::vector<double>>				   M_NN_N, M_NI_N;
std::vector<std::vector<double>>					       M_II_N;

std::vector<std::vector<double>> j_C, j_CT, J_FULL, I_C;

std::vector<std::vector<int>> B2N, N2B, N2O;

int N_B, N_N;
int N_O, N_I, N_R, N_C;

std::vector<int> C_Node, C_Branch;