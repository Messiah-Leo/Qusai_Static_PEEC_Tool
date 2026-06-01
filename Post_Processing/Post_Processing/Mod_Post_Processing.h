#pragma once

#include <filesystem>

std::filesystem::path ResolveDefaultDataRoot();
void RunPostProcessing(const std::filesystem::path& dataRoot);
void RunPostProcessingSelfTest();
