#pragma once
#ifndef MOD_CIR_FUSION_TOOL_FUNC_H
#define MOD_CIR_FUSION_TOOL_FUNC_H

#include <cassert>

#include"Mod_MKL_Interface.h"

void Update_N2B(
    const std::vector<std::vector<int>>& B2N,
    int N_B,
    int N_N,
    std::vector<std::vector<int>>& N2B
);

void Sort_Index(
    const std::vector<int>& ARRAY_IN,
    int N_MAT,
    std::vector<int>& ARRAY_OUT,
    int& COUNT
);

void Combine_OO_Branches(
    int IND_A,
    int IND_B,
    std::vector<std::vector<double>>& M_OO,
    std::vector<std::vector<double>>& M_RO,
    std::vector<std::vector<double>>& P_OO,
    int N_O,
    int N_R
);

void Combine_RR_Branches(
    int IND_A,
    int IND_B,
    std::vector<std::vector<double>>& M_RR,
    std::vector<std::vector<double>>& M_OO,
    std::vector<std::vector<double>>& M_RO,
    int N_R,
    int N_O
);

double LL_DIS(
    const std::vector<std::vector<double>>& M,
    int A,
    int B
);

void Arrange_Min_Loop(
    const std::vector<std::vector<double>>& M_RR,
    int N_C,
    const std::vector<int>& N2B_X,
    std::vector<int>& C_Branch
);

double Node_Significance(
    const std::vector<std::vector<double>>& M_RR,
    const std::vector<std::vector<double>>& M_RO,
    const std::vector<std::vector<double>>& M_OO,
    const std::vector<std::vector<double>>& P_OO,
    const std::vector<std::vector<int>>& N2B,
    const std::vector<std::vector<int>>& B2N,
    int Node_N,
    double W
);

void Check_PL(
    const std::vector<std::vector<double>>& P,
    int n
);

/*
 @brief
 Its purpose is to remove specified rows and columns from a 2D integer matrix.
 The operation is performed IN-PLACE on the matrix.

 The indices of rows to be removed are provided in IND_RMV_R,
 and the indices of columns to be removed are provided in IND_RMV_C.

 Internally, the function relies on a sorting routine (Sort_Index in Fortran)
 to ensure that the removal indices are processed in correct order.

 @param[in,out] Mat
      - Input:  original 2D integer matrix with size N_R x N_C
      - Output: the same matrix after removing the specified rows and columns

 @param[in] N_R
      Original number of rows in the matrix

 @param[in] N_C
      Original number of columns in the matrix

 @param[in] IND_RMV_R
      A list of row indices to be removed

 @param[in] IND_RMV_C
      A list of column indices to be removed

 @return
      void
 */
template <typename T>
void Remove_Mat_Ele
(
    std::vector<std::vector<T>>& Mat,
    int N_R,
    int N_C,
    const std::vector<int>& IND_RMV_R,
    const std::vector<int>& IND_RMV_C
)
{
    int N_M[2];
    N_M[0] = N_R;
    N_M[1] = N_C;

    std::vector<int> AR_BUF;
    int COUNT = 0;

    /*==================================================
     * Step 1: Remove rows
     *==================================================*/

    AR_BUF.resize(IND_RMV_R.size() + 1);
    Sort_Index(IND_RMV_R, N_M[0], AR_BUF, COUNT);

    for (int I = 1; I <= COUNT; ++I)
    {
        int M_S = AR_BUF[I - 1] - I;
        int M_E = AR_BUF[I] - I - 1;

        for (int J = M_S; J < M_E; ++J)
        {
            Mat[J] = Mat[J + I];
        }
    }

    // zero trailing rows
    for (int i = N_M[0] - COUNT; i < N_M[0]; ++i)
    {
        std::fill(Mat[i].begin(),
            Mat[i].begin() + N_M[1],
            T{});
    }

    N_M[0] -= COUNT;

    /*==================================================
     * Step 2: Remove columns
     *==================================================*/

    AR_BUF.clear();
    AR_BUF.resize(IND_RMV_C.size() + 1);
    Sort_Index(IND_RMV_C, N_M[1], AR_BUF, COUNT);

    for (int I = 1; I <= COUNT; ++I)
    {
        int M_S = AR_BUF[I - 1] - I;
        int M_E = AR_BUF[I] - I - 1;

        for (int J = M_S; J < M_E; ++J)
        {
            for (int r = 0; r < N_M[0]; ++r)
            {
                Mat[r][J] = Mat[r][J + I];
            }
        }
    }

    // zero trailing columns
    for (int r = 0; r < N_M[0]; ++r)
    {
        for (int c = N_M[1] - COUNT; c < N_M[1]; ++c)
        {
            Mat[r][c] = T{};
        }
    }

    N_M[1] -= COUNT;
};

#endif