#include "Mod_Cir_Fusion_Exe_func.h"
#include <cerrno>
#include <direct.h>
#include <fstream>
#include <sstream>
float BIAS_W = 1.01;

void Ini_Data_Structure(
	int N_BRANCH,
	int N_NODE,
	const std::vector<std::vector<double>>& LL_00,
	const std::vector<std::vector<double>>& PP_00,
	const std::vector<std::vector<int>>& B2N_IN,
	const std::vector<std::vector<int>>& N2B_IN,
	double W_IN
)
{
	int I, J, BUFF_I, COUNT;

	/* ---------------- Basic sizes ---------------- */
	N_B = N_BRANCH;
	N_N = N_NODE;
	N_R = N_B;
	N_O = N_N;
	N_C = 0;

	/* ================= Potential (Original) ================= */

	if (P_OO_O.empty())
		P_OO_O.assign(N_N + N_Inc_M, std::vector<double>(N_N + N_Inc_M, 0.0));

	if (P_OA_O.empty())
		P_OA_O.assign(N_N, std::vector<double>(1, 0.0));

	if (P_AA_O.empty())
		P_AA_O.assign(1, std::vector<double>(1, 0.0));

	for (I = 0; I < N_N; ++I)
		for (J = 0; J < N_N; ++J)
			P_OO_O[I][J] = 0.5 * (PP_00[I][J] + PP_00[J][I]);

	/* ================= Potential (Updated) ================= */

	if (P_OO_N.empty())
		P_OO_N.assign(N_N, std::vector<double>(N_N, 0.0));

	if (P_OI_N.empty())
		P_OI_N.assign(N_N, std::vector<double>(N_Inc_M, 0.0));

	if (P_II_N.empty())
		P_II_N.assign(N_Inc_M, std::vector<double>(N_Inc_M, 0.0));

	/* ================= Inductive (Original) ================= */

	if (M_RR_O.empty())
		M_RR_O.assign(N_B, std::vector<double>(N_B, 0.0));

	if (M_RO_O.empty())
		M_RO_O.assign(N_B, std::vector<double>(N_N + N_Inc_M, 0.0));

	if (M_RC_O.empty())
		M_RC_O.assign(N_B, std::vector<double>(N_Inc_M, 0.0));

	if (M_RA_O.empty())
		M_RA_O.assign(N_B, std::vector<double>(1, 0.0));

	if (M_OO_O.empty())
		M_OO_O.assign(N_N + N_Inc_M, std::vector<double>(N_N + N_Inc_M, 0.0));

	if (M_OC_O.empty())
		M_OC_O.assign(N_N, std::vector<double>(N_Inc_M, 0.0));

	if (M_OA_O.empty())
		M_OA_O.assign(N_N, std::vector<double>(1, 0.0));

	if (M_CC_O.empty())
		M_CC_O.assign(N_Inc_M, std::vector<double>(N_Inc_M, 0.0));

	if (M_CA_O.empty())
		M_CA_O.assign(N_Inc_M, std::vector<double>(1, 0.0));

	if (M_AA_O.empty())
		M_AA_O.assign(1, std::vector<double>(1, 0.0));

	for (I = 0; I < N_B; ++I)
		for (J = 0; J < N_B; ++J)
			M_RR_O[I][J] = 0.5 * (LL_00[I][J] + LL_00[J][I]);

	/* ================= Inductive (Updated) ================= */

	if (M_RR_N.empty()) M_RR_N.assign(N_B, std::vector<double>(N_B, 0.0));
	if (M_RO_N.empty()) M_RO_N.assign(N_B, std::vector<double>(N_N, 0.0));
	if (M_RN_N.empty()) M_RN_N.assign(N_B, std::vector<double>(N_Inc_M, 0.0));
	if (M_RI_N.empty()) M_RI_N.assign(N_B, std::vector<double>(N_Inc_M, 0.0));

	if (M_OO_N.empty()) M_OO_N.assign(N_N, std::vector<double>(N_N, 0.0));
	if (M_ON_N.empty()) M_ON_N.assign(N_N, std::vector<double>(N_Inc_M, 0.0));
	if (M_OI_N.empty()) M_OI_N.assign(N_N, std::vector<double>(N_Inc_M, 0.0));

	if (M_NN_N.empty()) M_NN_N.assign(N_Inc_M, std::vector<double>(N_Inc_M, 0.0));
	if (M_NI_N.empty()) M_NI_N.assign(N_Inc_M, std::vector<double>(N_Inc_M, 0.0));
	if (M_II_N.empty()) M_II_N.assign(N_Inc_M, std::vector<double>(N_Inc_M, 0.0));

	/* ================= Connection ================= */

	if (B2N.empty())
		B2N.assign(N_B, std::vector<int>(2, 0));

	if (N2B.empty())
		N2B.assign(N_N, std::vector<int>(N_Inc_M + 1, 0));

	if (N2O.empty())
		N2O.assign(N_N, std::vector<int>(1, 0));

	for (I = 0; I < N_B; ++I)
		B2N[I] = B2N_IN[I];

	for (I = 0; I < N_N; ++I)
		for (J = 0; J <= N_Inc_M; ++J)
			N2B[I][J] = 0;

	for (I = 0; I < N_B; ++I)
	{
		if (B2N[I][0] > B2N[I][1])
		{
			for (J = 0; J < N_R; ++J)
			{
				M_RR_O[I][J] *= -1.0;
				M_RR_O[J][I] *= -1.0;
			}
			BUFF_I = B2N[I][0];
			B2N[I][0] = B2N[I][1];
			B2N[I][1] = BUFF_I;
		}

		COUNT = ++N2B[B2N[I][0] - 1][0];
		N2B[B2N[I][0] - 1][COUNT] = -(I + 1);

		COUNT = ++N2B[B2N[I][1] - 1][0];
		N2B[B2N[I][1] - 1][COUNT] = (I + 1);

	}
	for (I = 0; I < N_N; ++I)
		N2O[I][0] = I + 1;

	UP_W = W_IN;
}

