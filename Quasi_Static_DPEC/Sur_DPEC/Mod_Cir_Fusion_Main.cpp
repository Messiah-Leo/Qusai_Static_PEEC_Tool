#include "Mod_Cir_Fusion_Main.h"

#include <fstream>
#include <iomanip>
#include <string>

namespace {

int Circuit_Element_Count(int node_count, int branch_count)
{
	return node_count + branch_count;
}

void Write_DPEC_Log(
	const std::string& output_path,
	double reduction_time_ms,
	int nodes_before,
	int nodes_after,
	int branches_before,
	int branches_after,
	int reduction_steps)
{
	std::ofstream output(output_path + "DPEC.log");
	if (!output.is_open())
	{
		std::cerr << "Failed to write DPEC.log: " << output_path << "DPEC.log" << std::endl;
		return;
	}

	output << "report=DPEC reduction" << std::endl;
	output << "reduction_time_ms=" << std::fixed << std::setprecision(1) << reduction_time_ms << std::endl;
	output << "nodes_before=" << nodes_before << std::endl;
	output << "nodes_after=" << nodes_after << std::endl;
	output << "branches_before=" << branches_before << std::endl;
	output << "branches_after=" << branches_after << std::endl;
	output << "circuit_elements_definition=nodes + branches" << std::endl;
	output << "circuit_elements_before=" << Circuit_Element_Count(nodes_before, branches_before) << std::endl;
	output << "circuit_elements_after=" << Circuit_Element_Count(nodes_after, branches_after) << std::endl;
	output << "reduction_steps=" << reduction_steps << std::endl;

	std::cout << "Reduction report: " << output_path << "DPEC.log" << std::endl;
}

} // namespace

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
	const int Original_Circuit_B = static_cast<int>(LL_00.size());
	const int Original_Circuit_N = static_cast<int>(PP_00.size());

	std::cout << "============================================" << std::endl;
	std::cout << "   Circuit fusion start!                    " << std::endl;

	// ------------------------------------------------------------
	// Initialize circuit data structures
	// ------------------------------------------------------------
	C_N_Circuit_B = static_cast<int>(LL_00.size());
	C_N_Circuit_N = static_cast<int>(PP_00.size());
	Ini_Data_Structure(
		C_N_Circuit_B,
		C_N_Circuit_N,
		LL_00,
		PP_00,
		C_B2N,
		C_N2B,
		W
	);

	const double Reduction_Time_S = Get_Time();

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
		// --------------------------------------------------------
		// Find next insignificant node
		// --------------------------------------------------------
		Find_Insig_Node(Node_Num, Node_ER);
	}

	const double Reduction_Time_E = Get_Time();

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

	Save_PEEC_Model(PATH + "output\\");
	Write_DPEC_Log(
		PATH + "output\\",
		Reduction_Time_E - Reduction_Time_S,
		Original_Circuit_N,
		C_N_Circuit_N,
		Original_Circuit_B,
		C_N_Circuit_B,
		COUNT);

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