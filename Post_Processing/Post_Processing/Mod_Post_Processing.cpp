#include "Mod_Post_Processing.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define NOMINMAX
#include <windows.h>

namespace {

namespace fs = std::filesystem;
using Complex = std::complex<double>;
using Vector = std::array<double, 3>;
using ComplexVector = std::array<Complex, 3>;

struct Point {
	Vector x{};
};

struct Mesh {
	std::array<int, 3> point{};
	double area = 0.0;
	Vector midpoint{};
};

struct Connection {
	std::array<int, 2> mesh{};
	std::array<int, 2> freeVertex{};
};

struct Model {
	double dimensionScale = 1.0;
	int pecMeshCount = 0;
	std::vector<Point> points;
	std::vector<Mesh> meshes;
	std::vector<Connection> connections;
};

struct Source {
	double frequency = 0.0;
	int port = 1;
	std::vector<Complex> currents;
	std::vector<Complex> voltages;
	std::vector<Complex> magneticCurrents;
	std::vector<Complex> magneticVoltages;
};

struct Settings {
	bool enabled = true;
	bool plotCurrent = true;
	bool plotCharge = true;
	bool plotVoltage = true;
	bool plotMagneticCurrent = true;
	bool plotMagneticCharge = true;
	bool plotMagneticVoltage = true;
	bool plotSpaceField = false;
	double fieldEr = 1.0;
};

struct FieldGrid {
	std::vector<Point> points;
	std::vector<std::array<int, 3>> triangles;
};

struct SpaceFields {
	std::vector<ComplexVector> electric;
	std::vector<ComplexVector> magnetic;
};

struct EdgeIntegrals {
	ComplexVector dot{};
	ComplexVector curl{};
};

void ExpectToken(std::istream& input, const std::string& expected) {
	std::string token;
	if (!(input >> token) || token != expected) {
		throw std::runtime_error("Expected token '" + expected + "' in post-processing data.");
	}
}

bool IsTriangle(const Mesh& mesh) {
	return mesh.point[0] >= 0 && mesh.point[1] >= 0 && mesh.point[2] >= 0;
}

void ValidateModel(const Model& model) {
	if (model.dimensionScale == 0.0) {
		throw std::runtime_error("Post-processing model dimension scale must not be zero.");
	}
	if (model.pecMeshCount < 0 || model.pecMeshCount > static_cast<int>(model.meshes.size())) {
		throw std::runtime_error("Post-processing PEC mesh count is invalid.");
	}
	for (int i = 0; i < model.pecMeshCount; ++i) {
		if (!IsTriangle(model.meshes[i])) {
			throw std::runtime_error("A visible PEC mesh is not a triangle.");
		}
		for (const int point : model.meshes[i].point) {
			if (point < 0 || point >= static_cast<int>(model.points.size())) {
				throw std::runtime_error("A PEC mesh references an invalid point.");
			}
		}
	}
	for (const Connection& connection : model.connections) {
		for (int side = 0; side < 2; ++side) {
			if (connection.mesh[side] < 0 || connection.mesh[side] >= static_cast<int>(model.meshes.size())) {
				throw std::runtime_error("A source connection references an invalid mesh.");
			}
			if (connection.freeVertex[side] < 0 || connection.freeVertex[side] >= 3) {
				throw std::runtime_error("A source connection references an invalid free vertex.");
			}
		}
	}
}

Model ReadModel(const fs::path& path) {
	std::ifstream input(path);
	if (!input.is_open()) {
		throw std::runtime_error("Cannot open post-processing model: " + path.string());
	}

	Model model;
	ExpectToken(input, "PEEC_POST_MODEL_V1");
	ExpectToken(input, "DIM");
	input >> model.dimensionScale;

	int pointCount = 0;
	ExpectToken(input, "POINTS");
	input >> pointCount;
	if (pointCount < 0) {
		throw std::runtime_error("Post-processing point count is invalid.");
	}
	model.points.resize(pointCount);
	for (int i = 0; i < pointCount; ++i) {
		int number = -1;
		input >> number >> model.points[i].x[0] >> model.points[i].x[1] >> model.points[i].x[2];
		if (number != i) {
			throw std::runtime_error("Post-processing point numbers must be contiguous and zero based.");
		}
	}

	int meshCount = 0;
	int lineMeshCount = 0;
	ExpectToken(input, "MESHES");
	input >> meshCount >> model.pecMeshCount >> lineMeshCount;
	if (meshCount < 0 || lineMeshCount < 0) {
		throw std::runtime_error("Post-processing mesh count is invalid.");
	}
	model.meshes.resize(meshCount);
	for (Mesh& mesh : model.meshes) {
		input >> mesh.point[0] >> mesh.point[1] >> mesh.point[2]
			>> mesh.area >> mesh.midpoint[0] >> mesh.midpoint[1] >> mesh.midpoint[2];
	}

	int connectionCount = 0;
	ExpectToken(input, "CONNECTIONS");
	input >> connectionCount;
	if (connectionCount < 0) {
		throw std::runtime_error("Post-processing connection count is invalid.");
	}
	model.connections.resize(connectionCount);
	for (Connection& connection : model.connections) {
		input >> connection.mesh[0] >> connection.mesh[1]
			>> connection.freeVertex[0] >> connection.freeVertex[1];
	}

	if (!input.good() && !input.eof()) {
		throw std::runtime_error("Failed while reading post-processing model.");
	}
	ValidateModel(model);
	return model;
}

Source ReadSource(const fs::path& path) {
	std::ifstream input(path);
	if (!input.is_open()) {
		throw std::runtime_error("Cannot open post-processing source: " + path.string());
	}

	Source source;
	ExpectToken(input, "PEEC_POST_SOURCE_V1");
	ExpectToken(input, "FREQUENCY");
	input >> source.frequency;
	ExpectToken(input, "PORT");
	input >> source.port;

	int currentCount = 0;
	ExpectToken(input, "CURRENTS");
	input >> currentCount;
	if (currentCount < 0) {
		throw std::runtime_error("Post-processing current count is invalid.");
	}
	source.currents.resize(currentCount);
	for (Complex& value : source.currents) {
		double real = 0.0;
		double imaginary = 0.0;
		input >> real >> imaginary;
		value = Complex(real, imaginary);
	}

	int voltageCount = 0;
	ExpectToken(input, "VOLTAGES");
	input >> voltageCount;
	if (voltageCount < 0) {
		throw std::runtime_error("Post-processing voltage count is invalid.");
	}
	source.voltages.resize(voltageCount);
	for (Complex& value : source.voltages) {
		double real = 0.0;
		double imaginary = 0.0;
		input >> real >> imaginary;
		value = Complex(real, imaginary);
	}

	std::string section;
	while (input >> section) {
		int valueCount = 0;
		input >> valueCount;
		if (valueCount < 0) {
			throw std::runtime_error("Optional post-processing source count is invalid.");
		}

		std::vector<Complex>* values = nullptr;
		if (section == "MAGNETIC_CURRENTS") {
			values = &source.magneticCurrents;
		}
		else if (section == "MAGNETIC_VOLTAGES") {
			values = &source.magneticVoltages;
		}
		else {
			throw std::runtime_error("Unknown optional post-processing source section: " + section);
		}

		values->resize(valueCount);
		for (Complex& value : *values) {
			double real = 0.0;
			double imaginary = 0.0;
			input >> real >> imaginary;
			value = Complex(real, imaginary);
		}
	}

	if (!input.good() && !input.eof()) {
		throw std::runtime_error("Failed while reading post-processing source.");
	}
	if (source.magneticCurrents.empty()) {
		source.magneticCurrents.assign(source.currents.size(), Complex(0.0, 0.0));
	}
	if (source.magneticVoltages.empty()) {
		source.magneticVoltages.assign(source.voltages.size(), Complex(0.0, 0.0));
	}
	return source;
}

Settings ReadSettings(const fs::path& path) {
	Settings settings;
	std::ifstream input(path);
	if (!input.is_open()) {
		return settings;
	}

	std::string line;
	while (std::getline(input, line)) {
		std::istringstream parser(line);
		std::string key;
		double value = 0.0;
		parser >> key;
		if (key == "$End_Def") {
			break;
		}
		if (!(parser >> value)) {
			continue;
		}
		if (key == "ER") settings.fieldEr = value;
		else if (key == "POST_PROCESSING") settings.enabled = value != 0.0;
		else if (key == "POST_PLOT_J") settings.plotCurrent = value != 0.0;
		else if (key == "POST_PLOT_QE") settings.plotCharge = value != 0.0;
		else if (key == "POST_PLOT_VE") settings.plotVoltage = value != 0.0;
		else if (key == "POST_PLOT_M") settings.plotMagneticCurrent = value != 0.0;
		else if (key == "POST_PLOT_QM") settings.plotMagneticCharge = value != 0.0;
		else if (key == "POST_PLOT_VM") settings.plotMagneticVoltage = value != 0.0;
		else if (key == "POST_FIELD") settings.plotSpaceField = value != 0.0;
		else if (key == "POST_FIELD_ER") settings.fieldEr = value;
	}
	if (settings.fieldEr <= 0.0) {
		throw std::runtime_error("POST_FIELD_ER must be greater than zero.");
	}
	return settings;
}

FieldGrid ReadFieldGrid(const fs::path& path, double dimensionScale) {
	std::ifstream input(path);
	if (!input.is_open()) {
		throw std::runtime_error("Cannot open space field mesh: " + path.string());
	}

	std::string token;
	while (input >> token && token != "$Nodes") {}
	if (token != "$Nodes") {
		throw std::runtime_error("Space field mesh does not contain a $Nodes section.");
	}

	int pointCount = 0;
	input >> pointCount;
	if (pointCount <= 0) {
		throw std::runtime_error("Space field mesh point count is invalid.");
	}

	FieldGrid grid;
	grid.points.resize(pointCount);
	std::unordered_map<int, int> pointNumbers;
	for (int i = 0; i < pointCount; ++i) {
		int number = 0;
		input >> number >> grid.points[i].x[0] >> grid.points[i].x[1] >> grid.points[i].x[2];
		if (!input || pointNumbers.count(number) != 0) {
			throw std::runtime_error("Failed while reading space field mesh points.");
		}
		pointNumbers[number] = i;
		for (double& coordinate : grid.points[i].x) {
			coordinate /= dimensionScale;
		}
	}

	while (input >> token && token != "$Elements") {}
	if (token != "$Elements") {
		throw std::runtime_error("Space field mesh does not contain an $Elements section.");
	}

	int elementCount = 0;
	input >> elementCount;
	std::string line;
	std::getline(input, line);
	for (int i = 0; i < elementCount; ++i) {
		if (!std::getline(input, line)) {
			throw std::runtime_error("Failed while reading space field mesh elements.");
		}
		std::istringstream parser(line);
		int number = 0;
		int type = 0;
		int tagCount = 0;
		parser >> number >> type >> tagCount;
		for (int tag = 0; tag < tagCount; ++tag) {
			int ignored = 0;
			parser >> ignored;
		}
		if (type != 2) {
			continue;
		}

		std::array<int, 3> triangle{};
		for (int& point : triangle) {
			int pointNumber = 0;
			parser >> pointNumber;
			const auto found = pointNumbers.find(pointNumber);
			if (!parser || found == pointNumbers.end()) {
				throw std::runtime_error("A space field triangle references an invalid point.");
			}
			point = found->second;
		}
		grid.triangles.push_back(triangle);
	}
	if (grid.triangles.empty()) {
		throw std::runtime_error("Space field mesh does not contain triangle elements.");
	}
	return grid;
}

Vector Subtract(const Vector& left, const Vector& right) {
	return { left[0] - right[0], left[1] - right[1], left[2] - right[2] };
}

Vector Add(const Vector& left, const Vector& right) {
	return { left[0] + right[0], left[1] + right[1], left[2] + right[2] };
}

Vector Scale(const Vector& value, double scale) {
	return { value[0] * scale, value[1] * scale, value[2] * scale };
}

double Magnitude(const Vector& value) {
	return std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
}

Vector Cross(const Vector& left, const Vector& right) {
	return {
		left[1] * right[2] - left[2] * right[1],
		left[2] * right[0] - left[0] * right[2],
		left[0] * right[1] - left[1] * right[0],
	};
}

void AddScaled(ComplexVector& target, const Vector& value, const Complex& scale) {
	for (int component = 0; component < 3; ++component) {
		target[component] += value[component] * scale;
	}
}

void AddScaled(ComplexVector& target, const ComplexVector& value, const Complex& scale) {
	for (int component = 0; component < 3; ++component) {
		target[component] += value[component] * scale;
	}
}

double MeshScale(const Model& model, const Mesh& mesh) {
	return Magnitude(Subtract(model.points[mesh.point[1]].x, model.points[mesh.point[0]].x));
}

std::vector<ComplexVector> ConstructPointCurrent(const Model& model, const std::vector<Complex>& currents) {
	if (currents.size() != model.connections.size()) {
		throw std::runtime_error("Current count does not match model connection count.");
	}

	std::vector<ComplexVector> meshValues(model.meshes.size());
	for (size_t i = 0; i < model.connections.size(); ++i) {
		for (int side = 0; side < 2; ++side) {
			const Connection& connection = model.connections[i];
			const int meshIndex = connection.mesh[side];
			const Mesh& mesh = model.meshes[meshIndex];
			if (!IsTriangle(mesh) || std::abs(mesh.area) <= 1.0e-20) {
				continue;
			}

			const int pointIndex = mesh.point[connection.freeVertex[side]];
			const double sign = side == 0 ? 1.0 : -1.0;
			for (int component = 0; component < 3; ++component) {
				const double basis = sign * (mesh.midpoint[component] - model.points[pointIndex].x[component]) /
					(2.0 * mesh.area);
				meshValues[meshIndex][component] += currents[i] * basis;
			}
		}
	}

	std::vector<ComplexVector> pointValues(model.points.size());
	std::vector<int> weights(model.points.size(), 0);
	for (int meshIndex = 0; meshIndex < model.pecMeshCount; ++meshIndex) {
		for (const int pointIndex : model.meshes[meshIndex].point) {
			for (int component = 0; component < 3; ++component) {
				pointValues[pointIndex][component] += meshValues[meshIndex][component];
			}
			++weights[pointIndex];
		}
	}
	for (size_t point = 0; point < pointValues.size(); ++point) {
		if (weights[point] == 0) continue;
		for (Complex& component : pointValues[point]) {
			component /= static_cast<double>(weights[point]);
		}
	}
	return pointValues;
}

std::vector<Complex> ConstructRawCharge(const Model& model, const std::vector<Complex>& currents) {
	if (currents.size() != model.connections.size()) {
		throw std::runtime_error("Current count does not match model connection count.");
	}

	std::vector<Complex> values(model.meshes.size());
	for (size_t i = 0; i < model.connections.size(); ++i) {
		values[model.connections[i].mesh[0]] -= currents[i];
		values[model.connections[i].mesh[1]] += currents[i];
	}
	return values;
}

std::vector<Complex> ConstructChargeDensity(const Model& model, const std::vector<Complex>& currents) {
	std::vector<Complex> values = ConstructRawCharge(model, currents);
	for (int mesh = 0; mesh < model.pecMeshCount; ++mesh) {
		if (std::abs(model.meshes[mesh].area) > 1.0e-20) {
			values[mesh] /= model.meshes[mesh].area;
		}
	}
	return values;
}

std::vector<Complex> ConstructPointScalar(const Model& model, const std::vector<Complex>& meshValues) {
	if (meshValues.size() < static_cast<size_t>(model.pecMeshCount)) {
		throw std::runtime_error("Scalar mesh value count is smaller than the PEC mesh count.");
	}

	std::vector<Complex> pointValues(model.points.size());
	std::vector<int> weights(model.points.size(), 0);
	for (int mesh = 0; mesh < model.pecMeshCount; ++mesh) {
		for (const int point : model.meshes[mesh].point) {
			pointValues[point] += meshValues[mesh];
			++weights[point];
		}
	}
	for (size_t point = 0; point < pointValues.size(); ++point) {
		if (weights[point] != 0) {
			pointValues[point] /= static_cast<double>(weights[point]);
		}
	}
	return pointValues;
}

EdgeIntegrals EvaluateEdgeIntegrals(const Model& model, const Connection& connection, const Complex& source,
	const Vector& observation, double waveNumber) {
	static constexpr double barycentric[6][3] = {
		{ 0.81684757298, 0.09157621351, 0.09157621351 },
		{ 0.09157621351, 0.81684757298, 0.09157621351 },
		{ 0.09157621351, 0.09157621351, 0.81684757298 },
		{ 0.1081030182, 0.4459484909, 0.4459484909 },
		{ 0.4459484909, 0.10810301817, 0.4459484909 },
		{ 0.4459484909, 0.4459484909, 0.1081030182 },
	};
	static constexpr double weight[6] = {
		0.1099517436, 0.1099517436, 0.1099517436,
		0.2233815897, 0.2233815897, 0.2233815897,
	};
	static constexpr double pi = 3.14159265358979323846;
	static const Complex imaginary(0.0, 1.0);

	EdgeIntegrals result;
	if (std::abs(source) == 0.0) {
		return result;
	}
	for (int side = 0; side < 2; ++side) {
		const Mesh& mesh = model.meshes[connection.mesh[side]];
		if (!IsTriangle(mesh)) {
			continue;
		}
		const double meshScale = MeshScale(model, mesh);
		const double epsilon = meshScale * 1.0e-4;
		if (meshScale <= 0.0 || mesh.area <= meshScale * meshScale * 1.0e-4) {
			continue;
		}

		const Vector freePoint = model.points[mesh.point[connection.freeVertex[side]]].x;
		ComplexVector dotIntegral{};
		ComplexVector curlIntegral{};
		for (int quadrature = 0; quadrature < 6; ++quadrature) {
			Vector sourcePoint{};
			for (int vertex = 0; vertex < 3; ++vertex) {
				sourcePoint = Add(sourcePoint, Scale(model.points[mesh.point[vertex]].x, barycentric[quadrature][vertex]));
			}
			const Vector basis = Subtract(sourcePoint, freePoint);
			const Vector difference = Subtract(observation, sourcePoint);
			const double distance = Magnitude(difference);
			if (distance > epsilon) {
				AddScaled(dotIntegral, basis, weight[quadrature] * std::exp(-imaginary * waveNumber * distance) / distance);
			}

			const double regularizedDistance = distance + epsilon;
			const Vector radial = Scale(difference, 1.0 / regularizedDistance);
			const Complex kr = imaginary * waveNumber * regularizedDistance;
			const Complex greens = -(1.0 + kr) * std::exp(-kr) / (regularizedDistance * regularizedDistance);
			AddScaled(curlIntegral, Cross(radial, basis), weight[quadrature] * greens);
		}
		const double sign = side == 0 ? 1.0 : -1.0;
		AddScaled(result.dot, dotIntegral, source * sign / (8.0 * pi));
		AddScaled(result.curl, curlIntegral, source * sign / (8.0 * pi));
	}
	return result;
}

ComplexVector EvaluateGradient(const Model& model, const Mesh& mesh, const Complex& source,
	const Vector& observation, double waveNumber) {
	static constexpr double barycentric[6][3] = {
		{ 0.81684757298, 0.09157621351, 0.09157621351 },
		{ 0.09157621351, 0.81684757298, 0.09157621351 },
		{ 0.09157621351, 0.09157621351, 0.81684757298 },
		{ 0.1081030182, 0.4459484909, 0.4459484909 },
		{ 0.4459484909, 0.10810301817, 0.4459484909 },
		{ 0.4459484909, 0.4459484909, 0.1081030182 },
	};
	static constexpr double weight[6] = {
		0.1099517436, 0.1099517436, 0.1099517436,
		0.2233815897, 0.2233815897, 0.2233815897,
	};
	static constexpr double pi = 3.14159265358979323846;
	static const Complex imaginary(0.0, 1.0);

	ComplexVector result{};
	if (std::abs(source) == 0.0) {
		return result;
	}
	if (!IsTriangle(mesh)) {
		return result;
	}
	const double meshScale = MeshScale(model, mesh);
	const double epsilon = meshScale * 1.0e-4;
	if (meshScale <= 0.0 || mesh.area < meshScale * meshScale * 1.0e-4) {
		return result;
	}
	for (int quadrature = 0; quadrature < 6; ++quadrature) {
		Vector sourcePoint{};
		for (int vertex = 0; vertex < 3; ++vertex) {
			sourcePoint = Add(sourcePoint, Scale(model.points[mesh.point[vertex]].x, barycentric[quadrature][vertex]));
		}
		const Vector difference = Subtract(observation, sourcePoint);
		const double regularizedDistance = Magnitude(difference) + epsilon;
		const Vector radial = Scale(difference, 1.0 / regularizedDistance);
		const Complex kr = imaginary * waveNumber * regularizedDistance;
		const Complex greens = -(1.0 + kr) * std::exp(-kr) / (regularizedDistance * regularizedDistance);
		AddScaled(result, radial, source * weight[quadrature] * greens / (4.0 * pi));
	}
	return result;
}

SpaceFields EvaluateSpaceFields(const Model& model, const Source& source, const FieldGrid& grid, double er) {
	static constexpr double pi = 3.14159265358979323846;
	static constexpr double epsilon0 = 8.85418782e-12 * 4.4;
	static constexpr double mu0 = 4.0 * pi * 1.0e-7;
	static const Complex imaginary(0.0, 1.0);
	if (source.frequency <= 0.0) {
		throw std::runtime_error("Space field evaluation frequency must be greater than zero.");
	}
	if (source.currents.size() != model.connections.size() ||
		source.magneticCurrents.size() != model.connections.size()) {
		throw std::runtime_error("Space field source current count does not match model connection count.");
	}

	const double waveNumber = 2.0 * pi * source.frequency * std::sqrt(epsilon0 * mu0) * std::sqrt(er);
	const Complex lMu0 = imaginary * 2.0 * pi * mu0;
	const Complex lEpsilon0 = imaginary * 2.0 * pi * epsilon0;
	const std::vector<Complex> electricCharge = ConstructRawCharge(model, source.currents);
	const std::vector<Complex> magneticCharge = ConstructRawCharge(model, source.magneticCurrents);

	SpaceFields fields;
	fields.electric.resize(grid.points.size());
	fields.magnetic.resize(grid.points.size());
	std::atomic<size_t> nextPoint = 0;
	const unsigned int threadCount = std::max(1u, std::min<unsigned int>(
		std::thread::hardware_concurrency(), static_cast<unsigned int>(grid.points.size())));
	std::vector<std::thread> workers;
	workers.reserve(threadCount);
	for (unsigned int thread = 0; thread < threadCount; ++thread) {
		workers.emplace_back([&]() {
			while (true) {
				const size_t point = nextPoint.fetch_add(1);
				if (point >= grid.points.size()) {
					break;
				}
				ComplexVector electric{};
				ComplexVector magnetic{};
				const Vector& observation = grid.points[point].x;
				for (size_t edge = 0; edge < model.connections.size(); ++edge) {
					const EdgeIntegrals electricSource = EvaluateEdgeIntegrals(
						model, model.connections[edge], source.currents[edge], observation, waveNumber);
					AddScaled(electric, electricSource.dot, -lMu0 * source.frequency);
					AddScaled(magnetic, electricSource.curl, 1.0);

					const EdgeIntegrals magneticSource = EvaluateEdgeIntegrals(
						model, model.connections[edge], source.magneticCurrents[edge], observation, waveNumber);
					AddScaled(electric, magneticSource.curl, -1.0);
					AddScaled(magnetic, magneticSource.dot, -er * lEpsilon0 * source.frequency);
				}
				for (int mesh = 0; mesh < model.pecMeshCount; ++mesh) {
					const ComplexVector electricGradient = EvaluateGradient(
						model, model.meshes[mesh], electricCharge[mesh], observation, waveNumber);
					AddScaled(electric, electricGradient, -1.0 / (er * lEpsilon0 * source.frequency));

					const ComplexVector magneticGradient = EvaluateGradient(
						model, model.meshes[mesh], magneticCharge[mesh], observation, waveNumber);
					AddScaled(magnetic, magneticGradient, -1.0 / (lMu0 * source.frequency));
				}
				fields.electric[point] = electric;
				fields.magnetic[point] = magnetic;
			}
		});
	}
	for (std::thread& worker : workers) {
		worker.join();
	}
	return fields;
}

void WriteMesh(std::ostream& output, const Model& model) {
	output << "$MeshFormat\n2.2 0 8\n$EndMeshFormat\n";
	output << "$Nodes\n" << model.points.size() << '\n';
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < model.points.size(); ++i) {
		output << (i + 1) << ' '
			<< model.points[i].x[0] * model.dimensionScale << ' '
			<< model.points[i].x[1] * model.dimensionScale << ' '
			<< model.points[i].x[2] * model.dimensionScale << '\n';
	}
	output << "$EndNodes\n";
	output << "$Elements\n" << model.pecMeshCount << '\n';
	for (int i = 0; i < model.pecMeshCount; ++i) {
		output << (i + 1) << " 2 2 1 1 "
			<< (model.meshes[i].point[0] + 1) << ' '
			<< (model.meshes[i].point[1] + 1) << ' '
			<< (model.meshes[i].point[2] + 1) << '\n';
	}
	output << "$EndElements\n";
}