void Re_arrange_Mat(int Node_Num)
{
	int I, J, TEMP, B_IND;

	/* local buffers (Fortran DIMENSION(20)) */
	std::vector<int> RMV_Z(20, 0), RMV_N(20, 0), RMV_B(20, 0);

	/* number of connected branches */
	N_C = N2B[Node_Num][0];

	/*--------------------------------------------------------------*
	 * Update connection properties (branch direction normalization)
	 *--------------------------------------------------------------*/
	for (I = 0; I < N_C; ++I)
	{
		if (N2B[Node_Num][I + 1] < 0)
		{
			N2B[Node_Num][I + 1] = -N2B[Node_Num][I + 1];
			B_IND = N2B[Node_Num][I + 1] - 1;

			for (J = 0; J < N_R; ++J)
			{
				M_RR_O[B_IND][J] = -M_RR_O[B_IND][J];
				M_RR_O[J][B_IND] = -M_RR_O[J][B_IND];
			}

			/* flip sign of RO matrix */
			for (J = 0; J < N_O; ++J)
				M_RO_O[B_IND][J] = -M_RO_O[B_IND][J];

			/* swap node connection direction */
			TEMP = B2N[B_IND][0];
			B2N[B_IND][0] = B2N[B_IND][1];
			B2N[B_IND][1] = TEMP;
		}
	}

	/*--------------------------------------------------------------*
	 * Rearrange branch order around node
	 *--------------------------------------------------------------*/
	Arrange_Min_Loop(M_RR_O, N_C, N2B[Node_Num], C_Branch);

	for (I = 0; I < N_C; ++I)
		N2B[Node_Num][I + 1] = C_Branch[I] + 1;

	/* determine connected nodes */
	C_Node.assign(N_C, 0);
	for (I = 0; I < N_C; ++I)
	{
		if (B2N[C_Branch[I]][0] - 1 == Node_Num)
			C_Node[I] = B2N[C_Branch[I]][1] - 1;
		else
			C_Node[I] = B2N[C_Branch[I]][0] - 1;
	}
	//for (I = 0; I < N_C; ++I)
		//std::cout << I << "  " << C_Node[I] << "  " << N2O[C_Node[I]][0] + 1 << std::endl;
	/*--------------------------------------------------------------*
	 * Removal index lists
	 *--------------------------------------------------------------*/
	RMV_N[0] = Node_Num + 1;
	for (I = 0; I < N_C; ++I)
		RMV_B[I] = C_Branch[I] + 1;

	N_I = N_C;

	/*--------------------------------------------------------------*
	 * Potential matrices update
	 *--------------------------------------------------------------*/
	P_AA_O[0][0] = P_OO_O[Node_Num][Node_Num];

	for (I = 0; I < N_N; ++I)
		P_OA_O[I][0] = P_OO_O[I][Node_Num];

	Remove_Mat_Ele(P_OA_O, N_N, 1, RMV_N, RMV_Z);
	Remove_Mat_Ele(P_OO_O, N_N, N_N, RMV_N, RMV_N);


	/*--------------------------------------------------------------*
	 * Inductive matrices update
	 *--------------------------------------------------------------*/
	M_AA_O[0][0] = M_OO_O[Node_Num][Node_Num];

	for (I = 0; I < N_C; ++I)
		M_CA_O[I][0] = M_RO_O[C_Branch[I]][Node_Num];

	for (I = 0; I < N_N; ++I)
		M_OA_O[I][0] = M_OO_O[I][Node_Num];

	Remove_Mat_Ele(M_OA_O, N_N, 1, RMV_N, RMV_Z);

	for (I = 0; I < N_B; ++I)
		M_RA_O[I][0] = M_RO_O[I][Node_Num];

	Remove_Mat_Ele(M_RA_O, N_B, 1, RMV_B, RMV_Z);

	for (I = 0; I < N_C; ++I)
		for (J = 0; J < N_C; ++J) {
			M_CC_O[I][J] = M_RR_O[C_Branch[I]][C_Branch[J]];
			//std::cout << M_CC_O[I][J] << std::endl;
		}

	for (I = 0; I < N_C; ++I)
		for (J = 0; J < N_N; ++J)
			M_OC_O[J][I] = M_RO_O[C_Branch[I]][J];

	Remove_Mat_Ele(M_OC_O, N_N, N_C, RMV_N, RMV_Z);

	for (I = 0; I < N_C; ++I)
		for (J = 0; J < N_B; ++J)
			M_RC_O[J][I] = M_RR_O[J][C_Branch[I]];

	Remove_Mat_Ele(M_RC_O, N_B, N_C, RMV_B, RMV_Z);

	Remove_Mat_Ele(M_OO_O, N_N, N_N, RMV_N, RMV_N);
	Remove_Mat_Ele(M_RO_O, N_B, N_N, RMV_B, RMV_N);
	Remove_Mat_Ele(M_RR_O, N_B, N_B, RMV_B, RMV_B);

	/*--------------------------------------------------------------*
	 * Update sizes and connectivity
	 *--------------------------------------------------------------*/
	N_R = N_B - N_C;
	N_O = N_N - 1;

	Remove_Mat_Ele(B2N, N_B, 2, RMV_B, RMV_Z);
	Remove_Mat_Ele(N2O, N_N, 1, RMV_N, RMV_Z);

	N_B = N_B - N_C;
	N_N = N_N - 1;

	for (I = 0; I < N_C; ++I)
		if (C_Node[I] > Node_Num)
			--C_Node[I];

	for (I = 0; I < N_B; ++I)
		for (J = 0; J < 2; ++J)
			if (B2N[I][J] > Node_Num)
				--B2N[I][J];


	Update_N2B(B2N, N_B, N_N, N2B);

	/*--------------------------------------------------------------*
	 * Current matrices
	 *--------------------------------------------------------------*/
	j_C.assign(N_C, std::vector<double>(1, 1.0));
	j_CT.assign(1, std::vector<double>(N_C, 1.0));
	J_FULL.assign(N_C, std::vector<double>(N_C, 1.0));

	I_C.assign(N_C, std::vector<double>(N_C, 0.0));
	for (I = 0; I < N_C; ++I)
		I_C[I][I] = 1.0;
}

