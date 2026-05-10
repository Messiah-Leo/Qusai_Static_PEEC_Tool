#ifndef MOD_ELEMENT_BUILD_H
#define MOD_ELEMENT_BUILD_H

#pragma once

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "Mod_Common_Data.h"
#include "Mod_Function.h"
#include "Mod_Gaussian.h"
#include "Mod_Integral_By_Mesh.h"
#include "Mod_Save_Circuit.h"
#include "Mod_Type.h"

void Element_Build();

void Ini_Element_Sz();
void Build_MS_Ele();
void Re_Construct_Ele();
void Construct_LL_PP();
bool Save_PEEC_Model();

template<typename T>
bool Validate_Matrix_Shape(
	const std::vector<std::vector<T>>& matrix,
	size_t& rows,
	size_t& cols
)
{
	if (matrix.empty()) {
		Console::Error("Matrix is empty and cannot be written.");
		return false;
	}

	rows = matrix.size();
	cols = matrix[0].size();

	for (size_t i = 1; i < rows; ++i) {
		if (matrix[i].size() != cols) {
			Console::Error(
				"Irregular matrix row " + std::to_string(i) +
				": expected " + std::to_string(cols) +
				" columns, got " + std::to_string(matrix[i].size()) + ".");
			return false;
		}
	}

	return true;
}

inline std::string Matrix_Save_Progress_Label(const std::string& filename)
{
	const size_t name_start = filename.find_last_of("\\/");
	const std::string name = (name_start == std::string::npos)
		? filename
		: filename.substr(name_start + 1);
	return "Save " + name;
}

template<typename T>
bool Write_Matrix_To_File(
	const std::string& filename,
	const std::vector<std::vector<T>>& matrix,
	bool write_dimensions = true,
	int precision = std::numeric_limits<double>::max_digits10
)
{
	size_t rows = 0;
	size_t cols = 0;
	if (!Validate_Matrix_Shape(matrix, rows, cols)) {
		return false;
	}

	std::ofstream file(filename);
	if (!file.is_open()) {
		Console::Error("Cannot open file for writing: " + Console::ShortPath(filename));
		return false;
	}

	if constexpr (std::is_floating_point_v<T>) {
		file << std::scientific << std::setprecision(precision);
	}
	else {
		file << std::setprecision(precision);
	}

	if (write_dimensions) {
		file << rows << " " << cols << '\n';
	}

	const std::string progress_label = Matrix_Save_Progress_Label(filename);
	size_t last_percent = 0;
	Console::Progress(progress_label, static_cast<size_t>(0), rows);

	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			file << matrix[i][j];
			if (j + 1 < cols) {
				file << ' ';
			}
		}
		file << '\n';

		const size_t done = i + 1;
		const size_t percent = static_cast<size_t>(100.0 * done / rows);
		if (percent != last_percent || done == rows) {
			Console::Progress(progress_label, done, rows);
			last_percent = percent;
		}
	}

	if (!file.good()) {
		Console::Error("Failed to write text matrix: " + Console::ShortPath(filename));
		return false;
	}

	Console::Info("Saved text matrix: " + Console::ShortPath(filename));
	Console::Detail("Matrix shape", std::to_string(rows) + "x" + std::to_string(cols));

	return true;
}

template<typename T>
bool Write_Matrix_To_Binary_File(
	const std::string& filename,
	const std::vector<std::vector<T>>& matrix
)
{
	static_assert(std::is_trivially_copyable_v<T>, "Binary matrix output requires trivially copyable element types.");

	size_t rows = 0;
	size_t cols = 0;
	if (!Validate_Matrix_Shape(matrix, rows, cols)) {
		return false;
	}

	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		Console::Error("Cannot open binary file for writing: " + Console::ShortPath(filename));
		return false;
	}

	const char magic[8] = { 'P', 'E', 'E', 'C', 'M', 'A', 'T', '1' };
	const std::uint64_t row_count = static_cast<std::uint64_t>(rows);
	const std::uint64_t col_count = static_cast<std::uint64_t>(cols);

	file.write(magic, sizeof(magic));
	file.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));
	file.write(reinterpret_cast<const char*>(&col_count), sizeof(col_count));

	const std::string progress_label = Matrix_Save_Progress_Label(filename);
	size_t last_percent = 0;
	Console::Progress(progress_label, static_cast<size_t>(0), rows);

	for (size_t i = 0; i < rows; ++i) {
		file.write(reinterpret_cast<const char*>(matrix[i].data()), static_cast<std::streamsize>(cols * sizeof(T)));

		const size_t done = i + 1;
		const size_t percent = static_cast<size_t>(100.0 * done / rows);
		if (percent != last_percent || done == rows) {
			Console::Progress(progress_label, done, rows);
			last_percent = percent;
		}
	}

	if (!file.good()) {
		Console::Error("Failed to write binary matrix: " + Console::ShortPath(filename));
		return false;
	}

	Console::Info("Saved binary matrix: " + Console::ShortPath(filename));
	Console::Detail("Matrix shape", std::to_string(rows) + "x" + std::to_string(cols));

	return true;
}

#endif // MOD_ELEMENT_BUILD_H

