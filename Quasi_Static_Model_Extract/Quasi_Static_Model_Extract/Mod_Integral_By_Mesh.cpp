#include "Mod_Integral_By_Mesh.h"

#include <utility>

namespace {

	constexpr int kTriangleNodeCount = 3;
	constexpr int kNearFieldGaussOrder = 9;

	using Matrix3 = std::array<std::array<double, kTriangleNodeCount>, kTriangleNodeCount>;

	void TransposeMatrix(Matrix3& matrix) {
		for (int row = 0; row < kTriangleNodeCount; ++row) {
			for (int col = row + 1; col < kTriangleNodeCount; ++col) {
				std::swap(matrix[row][col], matrix[col][row]);
			}
		}
	}

	Vector MakeVectorFromPoints(const Vector& from, const Vector& to) {
		Vector result{};
		for (int axis = 0; axis < kTriangleNodeCount; ++axis) {
			result[axis] = to[axis] - from[axis];
		}
		return result;
	}

	Vector InterpolateTrianglePoint(const std::array<Point, 3>& vertices, int order, int gauss_index) {
		Vector point{};
		for (int axis = 0; axis < kTriangleNodeCount; ++axis) {
			for (int vertex = 0; vertex < kTriangleNodeCount; ++vertex) {
				point[axis] += vertices[vertex].X[axis] * SG[order][gauss_index][vertex];
			}
		}
		return point;
	}

	void LoadTriangleVertices(const Mesh& mesh, std::array<Point, 3>& vertices) {
		for (int i = 0; i < kTriangleNodeCount; ++i) {
			vertices[i] = PT[mesh.P[i]];
		}
	}

} // namespace

int N_Gaussin_P(double R, double Lamda) {
	const double bounded_r = MIN(MAX(R, 0.1 * Lamda), 0.5 * Lamda);
	return static_cast<int>(3.0 * std::log(0.5 * Lamda / bounded_r) + 1.0);
}

Ele_Mesh_Int Trans_Ele(int IND_A, int IND_B) {
	Ele_Mesh_Int result;
	if ((Sur_M[IND_A].M_TYP[0] != Sur_M[IND_A].M_TYP[1]) && Sur_M[IND_A].IS_PEC == 1) {
		result = Ele_Cal(IND_A, IND_B);
	}
	else {
		result = MS_ELE[IND_B][IND_A];
		TransposeMatrix(result.L);
		TransposeMatrix(result.C);
		TransposeMatrix(result.CS);
	}
	return result;
}

Ele_Mesh_Int Ele_Cal(int IND_A, int IND_B) {
	Ele_Mesh_Int result;
	const double distance = Distance(Sur_M[IND_A].MID_CO, Sur_M[IND_B].MID_CO);
	if (distance < 2 * MAX_L) {
		result = Ele_Integral_T(IND_A, IND_B);
	}
	else {
		result = Ele_Integral_F(IND_A, IND_B);
	}

	//if (IND_A == IND_B) {
	//	for (int i = 0; i < kTriangleNodeCount; ++i) {
	//		for (int j = 0; j < kTriangleNodeCount; ++j) {
	//			result.CS[i][j] = 0.0;
	//		}
	//	}
	//}

	return result;
}