void Update_PORT(int Node_Num)
{
	for (int i = 0; i < N_PORT; i++)
	{
		if (PORT_DATA[i].N[0] > Node_Num)
			PORT_DATA[i].N[0] -= 1;
		if (PORT_DATA[i].N[1] > Node_Num)
			PORT_DATA[i].N[1] -= 1;
	}
}

void Update_Cir_Matrix()
{
	int I, J;

	/*--------------------------------------------------*
	 * Update Incremental Elements (RI / OI / NI / II)
	 *--------------------------------------------------*/

	 /* M_RI_N = m_RA_O * j_CT + M_RC_O */
	M_RI_N.resize(N_R, std::vector<double>(N_C, 0.0));
	M_RI_N = Product_M(M_RA_O, j_CT);
	for (I = 0; I < N_R; ++I)
		for (J = 0; J < N_C; ++J)
			M_RI_N[I][J] += M_RC_O[I][J];

	/* M_OI_N = m_OA_O * j_CT + M_OC_O */
	M_OI_N.resize(N_O, std::vector<double>(N_C, 0.0));
	M_OI_N = Product_M(M_OA_O, j_CT);
	for (I = 0; I < N_O; ++I)
		for (J = 0; J < N_C; ++J)
			M_OI_N[I][J] += M_OC_O[I][J];

	/* M_NI_N */
	for (I = 0; I < N_C; ++I) {
		for (J = 0; J < N_C; ++J) {
			M_NI_N[I][J] =
				M_CC_O[I][J] - M_CC_O[(I + 1) % N_C][J] + M_CA_O[I][0] - M_CA_O[(I + 1) % N_C][0];
			//std::cout << I << " " << J << " " << M_NI_N[I][J] << std::endl;
		}
	}
	/* M_II_N (diagonal only) */
	M_II_N.resize(N_C, std::vector<double>(N_C, 0.0));
	for (I = 0; I < N_C; ++I)
	{
		M_II_N[I][I] = N_C * M_AA_O[0][0];
		for (J = 0; J < N_C; ++J) {
			M_II_N[I][I] += M_CC_O[I][J] + M_CA_O[I][0] + M_CA_O[J][0];
		}
		//std::cout << I << " " << M_II_N[I][I] << std::endl;
	}

	/* P_OI_N = p_OA_O * j_CT */
	P_OI_N.resize(N_O, std::vector<double>(N_C, 0.0));
	P_OI_N = Product_M(P_OA_O, j_CT);

	/* P_II_N */
	P_II_N.resize(N_C, std::vector<double>(N_C, 0.0));
	for (I = 0; I < N_C; ++I) {
		P_II_N[I][I] = N_C * P_AA_O[0][0];
		//std::cout << I << " " << P_II_N[I][I] << std::endl;
	}

	/*--------------------------------------------------*
	 * Update Incremental NN / RN / ON
	 *--------------------------------------------------*/

	for (I = 0; I < N_R; ++I)
		for (J = 0; J < N_C; ++J)
			M_RN_N[I][J] = M_RC_O[I][J] - M_RC_O[I][(J + 1) % N_C];

	for (I = 0; I < N_O; ++I)
		for (J = 0; J < N_C; ++J)
			M_ON_N[I][J] = M_OC_O[I][J] - M_OC_O[I][(J + 1) % N_C];

	for (I = 0; I < N_C; ++I)
	{
		for (J = 0; J < N_C; ++J)
		{
			M_NN_N[I][J] =
				M_CC_O[I][J]
				+ M_CC_O[(I + 1) % N_C][(J + 1) % N_C]
				- M_CC_O[(I + 1) % N_C][J]
				- M_CC_O[I][(J + 1) % N_C];
			//std::cout << I << " " << J << " " << M_NN_N[I][J] << std::endl;
		}
		M_NN_N[I][I] *= BIAS_W;
	}

	/*--------------------------------------------------*
	 * Expand Global Matrices
	 *--------------------------------------------------*/

	for (I = 0; I < N_C; ++I)
	{
		/* RR */
		for (J = 0; J < N_C; ++J)
			M_RR_O[N_R + J][N_R + I] = M_NN_N[J][I];

		for (J = 0; J < N_R; ++J)
		{
			M_RR_O[J][N_R + I] = M_RN_N[J][I];
			M_RR_O[N_R + I][J] = M_RN_N[J][I];
		}

		/* RO */
		for (J = 0; J < N_C; ++J)
			M_RO_O[N_R + J][N_O + I] = M_NI_N[J][I];

		for (J = 0; J < N_R; ++J)
			M_RO_O[J][N_O + I] = M_RI_N[J][I];

		for (J = 0; J < N_O; ++J)
			M_RO_O[N_R + I][J] = M_ON_N[J][I];

		/* OO */
		for (J = 0; J < N_C; ++J)
			M_OO_O[N_O + J][N_O + I] = M_II_N[J][I];

		for (J = 0; J < N_O; ++J)
		{
			M_OO_O[J][N_O + I] = M_OI_N[J][I];
			M_OO_O[N_O + I][J] = M_OI_N[J][I];
		}

		/* PO */
		for (J = 0; J < N_C; ++J)
			P_OO_O[N_O + J][N_O + I] = P_II_N[J][I];

		for (J = 0; J < N_O; ++J)
		{
			P_OO_O[J][N_O + I] = P_OI_N[J][I];
			P_OO_O[N_O + I][J] = P_OI_N[J][I];
		}
	}

	//for (I = 0; I < N_B; ++I)
	//    for (J = 0; J < N_B; ++J)
	//        std::cout<<I<<"   "<<J<<"   " << M_RR_O[I][J] << std::endl;
	/*--------------------------------------------------*
	 * Update sizes
	 *--------------------------------------------------*/
	N_O += N_C;
	N_R += N_C;

	/*--------------------------------------------------*
	 * Update connectivity
	 *--------------------------------------------------*/
	for (I = 0; I < N_C; ++I)
	{
		B2N[N_B + I][0] = C_Node[I] + 1;
		B2N[N_B + I][1] = C_Node[(I + 1) % N_C] + 1;
	}

	N_B += N_C;
	Update_N2B(B2N, N_B, N_N, N2B);
}