void WriteMesh(std::ostream& output, const FieldGrid& grid, double dimensionScale) {
	output << "$MeshFormat\n2.2 0 8\n$EndMeshFormat\n";
	output << "$Nodes\n" << grid.points.size() << '\n';
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < grid.points.size(); ++i) {
		output << (i + 1) << ' '
			<< grid.points[i].x[0] * dimensionScale << ' '
			<< grid.points[i].x[1] * dimensionScale << ' '
			<< grid.points[i].x[2] * dimensionScale << '\n';
	}
	output << "$EndNodes\n";
	output << "$Elements\n" << grid.triangles.size() << '\n';
	for (size_t i = 0; i < grid.triangles.size(); ++i) {
		output << (i + 1) << " 2 2 1 1 "
			<< (grid.triangles[i][0] + 1) << ' '
			<< (grid.triangles[i][1] + 1) << ' '
			<< (grid.triangles[i][2] + 1) << '\n';
	}
	output << "$EndElements\n";
}

void WriteNodeDataHeader(std::ostream& output, const std::string& name, double frequency, int components, size_t points) {
	output << "$NodeData\n1\n\"" << name << "\"\n1\n" << frequency << "\n3\n0\n"
		<< components << '\n' << points << '\n';
}

void WriteVectorPlot(const fs::path& path, const Model& model, const Source& source,
	const std::vector<ComplexVector>& values, const std::string& name, bool imaginary) {
	std::ofstream output(path);
	if (!output.is_open()) {
		throw std::runtime_error("Cannot open plot file for writing: " + path.string());
	}
	WriteMesh(output, model);
	WriteNodeDataHeader(output, name + (imaginary ? " Imaginary" : " Real"), source.frequency, 3, values.size());
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < values.size(); ++i) {
		output << (i + 1);
		for (const Complex& component : values[i]) {
			output << ' ' << (imaginary ? component.imag() : component.real());
		}
		output << '\n';
	}
	output << "$EndNodeData\n";
}

