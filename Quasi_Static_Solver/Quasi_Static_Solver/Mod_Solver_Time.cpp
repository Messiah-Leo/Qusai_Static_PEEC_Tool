#include "Mod_Solver_Time.h"

int Source_Sz = 1;
int N_TPT;
double T_STEP;
std::vector<std::vector<double>> PP_AET;
std::vector<std::vector<double>> VI, D_VI, VI_TEMP, D_VI_TEMP;
std::vector<std::vector<double>> A_M, B_M;
std::vector<std::vector<std::vector<double>>> x_t;

std::vector<int> Generate_PRBS_Sequence(int order, int length) {
	std::vector<int> seq;
	seq.reserve(length);

	uint32_t lfsr = (1 << order) - 1;

	for (int i = 0; i < length; ++i) {
		int output_bit = lfsr & 1;
		seq.push_back(output_bit);

		unsigned int feedback = 0;
		switch (order) {
		case 5:
			feedback = ((lfsr >> 0) ^ (lfsr >> 2)) & 1;
			break;
		case 7:
			feedback = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
			break;
		case 9:
			feedback = ((lfsr >> 0) ^ (lfsr >> 4)) & 1;
			break;
		case 11:
			feedback = ((lfsr >> 0) ^ (lfsr >> 2)) & 1;
			break;
		case 13:
			feedback = ((lfsr >> 0) ^ (lfsr >> 1) ^ (lfsr >> 2) ^ (lfsr >> 5)) & 1;
			break;
		case 15:
			feedback = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
			break;
		case 23:
			feedback = ((lfsr >> 0) ^ (lfsr >> 5)) & 1;
			break;
		case 31:
			feedback = ((lfsr >> 0) ^ (lfsr >> 3)) & 1;
			break;
		default:
			feedback = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
			break;
		}
		lfsr = (lfsr >> 1) | (feedback << (order - 1));
	}
	return seq;
}

//¶ÁÈ¡ÅäÖÃÎÄ¼þ
bool Read_Source_Config(const std::string& configPath, SourceConfig& config) {
	std::ifstream fin(configPath);
	if (!fin.is_open()) return false;

	std::string line, key, equal;

	while (std::getline(fin, line)) {
		if (line.empty()) continue;
		std::stringstream ss(line);
		ss >> key >> equal; 

		if (key == "type") ss >> (int&)config.type;
		else if (key == "amplitude") ss >> config.amplitude;
		else if (key == "rise_time") ss >> config.rise_time;
		else if (key == "bit_rate") ss >> config.bit_rate;
		else if (key == "prbs_N") ss >> config.prbs_N;
		else if (key == "total_time") ss >> config.total_time;
		else if (key == "samples_per_bit") ss >> config.samples_per_bit;
	}

	fin.close();
	return true;
}

bool Generate_Source_File(const SourceConfig& config, const std::string& outputPath) {
	std::cout << "Generating source waveform to file..." << std::endl;

	double bit_duration = 1.0 / config.bit_rate;
	double t_step_local = bit_duration / (double)config.samples_per_bit;
	int n_tpt_local = static_cast<int>(std::round(config.total_time / t_step_local));

	std::ofstream fout(outputPath);
	if (!fout.is_open()) return false;

	fout << std::scientific << std::setprecision(10);
	fout << t_step_local << " " << n_tpt_local << "\n";

	std::vector<int> prbs_bits;
	if (config.type == SRC_PRBS) {
		int total_bits = static_cast<int>(config.total_time * config.bit_rate) + 5;
		prbs_bits = Generate_PRBS_Sequence(config.prbs_N, total_bits);
	}

	for (int i = 1; i <= n_tpt_local; ++i) {
		double t = i * t_step_local;
		double val = 0.0;

		if (config.type == SRC_STEP) {
			if (t < config.rise_time) {
				double frac = t / config.rise_time;
				val = config.amplitude * 0.5 * (1.0 - std::cos(PI * frac));
			}
			else {
				val = config.amplitude;
			}
		}
		else if (config.type == SRC_PRBS) {
			int current_idx = static_cast<int>(t / bit_duration);
			if (current_idx >= prbs_bits.size()) current_idx = prbs_bits.size() - 1;

			int curr_bit = prbs_bits[current_idx];
			int prev_bit = (current_idx > 0) ? prbs_bits[current_idx - 1] : 0;
			double t_in_bit = t - current_idx * bit_duration;

			if (t_in_bit < config.rise_time) {
				double frac = t_in_bit / config.rise_time;
				double smooth_factor = 0.5 * (1.0 - std::cos(PI * frac));
				val = config.amplitude * ((double)prev_bit + (double)(curr_bit - prev_bit) * smooth_factor);
			}
			else {
				val = config.amplitude * curr_bit;
			}
		}
		fout << val << "\n";
	}

	fout.close();
	return true;
}