void Combine_Node_Circuits()
{
	int I, IND_A, IND_B;
	int COUNT = 0;

	/* removal index buffers (Fortran DIMENSION(20)) */
	std::vector<int> RMV_I(20, 0);
	std::vector<int> RMV_Z(20, 0);

	/*--------------------------------------------------*
	 * Combine OO branches connected to C_Nodes
	 *--------------------------------------------------*/
	for (I = 0; I < N_C; ++I)
	{

		IND_A = C_Node[I];
		IND_B = N_N + I;

		/* combine OO branches */
		Combine_OO_Branches(
			IND_A,
			IND_B,
			M_OO_O,
			M_RO_O,
			P_OO_O,
			N_O,
			N_R
		);

		/* record removed OO index */
		RMV_I[COUNT] = IND_B + 1;
		++COUNT;
	}

	/*--------------------------------------------------*
	 * Remove absorbed OO branches
	 *--------------------------------------------------*/
	Remove_Mat_Ele(M_OO_O, N_O, N_O, RMV_I, RMV_I);
	Remove_Mat_Ele(M_RO_O, N_R, N_O, RMV_Z, RMV_I);
	Remove_Mat_Ele(P_OO_O, N_O, N_O, RMV_I, RMV_I);

	/* update OO size */
	N_O -= COUNT;
}

