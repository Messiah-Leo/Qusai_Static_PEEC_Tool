#include "Mod_Solver_Freq.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#define NOMINMAX
#include <windows.h>

namespace {

using Complex = std::complex<double>;
using ComplexMatrix = std::vector<std::vector<Complex>>;

ComplexMatrix Z0;
ComplexMatrix Y0;
ComplexMatrix SQRT_Z0;
ComplexMatrix SQRT_Y0;
std::vector<std::vector<Complex>> H_W;

double MAX_E = 0.0;
const Complex IJ(0.0, 1.0);

struct FreqSolveWorkspace {
	ComplexMatrix cp_m;
	ComplexMatrix source;
	ComplexMatrix z_temp;
	ComplexMatrix y_temp;
	ComplexMatrix s_temp;
};

struct FreqPointAuxData {
	std::vector<Complex> normalized_voltage;
	std::vector<Complex> post_current;
	std::vector<Complex> post_voltage;
	double error = 0.0;
};

struct WorkerPlan {
	int worker_count = 1;
	int cpu_limit = 1;
	std::uint64_t total_physical_bytes = 0;
	std::uint64_t available_physical_bytes = 0;
	std::uint64_t memory_budget_bytes = 0;
	std::uint64_t per_worker_peak_bytes = 0;
	bool memory_info_ok = false;
	bool limited_by_memory = false;
};

constexpr double kMemoryUsageLimitRatio = 0.90;
constexpr double kWorkerSafetyMarginRatio = 1.20;

int ResolvePostFrequencyPosition() {
	if (N_FP <= 0) {
		return -1;
	}
	if (POST_FREQ_INDEX <= 0) {
		return N_FP - 1;
	}
	return std::min(POST_FREQ_INDEX - 1, N_FP - 1);
}

int ResolvePostPortPosition() {
	if (N_PORT <= 0) {
		return -1;
	}
	return std::clamp(POST_PORT - 1, 0, N_PORT - 1);
}

bool SavePostProcessingSource(const std::vector<FreqPointAuxData>& aux_data) {
	if (POST_PROCESSING == 0) {
		return true;
	}

	const int freq_pos = ResolvePostFrequencyPosition();
	const int port_pos = ResolvePostPortPosition();
	if (freq_pos < 0 || port_pos < 0 || freq_pos >= static_cast<int>(aux_data.size())) {
		Console::Warn("Post-processing source export skipped: no solved frequency point or excitation port.");
		return true;
	}

	const FreqPointAuxData& aux = aux_data[freq_pos];
	if (aux.post_current.size() != static_cast<size_t>(C_PEC_N) ||
		aux.post_voltage.size() != static_cast<size_t>(PEC_N)) {
		Console::Error("Post-processing source snapshot is incomplete.");
		return false;
	}

	const std::filesystem::path output_path = std::filesystem::path(MAP_PATH) / "Post_Source.txt";
	std::ofstream fout(output_path);
	if (!fout.is_open()) {
		Console::Error("Cannot open post-processing source file for writing.");
		return false;
	}

	fout << std::scientific << std::setprecision(std::numeric_limits<double>::max_digits10);
	fout << "PEEC_POST_SOURCE_V1\n";
	fout << "FREQUENCY " << CF[freq_pos] << '\n';
	fout << "PORT " << (port_pos + 1) << '\n';
	fout << "CURRENTS " << aux.post_current.size() << '\n';
	for (const Complex& value : aux.post_current) {
		fout << value.real() << ' ' << value.imag() << '\n';
	}
	fout << "VOLTAGES " << aux.post_voltage.size() << '\n';
	for (const Complex& value : aux.post_voltage) {
		fout << value.real() << ' ' << value.imag() << '\n';
	}
	fout << "MAGNETIC_CURRENTS " << aux.post_current.size() << '\n';
	for (size_t i = 0; i < aux.post_current.size(); ++i) {
		fout << "0 0\n";
	}
	fout << "MAGNETIC_VOLTAGES " << aux.post_voltage.size() << '\n';
	for (size_t i = 0; i < aux.post_voltage.size(); ++i) {
		fout << "0 0\n";
	}

	if (!fout.good()) {
		Console::Error("Failed to write post-processing source file.");
		return false;
	}

	Console::Info("Saved post-processing source: " + output_path.string());
	Console::Detail("Post frequency", CF[freq_pos]);
	Console::Detail("Post excitation port", port_pos + 1);
	return true;
}

std::uint64_t SafeMultiply(std::uint64_t a, std::uint64_t b) {
	if (a == 0 || b == 0) {
		return 0;
	}
	if (a > std::numeric_limits<std::uint64_t>::max() / b) {
		return std::numeric_limits<std::uint64_t>::max();
	}
	return a * b;
}

std::uint64_t EstimateComplexMatrixBytes(std::uint64_t rows, std::uint64_t cols) {
	const std::uint64_t outer_bytes = sizeof(ComplexMatrix);
	const std::uint64_t row_meta_bytes = SafeMultiply(rows, static_cast<std::uint64_t>(sizeof(std::vector<Complex>)));
	const std::uint64_t data_bytes = SafeMultiply(SafeMultiply(rows, cols), static_cast<std::uint64_t>(sizeof(Complex)));
	return outer_bytes + row_meta_bytes + data_bytes;
}

std::uint64_t EstimateVectorBytes(std::uint64_t count) {
	return sizeof(std::vector<Complex>) + SafeMultiply(count, static_cast<std::uint64_t>(sizeof(Complex)));
}

std::uint64_t EstimateFreqWorkerPeakBytes() {
	const std::uint64_t m_sz = static_cast<std::uint64_t>(std::max(0, M_SZ));
	const std::uint64_t n_port = static_cast<std::uint64_t>(std::max(0, N_PORT));

	std::uint64_t persistent_bytes = 0;
	persistent_bytes += EstimateComplexMatrixBytes(m_sz, m_sz);
	persistent_bytes += EstimateComplexMatrixBytes(m_sz, n_port);
	persistent_bytes += EstimateComplexMatrixBytes(n_port, n_port) * 3;

	std::uint64_t transient_bytes = 0;
	transient_bytes += SafeMultiply(SafeMultiply(m_sz, m_sz), static_cast<std::uint64_t>(sizeof(Complex)));
	transient_bytes += SafeMultiply(SafeMultiply(m_sz, n_port), static_cast<std::uint64_t>(sizeof(Complex)));
	transient_bytes += EstimateComplexMatrixBytes(n_port, n_port) * 6;
	transient_bytes += SafeMultiply(SafeMultiply(n_port, n_port), static_cast<std::uint64_t>(sizeof(Complex))) * 4;
	transient_bytes += EstimateVectorBytes(n_port) * 2;

	const std::uint64_t estimated_peak_bytes = persistent_bytes + transient_bytes;
	return static_cast<std::uint64_t>(estimated_peak_bytes * kWorkerSafetyMarginRatio);
}

bool QuerySystemMemory(std::uint64_t& total_physical_bytes, std::uint64_t& available_physical_bytes) {
	MEMORYSTATUSEX memory_status{};
	memory_status.dwLength = sizeof(memory_status);
	if (!GlobalMemoryStatusEx(&memory_status)) {
		return false;
	}

	total_physical_bytes = static_cast<std::uint64_t>(memory_status.ullTotalPhys);
	available_physical_bytes = static_cast<std::uint64_t>(memory_status.ullAvailPhys);
	return true;
}

std::string FormatBytes(std::uint64_t bytes) {
	static const char* kUnits[] = { "B", "KB", "MB", "GB", "TB" };
	double value = static_cast<double>(bytes);
	int unit_index = 0;
	while (value >= 1024.0 && unit_index < 4) {
		value /= 1024.0;
		++unit_index;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(unit_index == 0 ? 0 : 2) << value << ' ' << kUnits[unit_index];
	return oss.str();
}

FreqSolveWorkspace CreateWorkspace() {
	FreqSolveWorkspace ws;
	ws.cp_m.assign(M_SZ, std::vector<Complex>(M_SZ, Complex(0.0, 0.0)));
	ws.source.assign(M_SZ, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ws.z_temp.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ws.y_temp.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ws.s_temp.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	return ws;
}

WorkerPlan DetermineWorkerPlan() {
	WorkerPlan plan;
	const unsigned int hw_threads = std::thread::hardware_concurrency();
	const int fallback_threads = 4;
	plan.cpu_limit = std::max(1, std::min(N_FP, hw_threads == 0 ? fallback_threads : static_cast<int>(hw_threads)));
	plan.worker_count = plan.cpu_limit;
	plan.per_worker_peak_bytes = EstimateFreqWorkerPeakBytes();

	if (!QuerySystemMemory(plan.total_physical_bytes, plan.available_physical_bytes) || plan.total_physical_bytes == 0) {
		return plan;
	}

	plan.memory_info_ok = true;
	const std::uint64_t used_physical_bytes = plan.total_physical_bytes - std::min(plan.total_physical_bytes, plan.available_physical_bytes);
	const std::uint64_t usage_limit_bytes = static_cast<std::uint64_t>(static_cast<double>(plan.total_physical_bytes) * kMemoryUsageLimitRatio);
	plan.memory_budget_bytes = used_physical_bytes >= usage_limit_bytes ? 0 : (usage_limit_bytes - used_physical_bytes);

	if (plan.per_worker_peak_bytes == 0) {
		return plan;
	}

	std::uint64_t memory_limited_workers = plan.memory_budget_bytes / plan.per_worker_peak_bytes;
	if (memory_limited_workers == 0) {
		memory_limited_workers = 1;
	}

	const int memory_limit = static_cast<int>(std::min<std::uint64_t>(
		memory_limited_workers,
		static_cast<std::uint64_t>(std::max(1, N_FP))));
	plan.worker_count = std::max(1, std::min(plan.cpu_limit, memory_limit));
	plan.limited_by_memory = plan.worker_count < plan.cpu_limit;
	return plan;
}

void Fill_Branch_L(ComplexMatrix& cp_m, int pos) {
	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < C_PEC_N; ++j) {
			cp_m[i + S_BL[1]][j + S_BL[1]] = IJ * 2.0 * PI * CF[pos] * LL[i][j];
		}
	}
}

void Fill_Node_PP(ComplexMatrix& cp_m, int pos) {
	for (int i = 0; i < PEC_N; ++i) {
		for (int j = 0; j < PEC_N; ++j) {
			cp_m[S_BL[0] + i][S_BL[0] + j] = PP[i][j] * (IJ * CF[pos] * 2.0 * PI);
		}
	}
}

void Fill_Connective(ComplexMatrix& cp_m) {
	for (int i = 0; i < C_PEC_N + N_PORT; ++i) {
		for (int j = 0; j < NODE_N; ++j) {
			cp_m[S_BL[1] + i][S_BL[0] + j] = -A_E[i][j];
		}
	}

	for (int i = 0; i < NODE_N; ++i) {
		for (int j = 0; j < C_PEC_N + N_PORT; ++j) {
			cp_m[S_BL[0] + i][S_BL[1] + j] = -A_ET[i][j];
		}
	}
}

void Fill_Matrix_Q(FreqSolveWorkspace& ws, int pos) {
	for (auto& row : ws.cp_m) {
		std::fill(row.begin(), row.end(), Complex(0.0, 0.0));
	}

	Fill_Branch_L(ws.cp_m, pos);
	Fill_Node_PP(ws.cp_m, pos);
	Fill_Connective(ws.cp_m);

	for (int i = 0; i < M_SZ; ++i) {
		for (int j = 0; j < M_SZ; ++j) {
			if (std::isnan(std::abs(ws.cp_m[i][j]))) {
				throw std::runtime_error("NaN detected in frequency system matrix.");
			}
		}
	}
}

void Fill_Source(FreqSolveWorkspace& ws) {
	for (auto& row : ws.source) {
		std::fill(row.begin(), row.end(), Complex(0.0, 0.0));
	}

	for (int i = 0; i < N_PORT; ++i) {
		ws.source[S_BL[2] + i][i] = Complex(1.0, 0.0);
	}
}

void Z_TO_S(const ComplexMatrix& z_temp, ComplexMatrix& s_temp) {
	ComplexMatrix temp_1(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ComplexMatrix temp_2(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ComplexMatrix temp_3(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	ComplexMatrix temp_4(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));

	for (int i = 0; i < N_PORT; ++i) {
		for (int j = 0; j < N_PORT; ++j) {
			s_temp[i][j] = Complex(0.0, 0.0);
		}
	}

	temp_1 = Product_M(z_temp, SQRT_Y0);
	temp_2 = Product_M(SQRT_Y0, temp_1);
	for (int i = 0; i < N_PORT; ++i) {
		temp_2[i][i] -= Complex(1.0, 0.0);
	}

	temp_3 = Product_M(z_temp, SQRT_Y0);
	temp_4 = Product_M(SQRT_Y0, temp_3);
	for (int i = 0; i < N_PORT; ++i) {
		temp_4[i][i] += Complex(1.0, 0.0);
	}

	Inverse_M(temp_4, 0);
	s_temp = Product_M(temp_2, temp_4);
}

FreqPointAuxData Solver_Freq_Point(FreqSolveWorkspace& ws, int pos) {
	FreqPointAuxData aux;
	std::vector<std::vector<Complex>> v_normalize;
	int mathType = 0;

	Solve_M(ws.cp_m, ws.source, mathType);

	if (POST_PROCESSING != 0 && pos == ResolvePostFrequencyPosition()) {
		const int port_pos = ResolvePostPortPosition();
		if (port_pos >= 0) {
			aux.post_voltage.resize(PEC_N);
			aux.post_current.resize(C_PEC_N);
			for (int i = 0; i < PEC_N; ++i) {
				aux.post_voltage[i] = ws.source[S_BL[0] + i][port_pos];
			}
			for (int i = 0; i < C_PEC_N; ++i) {
				aux.post_current[i] = ws.source[S_BL[1] + i][port_pos];
			}
		}
	}

	for (int i = 0; i < N_PORT; ++i) {
		for (int j = 0; j < N_PORT; ++j) {
			ws.y_temp[i][j] = ws.source[S_BL[2] + i][j];
		}
	}

	ws.z_temp = ws.y_temp;
	Inverse_M(ws.z_temp, 0);

	ComplexMatrix y_update = ws.y_temp;
	for (int i = 0; i < N_PORT; ++i) {
		y_update[i][i] += Y0[i][i];
	}

	v_normalize.assign(N_PORT, std::vector<Complex>(1, Complex(0.0, 0.0)));
	if (N_PORT > 0) {
		v_normalize[0][0] = Y0[0][0];
		Solve_M(y_update, v_normalize, mathType);
	}

	Z_TO_S(ws.z_temp, ws.s_temp);

	if (N_PORT == 2) {
		aux.error = std::abs(std::abs(ws.s_temp[0][0] * ws.s_temp[0][0]) + std::abs(ws.s_temp[0][1] * ws.s_temp[0][1]) - 1) * 100;
		aux.normalized_voltage.resize(N_PORT, Complex(0.0, 0.0));
		for (int i = 0; i < N_PORT; ++i) {
			aux.normalized_voltage[i] = v_normalize[i][0];
		}
	}

	for (int i = 0; i < N_PORT; ++i) {
		for (int j = 0; j < N_PORT; ++j) {
			S_PAR_ABS[pos][i][j] = ABSS(ws.s_temp[i][j]);
			S_PAR_PHA[pos][i][j] = Phase(ws.s_temp[i][j]);
			Z_PT[pos][i][j] = ws.z_temp[i][j];
			Y_PT[pos][i][j] = ws.y_temp[i][j];
		}
	}

	return aux;
}

void Fill_Solution(std::ofstream& file_S, std::ofstream& file_Z, std::ofstream& file_Y, int pos) {
	if (N_PORT == 1) {
		file_S << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << S_PAR_ABS[pos][0][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[pos][0][0] << std::endl;

		file_Z << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << Z_PT[pos][0][0].real() << ",  "
			<< std::setw(12) << Z_PT[pos][0][0].imag() << std::endl;

		file_Y << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << Y_PT[pos][0][0].real() << ",  "
			<< std::setw(12) << Y_PT[pos][0][0].imag() << std::endl;
	}
	else if (N_PORT == 2) {
		file_S << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << S_PAR_ABS[pos][0][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[pos][0][0] << ",  "
			<< std::setw(12) << S_PAR_ABS[pos][0][1] << ",  "
			<< std::setw(12) << S_PAR_PHA[pos][0][1] << ",  "
			<< std::setw(12) << S_PAR_ABS[pos][1][0] << ",  "
			<< std::setw(12) << S_PAR_PHA[pos][1][0] << ",  "
			<< std::setw(12) << S_PAR_ABS[pos][1][1] << ",  "
			<< std::setw(12) << S_PAR_PHA[pos][1][1] << std::endl;

		file_Z << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << Z_PT[pos][0][0].real() << ",  "
			<< std::setw(12) << Z_PT[pos][0][0].imag() << ",  "
			<< std::setw(12) << Z_PT[pos][0][1].real() << ",  "
			<< std::setw(12) << Z_PT[pos][0][1].imag() << ",  "
			<< std::setw(12) << Z_PT[pos][1][0].real() << ",  "
			<< std::setw(12) << Z_PT[pos][1][0].imag() << ",  "
			<< std::setw(12) << Z_PT[pos][1][1].real() << ",  "
			<< std::setw(12) << Z_PT[pos][1][1].imag() << std::endl;

		file_Y << std::scientific << std::setprecision(6)
			<< std::setw(15) << CF[pos] << ",  "
			<< std::setw(12) << Y_PT[pos][0][0].real() << ",  "
			<< std::setw(12) << Y_PT[pos][0][0].imag() << ",  "
			<< std::setw(12) << Y_PT[pos][0][1].real() << ",  "
			<< std::setw(12) << Y_PT[pos][0][1].imag() << ",  "
			<< std::setw(12) << Y_PT[pos][1][0].real() << ",  "
			<< std::setw(12) << Y_PT[pos][1][0].imag() << ",  "
			<< std::setw(12) << Y_PT[pos][1][1].real() << ",  "
			<< std::setw(12) << Y_PT[pos][1][1].imag() << std::endl;
	}
}

void Rebuild_HW(const std::vector<FreqPointAuxData>& aux_data) {
	if (N_PORT != 2) {
		return;
	}

	for (int pos = 0; pos < N_FP; ++pos) {
		if (aux_data[pos].normalized_voltage.size() < 2) {
			continue;
		}

		H_W[pos][0] = aux_data[pos].normalized_voltage[0];
		H_W[pos][1] = aux_data[pos].normalized_voltage[1];
		H_W[2 * pos][0] = std::conj(H_W[pos][0]);
		H_W[2 * pos][1] = std::conj(H_W[pos][1]);
	}
}

} // namespace

void Ini_Data() {
	if (PRECISION > 1) {
		if (Swap_Log == 0) {
			N_FP = static_cast<int>((FE - FS) / PRECISION);
			N_FP += 1;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = i * PRECISION + FS;
			}
		}
		else {
			N_FP = static_cast<int>((std::log10(FE) - std::log10(FS)) / PRECISION);
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = FS * std::pow(10.0, i * PRECISION);
			}
		}
	}
	else {
		if (Swap_Log == 0) {
			PRECISION = (FE - FS) / N_FP;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = (i + 1) * PRECISION + FS;
			}
		}
		else {
			PRECISION = (std::log10(FE) - std::log10(FS)) / N_FP;
			CF.clear();
			CF.resize(N_FP);
			for (int i = 0; i < N_FP; ++i) {
				CF[i] = FS * std::pow(10.0, (i + 1) * PRECISION);
			}
		}
	}

	PEC_N = static_cast<int>(PP.size());
	C_PEC_N = static_cast<int>(LL.size());
	NODE_N = PEC_N;
	L_BL.resize(3);
	S_BL.resize(3);
	L_BL[0] = PEC_N;
	L_BL[1] = C_PEC_N;
	L_BL[2] = N_PORT;

	S_BL[0] = 0;
	S_BL[1] = S_BL[0] + L_BL[0];
	S_BL[2] = S_BL[1] + L_BL[1];
	M_SZ = S_BL[2] + L_BL[2];

	Z0.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	Y0.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	SQRT_Z0.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	SQRT_Y0.assign(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
	for (int i = 0; i < N_PORT; ++i) {
		Z0[i][i] = PORT_DATA[i].Zin;
		Y0[i][i] = 1.0 / PORT_DATA[i].Zin;
		SQRT_Z0[i][i] = std::sqrt(Z0[i][i]);
		SQRT_Y0[i][i] = std::sqrt(Y0[i][i]);
	}

	if (N_PORT != 0) {
		S_PAR_ABS.assign(N_FP, std::vector<std::vector<double>>(N_PORT, std::vector<double>(N_PORT, 0.0)));
		S_PAR_PHA.assign(N_FP, std::vector<std::vector<double>>(N_PORT, std::vector<double>(N_PORT, 0.0)));
		Z_PT.assign(N_FP, std::vector<std::vector<Complex>>(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0))));
		Y_PT.assign(N_FP, std::vector<std::vector<Complex>>(N_PORT, std::vector<Complex>(N_PORT, Complex(0.0, 0.0))));

		H_W.assign(2 * N_FP + 2, std::vector<Complex>(N_PORT, Complex(0.0, 0.0)));
		std::fill(H_W[0].begin(), H_W[0].end(), Complex(0.5, 0.0));
		std::fill(H_W[2 * N_FP + 1].begin(), H_W[2 * N_FP + 1].end(), Complex(0.5, 0.0));
	}