bool Read_Signal_File(std::string File_Path) {
	std::ifstream fin(File_Path);
	if (!fin.is_open()) return false;

	if (!(fin >> T_STEP >> N_TPT)) return false;

	x_t.assign(N_TPT + 1, std::vector<std::vector<double>>(Source_Sz, std::vector<double>(Source_Sz, 0.0)));

	double val;
	for (int i = 1; i <= N_TPT; ++i) {
		if (fin >> val) x_t[i][0][0] = val;
		else break;
	}
	fin.close();
	return true;
}

void Ini_Element() {
	std::cout << "Ini_Elemnet..." << std::endl;

	PEC_N = PP.size();
	C_PEC_N = LL.size();
	NODE_N = PEC_N;
	L_BL.resize(3);
	S_BL.resize(3);
	L_BL[0] = PEC_N;
	L_BL[1] = C_PEC_N;
	L_BL[2] = N_PORT;

	S_BL[0] = 0;
	S_BL[1] = S_BL[0] + L_BL[0];
	S_BL[2] = S_BL[1] + L_BL[1];
	M_SZ = S_BL[2] + L_BL[2];

	A_E.assign(C_PEC_N + N_PORT, std::vector<double>(NODE_N, 0.0));
	A_ET.assign(NODE_N, std::vector<double>(C_PEC_N + N_PORT, 0.0));

	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < 2; ++j) {
			int idx = CN[i][j];
			if (idx < NODE_N && idx >= 0) {
				A_E[i][idx] = std::pow(-1.0, j + 2);
				A_ET[idx][i] = std::pow(-1.0, j + 1);
			}
		}
	}

	for (int i = 0; i < N_PORT; ++i) {
		int idx1 = PORT_DATA[i].N[0];
		int idx2 = PORT_DATA[i].N[1];
		if (idx1 < NODE_N && idx1 >= 0) {
			A_E[i + C_PEC_N][idx1] = 1.0;
			A_ET[idx1][i + C_PEC_N] = -1.0;
		}
		if (idx2 < NODE_N && idx2 >= 0) {
			A_E[i + C_PEC_N][idx2] = -1.0;
			A_ET[idx2][i + C_PEC_N] = 1.0;
		}
	}

	VI.assign(M_SZ, std::vector<double>(Source_Sz, 0.0));
	D_VI.assign(M_SZ, std::vector<double>(Source_Sz, 0.0));
	VI_TEMP.assign(M_SZ, std::vector<double>(Source_Sz, 0.0));
	D_VI_TEMP.assign(M_SZ, std::vector<double>(Source_Sz, 0.0));

}

void Calc_PP_AET() {
	std::cout << "Calculating PP_AET..." << std::endl;
	PP_AET.assign(NODE_N, std::vector<double>(C_PEC_N + N_PORT, 0.0));
	PP_AET = Product_M(PP, A_ET);
}