void WriteScalarPlot(const fs::path& path, const Model& model, const Source& source,
	const std::vector<Complex>& values, const std::string& name, int part) {
	std::ofstream output(path);
	if (!output.is_open()) {
		throw std::runtime_error("Cannot open plot file for writing: " + path.string());
	}
	WriteMesh(output, model);
	WriteNodeDataHeader(output, name, source.frequency, 1, values.size());
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < values.size(); ++i) {
		const double value = part == 0 ? std::abs(values[i]) : (part == 1 ? values[i].real() : values[i].imag());
		output << (i + 1) << ' ' << value << '\n';
	}
	output << "$EndNodeData\n";
}

void WriteVectorPlot(const fs::path& path, const FieldGrid& grid, double dimensionScale, const Source& source,
	const std::vector<ComplexVector>& values, const std::string& name, bool imaginary) {
	std::ofstream output(path);
	if (!output.is_open()) {
		throw std::runtime_error("Cannot open space field plot file for writing: " + path.string());
	}
	WriteMesh(output, grid, dimensionScale);
	WriteNodeDataHeader(output, name + (imaginary ? " Imaginary" : " Real"), source.frequency, 3, values.size());
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < values.size(); ++i) {
		output << (i + 1);
		for (const Complex& component : values[i]) {
			output << ' ' << (imaginary ? component.imag() : component.real());
		}
		output << '\n';
	}
	output << "$EndNodeData\n";
}

