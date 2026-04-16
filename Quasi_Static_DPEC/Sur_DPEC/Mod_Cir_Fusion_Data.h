#ifndef MOD_CIR_FUSION_DATA_H
#define MOD_CIR_FUSION_DATA_H

#include "Mod_Common_Data.h"

#pragma once

const int N_Inc_M = 100;
extern double UP_W;

extern std::vector<std::vector<double>> P_OO_O, P_OA_O, P_AA_O;
extern std::vector<std::vector<double>> P_OO_N, P_OI_N, P_II_N;

extern std::vector<std::vector<double>> M_RR_O, M_RO_O, M_RC_O, M_RA_O;

extern std::vector<std::vector<double>>         M_OO_O, M_OC_O, M_OA_O;
extern std::vector<std::vector<double>>				  M_CC_O, M_CA_O;
extern std::vector<std::vector<double>>					      M_AA_O;

extern std::vector<std::vector<double>> M_RR_N, M_RO_N, M_RN_N, M_RI_N;
extern std::vector<std::vector<double>>		  M_OO_N, M_ON_N, M_OI_N;
extern std::vector<std::vector<double>>				  M_NN_N, M_NI_N;
extern std::vector<std::vector<double>>					      M_II_N;

extern std::vector<std::vector<double>> j_C, j_CT, J_FULL, I_C;

extern std::vector<std::vector<int>> B2N, N2B, N2O;
extern int N_B, N_N;
extern int N_O, N_I, N_R, N_C;
extern std::vector<int> C_Node, C_Branch;


#endif