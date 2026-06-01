#include "Mod_Post_Processing.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

void PrintUsage() {
	std::cout << "Usage: Post_Processing.exe [--data-root <path>] [--self-test] [--pause]\n";
}

}

int main(int argc, char* argv[]) {
	std::filesystem::path dataRoot;
	bool selfTest = false;
	bool pause = false;

	for (int i = 1; i < argc; ++i) {
		const std::string argument = argv[i];
		if (argument == "--data-root" && i + 1 < argc) {
			dataRoot = argv[++i];
		}
		else if (argument == "--self-test") {
			selfTest = true;
		}
		else if (argument == "--pause") {
			pause = true;
		}
		else {
			PrintUsage();
			return 2;
		}
	}

	try {
		if (selfTest) {
			RunPostProcessingSelfTest();
		}
		else {
			if (dataRoot.empty()) {
				dataRoot = ResolveDefaultDataRoot();
			}
			RunPostProcessing(dataRoot);
		}
	}
	catch (const std::exception& exception) {
		std::cerr << "[ERROR] " << exception.what() << '\n';
		if (pause) system("pause");
		return 1;
	}

	if (pause) system("pause");
	return 0;
}
