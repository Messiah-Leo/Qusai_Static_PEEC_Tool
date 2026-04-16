#pragma once
#ifndef MOD_COMPACT_VARIABLE_H
#define MOD_COMPACT_VARIABLE_H

#include <vector>
#include <complex>

// ------------------------------------------------------------
// Compact circuit matrices
// ------------------------------------------------------------
extern std::vector<std::vector<double>> LL_00;
extern std::vector<std::vector<double>> LN_D0;
extern std::vector<std::vector<double>> PP_00;
extern std::vector<std::vector<double>> PP_0D;
extern std::vector<std::vector<double>> PN_DD;
extern std::vector<std::vector<double>> PN_D0;

extern std::vector<std::vector<double>> ROLL_D_PP_D0;
extern std::vector<std::vector<double>> ROLL_D_LL_D0;

// ------------------------------------------------------------
// Topology mapping
// ------------------------------------------------------------
extern std::vector<std::vector<int>> C_B2N;
extern std::vector<std::vector<int>> C_N2B;

// ------------------------------------------------------------
// Circuit size info
// ------------------------------------------------------------
extern int C_N_Circuit_B;
extern int C_N_Circuit_N;

#endif