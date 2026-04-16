#include"Mod_Solver_Freq.h"

std::vector<std::vector<std::complex<double>>> Z0, Y0, SQRT_Z0, SQRT_Y0;
std::vector<std::vector<std::complex<double>>> CP_M, Source;
std::vector<std::vector<std::complex<double>>> Z_TEMP, Y_TEMP, S_TEMP;
std::vector<std::vector<std::complex<double>>> V_TEMP, I_TEMP;
std::vector<std::vector<std::complex<double>>> H_W;


int POS;
double MAX_E = 0.0;
const std::complex<double> IJ(0.0, 1.0);


void Ini_Data() {
	if (PRECISION > 1) {
		if (Swap_Log == 0) {
			N_FP = static_cast<int>((FE - FS) / PRECISION);
			N_FP += 1;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = i * PRECISION + FS;
			}
		}
		else {
			N_FP = static_cast<int>((std::log10(FE) - std::log10(FS)) / PRECISION);
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = FS * std::pow(10.0, i * PRECISION);
			}
		}
	}
	else {
		if (Swap_Log == 0) {
			PRECISION = (FE - FS) / N_FP;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = (i + 1) * PRECISION + FS;
			}
		}
		else {
			PRECISION = (std::log10(FE) - std::log10(FS)) / N_FP;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = FS * std::pow(10.0, (i + 1) * PRECISION);
			}
		}
	}
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
	Z0.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	Y0.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	SQRT_Z0.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	SQRT_Y0.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	for (int i = 0; i < N_PORT; ++i) {
		Z0[i][i] = PORT_DATA[i].Zin;
		Y0[i][i] = 1.0 / PORT_DATA[i].Zin;
		SQRT_Z0[i][i] = std::sqrt(Z0[i][i]);
		SQRT_Y0[i][i] = std::sqrt(Y0[i][i]);
	}

	CP_M.assign(M_SZ, std::vector<std::complex<double>>(M_SZ, { 0.0, 0.0 }));
	Source.assign(M_SZ, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));

	Z_TEMP.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	Y_TEMP.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	S_TEMP.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));

	V_TEMP.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	I_TEMP.assign(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));

	if (N_PORT != 0) {
		S_PAR_ABS.assign(N_FP, std::vector<std::vector<double>>(N_PORT, std::vector<double>(N_PORT, 0.0)));
		S_PAR_PHA.assign(N_FP, std::vector<std::vector<double>>(N_PORT, std::vector<double>(N_PORT, 0.0)));
		Z_PT.assign(N_FP, std::vector<std::vector<std::complex<double>>>(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 })));
		Y_PT.assign(N_FP, std::vector<std::vector<std::complex<double>>>(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 })));

		H_W.assign(2 * N_FP + 2, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
		std::fill(H_W[0].begin(), H_W[0].end(), 0.5);
		std::fill(H_W[2 * N_FP + 1].begin(), H_W[2 * N_FP + 1].end(), 0.5);
	}

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
}



void Fill_Branch_L() {
	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < C_PEC_N; ++j) {
			CP_M[i + S_BL[1]][j + S_BL[1]] = IJ * 2.0 * PI * CF[POS] * LL[i][j];

		}
	}
}

void Fill_Node_PP() {
	for (int i = 0; i < PEC_N; ++i) {
		for (int j = 0; j < PEC_N; ++j) {
			CP_M[S_BL[0] + i][S_BL[0] + j] = PP[i][j] * (IJ * CF[POS] * 2.0 * PI);
		}
	}
}

void Fill_Connective() {
	// 第一部分：块赋值
	// Cp_M( S_BL(2) + 1: S_BL(3) + L_BL(3), S_BL(1) + 1:S_BL(1) + L_BL(1) ) = -A_E(1:C_PEC_N + N_PORT, 1:NODE_N )
	for (int i = 0; i < C_PEC_N + N_PORT; ++i) {
		for (int j = 0; j < NODE_N; ++j) {
			CP_M[S_BL[1] + i][S_BL[0] + j] = -A_E[i][j];
		}
	}

	// 第二部分：逐元素赋值
	for (int i = 0; i < NODE_N; ++i) {
		for (int j = 0; j < C_PEC_N + N_PORT; ++j) {
			CP_M[S_BL[0] + i][S_BL[1] + j] = -A_ET[i][j];
		}
	}
}

