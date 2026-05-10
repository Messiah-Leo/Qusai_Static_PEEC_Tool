#include"Mod_Read_File.h"

// 读取所有输入文件的总入口
void Read_File()
{
	int TIME_S, TIME_E;
	Console::Section("P1", "Read Input Files");
	TIME_S = Get_Time();
	Read_Path();
	Read_Setting();
	Read_Dielectric();
	Read_Mesh();
	TIME_E = Get_Time();
	Time_Diff(TIME_S, TIME_E);
}

// 读取网格文件
void Read_Mesh()
{
	int i, j, k;
	std::string line;
	std::ifstream fin(INPUT_FILE);
	if (!fin.is_open()) {
		Console::Error("Failed to open mesh file: " + INPUT_FILE);
		exit(EXIT_FAILURE);
	}
	double VERSION = 0.0;
	int UN_1, UN_2, UN_3;
	NUMBER_OF_TYPE = { 0 };

	while (std::getline(fin, line)) {
		if (line.find("$MeshFormat") != std::string::npos) {
			std::getline(fin, line);
			std::istringstream iss(line);
			iss >> VERSION >> UN_1 >> UN_2;
			// 仅支持Gmsh版本2.2
			if (VERSION != 2.2) {
				Console::Error("Only Gmsh version 2.2 is supported.");
				fin.close();
				exit(EXIT_FAILURE);
			}
			// 跳过 $EndMeshFormat 行
			std::getline(fin, line);
		}
		else if (line.find("$Nodes") != std::string::npos) {
			std::getline(fin, line);
			std::istringstream iss(line);
			iss >> N_P;
			PT.resize(N_P);
			for (i = 0; i < N_P; ++i) {
				std::getline(fin, line);
				std::istringstream iss(line);
				iss >> PT[i].number >> PT[i].X[0] >> PT[i].X[1] >> PT[i].X[2];
				PT[i].X = V_Div(DIM, PT[i].X); // 单位转换可在此处进行
				PT[i].number--; // 转换为0-based索引
			}
			// 跳过 $EndNodes 行
			std::getline(fin, line);
		}
		else if (line.find("$Elements") != std::string::npos) {
			std::getline(fin, line);
			std::istringstream iss(line);
			iss >> T_MS_N;
			T_MS.resize(T_MS_N);
			for (i = 0; i < T_MS_N; ++i) {
				std::getline(fin, line);
				std::istringstream iss(line);
				iss >> k >> T_MS[i].TYP >> UN_3 >> T_MS[i].M_TYP >> T_MS[i].P_TYP;
				int len = TYP_LENGTH[T_MS[i].TYP];
				for (j = 0; j < len; ++j) {
					iss >> T_MS[i].P[j];
					T_MS[i].P[j]--; // 转换为0-based索引
				}
			}
			// 核对读取到的最后一行是否为"$EndElements"
			std::getline(fin, line);
			if (line != "$EndElements") {
				Console::Error("Mesh parsing failed: missing $EndElements.");
				fin.close();
				exit(EXIT_FAILURE);
			}
		}
	}
	fin.close();
	Console::Detail("Mesh points", N_P);
	Console::Detail("Raw elements", T_MS_N);

	//test
	//std::cout << "Mesh reading complete. Total " << N_P << " points and " << N_S << " elements." << std::endl;
}

// 读取设置文件并设置全局参数
void Read_Setting()
{
	std::ifstream fin(SET_FILE);
	if (!fin.is_open()) {
		Console::Warn("Setting file not found, using defaults.");
		return;
	}

	std::string temp_char;
	long double set;
	while (true) {
		std::string line;
		if (!std::getline(fin, line)) break;
		std::istringstream iss(line);
		iss >> temp_char >> set;

		if (temp_char == "$End_Def") break;

		if (temp_char == "Solver_SET") {
			Solver_SET = set;
		}
		else if (temp_char == "FS") {
			FS = set;
		}
		else if (temp_char == "FE") {
			FE = set;
		}
		else if (temp_char == "N_FP") {
			N_FP = set;
		}
		else if (temp_char == "PRECISION") {
			PRECISION = set;
		}
		else if (temp_char == "DIM") {
			DIM = set;
		}
		else if (temp_char == "SAVE_TXT" || temp_char == "SAVE_TEXT" || temp_char == "SAVE_TEXT_MATRIX") {
			SAVE_TXT = (set != 0.0L) ? 1 : 0;
		}
		// 其他case可按需添加
	}
	fin.close();
	LAMDA_E = 0.1 * 3.0e8 / FE;
	Console::Detail("Frequency start", static_cast<double>(FS));
	Console::Detail("Frequency end", static_cast<double>(FE));
	Console::Detail("Dimension scale", static_cast<double>(DIM));
	Console::Detail("Save text matrices", SAVE_TXT ? "yes" : "no");
	//test
	//std::cout << "SETTING file read complete." << std::endl;	
}

// 读取介电常数文件并设置全局 parameters
void Read_Dielectric()
{
	std::ifstream infile(DIELECTRIC_FILE);
	if (!infile.is_open()) {
		Console::Warn("Dielectric file not found.");
		return;
	}

	int D_NUM;
	infile >> D_NUM;

	// 预设为 (1.0, 0.0)
	DIELECTRIC.assign(D_NUM + 1, std::complex<double>(1.0f, 0.0f));

	double DR, DI;
	for (int i = 1; i <= D_NUM; ++i) {
		infile >> DR >> DI;
		DIELECTRIC[i] = std::complex<double>(DR, DI);
	}

	infile.close();
	Console::Detail("Dielectric count", D_NUM);

	//test
	//std::cout << DIELECTRIC_FILE << " read complete. Total " << D_NUM << " dielectric constants." << std::endl;
}

void Read_Path() {
	char buffer[260];
	if (_getcwd(buffer, 260)) {
		Console::Detail("Workspace", buffer);
	}
	else {
		Console::Error("Failed to get current path.");
	}
	// 当前路径字符串
	std::string curr(buffer);


	std::string parent = curr;

	// 找到上一级目录（往上退一层）
	// 打包的时候注释这一段，因为打包后路径结构会改变，直接使用当前路径即可
	//std::size_t pos = parent.find_last_of("\\");
	//if (pos != std::string::npos) {
	//	parent = parent.substr(0, pos);
	//}

	PATH = parent + "\\Data\\";
	INPUT_FILE = PATH + "model.msh";
	SET_FILE = PATH + "set.txt";
	DIELECTRIC_FILE = PATH + "DIELECTRIC.txt";
	MAP_PATH = PATH + "output\\";
	Console::Detail("Data root", PATH);
	Console::Detail("Input mesh", Console::ShortPath(INPUT_FILE));
	Console::Detail("Output dir", Console::ShortPath(MAP_PATH));
}
