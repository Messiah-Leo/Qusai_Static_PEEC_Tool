#include"Mod_Cir_Fusion_Tool_Func.h"

/*
 @brief Update_N2B

 @details
 It updates the node-to-branch mapping array N2B according to the
 branch-to-node connectivity matrix B2N.

 The mapping is modified in-place.

 @param[in] B2N
	  Branch-to-node connectivity mapping matrix (size N_B x 2)

 @param[in] N_B
	  Number of branches

 @param[in] N_N
	  Number of nodes

 @param[in,out] N2B
	  Node-to-branch mapping array to be updated

 @return
	  void
 */
void Update_N2B(
	const std::vector<std::vector<int>>& B2N,
	int N_B,
	int N_N,
	std::vector<std::vector<int>>& N2B
)
{

	for (int i = 0; i < N_N; ++i) {
		std::fill(N2B[i].begin(), N2B[i].end(), 0);
	}

	for (int i = 0; i < N_B; ++i) {

		int node1 = B2N[i][0] - 1;
		int count1 = N2B[node1][0] + 1;
		N2B[node1][0] = count1;
		N2B[node1][count1] = -(i + 1);

		int node2 = B2N[i][1] - 1;
		int count2 = N2B[node2][0] + 1;
		N2B[node2][0] = count2;
		N2B[node2][count2] = (i + 1);
	}
}

/*
 @brief Sort_Index

 @details
 This function performs the following tasks:
  1. Reads an input integer index array (ARRAY_IN),
	 which is assumed to be terminated by a zero element.
  2. Counts the number of valid (non-zero) indices and outputs COUNT.
  3. Sorts the valid indices in ascending order.
  4. Stores the sorted indices into ARRAY_OUT.
  5. Appends an additional sentinel value (N_MAT + 1)
	 at position COUNT in ARRAY_OUT for later processing.

 @param[in] ARRAY_IN
	  Input vector containing indices to be sorted

 @param[in] N_MAT
	  Integer scalar used to generate sentinel index

 @param[out] ARRAY_OUT
	  Output vector that will contain sorted indices
	  and an extra sentinel element

 @param[out] COUNT
	  Number of valid indices found in ARRAY_IN

 @return
	  void
 */
void Sort_Index(
	const std::vector<int>& ARRAY_IN,
	int N_MAT,
	std::vector<int>& ARRAY_OUT,
	int& COUNT
)
{
	/*----------------------------------
	 * Determine COUNT (until first 0)
	 *----------------------------------*/
	COUNT = 0;
	while (COUNT < static_cast<int>(ARRAY_IN.size()) &&
		ARRAY_IN[COUNT] != 0)
	{
		++COUNT;
	}

	/*----------------------------------
	 * Initialize ARRAY_OUT
	 *----------------------------------*/
	ARRAY_OUT.assign(ARRAY_OUT.size(), 0);

	for (int i = 0; i < COUNT; ++i)
	{
		ARRAY_OUT[i] = ARRAY_IN[i];
	}

	/*----------------------------------
	 * Selection sort (ascending)
	 *----------------------------------*/
	for (int i = 0; i < COUNT; ++i)
	{
		int MIN = ARRAY_OUT[i];
		int IND_MIN = i;

		for (int j = i + 1; j < COUNT; ++j)
		{
			if (ARRAY_OUT[j] != 0 && MIN > ARRAY_OUT[j])
			{
				MIN = ARRAY_OUT[j];
				IND_MIN = j;
			}
		}

		ARRAY_OUT[IND_MIN] = ARRAY_OUT[i];
		ARRAY_OUT[i] = MIN;
	}

	/*----------------------------------
	 * Append sentinel
	 *----------------------------------*/
	if (COUNT < static_cast<int>(ARRAY_OUT.size()))
	{
		ARRAY_OUT[COUNT] = N_MAT + 1;
	}
}
/*
 @brief Combine_OO_Branches

 @details
 This routine combines two complex OO branches identified by indices
 IND_A and IND_B. After combination, the related matrices are updated
 in-place:
   - M_OO : complex matrix between O nodes
   - M_RO : complex matrix from R nodes to O nodes
   - P_OO : complex parameter matrix between O nodes

 Input dimensions N_O and N_R specify the original sizes of
 the matrices.

 @param[in] IND_A
	  Target branch index to which the other branch will be merged

 @param[in] IND_B
	  Branch index that will be absorbed/removed

 @param[in,out] M_OO
	  - Input:  original complex OO matrix (size N_O x N_O)
	  - Output: updated OO matrix after branch combination

 @param[in,out] M_RO
	  - Input:  original complex R-O matrix (size N_R x N_O)
	  - Output: updated matrix after branch combination

 @param[in,out] P_OO
	  - Input:  original complex P_OO matrix (size N_O x N_O)
	  - Output: updated P_OO matrix after branch combination

 @param[in] N_O
	  Original dimension of O nodes (rows/cols of M_OO and P_OO)

 @param[in] N_R
	  Original dimension of R nodes (rows of M_RO)

 @return
	  void
 */
