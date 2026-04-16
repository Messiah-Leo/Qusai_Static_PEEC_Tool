#ifndef MOD_ELEMENT_BUILD_H
#define MOD_ELEMENT_BUILD_H

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include"Mod_Common_Data.h"
#include "Mod_Function.h"
#include "Mod_Gaussian.h"
#include "Mod_Integral_By_Mesh.h"
#include "Mod_Save_Circuit.h"
#include "Mod_Type.h"

// 主调度函数
void Element_Build();

// 内部实现函数
void Ini_Element_Sz();
void Build_MS_Ele();
void Re_Construct_Ele();
void Construct_LL_PP();
bool Save_PEEC_Model();
//void Built_Branch_L();
//void Built_Node_P();

template<typename T>
bool Write_Matrix_To_File(
	const std::string& filename,
	const std::vector<std::vector<T>>& matrix,
	bool write_dimensions = true,
	int precision = 6
)
{
	if (matrix.empty()) {
		std::cerr << "矩阵为空，无法写入文件" << std::endl;
		return false;
	}

	// 检查矩阵是否规则（所有行长度相同）
	size_t rows = matrix.size();
	size_t cols = matrix[0].size();

	for (size_t i = 1; i < rows; ++i) {
		if (matrix[i].size() != cols) {
			std::cerr << "矩阵不规则，第" << i << "行有"
				<< matrix[i].size() << "列，但第一行有"
				<< cols << "列" << std::endl;
			return false;
		}
	}

	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "无法打开文件用于写入: " << filename << std::endl;
		return false;
	}

	// 设置输出精度（对浮点数有效）
	file << std::setprecision(precision);

	// 可选：写入矩阵尺寸
	if (write_dimensions) {
		file << rows << " " << cols << std::endl;
	}

	// 写入矩阵数据
	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			file << matrix[i][j];
			if (j < cols - 1) {
				file << " ";  // 列间用空格分隔
			}
		}
		file << std::endl;  // 每行后换行
	}

	file.close();

	std::cout << "矩阵已写入文件: " << filename << std::endl;
	std::cout << "矩阵尺寸: " << rows << "x" << cols << std::endl;

	return true;
}


#endif // MOD_ELE_BUILT_H