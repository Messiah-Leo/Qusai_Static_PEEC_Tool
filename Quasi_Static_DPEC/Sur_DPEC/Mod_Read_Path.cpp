#include <cstring>
#include <direct.h>
#include <filesystem>
#include <iostream>
#include <string>

#include "Mod_Read_Path.h"

// Read all input files - entry point
void Read_File()
{
	int TIME_S, TIME_E;
	std::cout << "P1: Reading Path..." << std::endl;
	TIME_S = Get_Time();
	Read_Path();
	Read_Setting();
	TIME_E = Get_Time();
	Time_Diff(TIME_S, TIME_E);
}

void Read_Path() {
	char buffer[260];
	if (_getcwd(buffer, 260)) {
		std::cout << "Current Path: " << buffer << std::endl;
	}
	else {
		std::cerr << "Fail to get Current Path" << std::endl;
		return;
	}

	const std::filesystem::path cwd(buffer);
	const std::filesystem::path packaged_data = cwd / "Data";
	const std::filesystem::path source_layout_data = cwd.parent_path() / "Data";
	const std::filesystem::path data_root =
		std::filesystem::exists(packaged_data) ? packaged_data : source_layout_data;

	PATH = data_root.string() + "\\";
	FUSION_SET_FILE = PATH + "MOR_SET.txt";
}

void Read_Setting()
{
	std::ifstream fin(FUSION_SET_FILE);
	if (!fin.is_open()) {
		std::cout << "Fail to read the input SETTING file!, Use default Setting." << std::endl;
		return;
	}

	while (true) {
		std::string line;
		if (!std::getline(fin, line)) break;
		std::istringstream iss(line);
		std::string temp_char;
		std::string value;
		iss >> temp_char >> value;

		if (temp_char == "$End_Def") break;
		if (temp_char.empty() || temp_char[0] == '#' || value.empty()) continue;
		if (temp_char == "MOR_METHOD") continue;

		const long double set = std::stold(value);

		if (temp_char == "MAX_ERROR") {
			MAX_ERROR = set;
		}
		else if (temp_char == "MAX_FREQ") {
			MAX_FREQ = set;
		}
		else if (temp_char == "MAX_NODE") {
			MAX_NODE = static_cast<int>(set);
		}
	}
	fin.close();
}
