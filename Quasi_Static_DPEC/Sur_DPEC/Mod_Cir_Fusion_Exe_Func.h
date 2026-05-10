#pragma once
#ifndef MOD_CIR_FUSION_EXE_FUNC_H
#define MOD_CIR_FUSION_EXE_FUNC_H

extern float BIAS_W;

#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "Mod_Cir_Fusion_data.h"
#include "Mod_Cir_Fusion_Tool_Func.h"
#include "Mod_Compact_Variable.h"
#include "Mod_Function.h"
#include "Mod_MKL_Interface.h"
#include "Mod_Type.h"

bool Read_PEEC_Model(std::string File_Path);
bool Save_PEEC_Model(std::string File_Path);

void Ini_Data_Structure(
	int N_BRANCH,
	int N_NODE,
	const std::vector<std::vector<double>>& LL_00,
	const std::vector<std::vector<double>>& PP_00,
	const std::vector<std::vector<int>>& B2N_IN,
	const std::vector<std::vector<int>>& N2B_IN,
	double W_IN
);

void Re_arrange_Mat(int Node_Num);

void Update_PORT(int Node_Num);

void Update_Cir_Matrix();

void Combine_Node_Circuits();

void Combine_Branch_Circuits();

void Find_Insig_Node(int& Node_Num, double& Node_er);

void Reconfig_Circuit_Model();

void Fused_Circuit_Model(
	std::vector<std::vector<double>>& LL_00,
	std::vector<std::vector<double>>& PP_00,
	int& N_BRANCH,
	int& N_NODE,
	std::vector<std::vector<int>>& B2N_OUT,
	std::vector<Port>& PORT_DATA,
	int N_port
);

template<typename T>
bool Read_Matrix_From_File(
	const std::string& filename,
	std::vector<std::vector<T>>& matrix
)
{
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << filename << std::endl;
		system("pause");
		return false;
	}

	int rows = 0, cols = 0;
	file >> rows >> cols;

	if (rows <= 0 || cols <= 0) {
		std::cerr << "无效的矩阵尺寸: " << rows << "x" << cols << std::endl;
		system("pause");
		return false;
	}

	matrix.clear();
	matrix.resize(rows, std::vector<T>(cols));

	// 读取矩阵数据
	for (int i = 0; i < rows; ++i) {
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
				system("pause");
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
		system("pause");
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
			system("pause");
			return false;
		}
	}

	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "无法打开文件用于写入: " << filename << std::endl;
		system("pause");
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


inline bool File_Exists(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary);
	return file.good();
}

template<typename T>
bool Read_Matrix_From_Binary_File(
	const std::string& filename,
	std::vector<std::vector<T>>& matrix
)
{
	static_assert(std::is_trivially_copyable<T>::value, "Binary matrix input requires trivially copyable element types.");

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Cannot open binary matrix file: " << filename << std::endl;
		return false;
	}

	char magic[8] = {};
	file.read(magic, sizeof(magic));
	const char expected[8] = { 'P', 'E', 'E', 'C', 'M', 'A', 'T', '1' };
	for (int i = 0; i < 8; ++i) {
		if (magic[i] != expected[i]) {
			std::cerr << "Invalid binary matrix header: " << filename << std::endl;
			return false;
		}
	}

	std::uint64_t row_count = 0;
	std::uint64_t col_count = 0;
	file.read(reinterpret_cast<char*>(&row_count), sizeof(row_count));
	file.read(reinterpret_cast<char*>(&col_count), sizeof(col_count));

	if (!file.good() || row_count == 0 || col_count == 0 ||
		row_count > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
		col_count > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
		std::cerr << "Invalid binary matrix shape: " << row_count << "x" << col_count << std::endl;
		return false;
	}

	const std::size_t rows = static_cast<std::size_t>(row_count);
	const std::size_t cols = static_cast<std::size_t>(col_count);
	matrix.assign(rows, std::vector<T>(cols));

	for (std::size_t i = 0; i < rows; ++i) {
		file.read(
			reinterpret_cast<char*>(matrix[i].data()),
			static_cast<std::streamsize>(cols * sizeof(T))
		);
		if (!file.good()) {
			std::cerr << "Failed to read binary matrix data: " << filename << std::endl;
			return false;
		}
	}

	std::cout << "Binary matrix loaded: " << filename << std::endl;
	std::cout << "Matrix shape: " << rows << "x" << cols << std::endl;
	return true;
}

template<typename T>
bool Write_Matrix_To_Binary_File(
	const std::string& filename,
	const std::vector<std::vector<T>>& matrix
)
{
	static_assert(std::is_trivially_copyable<T>::value, "Binary matrix output requires trivially copyable element types.");

	if (matrix.empty()) {
		std::cerr << "Matrix is empty and cannot be written: " << filename << std::endl;
		return false;
	}

	const std::size_t rows = matrix.size();
	const std::size_t cols = matrix[0].size();
	if (cols == 0) {
		std::cerr << "Matrix has zero columns and cannot be written: " << filename << std::endl;
		return false;
	}

	for (std::size_t i = 1; i < rows; ++i) {
		if (matrix[i].size() != cols) {
			std::cerr << "Irregular matrix row " << i << " while writing: " << filename << std::endl;
			return false;
		}
	}

	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Cannot open binary matrix file for writing: " << filename << std::endl;
		return false;
	}

	const char magic[8] = { 'P', 'E', 'E', 'C', 'M', 'A', 'T', '1' };
	const std::uint64_t row_count = static_cast<std::uint64_t>(rows);
	const std::uint64_t col_count = static_cast<std::uint64_t>(cols);
	file.write(magic, sizeof(magic));
	file.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));
	file.write(reinterpret_cast<const char*>(&col_count), sizeof(col_count));

	for (std::size_t i = 0; i < rows; ++i) {
		file.write(
			reinterpret_cast<const char*>(matrix[i].data()),
			static_cast<std::streamsize>(cols * sizeof(T))
		);
	}

	if (!file.good()) {
		std::cerr << "Failed to write binary matrix data: " << filename << std::endl;
		return false;
	}

	std::cout << "Binary matrix written: " << filename << std::endl;
	std::cout << "Matrix shape: " << rows << "x" << cols << std::endl;
	return true;
}

#endif