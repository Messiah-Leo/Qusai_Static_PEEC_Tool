#include "Mod_Read_File.h"
#include "Mod_Solver_Freq.h"
#include "Mod_Solver_Time.h"

#include <exception>
#include <iostream>

namespace {

void WaitBeforeExit() {
	std::cout << std::endl;
	std::cout << "[EXIT] Press Enter to exit..." << std::flush;
	std::cin.get();
}

}

int main() {
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

		WaitBeforeExit();
		return 0;
	}
	catch (const std::exception& ex) {
		Console::Error("Program terminated with an error.");
		Console::Error("Reason: " + std::string(ex.what()));
		Console::Warn("Please check input files, runtime paths, and solver settings.");
		WaitBeforeExit();
		return 1;
	}
	catch (...) {
		Console::Error("Program terminated with an unknown error.");
		Console::Warn("Please check input files, runtime paths, and solver settings.");
		WaitBeforeExit();
		return 1;
	}
}

