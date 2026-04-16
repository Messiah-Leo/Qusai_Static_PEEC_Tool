#include "Mod_PEEC_Model.h"

bool Read_PEEC_Model(std::string File_Path)
{
	if (!Read_Matrix_From_File(File_Path + "LL_OO.txt", LL)) return false;
	std::cout << "Read LL_OO.txt success" << std::endl;
	if (!Read_Matrix_From_File(File_Path + "PP_OO.txt", PP)) return false;
	std::cout << "Read PP_OO.txt success" << std::endl;
	if (!Read_Matrix_From_File(File_Path + "B2N.txt", CN)) return false;


	PEC_N = PP.size();
	C_PEC_N = LL.size();

	std::ifstream fin(File_Path + "PORT.txt");
	if (!fin.is_open()) {
		std::cerr << "无法打开文件: " << MAP_PATH + "PORT.txt" << std::endl;
		return false;
	}

	// 读取第一行，获取端口数量
	std::string line;
	if (!std::getline(fin, line)) {
		std::cerr << "文件为空或格式错误" << std::endl;
		return false;
	}
	// 解析端口数量
	std::stringstream ss(line);
	if (!(ss >> N_PORT)) {
		std::cerr << "无法解析端口数量" << std::endl;
		return false;
	}

	// 清空并预留空间
	PORT_DATA.clear();
	PORT_DATA.resize(N_PORT);

	// 读取每个端口的数据
	for (int i = 0; i < N_PORT; ++i) {
		if (!std::getline(fin, line)) {
			std::cerr << "文件行数不足，期望 " << N_PORT << " 行，但只有 " << i << " 行" << std::endl;
			return false;
		}

		std::stringstream line_ss(line);
		if (!(line_ss >> PORT_DATA[i].N[0] >> PORT_DATA[i].N[1] >> PORT_DATA[i].Zin)) {
			std::cerr << "第 " << i + 1 << " 行数据格式错误" << std::endl;
			return false;
		}
	}

	fin.close();
	std::cout << "成功读取 " << N_PORT << " 个端口数据" << std::endl;
	return true;
}