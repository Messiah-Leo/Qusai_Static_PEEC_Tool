#ifndef MOD_COMMON_DATA_H
#define MOD_COMMON_DATA_H

#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <string>
#include <vector>



#include "Mod_Type.h"


const double PI = 4.0 * std::atan(1.0);
const double E0 = 8.85418782e-12;
const double U0 = 4.0 * PI * 1e-7;
const double IMPD = std::sqrt(U0 / E0);
const double IMPD_P2 = IMPD * IMPD;
const double SPEED = 1.0 / std::sqrt(E0 * U0);


extern std::string PATH;
extern std::string INPUT_FILE;
extern std::string SET_FILE;
extern std::string DIELECTRIC_FILE;
extern std::string MAP_PATH;
extern std::string SOURCE_PATH;


constexpr std::array<int, 15> TYP_LENGTH = { 2, 3, 4, 4, 8, 6, 5, 3, 6, 9, 10, 27, 18, 14, 1 };
constexpr std::array<int, 15> TYP_FACE = { 0, 5, 6, 4, 6, 6, 5, 0, 1, 1, 4, 6, 6, 5, 0 };

/*
elm - type
defines the geometrical type of the n - th element :

1
2 - node line.

2
3 - node triangle.

3
4 - node quadrangle.

4
4 - node tetrahedron.

5
8 - node hexahedron.

6
6 - node prism.

7
5 - node pyramid.

8
3 - node second order line(2 nodes associated with the vertices and 1 with the edge).

9
6 - node second order triangle(3 nodes associated with the vertices and 3 with the edges).

10
9 - node second order quadrangle(4 nodes associated with the vertices, 4 with the edges and 1 with the face).

11
10 - node second order tetrahedron(4 nodes associated with the vertices and 6 with the edges).

12
27 - node second order hexahedron(8 nodes associated with the vertices, 12 with the edges, 6 with the faces and 1 with the volume).

13
18 - node second order prism(6 nodes associated with the vertices, 9 with the edges and 3 with the quadrangular faces).

14
14 - node second order pyramid(5 nodes associated with the vertices, 8 with the edges and 1 with the quadrangular face).

15
1 - node point.
*/




constexpr std::array<int, 2> S_LENGTH = { 3, 9 };


const int P_Y = 10;
const int PM_X = 100;


const int UN_S_CONS = 1e9;
const int MAX_PORT = 4;

extern std::array<int, 15> NUMBER_OF_TYPE;


extern std::vector<Point> PT;
extern int N_P;


extern std::vector<Temp_Mesh> T_MS;
extern int T_MS_N;


extern std::vector<Port> PORT_DATA;
extern int N_PORT;


extern int NODE_N;

//!DIE & PEC & LINE & PORT MESH
extern int N_S;
extern int DS_N;
extern int PEC_N;
extern int L_N;

extern std::vector<Mesh> Sur_M;


extern int CN_N;
extern int C_DS_N;
extern int C_PEC_N;


extern std::vector<int> DS_PEC_Branch;
extern std::vector<std::vector<int>> CN;
extern std::vector<std::vector<int>> CN_P;


extern std::vector<std::vector<Ele_BL>> B_IND;
extern std::vector<std::vector<Ele_BC>> B_CAP;
extern std::vector<std::vector<Ele_P>> N_CAP;
extern std::vector<std::vector<Ele_CS>> B_CS;
extern std::vector<std::vector<Ele_CS>> N_CS;


extern std::vector<std::vector<double>> PDD_N;
extern std::vector<std::vector<double>> PD0_N;
extern std::vector<std::vector<double>> LD0_N;
extern std::vector<std::vector<Vector>> PV_DD;
extern std::vector<std::vector<Vector>> PV_D0;
extern std::vector<std::vector<Vector>> LV_D0;


extern std::vector<std::vector<double>> B_RR;


extern std::vector<std::vector<Ele_Mesh_Int>> MS_ELE;


extern std::vector<std::vector<Ele_BC>> TC_CAP;
extern std::vector<std::vector<Ele_CS>> TC_CS;
extern std::vector<std::vector<Ele_P>> N_TC_CAP;
extern int C_TC_N;
extern int TC_N;


extern std::vector<std::vector<int>> TC_B2M;
extern std::vector<std::vector<int>> TC_B2M_P;
extern std::vector<int> TC_N2M;
extern std::vector<int> TC_M2N;


extern std::array<int, 30> BLK_D;


extern int N_SP;
extern int N_BLOCK;


extern std::array<std::array<int, P_Y>, PM_X> PHYZ;
extern std::array<int, PM_X> DIE;


extern std::vector<std::vector<std::vector<double>>> S_PAR_ABS;
extern std::vector<std::vector<std::vector<double>>> S_PAR_PHA;


extern std::vector<std::vector<std::vector<std::complex<double>>>> Z_PT;
extern std::vector<std::vector<std::vector<std::complex<double>>>> Y_PT;


extern int INFI_G;
extern double Z_G;
extern int SLV_W;
extern int Swap_Log;
extern long long FS, FE;
extern int N_FP;
extern double PRECISION;
extern int DIM;
extern int POST_PROCESSING;                     // whether to export post-processing sources
extern int POST_FREQ_INDEX;                     // one-based frequency index; <= 0 selects the last point
extern int POST_PORT;                           // one-based excitation port
extern std::vector<std::complex<double>> DIELECTRIC;

extern std::vector<double> CF;
extern double W;
extern double LAMDA_E;
extern double MAX_L;

extern ElementInfo ELM_TYPE;
extern int Solver_SET;
extern std::vector<std::vector<double>> LL, PP;
extern std::vector<std::vector<double>> A_E, A_ET;
extern int M_SZ;

extern std::vector<int> L_BL; 
extern std::vector<int> S_BL;
#endif