Ele_Mesh_Int Ele_Integral_T(int IND_A, int IND_B) {
	std::array<Point, 3> V_PA;
	std::array<Point, 3> V_PB;
	std::array<Vector, 3> V_A;

	Ele_Mesh_Int MS_Ele;
	FKNResult result_F_K_N{};

	std::array<Vector, 3> LC_SINGU{};
	double P_SINGU = 0.0;

	Matrix3 L_TEMP{};
	double P_TEMP = 0.0;

	const Mesh& MS_A = Sur_M[IND_A];
	const Mesh& MS_B = Sur_M[IND_B];

	LoadTriangleVertices(MS_A, V_PA);
	LoadTriangleVertices(MS_B, V_PB);

	const int num_gauss_points = SN[kNearFieldGaussOrder];
	for (int gauss_index = 0; gauss_index < num_gauss_points; ++gauss_index) {
		const Vector XA = InterpolateTrianglePoint(V_PA, kNearFieldGaussOrder, gauss_index);
		Point integration_point{};
		integration_point.X = XA;

		for (int i = 0; i < kTriangleNodeCount; ++i) {
			V_A[i] = MakeVectorFromPoints(V_PA[i].X, XA);
		}

		result_F_K_N = F_K_N(V_PB, integration_point);
		LC_SINGU = result_F_K_N.L_BUF;
		P_SINGU = result_F_K_N.P_BUF;

		for (int i = 0; i < kTriangleNodeCount; ++i) {
			for (int j = 0; j < kTriangleNodeCount; ++j) {
				L_TEMP[i][j] += SW[kNearFieldGaussOrder][gauss_index] * V_Dot(V_A[i], LC_SINGU[j]);
			}
		}
		P_TEMP += SW[kNearFieldGaussOrder][gauss_index] * P_SINGU;
	}

	for (int i = 0; i < kTriangleNodeCount; ++i) {
		for (int j = 0; j < kTriangleNodeCount; ++j) {
			L_TEMP[i][j] /= MS_B.AREA;
			MS_Ele.L[i][j] = L_TEMP[i][j] / (16.0 * PI);
		}
	}
	P_TEMP /= MS_B.AREA;
	MS_Ele.P = P_TEMP / (4.0 * PI);

	return MS_Ele;
}

Ele_Mesh_Int Ele_Integral_F(int IND_A, int IND_B) {
	std::array<Point, 3> V_PA;
	std::array<Point, 3> V_PB;
	Vector XA{};
	Vector XB{};

	Matrix3 L_TEMP{};
	double P_TEMP = 0.0;

	const Mesh& MS_A = Sur_M[IND_A];
	const Mesh& MS_B = Sur_M[IND_B];

	LoadTriangleVertices(MS_A, V_PA);
	LoadTriangleVertices(MS_B, V_PB);

	const double distance = Distance(MS_A.MID_CO, MS_B.MID_CO);
	const int order_a = N_Gaussin_P(distance, LAMDA_E);
	const int order_b = N_Gaussin_P(distance, LAMDA_E);
	const int num_gauss_points_A = SN[order_a];
	const int num_gauss_points_B = SN[order_b];

	for (int gauss_a = 0; gauss_a < num_gauss_points_A; ++gauss_a) {
		XA = InterpolateTrianglePoint(V_PA, order_a, gauss_a);
		std::array<Vector, 3> V_A{};
		for (int i = 0; i < kTriangleNodeCount; ++i) {
			V_A[i] = MakeVectorFromPoints(V_PA[i].X, XA);
		}

		for (int gauss_b = 0; gauss_b < num_gauss_points_B; ++gauss_b) {
			XB = InterpolateTrianglePoint(V_PB, order_b, gauss_b);
			std::array<Vector, 3> V_B{};
			for (int i = 0; i < kTriangleNodeCount; ++i) {
				V_B[i] = MakeVectorFromPoints(V_PB[i].X, XB);
			}

			const double R = Distance(XA, XB);
			const double weight = SW[order_a][gauss_a] * SW[order_b][gauss_b] / R;

			P_TEMP += weight;
			for (int i = 0; i < kTriangleNodeCount; ++i) {
				for (int j = 0; j < kTriangleNodeCount; ++j) {
					L_TEMP[i][j] += weight * V_Dot(V_A[i], V_B[j]);
				}
			}
		}
	}

	Ele_Mesh_Int MS_Ele;
	for (int i = 0; i < kTriangleNodeCount; ++i) {
		for (int j = 0; j < kTriangleNodeCount; ++j) {
			MS_Ele.L[i][j] = L_TEMP[i][j] / (16.0 * PI);
		}
	}
	MS_Ele.P = P_TEMP / (4.0 * PI);

	return MS_Ele;
}
