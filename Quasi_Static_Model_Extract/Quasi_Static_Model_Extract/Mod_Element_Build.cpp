#include "Mod_Element_Build.h"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <direct.h>
#include <thread>
#include <utility>

namespace {

constexpr double kBytesPerMegabyte = 1e6;
constexpr int kMatrixTextPrecision = 16;

void TransposeMatrix3(std::array<std::array<double, 3>, 3>& matrix) {
	for (int row = 0; row < 3; ++row) {
		for (int col = row + 1; col < 3; ++col) {
			std::swap(matrix[row][col], matrix[col][row]);
		}
	}
}

Ele_Mesh_Int TransposeElementIntegral(Ele_Mesh_Int element) {
	TransposeMatrix3(element.L);
	TransposeMatrix3(element.C);
	TransposeMatrix3(element.CS);
	return element;
}

int ResolveWorkerCount(int total_tasks) {
	unsigned int hw_threads = std::thread::hardware_concurrency();
	if (hw_threads == 0) {
		hw_threads = 4;
	}

	int worker_count = static_cast<int>(hw_threads);
	return (worker_count > total_tasks) ? total_tasks : worker_count;
}

} // namespace


// 构建矩阵元素的主函数
void Element_Build() {
	Console::Section("P3", "Build Matrix Elements");

	double TIME_S = Get_Time();

	Ini_Element_Sz();

	double ms_ele_mem = static_cast<double>(N_S) * N_S * sizeof(Ele_Mesh_Int) / kBytesPerMegabyte;
	double b_ind_mem = static_cast<double>(CN_N) * C_PEC_N * sizeof(Ele_BL) / kBytesPerMegabyte;
	double n_cap_mem = static_cast<double>(N_S) * N_S * sizeof(Ele_P) / kBytesPerMegabyte;
	double b_cap_mem = static_cast<double>(CN_N) * CN_N * sizeof(Ele_BC) / kBytesPerMegabyte;
	double b_cs_mem = static_cast<double>(CN_N) * sizeof(Ele_CS) / kBytesPerMegabyte;

	Console::Detail("MS memory", std::to_string(static_cast<int>(ms_ele_mem)) + " MB");
	Console::Detail("LL memory", std::to_string(static_cast<int>(b_ind_mem)) + " MB");
	Console::Detail("CC memory", std::to_string(static_cast<int>(b_cap_mem)) + " MB");
	Console::Detail("PP memory", std::to_string(static_cast<int>(n_cap_mem)) + " MB");
	Console::Detail("CS memory", std::to_string(static_cast<int>(b_cs_mem)) + " MB");

	// 核心计算流程
	Build_MS_Ele();
	Re_Construct_Ele();
	Construct_LL_PP();
	MS_ELE.clear();

	if (!Save_PEEC_Model()) {
		Console::Error("Failed to save PEEC model.");
		exit(EXIT_FAILURE);
	}

	Inverse_M(PP, 0);

	//Save_PEEC_Circuit();
	//Save_Netlist();

	double TIME_E = Get_Time();

	// 计算并打印耗时
	Time_Diff(TIME_S, TIME_E);
}


// 分配内存
void Ini_Element_Sz() {

	B_IND.resize(CN_N, std::vector<Ele_BL>(C_PEC_N));
	N_CAP.resize(N_S, std::vector<Ele_P>(N_S));
	MS_ELE.resize(N_S, std::vector<Ele_Mesh_Int>(N_S));

}


