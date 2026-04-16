#ifndef MOD_SOLVER_TIME
#define MOD_SOLVER_TIME

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

// 激励类型定义
enum SourceType {
    SRC_STEP = 0,
    SRC_PRBS = 1
};

// 激励配置结构体
struct SourceConfig {
    SourceType type;      // 信号类型 (0: Step, 1: PRBS)
    double amplitude;     // 幅度
    double rise_time;     // 上升时间 (s)
    double bit_rate;      // 比特率 (bps)
    int prbs_N;           // PRBS阶数
    double total_time;    // 总仿真时长 (s)
    int samples_per_bit;  // 每个比特的采样点数
};

void Time_Solver();

// 新增功能函数
bool Read_Source_Config(const std::string& configPath, SourceConfig& config);
bool Generate_Source_File(const SourceConfig& config, const std::string& outputPath);
bool Read_Signal_File(std::string File_Path);

#endif