void Combine_OO_Branches(
	int IND_A,
	int IND_B,
	std::vector<std::vector<double>>& M_OO,
	std::vector<std::vector<double>>& M_RO,
	std::vector<std::vector<double>>& P_OO,
	int N_O,
	int N_R
)
{
	assert(IND_A >= 0 && IND_A < N_O);
	assert(IND_B >= 0 && IND_B < N_O);

	/* ---------- temporary vectors ---------- */
	std::vector<double> m_OI(N_O), m_OS(N_O), m_O(N_O);
	std::vector<double> p_OI(N_O), p_OS(N_O), p_O(N_O);
	std::vector<double> m_RI(N_R), m_RS(N_R), m_R(N_R);

	/* ---------- extract columns ---------- */
	for (int i = 0; i < N_O; ++i) {
		m_OI[i] = M_OO[IND_B][i];
		m_OS[i] = M_OO[IND_A][i];
		p_OI[i] = P_OO[IND_B][i];
		p_OS[i] = P_OO[IND_A][i];
	}

	for (int i = 0; i < N_R; ++i) {
		m_RI[i] = M_RO[i][IND_B];
		m_RS[i] = M_RO[i][IND_A];
	}

	/* ---------- zero diagonal entries ---------- */
	p_OS[IND_A] = 0.0;
	p_OI[IND_A] = 0.0;
	p_OS[IND_B] = 0.0;
	p_OI[IND_B] = 0.0;

	/* ---------- differences ---------- */
	for (int i = 0; i < N_O; ++i) {
		m_O[i] = m_OS[i] - m_OI[i];
		p_O[i] = p_OS[i] - p_OI[i];
	}
	for (int i = 0; i < N_R; ++i) {
		m_R[i] = m_RS[i] - m_RI[i];
	}

	/* ---------- scalar coefficients ---------- */
	double m_SS = M_OO[IND_A][IND_A];
	double m_II = M_OO[IND_B][IND_B];
	double m_SI = M_OO[IND_A][IND_B];

	double p_SS = P_OO[IND_A][IND_A];
	double p_II = P_OO[IND_B][IND_B];
	double p_SI = P_OO[IND_A][IND_B];

	double e1 = 1.0 / (p_SS + p_II - 2.0 * p_SI);

	double e2 = e1 * (p_II - p_SI);
	double e3 = e1 * (p_SS - p_SI);
	double e4 = p_SS * p_II - p_SI * p_SI;
	double e5 = p_II * m_SS + p_SS * m_II - 2.0 * p_SI * m_SI;
	double e6 = m_SS + m_II - 2.0 * m_SI;

	/* =======================================================
	 * Update P_OO
	 * ======================================================= */
	for (int i = 0; i < N_O; ++i) {
		for (int j = 0; j < N_O; ++j) {
			P_OO[i][j] -= e1 * p_O[i] * p_O[j];
		}
	}

	for (int i = 0; i < N_O; ++i) {
		P_OO[IND_A][i] = e2 * p_OS[i] + e3 * p_OI[i];
		P_OO[i][IND_A] = P_OO[IND_A][i];
	}
	P_OO[IND_A][IND_A] = e1 * e4;

	/* =======================================================
	 * Update M_OO
	 * ======================================================= */
	for (int i = 0; i < N_O; ++i) {
		for (int j = 0; j < N_O; ++j) {
			M_OO[i][j] -=
				e1 * (m_O[i] * p_O[j] + p_O[i] * m_O[j]);
		}
	}

	for (int i = 0; i < N_O; ++i) {
		M_OO[IND_A][i] = e2 * m_OS[i] + e3 * m_OI[i];
		M_OO[i][IND_A] = M_OO[IND_A][i];
	}
	M_OO[IND_A][IND_A] = e1 * (e5 - e1 * e4 * e6);

	/* =======================================================
	 * Update M_RO
	 * ======================================================= */
	for (int i = 0; i < N_R; ++i) {
		for (int j = 0; j < N_O; ++j) {
			M_RO[i][j] -= e1 * m_R[i] * p_O[j];
		}
	}

	for (int i = 0; i < N_R; ++i) {
		M_RO[i][IND_A] = e2 * m_RS[i] + e3 * m_RI[i];
	}

	/* =======================================================
	 * Absorb branch IND_B
	 * ======================================================= */
	for (int i = 0; i < N_O; ++i) {
		M_OO[IND_B][i] = 0.0;
		M_OO[i][IND_B] = 0.0;
		P_OO[IND_B][i] = 0.0;
		P_OO[i][IND_B] = 0.0;
	}

	for (int i = 0; i < N_R; ++i) {
		M_RO[i][IND_B] = 0.0;
	}
}

