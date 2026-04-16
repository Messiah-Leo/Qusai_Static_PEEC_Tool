#include "Mod_circuit.h"
#include "Mod_PEEC_Model.h"
#include <direct.h>


int main()
{
	char buffer[260];
	if (_getcwd(buffer, 260)) {
		std::cout << "Current Path: " << buffer << std::endl;
	}
	else {
		std::cerr << "Fail to get Current Path" << std::endl;
	}
	// 当前路径字符串
	std::string curr(buffer);

	// 找到上一级目录（往上退一层）
	std::string parent = curr;
	std::size_t pos = parent.find_last_of("\\");
	if (pos != std::string::npos) {
		parent = parent.substr(0, pos);
	}
	PATH = parent + "\\Data\\";
	if (!Read_PEEC_Model(PATH)) {
		std::cerr << "Error reading PEEC model." << std::endl;
		return 1;
	}
	for (int i = 0; i < C_PEC_N; i++) {
		CN[i][0] -= 1;
		CN[i][1] -= 1;
	}

	Inverse_M(PP, 0);

	Save_PEEC_Circuit();
	Save_Netlist();
}