void Fill_Matrix_Q() {
	// 清零矩阵
	for (int i = 0; i < CP_M.size(); ++i)
		for (int j = 0; j < CP_M[i].size(); ++j)
			CP_M[i][j] = std::complex<double>(0.0, 0.0);

	// 填充各部分
	Fill_Branch_L();
	Fill_Node_PP();
	Fill_Connective();

	// 检查是否有 NaN
	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < M_SZ; ++j) {
			if (std::isnan(std::abs(CP_M[i][j]))) {
				// 可选：调试输出
				std::cout << "NaN detected at (" << i << "," << j << ")" << std::endl;
				system("pause");
			}
		}
	}
}

void Fill_Source() {
	// 清零 Source
	for (auto& row : Source)
		std::fill(row.begin(), row.end(), std::complex<double>(0.0, 0.0));

	// 填充端口激励
	for (int i = 0; i < N_PORT; ++i) {
		Source[S_BL[2] + i][i] = std::complex<double>(1.0, 0.0);
	}
}

void Z_TO_S(const std::vector<std::vector<std::complex<double>>>& Z_TEMP,
	std::vector<std::vector<std::complex<double>>>& S_TEMP,
	int N_PORT)
{
	// 临时变量
	std::vector<std::vector<std::complex<double>>> TEMP_1(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	std::vector<std::vector<std::complex<double>>> TEMP_2(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	std::vector<std::vector<std::complex<double>>> TEMP_3(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));
	std::vector<std::vector<std::complex<double>>> TEMP_4(N_PORT, std::vector<std::complex<double>>(N_PORT, { 0.0, 0.0 }));

	// S_TEMP清零
	for (int i = 0; i < N_PORT; ++i)
		for (int j = 0; j < N_PORT; ++j)
			S_TEMP[i][j] = { 0.0, 0.0 };

	// TEMP_1 = Z_TEMP * SQRT_Y0
	TEMP_1 = Product_M(Z_TEMP, SQRT_Y0);

	// TEMP_2 = SQRT_Y0 * TEMP_1
	TEMP_2 = Product_M(SQRT_Y0, TEMP_1);

	// TEMP_2(i,i) -= 1.0
	for (int i = 0; i < N_PORT; ++i)
		TEMP_2[i][i] -= std::complex<double>(1.0, 0.0);

	// TEMP_3 = Z_TEMP * SQRT_Y0
	TEMP_3 = Product_M(Z_TEMP, SQRT_Y0);

	// TEMP_4 = SQRT_Y0 * TEMP_3
	TEMP_4 = Product_M(SQRT_Y0, TEMP_3);

	// TEMP_4(i,i) += 1.0
	TEMP_4 = Product_M(SQRT_Y0, TEMP_3);

	// TEMP_4(i,i) += 1.0
	for (int i = 0; i < N_PORT; ++i)
		TEMP_4[i][i] += std::complex<double>(1.0, 0.0);

	// TEMP_4逆
	Inverse_M(TEMP_4, 0);

	// S_TEMP = TEMP_2 * TEMP_4
	S_TEMP = Product_M(TEMP_2, TEMP_4);
}
void Solver_Freq_Point() {
	std::vector<std::vector<std::complex<double>>> V_Normalize;
	int mathType = 0;
	// 求解线性方程组
	Solve_M(CP_M, Source, mathType);
	//std::ofstream fout1(MAP_PATH+"VI.txt");
	//fout1 << M_SZ << " " << N_PORT << std::endl;
	//fout1 << std::scientific << std::setprecision(12);
	//for (int i = 0; i < M_SZ; ++i) {
	//    for (int j = 0; j < N_PORT; ++j) {
	//        fout1 << Source[i][j];
	//        if (j < N_PORT - 1) fout1 << " ";
	//    }
	//    fout1 << std::endl;
	//}
	//fout1.close();

	for (int i = 0; i < N_PORT; ++i) {
		for (int j = 0; j < N_PORT; ++j) {
			Y_TEMP[i][j] = Source[S_BL[2] + i][j];
		}
	}

	Z_TEMP = Y_TEMP;

	// 求逆
	Inverse_M(Z_TEMP, 0);

	// 更新 Y
	std::vector<std::vector<std::complex<double>>> Y_Update = Y_TEMP;
	for (int i = 0; i < N_PORT; ++i) {
		Y_Update[i][i] += Y0[i][i];
	}

	// 归一化
	V_Normalize.assign(N_PORT, std::vector<std::complex<double>>(1, { 0.0, 0.0 }));
	V_Normalize[0][0] = Y0[0][0];

	Solve_M(Y_Update, V_Normalize, mathType);

	// 端口为2时，特殊处理
	if (N_PORT == 2) {
		H_W[POS][0] = V_Normalize[0][0];
		H_W[POS][1] = V_Normalize[1][0];
		// 共轭赋值
		H_W[2 * POS][0] = std::conj(H_W[POS][0]);
		H_W[2 * POS][1] = std::conj(H_W[POS][1]);
	}

	// S参数转换
	Z_TO_S(Z_TEMP, S_TEMP, N_PORT);

	if (N_PORT == 2) {
		double err = std::abs(std::abs(S_TEMP[0][0] * S_TEMP[0][0]) + std::abs(S_TEMP[0][1] * S_TEMP[0][1]) - 1) * 100;
		MAX_E = MAX(MAX_E, err);
	}

	// 结果保存
	for (int i = 0; i < N_PORT; ++i) {
		for (int j = 0; j < N_PORT; ++j) {
			S_PAR_ABS[POS][i][j] = ABSS(S_TEMP[i][j]);
			S_PAR_PHA[POS][i][j] = Phase(S_TEMP[i][j]);
			Z_PT[POS][i][j] = Z_TEMP[i][j];
			Y_PT[POS][i][j] = Y_TEMP[i][j];
		}
	}
}

void Fill_Solution(std::ofstream& file_S, std::ofstream& file_Z, std::ofstream& file_Y) {
	if (N_PORT == 1) {
		// S参数
		file_S << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << S_PAR_ABS[POS][0][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[POS][0][0] << std::endl;

		// Z参数
		file_Z << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << Z_PT[POS][0][0].real() << ",  "
			<< std::setw(12) << Z_PT[POS][0][0].imag() << std::endl;

		// Y参数
		file_Y << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << Y_PT[POS][0][0].real() << ",  "
			<< std::setw(12) << Y_PT[POS][0][0].imag() << std::endl;
	}
	else if (N_PORT == 2) {
		// S参数
		file_S << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << S_PAR_ABS[POS][0][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[POS][0][0] << ",  "
			<< std::setw(12) << S_PAR_ABS[POS][0][1] << ",  "
			<< std::setw(12) << S_PAR_PHA[POS][0][1] << ",  "
			<< std::setw(12) << S_PAR_ABS[POS][1][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[POS][1][0] << ",  "
			<< std::setw(12) << S_PAR_ABS[POS][1][1] << ",  "
			<< std::setw(12) << S_PAR_PHA[POS][1][1] << std::endl;

		// Z参数
		file_Z << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << Z_PT[POS][0][0].real() << ",  "
			<< std::setw(12) << Z_PT[POS][0][0].imag() << ",  "
			<< std::setw(12) << Z_PT[POS][0][1].real() << ",  "
			<< std::setw(12) << Z_PT[POS][0][1].imag() << ",  "
			<< std::setw(12) << Z_PT[POS][1][0].real() << ",  "
			<< std::setw(12) << Z_PT[POS][1][0].imag() << ",  "
			<< std::setw(12) << Z_PT[POS][1][1].real() << ",  "
			<< std::setw(12) << Z_PT[POS][1][1].imag() << std::endl;

		// Y参数
		file_Y << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[POS] << ",  "
			<< std::setw(12) << Y_PT[POS][0][0].real() << ",  "
			<< std::setw(12) << Y_PT[POS][0][0].imag() << ",  "
			<< std::setw(12) << Y_PT[POS][0][1].real() << ",  "
			<< std::setw(12) << Y_PT[POS][0][1].imag() << ",  "
			<< std::setw(12) << Y_PT[POS][1][0].real() << ",  "
			<< std::setw(12) << Y_PT[POS][1][0].imag() << ",  "
			<< std::setw(12) << Y_PT[POS][1][1].real() << ",  "
			<< std::setw(12) << Y_PT[POS][1][1].imag() << std::endl;
	}
}


void Freq_Solver() {
	std::cout << "P4: Solving Full  Circuit ..." << std::endl;
	double TIME_S = Get_Time();

	Read_PEEC_Model(MAP_PATH);
	Inverse_M(PP, 0);
	Ini_Data();
	std::cout << "    Matrix size: " << M_SZ << std::endl;
	std::cout << "    Solving matrix..." << std::endl;

	// 打开文件
	std::ofstream file_S(MAP_PATH + "map.txt");
	std::ofstream file_Z(MAP_PATH + "Z_in.txt");
	std::ofstream file_Y(MAP_PATH + "Y_in.txt");

	for (POS = 0; POS < N_FP; ++POS) {
		Fill_Matrix_Q();
		Fill_Source();
		Solver_Freq_Point();
		Fill_Solution(file_S, file_Z, file_Y);
		PRT(POS + 1, N_FP, 1);
	}

	file_S.close();
	file_Z.close();
	file_Y.close();
	std::cout << " Maxmimum error: " << MAX_E << std::endl;
	double TIME_E = Get_Time();
	Time_Diff(TIME_S, TIME_E);
}