/*
 @brief Combine_RR_Branches

 @details
 It combines two complex RR branches identified by IND_A and IND_B.
 After the combination, all related matrices are updated in-place.

 Matrices involved:
   - M_RR : complex matrix between R nodes (size N_R x N_R)
   - M_OO : complex matrix between O nodes (size N_O x N_O)
   - M_RO : complex matrix from R nodes to O nodes (size N_R x N_O)

 The function performs several consistency checks on matrix elements
 to avoid numerical singularity during combination.

 @param[in] IND_A
	  Target RR branch index

 @param[in] IND_B
	  RR branch index that will be absorbed and merged

 @param[in,out] M_RR
	  - Input:  original RR complex matrix
	  - Output: updated RR matrix after branch combination

 @param[in,out] M_OO
	  - Input:  original OO complex matrix
	  - Output: updated OO matrix related to the RR combination

 @param[in,out] M_RO
	  - Input:  original R-O complex matrix
	  - Output: updated matrix after branch combination

 @param[in] N_R
	  Original number of R nodes

 @param[in] N_O
	  Original number of O nodes

 @return
	  void
 */
void Combine_RR_Branches(
	int IND_A,
	int IND_B,
	std::vector<std::vector<double>>& M_RR,
	std::vector<std::vector<double>>& M_OO,
	std::vector<std::vector<double>>& M_RO,
	int N_R,
	int N_O
)
{
	// --------------------------------------------------
	// Allocate temporary matrices (Fortran: (N,1))
	// --------------------------------------------------
	std::vector<std::vector<double>> m_OI(N_O, std::vector<double>(1));
	std::vector<std::vector<double>> m_RI(N_R, std::vector<double>(1));
	std::vector<std::vector<double>> m_OS(N_O, std::vector<double>(1));
	std::vector<std::vector<double>> m_RS(N_R, std::vector<double>(1));
	std::vector<std::vector<double>> m_O(N_O, std::vector<double>(1));
	std::vector<std::vector<double>> m_R(N_R, std::vector<double>(1));

	// --------------------------------------------------
	// Extract rows / columns
	// --------------------------------------------------
	for (int i = 0; i < N_O; ++i)
		m_OI[i][0] = M_RO[IND_B][i];

	for (int i = 0; i < N_R; ++i)
		m_RI[i][0] = M_RR[i][IND_B];

	for (int i = 0; i < N_O; ++i)
		m_OS[i][0] = M_RO[IND_A][i];

	for (int i = 0; i < N_R; ++i)
		m_RS[i][0] = M_RR[i][IND_A];

	// --------------------------------------------------
	// m_O = m_OS - m_OI
	// m_R = m_RS - m_RI
	// --------------------------------------------------
	for (int i = 0; i < N_O; ++i)
		m_O[i][0] = m_OS[i][0] - m_OI[i][0];

	for (int i = 0; i < N_R; ++i)
		m_R[i][0] = m_RS[i][0] - m_RI[i][0];

	// --------------------------------------------------
	// Scalar terms
	// --------------------------------------------------
	double m_SS = M_RR[IND_A][IND_A];
	double m_II = M_RR[IND_B][IND_B];
	double m_SI = M_RR[IND_A][IND_B];

	if (m_SI < 0.0) {
		std::cerr << "ERROR IN Combine_RR_Branches\n";
		std::cerr << m_SS << " " << m_II << " " << m_SI << std::endl;
	}

	if (std::abs(m_SI * m_SI / (m_SS * m_II)) >= 1.0 ||
		std::abs(m_SS + m_II - 2.0 * m_SI) < 1e-20)
	{
		m_SI = 0.9999 * std::sqrt(m_SS * m_II);
	}

	double g1 = 1.0 / (m_SS + m_II - 2.0 * m_SI);
	double g2 = g1 * (m_II - m_SI);
	double g3 = g1 * (m_SS - m_SI);

	// ==================================================
	// update M_OO = M_OO - g1 * (m_O * m_O^T)
	// ==================================================
	std::vector<std::vector<double>> m_OT(1, std::vector<double>(N_O));
	for (int i = 0; i < N_O; ++i)
		m_OT[0][i] = m_O[i][0];

	auto BUFF_OO = Product_M(m_O, m_OT);

	for (int i = 0; i < N_O; ++i)
		for (int j = 0; j < N_O; ++j)
			M_OO[i][j] -= g1 * BUFF_OO[i][j];

	// ==================================================
	// update M_RR = M_RR - g1 * (m_R * m_R^T)
	// ==================================================
	std::vector<std::vector<double>> m_RT(1, std::vector<double>(N_R));
	for (int i = 0; i < N_R; ++i)
		m_RT[0][i] = m_R[i][0];

	auto BUFF_RR = Product_M(m_R, m_RT);

	for (int i = 0; i < N_R; ++i)
		for (int j = 0; j < N_R; ++j)
			M_RR[i][j] -= g1 * BUFF_RR[i][j];

	// --------------------------------------------------
	// Update merged RR row / column
	// --------------------------------------------------
	for (int i = 0; i < N_R; ++i)
		M_RR[IND_A][i] = g2 * m_RS[i][0] + g3 * m_RI[i][0];

	for (int i = 0; i < N_R; ++i)
		M_RR[i][IND_A] = M_RR[IND_A][i];

	M_RR[IND_A][IND_A] = g1 * (m_SS * m_II - m_SI * m_SI);

	// ==================================================
	// update M_RO = M_RO - g1 * (m_R * m_O^T)
	// ==================================================
	auto BUFF_RO = Product_M(m_R, m_OT);

	for (int i = 0; i < N_R; ++i)
		for (int j = 0; j < N_O; ++j)
			M_RO[i][j] -= g1 * BUFF_RO[i][j];

	// --------------------------------------------------
	// Update merged RO row
	// --------------------------------------------------
	for (int j = 0; j < N_O; ++j)
		M_RO[IND_A][j] = g2 * m_OS[j][0] + g3 * m_OI[j][0];

	//std::cout << "xxx" << M_RO[IND_A][0] << "   " << M_RO[IND_A][1] << std::endl;
}