void WriteMagnitudePlot(const fs::path& path, const FieldGrid& grid, double dimensionScale, const Source& source,
	const std::vector<ComplexVector>& values, const std::string& name) {
	std::ofstream output(path);
	if (!output.is_open()) {
		throw std::runtime_error("Cannot open space field plot file for writing: " + path.string());
	}
	WriteMesh(output, grid, dimensionScale);
	WriteNodeDataHeader(output, name, source.frequency, 1, values.size());
	output << std::scientific << std::setprecision(12);
	for (size_t i = 0; i < values.size(); ++i) {
		double magnitudeSquared = 0.0;
		for (const Complex& component : values[i]) {
			magnitudeSquared += std::norm(component);
		}
		output << (i + 1) << ' ' << std::sqrt(magnitudeSquared) << '\n';
	}
	output << "$EndNodeData\n";
}

fs::path ParentPath(const fs::path& path) {
	return path.has_parent_path() ? path.parent_path() : path;
}

fs::path ExecutableDirectory() {
	char buffer[MAX_PATH];
	const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	if (length == 0 || length >= MAX_PATH) {
		return fs::current_path();
	}
	return fs::path(buffer).parent_path();
}

void WriteSelfTestInput(const fs::path& dataRoot) {
	fs::create_directories(dataRoot / "COMMON_DATA");
	{
		std::ofstream model(dataRoot / "COMMON_DATA" / "Post_Model.txt");
		model << "PEEC_POST_MODEL_V1\n"
			<< "DIM 1\n"
			<< "POINTS 4\n"
			<< "0 0 0 0\n1 1 0 0\n2 0 1 0\n3 1 1 0\n"
			<< "MESHES 2 2 0\n"
			<< "0 1 2 0.5 0.333333333333 0.333333333333 0\n"
			<< "1 3 2 0.5 0.666666666667 0.666666666667 0\n"
			<< "CONNECTIONS 1\n"
			<< "0 1 0 1\n";
	}
	{
		std::ofstream source(dataRoot / "COMMON_DATA" / "Post_Source.txt");
		source << "PEEC_POST_SOURCE_V1\n"
			<< "FREQUENCY 1000000000\n"
			<< "PORT 1\n"
			<< "CURRENTS 1\n1 0.5\n"
			<< "VOLTAGES 2\n2 1\n3 -1\n"
			<< "MAGNETIC_CURRENTS 1\n0.25 -0.5\n"
			<< "MAGNETIC_VOLTAGES 2\n-2 0.5\n4 -1\n";
	}
	{
		std::ofstream settings(dataRoot / "set.txt");
		settings << "POST_PROCESSING 1\n"
			<< "POST_FIELD 1\n"
			<< "POST_FIELD_ER 1\n"
			<< "$End_Def\n";
	}
	{
		std::ofstream grid(dataRoot / "Space_Mesh.msh");
		grid << "$MeshFormat\n2.2 0 8\n$EndMeshFormat\n"
			<< "$Nodes\n3\n"
			<< "1 0 0 1\n2 1 0 1\n3 0 1 1\n"
			<< "$EndNodes\n"
			<< "$Elements\n1\n"
			<< "1 2 2 1 1 1 2 3\n"
			<< "$EndElements\n";
	}
}

} // namespace