void Build_CP_M() {
	std::cout << "Building System Matrices (A_M, B_M)..." << std::endl;

	std::vector<std::vector<double>> E_TEMP(M_SZ, std::vector<double>(M_SZ, 0.0));
	std::vector<std::vector<double>> A_TEMP(M_SZ, std::vector<double>(M_SZ, 0.0));
	std::vector<std::vector<double>> B_TEMP(M_SZ, std::vector<double>(Source_Sz, 0.0));

	for (int i = 0; i < PEC_N; ++i) {
		E_TEMP[i][i] = 1.0;
	}

	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < C_PEC_N; ++j) {
			E_TEMP[i + S_BL[1]][j + S_BL[1]] = LL[i][j];
		}
	}

	for (int i = 0; i < NODE_N; ++i) {
		for (int j = 0; j < C_PEC_N + N_PORT; ++j) {
			A_TEMP[i][j + S_BL[1]] = PP_AET[i][j];
			A_TEMP[j + S_BL[1]][i] = A_E[j][i];
		}
	}

	for (int i = 0; i < C_PEC_N; ++i) {
		A_TEMP[i + S_BL[1]][i + S_BL[1]] = -1e-10;
	}

	for (int i = 0; i < N_PORT; ++i) {
		A_TEMP[i + S_BL[2]][i + S_BL[2]] = -PORT_DATA[i].Zin;
	}

	if (M_SZ > 0 && Source_Sz > 0) {
		B_TEMP[M_SZ - 1][Source_Sz - 1] = -1.0;
	}

	std::vector<std::vector<double>> E_HA(M_SZ, std::vector<double>(M_SZ, 0.0));
	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < M_SZ; ++j) {
			E_HA[i][j] = E_TEMP[i][j] - 0.5 * T_STEP * A_TEMP[i][j];
		}
	}

	bool a = Inverse_M(E_HA, 0);
	/*if (!success) {
		std::cerr << "Error: Matrix Inversion failed!" << std::endl;
		return;
	}*/

	A_M.assign(M_SZ, std::vector<double>(M_SZ, 0.0));
	A_M = Product_M(E_HA, A_TEMP); 

	B_M.assign(M_SZ, std::vector<double>(Source_Sz, 0.0));
	B_M = Product_M(E_HA, B_TEMP);
};

void Update_VI(const std::vector<std::vector<double>>& S_in) {
	std::vector<std::vector<double>> A_M_BUF(M_SZ, std::vector<double>(Source_Sz, 0.0));
	std::vector<std::vector<double>> B_M_BUF(M_SZ, std::vector<double>(Source_Sz, 0.0));

	D_VI_TEMP = D_VI;
	VI_TEMP = VI;

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < Source_Sz; ++j) {
			VI[i][j] += 0.5 * T_STEP * D_VI[i][j];
		}
	}

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < Source_Sz; ++j) {
			for (int k = 0; k < M_SZ; ++k) {
				A_M_BUF[i][j] += A_M[i][k] * VI[k][j];
			}
		}
	}

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < Source_Sz; ++j) {
			for (int k = 0; k < Source_Sz; ++k) {
				B_M_BUF[i][j] += B_M[i][k] * S_in[k][j];
			}
		}
	}

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < Source_Sz; ++j) {
			D_VI[i][j] = A_M_BUF[i][j] + B_M_BUF[i][j];
		}
	}

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < Source_Sz; ++j) {
			VI[i][j] = VI_TEMP[i][j] + 0.5 * T_STEP * (D_VI_TEMP[i][j] + D_VI[i][j]);
		}
	}
};

void Time_Solver() {
	std::cout << "Starting Time Domain Solver..." << std::endl;

	if (!Read_PEEC_Model(MAP_PATH)) return;
	Ini_Element();

	SourceConfig config;
	std::string config_txt = SOURCE_PATH + "SourceConfig.txt";
	std::string source_txt = SOURCE_PATH + "Source.txt";

	if (Read_Source_Config(config_txt, config)) {
		Generate_Source_File(config, source_txt);
	}
	else {
		std::cout << "Config file not found, trying to use existing Source.txt" << std::endl;
	}

	if (!Read_Signal_File(source_txt)) {
		std::cerr << "Error: No source data available." << std::endl;
		return;
	}

	Calc_PP_AET();
	Build_CP_M();

	std::string out_file = MAP_PATH + "output_voltage.txt";
	std::ofstream outFile(out_file);
	if (!outFile.is_open()) {
		std::cerr << "Error: Could not open output file: " << out_file << std::endl;
		return;
	}

	outFile << "Time";
	for (int k = 0; k < N_PORT; ++k) {
		outFile << ", Port_" << k + 1 << "_Voltage";
	}
	outFile << std::endl;

	std::cout << "Solving steps..." << std::endl;
	for (int i = 1; i <= N_TPT; ++i) {
		Update_VI(x_t[i]);

		outFile << i * T_STEP << ", "
			<< x_t[i][0][0] << ", "
			<< VI[PORT_DATA[0].N[0]][0] - VI[PORT_DATA[0].N[1]][0] << ", "
			<< VI[PORT_DATA[1].N[0]][0] - VI[PORT_DATA[1].N[1]][0] << std::endl;
	}
	std::cout << std::endl;

	outFile.close();
	std::cout << "Solver Finished. Results saved to " << out_file << std::endl;
}