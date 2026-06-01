#include "Mod_Read_File.h"
#include "Mod_Solver_Freq.h"
#include "Mod_Solver_Time.h"

#include <cstdlib>
#include <exception>
#include <iostream>

namespace {

void WaitBeforeExit() {
	system("pause");
}

bool ShouldPause(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "--pause") {
			return true;
		}
	}
	return false;
}

}

int main(int argc, char* argv[]) {
	const bool should_pause = ShouldPause(argc, argv);
	try {
		Console::Section("BOOT", "Quasi Static Solver");
		Read_File();
		if (Solver_SET == 0) {
			Freq_Solver();
		}
		else if (Solver_SET == 1) {
			Time_Solver();
		}
		else {
			throw std::runtime_error("Invalid Solver_SET value. Expected 0 (frequency) or 1 (time).");
		}

		Console::Info("All stages finished successfully.");

		if (should_pause) WaitBeforeExit();
		return 0;
	}
	catch (const std::exception& ex) {
		Console::Error("Program terminated with an error.");
		Console::Error("Reason: " + std::string(ex.what()));
		Console::Warn("Please check input files, runtime paths, and solver settings.");
		if (should_pause) WaitBeforeExit();
		return 1;
	}
	catch (...) {
		Console::Error("Program terminated with an unknown error.");
		Console::Warn("Please check input files, runtime paths, and solver settings.");
		if (should_pause) WaitBeforeExit();
		return 1;
	}
}