std::filesystem::path ResolveDefaultDataRoot() {
	const fs::path executableDirectory = ExecutableDirectory();
	const fs::path currentDirectory = fs::current_path();
	const fs::path candidates[] = {
		executableDirectory / "Data",
		currentDirectory / "Data",
		ParentPath(executableDirectory) / "Data",
		ParentPath(currentDirectory) / "Data",
	};
	for (const fs::path& candidate : candidates) {
		if (fs::is_directory(candidate)) {
			return fs::absolute(candidate);
		}
	}
	return fs::absolute(executableDirectory / "Data");
}

void RunPostProcessing(const std::filesystem::path& dataRoot) {
	const fs::path absoluteDataRoot = fs::absolute(dataRoot);
	const Settings settings = ReadSettings(absoluteDataRoot / "set.txt");
	std::cout << "[Post Processing]\n";
	std::cout << "  Data root: " << absoluteDataRoot.string() << '\n';
	if (!settings.enabled) {
		std::cout << "  Disabled by POST_PROCESSING=0.\n";
		return;
	}

	const Model model = ReadModel(absoluteDataRoot / "COMMON_DATA" / "Post_Model.txt");
	const Source source = ReadSource(absoluteDataRoot / "COMMON_DATA" / "Post_Source.txt");
	if (source.voltages.size() != static_cast<size_t>(model.pecMeshCount)) {
		throw std::runtime_error("Voltage count does not match PEC mesh count.");
	}
	if (source.magneticVoltages.size() != static_cast<size_t>(model.pecMeshCount)) {
		throw std::runtime_error("Magnetic voltage count does not match PEC mesh count.");
	}

	const fs::path outputDirectory = absoluteDataRoot / "Source_Plot";
	fs::create_directories(outputDirectory);
	if (settings.plotCurrent) {
		const std::vector<ComplexVector> pointCurrent = ConstructPointCurrent(model, source.currents);
		WriteVectorPlot(outputDirectory / "J_vec.msh_Re", model, source, pointCurrent, "J", false);
		WriteVectorPlot(outputDirectory / "J_vec.msh_Im", model, source, pointCurrent, "J", true);
	}
	if (settings.plotCharge) {
		const std::vector<Complex> pointCharge = ConstructPointScalar(model, ConstructChargeDensity(model, source.currents));
		WriteScalarPlot(outputDirectory / "Qe.msh_Mag", model, source, pointCharge, "Qe Magnitude", 0);
		WriteScalarPlot(outputDirectory / "Qe.msh_Re", model, source, pointCharge, "Qe Real", 1);
		WriteScalarPlot(outputDirectory / "Qe.msh_Im", model, source, pointCharge, "Qe Imaginary", 2);
	}
	if (settings.plotVoltage) {
		const std::vector<Complex> pointVoltage = ConstructPointScalar(model, source.voltages);
		WriteScalarPlot(outputDirectory / "Ve.msh_Re", model, source, pointVoltage, "Ve Real", 1);
		WriteScalarPlot(outputDirectory / "Ve.msh_Im", model, source, pointVoltage, "Ve Imaginary", 2);
	}
	if (settings.plotMagneticCurrent) {
		const std::vector<ComplexVector> pointCurrent = ConstructPointCurrent(model, source.magneticCurrents);
		WriteVectorPlot(outputDirectory / "M_vec.msh_Re", model, source, pointCurrent, "M", false);
		WriteVectorPlot(outputDirectory / "M_vec.msh_Im", model, source, pointCurrent, "M", true);
	}
	if (settings.plotMagneticCharge) {
		const std::vector<Complex> pointCharge = ConstructPointScalar(model, ConstructChargeDensity(model, source.magneticCurrents));
		WriteScalarPlot(outputDirectory / "Qm.msh_Mag", model, source, pointCharge, "Qm Magnitude", 0);
		WriteScalarPlot(outputDirectory / "Qm.msh_Re", model, source, pointCharge, "Qm Real", 1);
		WriteScalarPlot(outputDirectory / "Qm.msh_Im", model, source, pointCharge, "Qm Imaginary", 2);
	}
	if (settings.plotMagneticVoltage) {
		const std::vector<Complex> pointVoltage = ConstructPointScalar(model, source.magneticVoltages);
		WriteScalarPlot(outputDirectory / "Vm.msh_Re", model, source, pointVoltage, "Vm Real", 1);
		WriteScalarPlot(outputDirectory / "Vm.msh_Im", model, source, pointVoltage, "Vm Imaginary", 2);
	}
	if (settings.plotSpaceField) {
		const FieldGrid grid = ReadFieldGrid(absoluteDataRoot / "Space_Mesh.msh", model.dimensionScale);
		std::cout << "  Evaluating space field at " << grid.points.size() << " points...\n";
		const SpaceFields fields = EvaluateSpaceFields(model, source, grid, settings.fieldEr);
		const fs::path fieldDirectory = absoluteDataRoot / "Field_Plot";
		fs::create_directories(fieldDirectory);
		WriteVectorPlot(fieldDirectory / "E_vec.msh_Re", grid, model.dimensionScale, source, fields.electric, "E", false);
		WriteVectorPlot(fieldDirectory / "E_vec.msh_Im", grid, model.dimensionScale, source, fields.electric, "E", true);
		WriteMagnitudePlot(fieldDirectory / "E_scalar.msh_Mag", grid, model.dimensionScale, source, fields.electric, "E Magnitude");
		WriteVectorPlot(fieldDirectory / "H_vec.msh_Re", grid, model.dimensionScale, source, fields.magnetic, "H", false);
		WriteVectorPlot(fieldDirectory / "H_vec.msh_Im", grid, model.dimensionScale, source, fields.magnetic, "H", true);
		WriteMagnitudePlot(fieldDirectory / "H_scalar.msh_Mag", grid, model.dimensionScale, source, fields.magnetic, "H Magnitude");
		std::cout << "  Field output: " << fieldDirectory.string() << '\n';
	}

	std::cout << "  Frequency: " << source.frequency << '\n';
	std::cout << "  Excitation port: " << source.port << '\n';
	std::cout << "  Output: " << outputDirectory.string() << '\n';
}

