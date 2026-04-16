#ifndef MOD_FUNCTION_H
#define MOD_FUNCTION_H

#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>  
#include <iostream>
#include <thread>
#include <vector>
#include <fstream>

#include "Mod_Common_Data.h"
#include "Mod_Type.h"

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

double ABSS(std::complex<double> s);  //计算复数的幅度并以db为单位返回
double Phase(std::complex<double> z); //计算复数的相位角

int TRANS_IN_LU(int IND_A, int IND_B, int SZ_A);
int TRANS_IN_SQ(int IND_A, int IND_B, int SZ_A);

void PRT(int i, int T, int A);

double V_Dot(const Vector& V1, const Vector& V2);  //两个三维向量点乘

Vector V_Add(const Vector& V1, const Vector& V2); //两个三维向量相加

Vector V_Sub(const Vector& V1, const Vector& V2); //两个三维向量相减

Vector V_Mul(const double s, const Vector& V1); //三维向量乘法

Vector V_Div(const double s, const Vector& V1); //三维向量除法

Vector V_Cross(const Vector& V1, const Vector& V2);  //叉乘

double  Trian_Area(const Vector& P_A, const Vector& P_B, const Vector& P_C);  //计算三角形面积

double Distance(const Vector& V1, const Vector& V2);  //计算两个三维点的欧几里得距离

Vector Oth_S(std::array<Point, 2>& CP, Point& UP);  //计算两个点连线的正交向量

Vector Othognal_Vector(std::array<Point, 3>& CP, Point& UP); //面法向量

double Tetrahedral_Volum(std::array<Point, 4>& V);  //计算四面体体积,要改

int P_In_S(Point& P, std::array<Point, 3> MS);  //判断点是否在三角形内

//读取机器时间
double Get_Time();

//计算并打印时间差
void Time_Diff(double T_S, double T_E);

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

bool Read_PEEC_Model(std::string File_Path);

#endif // 

