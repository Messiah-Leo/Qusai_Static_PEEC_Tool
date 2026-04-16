#ifndef MOD_PEEC_MODEL_H
#define MOD_PEEC_MODEL_H

#pragma once

#include "Mod_Common_Data.h"
#include "Mod_Function.h"
#include "Mod_MKL_Interface.h"
#include "Mod_Type.h"
#include <array>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

bool Read_PEEC_Model(std::string File_Path);

template<typename T>
bool Read_Matrix_From_File(
	const std::string& filename,
	std::vector<std::vector<T>>& matrix
)
{
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << filename << std::endl;
		return false;
	}

	int rows = 0, cols = 0;
	file >> rows >> cols;

	if (rows <= 0 || cols <= 0) {
		std::cerr << "无效的矩阵尺寸: " << rows << "x" << cols << std::endl;
		return false;
	}

	matrix.clear();
	matrix.resize(rows, std::vector<T>(cols));

	// 读取矩阵数据
	for (int i = 0; i < rows; ++i) {
		std::cout << "Reading row " << i + 1 << " / " << rows << "\r";
		for (int j = 0; j < cols; ++j) {
			std::string token;
			file >> token;

			T value = T(0);
			auto result = std::from_chars(token.data(),
				token.data() + token.size(),
				value);

			if (result.ec != std::errc()) {
				std::cerr << "转换错误: " << token
					<< " 在第 " << i + 1 << " 行，第 " << j + 1 << " 列"
					<< std::endl;
				return false;
			}

			matrix[i][j] = value;
		}
	}

	file.close();

	//// 输出矩阵信息（前几行验证）
	//std::cout << "矩阵尺寸: " << rows << "x" << cols << std::endl;
	//std::cout << "前3行前3列数据:" << std::endl;
	//for (int i = 0; i < std::min(3, rows); ++i) {
	//    for (int j = 0; j < std::min(3, cols); ++j) {
	//        std::cout << matrix[i][j] << " ";
	//    }
	//    std::cout << std::endl;
	//}
	std::cout << "矩阵已读取: " << filename << std::endl;
	std::cout << "矩阵尺寸: " << rows << "x" << cols << std::endl;

	return true;
}
#endif