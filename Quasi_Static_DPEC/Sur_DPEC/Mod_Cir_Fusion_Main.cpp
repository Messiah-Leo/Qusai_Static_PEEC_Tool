#include "Mod_Cir_Fusion_Main.h"

void Circuit_Funsing_Main()
{
	// ------------------------------------------------------------
	// Local variables
	// ------------------------------------------------------------
	int    Node_Num = 0;
	double Node_ER = 0.0;
	double W;

	int COUNT = 0;

	// ------------------------------------------------------------
	// Angular frequency
	// ------------------------------------------------------------
	W = 2.0 * PI * MAX_FREQ;

	Read_PEEC_Model(PATH);

	std::cout << "============================================" << std::endl;
	std::cout << "   Circuit fusion start!                    " << std::endl;

	// ------------------------------------------------------------
	// Initialize circuit data structures
	// ------------------------------------------------------------
	C_N_Circuit_B = LL_00.size();
	C_N_Circuit_N = PP_00.size();
	Ini_Data_Structure(
		C_N_Circuit_B,
		C_N_Circuit_N,
		LL_00,
		PP_00,
		C_B2N,
		C_N2B,
		W
	);

	// ------------------------------------------------------------
	// Find first insignificant node
	// ------------------------------------------------------------
	Find_Insig_Node(Node_Num, Node_ER);
	// ------------------------------------------------------------
	// Iterative circuit fusion loop
	// ------------------------------------------------------------
	COUNT = 0;
	while (Node_Num != 0 && Node_ER < MAX_ERROR && COUNT < MAX_NODE)
	{
		COUNT++;

		std::cout << COUNT << "  "
			<< Node_ER << "  "
			<< Node_Num << std::endl;

		// --------------------------------------------------------
		// Re-arrange matrices based on the selected node
		// --------------------------------------------------------
		Re_arrange_Mat(Node_Num);
		Update_PORT(Node_Num);
		// Update node count and circuit matrices
		Update_Cir_Matrix();

		// --------------------------------------------------------
		// Combine branches and nodes
		// --------------------------------------------------------
		Combine_Branch_Circuits();
		Combine_Node_Circuits();

		//system("pause");
		// --------------------------------------------------------
		// Find next insignificant node
		// --------------------------------------------------------
		Find_Insig_Node(Node_Num, Node_ER);
	}


	// ------------------------------------------------------------
	// Final fused circuit model
	// ------------------------------------------------------------
	Fused_Circuit_Model(
		LL_00,
		PP_00,
		C_N_Circuit_B,
		C_N_Circuit_N,
		C_B2N,
		PORT_DATA,
		N_PORT
	);

	Save_PEEC_Model(PATH);

	std::cout << "Finished fusion" << std::endl;

	// ------------------------------------------------------------
	// (Optional) Eigenvalue check ¨C commented out in original code
	// ------------------------------------------------------------
	/*
	std::vector<std::complex<double>> Test_Eig(C_N_Circuit_B);
	std::vector<std::vector<double>> ll_BUF(
		C_N_Circuit_B,
		std::vector<double>(C_N_Circuit_B, 0.0)
	);

	for (int i = 0; i < C_N_Circuit_B; ++i)
		for (int j = 0; j < C_N_Circuit_B; ++j)
			ll_BUF[i][j] = std::real(LL_00[i][j]);

	Eigen_Decomposition(C_N_Circuit_B, ll_BUF, Test_Eig);

	for (int i = 0; i < C_N_Circuit_B; ++i)
	{
		if (std::real(Test_Eig[i]) < 0.0)
		{
			std::cout << "Negative eigenvalue at "
					  << i << " : "
					  << Test_Eig[i] << std::endl;
		}
	}
	*/
}