/*
 @brief LL_DIS

 @details
 It computes a distance-like metric between two nodes A and B
 based on the complex matrix M_RR.

 @param[in] M
	  Input complex matrix

 @param[in] A
	  Index of first node

 @param[in] B
	  Index of second node

 @return
	  Evaluated metric value as double
 */
double LL_DIS(
	const std::vector<std::vector<double>>& M,
	int a,
	int b
)
{
	const double& M_ab = M[a][b];
	const double& M_ba = M[b][a];
	const double& M_aa = M[a][a];
	const double& M_bb = M[b][b];

	return std::real(M_ab) * std::abs(M_ba) / std::real(M_aa * M_bb);
}

/*
 @brief Arrange_Min_Loop

 @details
 It arranges the ordering of branches according to a metric
 defined by LL_DIS.

 @param[in] M_RR
	  Input complex matrix

 @param[in] N_C
	  Number of branches

 @param[in] N2B_X
	  Mapping array from candidate index to branch index

 @param[out] C_Branch
	  Output arranged branch index array

 @return
	  void
 */
void Arrange_Min_Loop(
	const std::vector<std::vector<double>>& M_RR,
	int N_C,
	const std::vector<int>& N2B_input,
	std::vector<int>& C_Branch
)
{
	/* ---------- temporary arrays ---------- */
	std::vector<int> TEMP_ARR(N_C, 0);
	std::vector<int> PT_SEL(N_C, 0);
	std::vector<int> N2B_X;
	int COUNT = 0;
	int I_S = 0, J_S = 0;

	double MIN_LL = 1.0e10;
	N2B_X.resize(N_C);
	for (int i = 0; i < N_C; ++i) {
		N2B_X[i] = N2B_input[i + 1] - 1;
	}
	/* =======================================================
	 * Step 1: find closest initial pair
	 * ======================================================= */
	for (int i = 0; i < N_C; ++i) {
		for (int j = i + 1; j < N_C; ++j) {
			double D_ORI = LL_DIS(M_RR, N2B_X[i], N2B_X[j]);
			if (D_ORI < MIN_LL) {
				MIN_LL = D_ORI;
				I_S = i;
				J_S = j;
			}
		}
	}

	/* initialize */
	TEMP_ARR[0] = I_S;
	TEMP_ARR[1] = J_S;
	PT_SEL[I_S] = 1;
	PT_SEL[J_S] = 1;
	COUNT = 2;

	/* =======================================================
	 * Step 2: incremental insertion
	 * ======================================================= */
	while (COUNT < N_C) {
		MIN_LL = 1.0e10;
		I_S = -1;
		J_S = -1;

		for (int i = 0; i < COUNT; ++i) {
			int ip = (i + 1) % N_C;

			for (int j = 0; j < N_C; ++j) {
				if (PT_SEL[j] == 0) {
					double D_ORI = LL_DIS(M_RR, N2B_X[i], N2B_X[ip]);

					double D_I = LL_DIS(M_RR, N2B_X[i], N2B_X[j]);

					double D_J = LL_DIS(M_RR, N2B_X[ip], N2B_X[j]);

					double D_MODI = D_I + D_J - D_ORI;

					if (D_MODI < MIN_LL) {
						MIN_LL = D_MODI;
						I_S = i;
						J_S = j;
					}
				}
			}
		}

		if (J_S + 1) {
			for (int k = COUNT; k > I_S; --k) {
				TEMP_ARR[k] = TEMP_ARR[k - 1];
			}
			TEMP_ARR[I_S] = J_S;
			PT_SEL[J_S] = 1;
			COUNT++;
		}
		else {
			std::cerr << "ERROR!!!!!!!!!!!!! IN Arrange_Min_Loop\n";
			std::exit(EXIT_FAILURE);
		}
	}

	/* =======================================================
	 * Output mapping
	 * ======================================================= */
	C_Branch.resize(N_C);
	for (int i = 0; i < N_C; ++i) {
		C_Branch[i] = N2B_X[TEMP_ARR[i]];
	}
}