void Combine_Branch_Circuits()
{
	int I, J, IND_A, IND_B;
	int COUNT = 0;

	// Indices to be removed (Fortran DIMENSION(20))
	std::vector<int> RMV_I(20, 0);
	std::vector<int> RMV_Z(20, 0);

	// ------------------------------------------------------------
	// 1. Scan all branch pairs to find parallel branches
	// ------------------------------------------------------------
	for (I = 0; I < N_B; ++I)
	{
		for (J = I + 1; J < N_B; ++J)
		{
			// Check if two branches connect the same two nodes
			bool same_order =
				(B2N[I][0] == B2N[J][0] && B2N[I][1] == B2N[J][1]);

			bool reverse_order =
				(B2N[I][0] == B2N[J][1] && B2N[I][1] == B2N[J][0]);

			if (same_order || reverse_order)
			{
				IND_A = I;
				IND_B = J;
				//std::cout << I << "  " << B2N[I][0] << "  " << B2N[I][1] << std::endl;
				//std::cout << J << "  " << B2N[J][0] << "  " << B2N[J][1] << std::endl;
				//std::cout << M_RR_O[I][J] << std::endl;
				// ------------------------------------------------
				// 2. If node order is reversed, unify orientation
				// ------------------------------------------------
				if (reverse_order)
				{
					// Align node order
					B2N[J][0] = B2N[I][0];
					B2N[J][1] = B2N[I][1];

					// Flip sign of RR matrix row and column
					for (int k = 0; k < N_R; ++k)
					{
						M_RR_O[J][k] = -M_RR_O[J][k];
						M_RR_O[k][J] = -M_RR_O[k][J];
					}
					//std::cout << M_RR_O[I][J] << std::endl;
					// Flip sign of RO matrix row
					for (int k = 0; k < N_O; ++k)
					{
						M_RO_O[J][k] = -M_RO_O[J][k];
					}
				}

				// ------------------------------------------------
				// 3. Combine two RR branches
				// ------------------------------------------------
				Combine_RR_Branches(
					IND_A,
					IND_B,
					M_RR_O,
					M_OO_O,
					M_RO_O,
					N_R,
					N_O
				);

				// Record branch to be removed
				RMV_I[COUNT] = IND_B + 1;
				COUNT++;
			}
		}
	}

	// ------------------------------------------------------------
	// 4. Remove redundant matrix elements
	// ------------------------------------------------------------
	Remove_Mat_Ele(B2N, N_B, 2, RMV_I, RMV_Z);
	Remove_Mat_Ele(M_RR_O, N_R, N_R, RMV_I, RMV_I);
	Remove_Mat_Ele(M_RO_O, N_R, N_O, RMV_I, RMV_Z);

	// ------------------------------------------------------------
	// 5. Update branch and RR counts
	// ------------------------------------------------------------
	N_R -= COUNT;
	N_B -= COUNT;

	// ------------------------------------------------------------
	// 6. Update node-to-branch connectivity
	// ------------------------------------------------------------
	Update_N2B(B2N, N_B, N_N, N2B);
}

