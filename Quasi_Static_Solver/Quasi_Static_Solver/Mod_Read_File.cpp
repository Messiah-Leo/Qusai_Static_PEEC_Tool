#include"Mod_Read_File.h"

#include <windows.h>

namespace {

std::string PathWithTrailingSlash(std::string result) {
	if (!result.empty() && result.back() != '\\' && result.back() != '/') {
		result += "\\";
	}
	return result;
}

std::string ParentPath(std::string path) {
	while (!path.empty() && (path.back() == '\\' || path.back() == '/')) {
		path.pop_back();
	}
	const size_t pos = path.find_last_of("\\/");
	return pos == std::string::npos ? path : path.substr(0, pos);
}

std::string JoinPath(const std::string& base, const std::string& child) {
	if (base.empty()) {
		return child;
	}
	if (base.back() == '\\' || base.back() == '/') {
		return base + child;
	}
	return base + "\\" + child;
}

std::string AbsolutePath(const std::string& path) {
	char buffer[MAX_PATH];
	const DWORD length = GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr);
	if (length == 0 || length >= MAX_PATH) {
		return path;
	}
	return buffer;
}

bool DirectoryExists(const std::string& path) {
	const DWORD attributes = GetFileAttributesA(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::string GetExecutableDir() {
	char buffer[MAX_PATH];
	const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	if (length == 0 || length == MAX_PATH) {
		char cwd[MAX_PATH];
		return _getcwd(cwd, MAX_PATH) ? std::string(cwd) : std::string(".");
	}
	return ParentPath(buffer);
}

std::string ResolveDataRoot(const std::string& executable_dir, const std::string& current_dir) {
	const std::string exe_parent = ParentPath(executable_dir);
	const std::string cwd_parent = ParentPath(current_dir);
	const std::string candidates[] = {
		JoinPath(executable_dir, "Data"),
		JoinPath(current_dir, "Data"),
		JoinPath(exe_parent, "Input"),
		JoinPath(cwd_parent, "Input"),
		JoinPath(ParentPath(exe_parent), "Data"),
		JoinPath(ParentPath(cwd_parent), "Data"),
	};

	for (const auto& candidate : candidates) {
		if (DirectoryExists(candidate)) {
			return AbsolutePath(candidate);
		}
	}

	return AbsolutePath(JoinPath(executable_dir, "Data"));
}

void UseDefaultDielectric() {
	DIELECTRIC.assign(128, std::complex<double>(1.0, 0.0));
	Console::Warn("Dielectric file not found, using air dielectric defaults.");
}

} // namespace


void Read_File()
{
	int TIME_S, TIME_E;
	Console::Section("P1", "Read Input Files");
	TIME_S = Get_Time();
	Read_Path();
	Read_Setting();
	Read_Dielectric();
	//Read_Mesh();
	TIME_E = Get_Time();
	Time_Diff(TIME_S, TIME_E);
}


//void Read_Mesh()
//{
//	int i, j, k;
//	std::string line;
//	std::ifstream fin(INPUT_FILE);
//	if (!fin.is_open()) {
//		throw std::runtime_error("Failed to open mesh file: " + INPUT_FILE);
//	}
//	double VERSION = 0.0;
//	int UN_1, UN_2, UN_3;
//	NUMBER_OF_TYPE = { 0 };
//
//	while (std::getline(fin, line)) {
//		if (line.find("$MeshFormat") != std::string::npos) {
//			std::getline(fin, line);
//			std::istringstream iss(line);
//			iss >> VERSION >> UN_1 >> UN_2;

//			if (VERSION != 2.2) {
//				fin.close();
//				throw std::runtime_error("Unsupported Gmsh version in mesh file. Only version 2.2 is supported.");
//			}

//			std::getline(fin, line);
//		}
//		else if (line.find("$Nodes") != std::string::npos) {
//			std::getline(fin, line);
//			std::istringstream iss(line);
//			iss >> N_P;
//			PT.resize(N_P);
//			for (i = 0; i < N_P; ++i) {
//				std::getline(fin, line);
//				std::istringstream iss(line);
//				iss >> PT[i].number >> PT[i].X[0] >> PT[i].X[1] >> PT[i].X[2];


//			}

//			std::getline(fin, line);
//		}
//		else if (line.find("$Elements") != std::string::npos) {
//			std::getline(fin, line);
//			std::istringstream iss(line);
//			iss >> T_MS_N;
//			T_MS.resize(T_MS_N);
//			for (i = 0; i < T_MS_N; ++i) {
//				std::getline(fin, line);
//				std::istringstream iss(line);
//				iss >> k >> T_MS[i].TYP >> UN_3 >> T_MS[i].M_TYP >> T_MS[i].P_TYP;
//				int len = TYP_LENGTH[T_MS[i].TYP];
//				for (j = 0; j < len; ++j) {
//					iss >> T_MS[i].P[j];

//				}
//			}

//			std::getline(fin, line);
//			if (line != "$EndElements") {
//				fin.close();
//				throw std::runtime_error("Mesh file format error: missing $EndElements.");
//			}
//		}
//	}
//	fin.close();
//
//	//test
//	//std::cout << "Mesh reading complete. Total " << N_P << " points and " << N_S << " elements." << std::endl;
//}


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
		else if (temp_char == "POST_PROCESSING") {
			POST_PROCESSING = (set != 0.0L) ? 1 : 0;
		}
		else if (temp_char == "POST_FREQ_INDEX") {
			POST_FREQ_INDEX = static_cast<int>(set);
		}
		else if (temp_char == "POST_PORT") {
			POST_PORT = static_cast<int>(set);
		}

	}
	fin.close();
	LAMDA_E = 0.6 * 3.0e8 / 15e8;
	Console::Detail("Solver mode", Solver_SET == 0 ? "frequency" : (Solver_SET == 1 ? "time" : "unknown"));
	Console::Detail("Frequency start", static_cast<double>(FS));
	Console::Detail("Frequency end", static_cast<double>(FE));
	Console::Detail("Dimension scale", static_cast<double>(DIM));
	Console::Detail("Export post source", POST_PROCESSING ? "yes" : "no");
	//test
	//std::cout << "SETTING file read complete." << std::endl;	
}


void Read_Dielectric()
{
	std::ifstream infile(DIELECTRIC_FILE);
	if (!infile.is_open()) {
		UseDefaultDielectric();
		return;
	}

	int D_NUM;
	infile >> D_NUM;


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
		throw std::runtime_error("Failed to get current working directory.");
	}
	const std::string current_dir(buffer);
	const std::string executable_dir = GetExecutableDir();
	const std::string data_root = ResolveDataRoot(executable_dir, current_dir);

	PATH = PathWithTrailingSlash(data_root);
	INPUT_FILE = PATH + "model.msh";
	SET_FILE = PATH + "set.txt";
	DIELECTRIC_FILE = PATH + "DIELECTRIC.txt";
	MAP_PATH = PATH + "COMMON_DATA\\";
	SOURCE_PATH = PATH;
	Console::Detail("Data root", PATH);
	//Console::Detail("Model dir", Console::ShortPath(MAP_PATH));
	//Console::Detail("Source dir", Console::ShortPath(SOURCE_PATH));
}

