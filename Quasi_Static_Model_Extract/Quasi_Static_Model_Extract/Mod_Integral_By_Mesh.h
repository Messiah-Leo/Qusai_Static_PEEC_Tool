#ifndef MOD_INTEGRAL_BY_MESH_H
#define MOD_INTEGRAL_BY_MESH_H

#pragma once

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Mod_Common_Data.h"
#include "Mod_Function.h"   
#include "Mod_Gaussian.h"
#include "Mod_Singular_Treatment.h"
#include "Mod_Type.h"

int N_Gaussin_P(double R, double Lamda);

Ele_Mesh_Int Ele_Cal(int IND_A, int IND_B);
Ele_Mesh_Int Trans_Ele(int IND_A, int IND_B);

Ele_Mesh_Int Ele_Integral_T(int IND_A, int IND_B);
Ele_Mesh_Int Ele_Integral_F(int IND_A, int IND_B);



#endif   // MOD_INTEGRAL_BY_MESH_H