void Find_Insig_Node(int& Node_Num, double& Node_ER)
{
	double MIN_ER = 1.0e8;
	double TEMP_ER = 0.0;
	int TEMP_IND = -1;

	// ------------------------------------------------------------
	// Scan all nodes
	// ------------------------------------------------------------
	for (int I = 0; I < N_N; ++I)
	{
		// Equivalent to: IF( ABS(P_OO_O(I,I)) .GT. 0 )
		if (std::abs(P_OO_O[I][I]) > 0.0)
		{
			TEMP_ER = Node_Significance(
				M_RR_O,
				M_RO_O,
				M_OO_O,
				P_OO_O,
				N2B,
				B2N,
				I,      // node index (0-based)
				UP_W
			);
			//std::cout << I << "  " << std::setprecision(12) << TEMP_ER << std::endl;
			// Find minimum significance
			if (TEMP_ER < MIN_ER)
			{
				MIN_ER = TEMP_ER;
				TEMP_IND = I;
			}
		}
	}

	// ------------------------------------------------------------
	// Output results
	// -----------------------------------------------------------    
	Node_ER = MIN_ER;
	Node_Num = TEMP_IND;
}

void Reconfig_Circuit_Model()
{
	int I, J;

	// ------------------------------------------------------------
	// Allocate and build incidence matrix A and its transpose AT
	// A  : (N_B x N_N)
	// AT : (N_N x N_B)
	// ------------------------------------------------------------
	std::vector<std::vector<double>> A(N_B, std::vector<double>(N_N, 0.0));
	std::vector<std::vector<double>> AT(N_N, std::vector<double>(N_B, 0.0));

	for (I = 0; I < N_B; ++I)
	{
		int n1 = B2N[I][0];   // Fortran B2N(I,1)
		int n2 = B2N[I][1];   // Fortran B2N(I,2)

		A[I][n1] = 1.0;
		A[I][n2] = -1.0;

		AT[n1][I] = 1.0;
		AT[n2][I] = -1.0;
	}

	// ------------------------------------------------------------
	// Compute M_RO_AT = M_RO_O * AT
	// ------------------------------------------------------------
	std::vector<std::vector<double>> M_RO_AT(
		N_B, std::vector<double>(N_B, 0.0)
	);

	std::vector<std::vector<double>> MMT(
		N_B, std::vector<double>(N_N, 0.0)
	);

	// MMT = M_RO_O
	for (I = 0; I < N_B; ++I)
		for (J = 0; J < N_N; ++J)
			MMT[I][J] = M_RO_O[I][J];

	M_RO_AT = Product_M(MMT, AT);

	// ------------------------------------------------------------
	// Compute A_M_OO = A * M_OO_O
	// ------------------------------------------------------------
	std::vector<std::vector<double>> A_M_OO(
		N_B, std::vector<double>(N_N, 0.0)
	);

	A_M_OO = Product_M(A, M_OO_O);

	// ------------------------------------------------------------
	// Compute LL_00 = A * M_OO * A^T
	// ------------------------------------------------------------
	std::vector<std::vector<double>> LL_00(
		N_B, std::vector<double>(N_B, 0.0)
	);

	LL_00 = Product_M(A_M_OO, AT);

	// ------------------------------------------------------------
	// LL_00 = LL_00 - M_RO_AT - transpose(M_RO_AT)
	// ------------------------------------------------------------
	for (I = 0; I < N_B; ++I)
	{
		for (J = 0; J < N_B; ++J)
		{
			LL_00[I][J] -= M_RO_AT[I][J] + M_RO_AT[J][I];
		}
	}

	// ------------------------------------------------------------
	// M_RR_O = M_RR_O + LL_00
	// ------------------------------------------------------------
	for (I = 0; I < N_B; ++I)
	{
		for (J = 0; J < N_B; ++J)
		{
			M_RR_O[I][J] += LL_00[I][J];
		}
	}

	// ------------------------------------------------------------
	// Clear OO and RO matrices
	// ------------------------------------------------------------
	for (I = 0; I < N_O; ++I)
		for (J = 0; J < N_O; ++J)
			M_OO_O[I][J] = 0.0;

	for (I = 0; I < N_B; ++I)
		for (J = 0; J < N_O; ++J)
			M_RO_O[I][J] = 0.0;
}

