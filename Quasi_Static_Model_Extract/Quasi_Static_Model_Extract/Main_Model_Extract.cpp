#include "Mod_Connection.h"
#include "Mod_Element_Build.h"
#include "Mod_Gaussian.h"
#include "Mod_Read_File.h"

#include <cstdlib>
#include <exception>
#include <string>

namespace {
void WaitForUserBeforeExit() {
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
		Console::Section("BOOT", "Quasi Static Model Extract");
		Ini_Gaussion();
		Read_File();
		Creat_Connection();
		Element_Build();
		Console::Info("All stages finished successfully.");
	}
	catch (const std::exception& e) {
		Console::Error("Unhandled exception: " + std::string(e.what()));
		if (should_pause) WaitForUserBeforeExit();
		return 1;
	}
	catch (...) {
		Console::Error("Unknown unhandled exception.");
		if (should_pause) WaitForUserBeforeExit();
		return 2;
	}

	if (should_pause) WaitForUserBeforeExit();
	return 0;
}

