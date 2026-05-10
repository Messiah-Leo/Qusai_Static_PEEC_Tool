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
}

int main() {
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
		WaitForUserBeforeExit();
		return 1;
	}
	catch (...) {
		Console::Error("Unknown unhandled exception.");
		WaitForUserBeforeExit();
		return 2;
	}

	WaitForUserBeforeExit();
	return 0;
}