void Fused_Circuit_Model(
	std::vector<std::vector<double>>& LL_00,
	std::vector<std::vector<double>>& PP_00,
	int& N_BRANCH,
	int& N_NODE,
	std::vector<std::vector<int>>& B2N_OUT,
	std::vector<Port>& PORT_DATA,
	int N_port
)
{

	int I, J;

	// ------------------------------------------------------------
	// Build incidence matrix A and its transpose AT
	// A  : (N_B x N_N)
	// AT : (N_N x N_B)
	// ------------------------------------------------------------
	A.assign(N_B, std::vector<double>(N_N, 0.0));
	AT.assign(N_N, std::vector<double>(N_B, 0.0));

	for (I = 0; I < N_B; ++I)
	{
		int n1 = B2N[I][0] - 1;
		int n2 = B2N[I][1] - 1;

		A[I][n1] = 1.0;
		A[I][n2] = -1.0;

		AT[n1][I] = 1.0;
		AT[n2][I] = -1.0;
	}

	// ------------------------------------------------------------
	// M_RO_AT = M_RO_O * AT
	// ------------------------------------------------------------
	std::vector<std::vector<double>> M_RO_AT(
		N_B, std::vector<double>(N_B, 0.0)
	);

	std::vector<std::vector<double>> MMT(
		N_B, std::vector<double>(N_N, 0.0)
	);

	for (I = 0; I < N_B; ++I)
		for (J = 0; J < N_N; ++J)
			MMT[I][J] = M_RO_O[I][J];

	M_RO_AT = Product_M(MMT, AT);

	// ------------------------------------------------------------
	// A_M_OO = A * M_OO_O
	// ------------------------------------------------------------
	std::vector<std::vector<double>> A_M_OO(
		N_B, std::vector<double>(N_N, 0.0)
	);

	std::vector<std::vector<double>> M_OO_Buff(
		N_N, std::vector<double>(N_N, 0.0)
	);

	for (I = 0; I < N_N; ++I)
		for (J = 0; J < N_N; ++J)
			M_OO_Buff[I][J] = M_OO_O[I][J];

	A_M_OO = Product_M(A, M_OO_Buff);

	// ------------------------------------------------------------
	// LL_00 = A * M_OO * A^T
	// ------------------------------------------------------------
	LL_00.assign(N_B, std::vector<double>(N_B, 0.0));

	LL_00 = Product_M(A_M_OO, AT);

	// LL_00 = LL_00 - M_RO_AT - transpose(M_RO_AT)
	for (I = 0; I < N_B; ++I)
	{
		for (J = 0; J < N_B; ++J)
		{
			LL_00[I][J] -= M_RO_AT[I][J] + M_RO_AT[J][I];
		}
	}

	// Add original RR matrix
	for (I = 0; I < N_B; ++I)
	{
		for (J = 0; J < N_B; ++J)
		{
			LL_00[I][J] += M_RR_O[I][J];
		}
	}

	// ------------------------------------------------------------
	// Export PP_00 (node-node matrix)
	// ------------------------------------------------------------
	PP_00.assign(N_N, std::vector<double>(N_N, 0.0));

	for (I = 0; I < N_N; ++I)
		for (J = 0; J < N_N; ++J)
			PP_00[I][J] = P_OO_O[I][J];

	// ------------------------------------------------------------
	// Export branch-to-node connectivity
	// ------------------------------------------------------------
	B2N_OUT.assign(N_B, std::vector<int>(2, 0));

	for (I = 0; I < N_B; ++I)
	{
		B2N_OUT[I][0] = B2N[I][0];
		B2N_OUT[I][1] = B2N[I][1];
	}


	// ------------------------------------------------------------
	// Output sizes
	// ------------------------------------------------------------
	N_BRANCH = N_B;
	N_NODE = N_N;
}