	A_E.assign(C_PEC_N + N_PORT, std::vector<double>(NODE_N, 0.0));
	A_ET.assign(NODE_N, std::vector<double>(C_PEC_N + N_PORT, 0.0));

	for (int i = 0; i < C_PEC_N; ++i) {
		for (int j = 0; j < 2; ++j) {
			const int idx = CN[i][j];
			if (idx < NODE_N && idx >= 0) {
				A_E[i][idx] = std::pow(-1.0, j + 2);
				A_ET[idx][i] = std::pow(-1.0, j + 1);
			}
		}
	}

	for (int i = 0; i < N_PORT; ++i) {
		const int idx1 = PORT_DATA[i].N[0];
		const int idx2 = PORT_DATA[i].N[1];
		if (idx1 < NODE_N && idx1 >= 0) {
			A_E[i + C_PEC_N][idx1] = 1.0;
			A_ET[idx1][i + C_PEC_N] = -1.0;
		}
		if (idx2 < NODE_N && idx2 >= 0) {
			A_E[i + C_PEC_N][idx2] = -1.0;
			A_ET[idx2][i + C_PEC_N] = 1.0;
		}
	}
}

void Freq_Solver() {
	Console::Section("P4", "Frequency Domain Solve");
	const double TIME_S = Get_Time();

	if (!Read_PEEC_Model(MAP_PATH)) {
		throw std::runtime_error("Failed to load PEEC model from: " + MAP_PATH);
	}
	Inverse_M(PP, 0);
	Ini_Data();
	Console::Detail("Matrix size", M_SZ);
	Console::Info("Solving frequency points...");

	std::ofstream file_S(MAP_PATH + "map.txt");
	std::ofstream file_Z(MAP_PATH + "Z_in.txt");
	std::ofstream file_Y(MAP_PATH + "Y_in.txt");

	MAX_E = 0.0;
	const WorkerPlan worker_plan = DetermineWorkerPlan();
	const int worker_count = worker_plan.worker_count;
	const int old_mkl_threads = mkl_get_max_threads();
	SetMKLThreads(1);
	Console::Detail("Threads", worker_count);
	Console::Detail("Tasks", N_FP);
	Console::Detail("MKL threads", 1);
	if (worker_plan.memory_info_ok) {
		Console::Detail("Physical memory", FormatBytes(worker_plan.total_physical_bytes));
		Console::Detail("Available memory", FormatBytes(worker_plan.available_physical_bytes));
		Console::Detail("Solver mem budget", FormatBytes(worker_plan.memory_budget_bytes));
		Console::Detail("Worker mem est.", FormatBytes(worker_plan.per_worker_peak_bytes));
		if (worker_plan.limited_by_memory) {
			Console::Warn("Thread count reduced by memory budget to keep total physical memory usage under 90%.");
		}
	}
	else {
		Console::Warn("Unable to query system memory. Falling back to CPU-based thread count.");
	}

	std::atomic<int> next_pos{ 0 };
	std::atomic<int> completed{ 0 };
	std::vector<FreqPointAuxData> aux_data(N_FP);
	std::vector<std::thread> workers;
	workers.reserve(worker_count);
	Console::Progress("Frequency Solve", 0, N_FP);

	std::thread progress_thread([&]() {
		int last_printed = -1;
		while (true) {
			const int done = completed.load();
			if (done != last_printed) {
				Console::Progress("Frequency Solve", done, N_FP);
				last_printed = done;
			}
			if (done >= N_FP) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	});

	for (int worker_idx = 0; worker_idx < worker_count; ++worker_idx) {
		workers.emplace_back([&]() {
			FreqSolveWorkspace ws = CreateWorkspace();
			double local_max_error = 0.0;

			while (true) {
				const int pos = next_pos.fetch_add(1);
				if (pos >= N_FP) {
					break;
				}

				Fill_Matrix_Q(ws, pos);
				Fill_Source(ws);
				aux_data[pos] = Solver_Freq_Point(ws, pos);
				local_max_error = std::max(local_max_error, aux_data[pos].error);
				completed.fetch_add(1);
			}

			if (local_max_error > 0.0) {
				static std::mutex max_error_mutex;
				std::lock_guard<std::mutex> lock(max_error_mutex);
				MAX_E = std::max(MAX_E, local_max_error);
			}
		});
	}

	for (auto& worker : workers) {
		worker.join();
	}

	progress_thread.join();

	Rebuild_HW(aux_data);
	if (!SavePostProcessingSource(aux_data)) {
		throw std::runtime_error("Failed to save post-processing source snapshot.");
	}

	for (int pos = 0; pos < N_FP; ++pos) {
		Fill_Solution(file_S, file_Z, file_Y, pos);
	}

	SetMKLThreads(old_mkl_threads);

	file_S.close();
	file_Z.close();
	file_Y.close();

	Console::Detail("Maximum error", MAX_E);
	const double TIME_E = Get_Time();
	Time_Diff(TIME_S, TIME_E);
}