/*
 @brief Node_Significance

 @details
 It evaluates the significance of a given node based on several
 complex matrices related to R and O nodes.

 Matrices involved:
   - M_RR : complex matrix between R nodes
   - M_RO : complex matrix from R nodes to O nodes
   - M_OO : complex matrix between O nodes
   - P_OO : auxiliary complex matrix between O nodes
   - P_OO : another auxiliary matrix (named P_OO in Fortran)

 The result is used as a numerical indicator for node importance
 in model reduction and branch arrangement.

 @param[in] M_RR
	  Complex RR matrix

 @param[in] M_RO
	  Complex RO matrix

 @param[in] M_OO
	  Complex OO matrix

 @param[in] P_OO
	  Auxiliary complex matrix

 @param[in] N2B
	  Node-to-branch mapping array

 @param[in] B2N
	  Branch-to-node connectivity mapping

 @param[in] Node_N
	  Index of the node being evaluated

 @param[in] W
	  Weighting factor

 @return
	  Evaluated node significance as double
 */
double Node_Significance(
	const std::vector<std::vector<double>>& M_RR,
	const std::vector<std::vector<double>>& M_RO,
	const std::vector<std::vector<double>>& M_OO,
	const std::vector<std::vector<double>>& P_OO,
	const std::vector<std::vector<int>>& N2B,
	const std::vector<std::vector<int>>& B2N,
	int Node_N,
	double W
)
{

	int N_C = N2B[Node_N][0];   // number of connected branches

	/* ---------- early exit ---------- */
	if (N_C <= 2) {
		return 1.0e8;
	}

	/* ---------- temporary arrays ---------- */
	std::vector<int> L_B(N_C, 0);     // branch index
	std::vector<int> L_N(N_C, 0);     // neighbor node index
	std::vector<int> WT(N_C, 0);     // orientation (+1 / -1)

	std::vector<double> MM_II(N_C), MM_IO(N_C), MM_OO(N_C);
	std::vector<double> PP_II(N_C), PP_IO(N_C), PP_OO(N_C);

	/* =======================================================
	 * Collect connected branches and neighboring nodes
	 * ======================================================= */
	for (int i = 0; i < N_C; ++i) {
		int br = std::abs(N2B[Node_N][i + 1]);
		L_B[i] = br - 1;

		/* orientation */
		if (B2N[L_B[i]][0] - 1 == Node_N) {
			L_N[i] = B2N[L_B[i]][1] - 1;
			WT[i] = -1;
		}
		else {
			L_N[i] = B2N[L_B[i]][0] - 1;
			WT[i] = 1;
		}
	}
	/* =======================================================
	 * Build equivalent parameters
	 * ======================================================= */
	for (int i = 0; i < N_C; ++i) {

		if (std::abs(P_OO[L_N[i]][L_N[i]]) == 0.0) {
			return 1.0e8;
		}

		MM_II[i] =
			M_RR[L_B[i]][L_B[i]] + 2.0 * WT[i] * M_RO[L_B[i]][Node_N] + N_C * M_OO[Node_N][Node_N];

		MM_IO[i] =
			WT[i] * M_RO[L_B[i]][L_N[i]] + M_OO[Node_N][L_N[i]];

		MM_OO[i] =
			M_OO[L_N[i]][L_N[i]];

		PP_II[i] =
			N_C * P_OO[Node_N][Node_N];

		PP_IO[i] =
			P_OO[Node_N][L_N[i]];

		PP_OO[i] =
			P_OO[L_N[i]][L_N[i]];
	}

	/* =======================================================
	 * Accumulate node significance
	 * ======================================================= */
	double Node_Significance = 0.0;

	for (int i = 0; i < N_C; ++i) {
		Node_Significance += std::abs((MM_II[i] - MM_IO[i]) / (PP_II[i] - PP_IO[i]))
			+ std::abs((MM_OO[i] - MM_IO[i]) / (PP_OO[i] - PP_IO[i]));
	}

	Node_Significance *= W * W;
	return Node_Significance;
}
/*
 @brief Check_PL

 @details
 It checks the passive consistency of a complex matrix P.
 For each pair of nodes (I, J), the following condition is tested:

	 Re( P(I,I) P(J,J) ) < Re( P(I,J) P(J,I) )

 If the condition is violated, a warning message should be issued.

 @param[in] P
	  Input complex matrix to be checked (size n x n)

 @param[in] n
	  Dimension of the matrix

 @return
	  void
 */
void Check_PL(
	const std::vector<std::vector<double>>& P,
	int n
)
{
	for (int i = 0; i < n; ++i) {
		for (int j = i + 1; j < n; ++j) {

			double lhs =
				std::real(P[i][i] * P[j][j]);

			double rhs =
				std::real(P[i][j] * P[j][i]);

			if (lhs < rhs) {
				std::cout << "WARNING "
					<< (i + 1) << " "
					<< (j + 1) << " "
					<< P[i][i] << " "
					<< P[j][j] << " "
					<< P[i][j]
					<< std::endl;
			}
		}
	}
}