void Build_MS_Ele() {
	Console::Info("Building mesh integrals...");
	const int total = PEC_N * (PEC_N + 1) / 2;
	if (total <= 0) {
		Console::Warn("No PEC elements to process.");
		return;
	}
	const int worker_count = ResolveWorkerCount(total);
	Console::Detail("Threads", worker_count);
	Console::Detail("Tasks", total);
	std::atomic<int> next_row{ 0 };
	std::atomic<int> finished{ 0 };
	std::vector<std::thread> workers;
	workers.reserve(worker_count);
	auto worker = [&]() {
		while (true) {
			const int i = next_row.fetch_add(1, std::memory_order_relaxed);
			if (i >= PEC_N) {
				break;
			}
			for (int j = i; j < PEC_N; ++j) {
				Ele_Mesh_Int element = Ele_Cal(i, j);
				MS_ELE[i][j] = element;
				if (i != j) {
					MS_ELE[j][i] = TransposeElementIntegral(element);
				}
			}
			finished.fetch_add(PEC_N - i, std::memory_order_relaxed);
		}
	};
	for (int t = 0; t < worker_count; ++t) {
		workers.emplace_back(worker);
	}
	int last_percent = -1;
	while (finished.load(std::memory_order_relaxed) < total) {
		const int done = finished.load(std::memory_order_relaxed);
		const int percent = static_cast<int>(100.0 * done / total);
		if (percent != last_percent) {
			Console::Progress("Mesh Integral", done, total);
			last_percent = percent;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	for (auto& th : workers) {
		if (th.joinable()) {
			th.join();
		}
	}
	Console::Progress("Mesh Integral", total, total);
}


void Re_Construct_Ele() {
	Console::Info("Constructing potential coefficients...");
	for (int i = 0; i < PEC_N; ++i) {
		for (int j = 0; j < PEC_N; ++j) {
			N_CAP[i][j].VAL = MS_ELE[i][j].P;
			N_CAP[i][j].E[0] = DIELECTRIC[Sur_M[j].M_TYP[0]].real();
			N_CAP[i][j].E[1] = DIELECTRIC[Sur_M[j].M_TYP[1]].real();

			if (Sur_M[j].IS_PEC == 1) {
				N_CAP[i][j].E[0] *= 2.0;
				N_CAP[i][j].E[1] *= 2.0;
				N_CAP[i][j].E[0] = -N_CAP[i][j].E[0];
			}
		}
	}

	Console::Info("Constructing inductive coupling...");
	for (int i = 0; i < CN_N; ++i) {
		for (int j = 0; j < C_PEC_N; ++j) {
			double TEMP = 0.0;

			for (int ii = 0; ii < 2; ++ii) {
				for (int jj = 0; jj < 2; ++jj) {

					int cn_p_i = CN_P[i][ii];
					int cn_p_j = CN_P[j][jj];

					if (cn_p_i != -1 && cn_p_j != -1) {
						int PM = ((ii + jj) % 2 == 0) ? 1 : -1;

						TEMP += PM * MS_ELE[CN[i][ii]][CN[j][jj]].L[cn_p_i][cn_p_j];
					}
				}
			}
			B_IND[i][j].VAL = TEMP;

		}
	}
}
void Construct_LL_PP() {
	std::vector<std::vector<double>> PNPN, PP_0D, PP_MODI;
	std::vector<std::vector<double>> LL_MODI, LN, P_LN, A_E_TEMP;
	std::vector<std::vector<double>> PPAT_MODI_TEMP, A_ET_TEMP;
	std::vector<std::vector<double>> P0D_PN;

	LL.assign(C_PEC_N, std::vector<double>(C_PEC_N, 0.0));
	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < C_PEC_N; ++j) {
			LL[i][j] = U0 * B_IND[i][j].VAL;
		}
	}

	PP.assign(PEC_N, std::vector<double>(PEC_N, 0.0));
	for (int i = 0; i < PEC_N; ++i) {
		for (int j = 0; j < PEC_N; ++j) {
			PP[i][j] = ((1.0 / N_CAP[i][j].E[1] - 1.0 / N_CAP[i][j].E[0]) * N_CAP[i][j].VAL) / E0;
		}
	}
}

bool Save_PEEC_Model()
{
	std::string output_dir = MAP_PATH;
	while (!output_dir.empty() && (output_dir.back() == '\\' || output_dir.back() == '/')) {
		output_dir.pop_back();
	}

	errno = 0;
	if (_mkdir(output_dir.c_str()) != 0 && errno != EEXIST) {
		Console::Error("Cannot create output directory: " + Console::ShortPath(MAP_PATH));
		return false;
	}

	if (!Write_Matrix_To_Binary_File(MAP_PATH + "LL_OO.bin", LL)) return false;
	if (!Write_Matrix_To_Binary_File(MAP_PATH + "PP_OO.bin", PP)) return false;

	if (SAVE_TXT != 0) {
		if (!Write_Matrix_To_File(MAP_PATH + "LL_OO.txt", LL, true, kMatrixTextPrecision)) return false;
		if (!Write_Matrix_To_File(MAP_PATH + "PP_OO.txt", PP, true, kMatrixTextPrecision)) return false;
	}
	else {
		std::remove((MAP_PATH + "LL_OO.txt").c_str());
		std::remove((MAP_PATH + "PP_OO.txt").c_str());
	}

	if (!Write_Matrix_To_File(MAP_PATH + "B2N.txt", CN)) return false;

	std::ofstream fout(MAP_PATH + "PORT.txt");
	fout << N_PORT << std::endl;
	for (int i = 0; i < N_PORT; ++i)
	{
		fout << PORT_DATA[i].N[0] << "  " << PORT_DATA[i].N[1] << "  " << PORT_DATA[i].Zin << std::endl;
	}
	fout.close();
	return true;
}

//void Built_Branch_L() {
//    std::cout << "    Branch L calculating...\n";
//    for (int i = 0; i < CN_N; ++i) {
//        for (int j = 0; j < C_PEC_N; ++j) {
//            B_IND[i][j] = B_L_Cal(i, j);
//
//        }
//        PRT(i + 1, CN_N, 1);
//    }
//
//    // --- 检查矩阵性质 ---
//    for (int i = 0; i < C_PEC_N; ++i) {
//        for (int j = i + 1; j < C_PEC_N; ++j) {
//            if (B_IND[i][j].VAL * B_IND[j][i].VAL > B_IND[i][i].VAL * B_IND[j][j].VAL) {
//                std::cout << "Warning: Matrix property violated at (" << i << "," << j << ")\n";
//                std::cout << B_IND[i][j].VAL << "," << B_IND[j][i].VAL << std::endl;
//                std::cout << B_IND[i][i].VAL << "," << B_IND[j][j].VAL << std::endl;
//               
//            }
//        }
//    }
//}


//void Built_Node_P() {
//    std::cout << "    Node PP calculating...\n";
//    for (int i = 0; i < DS_N + PEC_N; ++i) {
//        for (int j = 0; j < DS_N + PEC_N; ++j) {
//            N_CAP[i][j] = N_PP_Cal(i, j);
//        }
//        PRT(i + 1, DS_N + PEC_N, 1);
//    }
//}


