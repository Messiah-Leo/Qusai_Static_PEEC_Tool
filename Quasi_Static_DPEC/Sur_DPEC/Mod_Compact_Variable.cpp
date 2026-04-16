#include "Mod_Compact_Variable.h"


double UP_W = 6.0E9;
// ------------------------------------------------------------
// Matrix containers (initially empty)
// ------------------------------------------------------------
std::vector<std::vector<double>> LL_00;
std::vector<std::vector<double>> LN_D0;
std::vector<std::vector<double>> PP_00;
std::vector<std::vector<double>> PP_0D;
std::vector<std::vector<double>> PN_DD;
std::vector<std::vector<double>> PN_D0;

std::vector<std::vector<double>> ROLL_D_PP_D0;
std::vector<std::vector<double>> ROLL_D_LL_D0;

// ------------------------------------------------------------
// Topology
// ------------------------------------------------------------
std::vector<std::vector<int>> C_B2N;
std::vector<std::vector<int>> C_N2B;

// ------------------------------------------------------------
// Circuit size
// ------------------------------------------------------------
int C_N_Circuit_B = 0;
int C_N_Circuit_N = 0;