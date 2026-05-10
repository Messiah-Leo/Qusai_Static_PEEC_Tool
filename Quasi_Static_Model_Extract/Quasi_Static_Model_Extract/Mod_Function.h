#ifndef MOD_FUNCTION_H
#define MOD_FUNCTION_H

#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>  
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Mod_Common_Data.h"
#include "Mod_Type.h"

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

namespace Console {

inline std::string FormatDurationMs(double ms) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(1) << ms << " ms";
	return oss.str();
}

inline std::string ShortPath(const std::string& full_path) {
	const std::string prefixes[] = { MAP_PATH, PATH };
	for (const auto& prefix : prefixes) {
		if (!prefix.empty() && full_path.rfind(prefix, 0) == 0) {
			return full_path.substr(prefix.size());
		}
	}
	return full_path;
}

template<typename T>
inline std::string ToString(const T& value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

inline void Blank() {
	std::cout << std::endl;
}

inline void Section(const std::string& step, const std::string& title) {
	std::cout << "\n[" << step << "] " << title << std::endl;
}

inline void Info(const std::string& message) {
	std::cout << "  [INFO] " << message << std::endl;
}

inline void Detail(const std::string& key, const std::string& value) {
	std::cout << "  - " << std::left << std::setw(18) << key << ": " << value << std::endl;
}

template<typename T>
inline void Detail(const std::string& key, const T& value) {
	Detail(key, ToString(value));
}

inline void Warn(const std::string& message) {
	std::cout << "  [WARN] " << message << std::endl;
}

inline void Error(const std::string& message) {
	std::cerr << "  [ERROR] " << message << std::endl;
}

inline void Progress(const std::string& label, size_t done, size_t total) {
	const size_t safe_total = (total == 0) ? 1 : total;
	const size_t clamped_done = (done > safe_total) ? safe_total : done;
	const int percent = static_cast<int>(100.0 * clamped_done / safe_total);
	const int bar_width = 24;
	const int filled = percent * bar_width / 100;

	std::cout << "\r  [RUN ] " << std::left << std::setw(20) << label << " [";
	for (int i = 0; i < bar_width; ++i) {
		std::cout << (i < filled ? '#' : '-');
	}
	std::cout << "] " << std::right << std::setw(3) << percent << "% (" << clamped_done << "/" << safe_total << ")" << std::flush;

	if (clamped_done >= safe_total) {
		std::cout << std::endl;
	}
}

inline void Progress(const std::string& label, int done, int total) {
	const int safe_done = (done < 0) ? 0 : done;
	const int safe_total = (total <= 0) ? 1 : total;
	Progress(label, static_cast<size_t>(safe_done), static_cast<size_t>(safe_total));
}

} // namespace Console

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

#endif // 