void RunPostProcessingSelfTest() {
	const fs::path dataRoot = fs::temp_directory_path() / "peec_post_processing_self_test";
	fs::remove_all(dataRoot);
	WriteSelfTestInput(dataRoot);
	RunPostProcessing(dataRoot);

	const fs::path outputDirectory = dataRoot / "Source_Plot";
	const char* expectedFiles[] = {
		"J_vec.msh_Re", "J_vec.msh_Im",
		"Qe.msh_Mag", "Qe.msh_Re", "Qe.msh_Im",
		"Ve.msh_Re", "Ve.msh_Im",
		"M_vec.msh_Re", "M_vec.msh_Im",
		"Qm.msh_Mag", "Qm.msh_Re", "Qm.msh_Im",
		"Vm.msh_Re", "Vm.msh_Im",
	};
	for (const char* expectedFile : expectedFiles) {
		if (!fs::is_regular_file(outputDirectory / expectedFile)) {
			throw std::runtime_error(std::string("Self-test output is missing: ") + expectedFile);
		}
	}
	const fs::path fieldDirectory = dataRoot / "Field_Plot";
	const char* expectedFieldFiles[] = {
		"E_vec.msh_Re", "E_vec.msh_Im", "E_scalar.msh_Mag",
		"H_vec.msh_Re", "H_vec.msh_Im", "H_scalar.msh_Mag",
	};
	for (const char* expectedFile : expectedFieldFiles) {
		if (!fs::is_regular_file(fieldDirectory / expectedFile)) {
			throw std::runtime_error(std::string("Self-test field output is missing: ") + expectedFile);
		}
	}

	fs::remove_all(dataRoot);
	std::cout << "[PASS] Post-processing self-test generated and cleaned 20 plot files.\n";
}
