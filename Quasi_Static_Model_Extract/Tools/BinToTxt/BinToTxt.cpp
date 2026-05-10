#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

constexpr std::array<char, 8> kMagic = { 'P', 'E', 'E', 'C', 'M', 'A', 'T', '1' };
constexpr int kPrecision = 16;

void PrintUsage(const char* exe_name)
{
	std::cerr
		<< "Usage:\n"
		<< "  " << exe_name << " <input.bin> [output.txt]\n\n"
		<< "Examples:\n"
		<< "  " << exe_name << " ..\\..\\Data\\Output\\LL_OO.bin\n"
		<< "  " << exe_name << " ..\\..\\Data\\Output\\PP_OO.bin ..\\..\\Data\\Output\\PP_OO.txt\n";
}

std::filesystem::path DefaultOutputPath(const std::filesystem::path& input_path)
{
	std::filesystem::path output_path = input_path;
	output_path.replace_extension(".txt");
	return output_path;
}

bool ReadUInt64(std::ifstream& input, std::uint64_t& value)
{
	input.read(reinterpret_cast<char*>(&value), sizeof(value));
	return input.good();
}

bool ConvertPeecMatrixBinToTxt(
	const std::filesystem::path& input_path,
	const std::filesystem::path& output_path
)
{
	std::ifstream input(input_path, std::ios::binary);
	if (!input.is_open()) {
		std::cerr << "Cannot open input file: " << input_path << '\n';
		return false;
	}

	std::array<char, 8> magic{};
	input.read(magic.data(), static_cast<std::streamsize>(magic.size()));
	if (!input.good() || magic != kMagic) {
		std::cerr << "Invalid binary matrix header: " << input_path << '\n';
		std::cerr << "Expected header: PEECMAT1\n";
		return false;
	}

	std::uint64_t rows = 0;
	std::uint64_t cols = 0;
	if (!ReadUInt64(input, rows) || !ReadUInt64(input, cols)) {
		std::cerr << "Cannot read matrix dimensions from: " << input_path << '\n';
		return false;
	}

	if (rows == 0 || cols == 0) {
		std::cerr << "Matrix dimensions must be non-zero.\n";
		return false;
	}

	const auto file_size = std::filesystem::file_size(input_path);
	const std::uint64_t header_size = kMagic.size() + 2 * sizeof(std::uint64_t);
	if (file_size < header_size) {
		std::cerr << "Input file is too small: " << input_path << '\n';
		return false;
	}

	const std::uint64_t payload_size = file_size - header_size;
	if (rows > std::numeric_limits<std::uint64_t>::max() / cols ||
		rows * cols > std::numeric_limits<std::uint64_t>::max() / sizeof(double)) {
		std::cerr << "Matrix dimensions are too large.\n";
		return false;
	}

	const std::uint64_t expected_payload = rows * cols * sizeof(double);
	if (payload_size != expected_payload) {
		std::cerr << "File size does not match matrix dimensions.\n"
			<< "Rows: " << rows << ", cols: " << cols << '\n'
			<< "Payload bytes: " << payload_size
			<< ", expected bytes: " << expected_payload << '\n';
		return false;
	}

	std::ofstream output(output_path);
	if (!output.is_open()) {
		std::cerr << "Cannot open output file: " << output_path << '\n';
		return false;
	}

	output << std::scientific << std::setprecision(kPrecision);
	output << rows << ' ' << cols << '\n';

	for (std::uint64_t i = 0; i < rows; ++i) {
		for (std::uint64_t j = 0; j < cols; ++j) {
			double value = 0.0;
			input.read(reinterpret_cast<char*>(&value), sizeof(value));
			if (!input.good()) {
				std::cerr << "Failed while reading matrix data.\n";
				return false;
			}

			output << value;
			if (j + 1 < cols) {
				output << ' ';
			}
		}
		output << '\n';
	}

	if (!output.good()) {
		std::cerr << "Failed while writing text file: " << output_path << '\n';
		return false;
	}

	std::cout << "Converted: " << input_path << '\n'
		<< "Output:    " << output_path << '\n'
		<< "Shape:     " << rows << " x " << cols << '\n';
	return true;
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc != 2 && argc != 3) {
		PrintUsage(argv[0]);
		return 1;
	}

	const std::filesystem::path input_path = argv[1];
	const std::filesystem::path output_path = (argc == 3)
		? std::filesystem::path(argv[2])
		: DefaultOutputPath(input_path);

	return ConvertPeecMatrixBinToTxt(input_path, output_path) ? 0 : 2;
}