namespace {

	std::string With_Trailing_Slash(std::string path)
	{
		if (!path.empty() && path.back() != '\\' && path.back() != '/') {
			path += "\\";
		}
		return path;
	}

	std::string Resolve_PEEC_Model_Path(const std::string& file_path)
	{
		const std::string root_path = With_Trailing_Slash(file_path);
		return root_path + "output\\";
	}


	bool Ensure_Directory(const std::string& path)
	{
		std::string dir = With_Trailing_Slash(path);
		while (!dir.empty() && (dir.back() == '\\' || dir.back() == '/')) {
			dir.pop_back();
		}

		errno = 0;
		if (_mkdir(dir.c_str()) != 0 && errno != EEXIST) {
			std::cerr << "Cannot create output directory: " << dir << std::endl;
			return false;
		}
		return true;
	}

	bool Read_Double_Matrix_Auto(
		const std::string& path,
		const std::string& stem,
		std::vector<std::vector<double>>& matrix
	)
	{
		const std::string binary_file = path + stem + ".bin";
		if (File_Exists(binary_file)) {
			return Read_Matrix_From_Binary_File(binary_file, matrix);
		}

		return Read_Matrix_From_File(path + stem + ".txt", matrix);
	}

} // namespace

bool Read_PEEC_Model(std::string File_Path)
{
	const std::string model_path = Resolve_PEEC_Model_Path(File_Path);
	std::cout << "PEEC model input path: " << model_path << std::endl;

	if (!Read_Double_Matrix_Auto(model_path, "LL_OO", LL_00)) return false;
	if (!Read_Double_Matrix_Auto(model_path, "PP_OO", PP_00)) return false;
	if (!Read_Matrix_From_File(model_path + "B2N.txt", C_B2N)) return false;

	for (int i = 0; i < C_B2N.size(); i++)
	{
		C_B2N[i][0]++; // 转换为内部1-based索引
		C_B2N[i][1]++; // 转换为内部1-based索引
	}

	std::ifstream fin(model_path + "PORT.txt");
	if (!fin.is_open()) {
		std::cerr << "无法打开文件: " << model_path + "PORT.txt" << std::endl;
		return false;
	}

	// 读取第一行，获取端口数量
	std::string line;
	if (!std::getline(fin, line)) {
		std::cerr << "文件为空或格式错误" << std::endl;
		return false;
	}
	// 解析端口数量
	std::stringstream ss(line);
	if (!(ss >> N_PORT)) {
		std::cerr << "无法解析端口数量" << std::endl;
		return false;
	}

	// 清空并预留空间
	PORT_DATA.clear();
	PORT_DATA.resize(N_PORT);

	// 读取每个端口的数据
	for (int i = 0; i < N_PORT; ++i) {
		if (!std::getline(fin, line)) {
			std::cerr << "文件行数不足，期望 " << N_PORT << " 行，但只有 " << i << " 行" << std::endl;
			return false;
		}

		std::stringstream line_ss(line);
		if (!(line_ss >> PORT_DATA[i].N[0] >> PORT_DATA[i].N[1] >> PORT_DATA[i].Zin)) {
			std::cerr << "第 " << i + 1 << " 行数据格式错误" << std::endl;
			return false;
		}
	}

	fin.close();
	std::cout << "成功读取 " << N_PORT << " 个端口数据" << std::endl;
	return true;
}

bool Save_PEEC_Model(std::string File_Path)
{
	const std::string output_path = With_Trailing_Slash(File_Path);
	if (!Ensure_Directory(output_path)) return false;

	for (int i = 0; i < C_B2N.size(); i++)
	{
		C_B2N[i][0]--; // 转换为文件0-based索引
		C_B2N[i][1]--; // 转换为文件0-based索引
	}

	if (!Write_Matrix_To_Binary_File(output_path + "LL_OO.bin", LL_00)) return false;
	if (!Write_Matrix_To_Binary_File(output_path + "PP_OO.bin", PP_00)) return false;

	if (!Write_Matrix_To_File(output_path + "B2N.txt", C_B2N)) return false;

	std::ofstream fout(output_path + "PORT.txt");
	fout << N_PORT << std::endl;
	for (int i = 0; i < N_PORT; ++i)
	{
		fout << PORT_DATA[i].N[0] << "  " << PORT_DATA[i].N[1] << "  " << PORT_DATA[i].Zin << std::endl;
	}
	fout.close();
	return true;
}