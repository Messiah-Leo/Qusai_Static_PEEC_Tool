#include <cstring>
#include <direct.h>
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
		system("pause");
		exit;
	}
	// current path string
	std::string curr(buffer);

	//find parent directory
	std::string parent = curr;

	//打包的时候注释这一段，避免路径问题
	std::size_t pos = parent.find_last_of("\\");
	if (pos != std::string::npos) {
		parent = parent.substr(0, pos);
	}

	PATH = parent + "\\Data\\";
	FUSION_SET_FILE = PATH + "Fusion_Set.txt";
}

void Read_Setting()
{
	std::ifstream fin(FUSION_SET_FILE);
	if (!fin.is_open()) {
		std::cout << "Fail to read the input SETTING file!, Use default Setting." << std::endl;
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

		if (temp_char == "MAX_ERROR") {
			MAX_ERROR = set;
		}
		else if (temp_char == "MAX_FREQ") {
			MAX_FREQ = set;
		}
		else if (temp_char == "MAX_NODE") {
			MAX_NODE = set;
		}
		// 其他case可按需添加
	}
	fin.close();
	//test
	//std::cout << "SETTING file read complete." << std::endl;	
}