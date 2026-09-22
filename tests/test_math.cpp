#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mesh.h"
#include "shapes.h"

// Pure math tests. No window, no GL, no files. Three groups:
//
//   Shapes     - every generator in shapes.h, checked on its face list before it ever
//                reaches HalfEdgeMesh, so a failure here is the generator's fault.
//   Helpers    - the free functions in helper.h.
//   Transform  - getTransform, getTransform3x3, WorldDirToLocalDir, and the Euler
//                angle conversion the rotate gizmo depends on.
namespace {

	// --- shared helpers ----------------------------------------------------

	// Direct results (one sin/cos away from the answer).
	constexpr float kEps = 1e-5f;
	// Results that went through a matrix product or an inverse.
	constexpr float kLooseEps = 1e-4f;

	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	// gtest can't find helper.h's operator<< for cy::Vec3 (argument-dependent lookup
	// only searches namespace cy), so vectors go through a string first.
	std::string Str(const Vec3f& v) {
		std::ostringstream out;
		out << v;
		return out.str();
	}

	::testing::AssertionResult Near(const Vec3f& a, const Vec3f& b, float eps) {
		if (std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps && std::abs(a.z - b.z) < eps)
			return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << "(" << Str(a) << ") vs (" << Str(b) << ")";
	}

	::testing::AssertionResult Near(const Matrix3f& a, const Matrix3f& b, float eps) {
		for (int i = 0; i < 9; ++i) {
			if (!(std::abs(a.cell[i] - b.cell[i]) < eps))
				return ::testing::AssertionFailure() << "cell[" << i << "]: " << a.cell[i] << " vs " << b.cell[i];
		}
		return ::testing::AssertionSuccess();
	}

	Matrix3f EulerToMatrix(const Vec3f& degrees) {
		Matrix3f m;
		m.SetRotationZYX(degreesToRadians(degrees.x), degreesToRadians(degrees.y), degreesToRadians(degrees.z));
		return m;
	}

	// --- face list checks --------------------------------------------------

	struct FaceList3 {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
	};

	std::vector<uint> Corners(const Vec4u& face) {
		std::vector<uint> c = { face.x, face.y, face.z };
		if (face.w != MAX_UINT) c.push_back(face.w);
		return c;
	}

	// Newell's method: works for quads that aren't perfectly planar, and its length
	// is twice the face's area.
	Vec3f NewellNormal(const FaceList3& s, const Vec4u& face) {
		std::vector<uint> c = Corners(face);
		Vec3f n(0.0f);
		for (size_t i = 0; i < c.size(); ++i) {
			const Vec3f& a = s.verts[c[i]];
			const Vec3f& b = s.verts[c[(i + 1) % c.size()]];
			n.x += (a.y - b.y) * (a.z + b.z);
			n.y += (a.z - b.z) * (a.x + b.x);
			n.z += (a.x - b.x) * (a.y + b.y);
		}
		return n;
	}

	Vec3f Centroid(const FaceList3& s, const Vec4u& face) {
		std::vector<uint> c = Corners(face);
		Vec3f sum(0.0f);
		for (uint v : c) sum += s.verts[v];
		return sum / float(c.size());
	}

	// Where "outward" is measured from. Convex shapes use their vertex average; the
	// torus uses the nearest point on its center ring, since its center is a hole.
	enum class Center { VertexAverage, TorusRing };

	struct ShapeCase {
		std::string name;
		FaceList3 shape;
		uint verts;
		uint faces;
		uint edges; // undirected
		int eulerCharacteristic;
		Center center;
		float torusR;
	};

	void PrintTo(const ShapeCase& c, std::ostream* out) { *out << c.name; }

	template <size_t V, size_t F>
	FaceList3 FromArrays(const std::array<Vec3f, V>& v, const std::array<Vec4u, F>& f) {
		return { std::vector<Vec3f>(v.begin(), v.end()), std::vector<Vec4u>(f.begin(), f.end()) };
	}

	ShapeCase Cylinder(uint n) {
		FaceList3 s;
		generateUnitCylinder(s.verts, s.faces, n);
		return { "Cylinder" + std::to_string(n), s, 2 * n, 3 * n - 4, 5 * n - 6, 2, Center::VertexAverage, 0.0f };
	}

	ShapeCase UVSphere(uint lat, uint lon) {
		FaceList3 s;
		generateUVSphere(s.verts, s.faces, lat, lon);
		return { "UVSphere" + std::to_string(lat) + "x" + std::to_string(lon), s,
			2 + (lat - 1) * lon, 2 * lon * (lat - 1), 3 * lon * (lat - 1), 2, Center::VertexAverage, 0.0f };
	}

	ShapeCase Torus(uint u, uint v, float R, float r) {
		FaceList3 s;
		generateUnitTorus(s.verts, s.faces, u, v, R, r);
		return { "Torus" + std::to_string(u) + "x" + std::to_string(v), s,
			u * v, 2 * u * v, 3 * u * v, 0, Center::TorusRing, R };
	}

	// The sizes the app uses (see displayCylinder etc. in viewport.cpp) plus the
	// smallest each generator accepts, which is where off-by-ones live.
	std::vector<ShapeCase> AllShapes() {
		return {
			{ "Cube", FromArrays(CUBE_VERTICES, CUBE_FACES), 8, 6, 12, 2, Center::VertexAverage, 0.0f },
			{ "Pyramid", FromArrays(PYRAMID_VERTICES, PYRAMID_FACES), 5, 5, 8, 2, Center::VertexAverage, 0.0f },
			{ "Icosphere", { ICOSPHERE_VERTICES, ICOSPHERE_FACES }, 12, 20, 30, 2, Center::VertexAverage, 0.0f },
			Cylinder(3),
			Cylinder(4),
			Cylinder(12),
			UVSphere(2, 3),
			UVSphere(3, 4),
			UVSphere(10, 10),
			Torus(3, 3, 1.0f, 0.3f),
			Torus(40, 20, 1.0f, 0.3f),
		};
	}

	class ShapeTest : public ::testing::TestWithParam<ShapeCase> {};
}

// --- Shapes: checks every shape must pass -----------------------------------

TEST_P(ShapeTest, CountsMatchFormula) {
	const ShapeCase& c = GetParam();
	EXPECT_EQ(c.shape.verts.size(), c.verts);
	EXPECT_EQ(c.shape.faces.size(), c.faces);
}

TEST_P(ShapeTest, IndicesInRange) {
	const ShapeCase& c = GetParam();
	const uint V = uint(c.shape.verts.size());
	for (size_t f = 0; f < c.shape.faces.size(); ++f) {
		const Vec4u& face = c.shape.faces[f];
		EXPECT_LT(face.x, V) << "face " << f;
		EXPECT_LT(face.y, V) << "face " << f;
		EXPECT_LT(face.z, V) << "face " << f;
		EXPECT_TRUE(face.w == MAX_UINT || face.w < V) << "face " << f;
	}
}

TEST_P(ShapeTest, NoDegenerateFaces) {
	const ShapeCase& c = GetParam();
	for (size_t f = 0; f < c.shape.faces.size(); ++f) {
		std::vector<uint> corners = Corners(c.shape.faces[f]);
		for (size_t i = 0; i < corners.size(); ++i)
			for (size_t j = i + 1; j < corners.size(); ++j)
				EXPECT_NE(corners[i], corners[j]) << "face " << f << " repeats a vertex";

		float area = NewellNormal(c.shape, c.shape.faces[f]).Length() * 0.5f;
		EXPECT_GT(area, 1e-6f) << "face " << f << " has zero area";
	}
}

// Every directed edge a->b appears once, and so does b->a. Fails on a hole, a
// duplicated face, or a face wound the opposite way to its neighbours.
TEST_P(ShapeTest, ClosedAndConsistentlyWound) {
	const ShapeCase& c = GetParam();
	std::map<std::pair<uint, uint>, int> directed;
	for (const Vec4u& face : c.shape.faces) {
		std::vector<uint> corners = Corners(face);
		for (size_t i = 0; i < corners.size(); ++i)
			++directed[{ corners[i], corners[(i + 1) % corners.size()] }];
	}

	for (const auto& [edge, count] : directed) {
		EXPECT_EQ(count, 1) << edge.first << "->" << edge.second << " is used by " << count << " faces";
		auto twin = directed.find({ edge.second, edge.first });
		EXPECT_TRUE(twin != directed.end()) << edge.first << "->" << edge.second << " has no opposite edge";
	}

	EXPECT_EQ(directed.size(), 2 * size_t(c.edges));
}

//AI Documentation: AI wrote every test below. I wrote the scaffolding above

TEST_P(ShapeTest, EulerCharacteristic) {
	const ShapeCase& c = GetParam();
	int V = int(c.shape.verts.size());
	int F = int(c.shape.faces.size());
	EXPECT_EQ(V - int(c.edges) + F, c.eulerCharacteristic);
}

TEST_P(ShapeTest, FacesPointOutward) {
	const ShapeCase& c = GetParam();

	Vec3f average(0.0f);
	for (const Vec3f& v : c.shape.verts) average += v;
	average /= float(c.shape.verts.size());

	for (size_t f = 0; f < c.shape.faces.size(); ++f) {
		Vec3f centroid = Centroid(c.shape, c.shape.faces[f]);
		Vec3f center = average;
		if (c.center == Center::TorusRing) {
			// Each vertex's tube center sits on the ring directly under it. Averaging
			// those per face, rather than taking the nearest point on a perfect circle,
			// stays correct when a low resolution makes the ring a polygon.
			std::vector<uint> corners = Corners(c.shape.faces[f]);
			center = Vec3f(0.0f);
			for (uint v : corners) {
				const Vec3f& p = c.shape.verts[v];
				center += Vec3f(p.x, 0.0f, p.z).GetNormalized() * c.torusR;
			}
			center /= float(corners.size());
		}

		EXPECT_GT(NewellNormal(c.shape, c.shape.faces[f]).Dot(centroid - center), 0.0f)
			<< "face " << f << " points inward";
	}
}

// Bridges to the half-edge invariants: a face list that passes everything above
// should load into a valid mesh with one half-edge per directed edge.
TEST_P(ShapeTest, LoadsIntoValidMesh) {
	const ShapeCase& c = GetParam();
	HalfEdgeMesh mesh;
	mesh.load(c.shape.verts, c.shape.faces);

	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), c.verts);
	EXPECT_EQ(mesh.FaceCount(), c.faces);
	EXPECT_EQ(mesh.EdgeCount(), 2 * c.edges);
}

INSTANTIATE_TEST_SUITE_P(Shapes, ShapeTest, ::testing::ValuesIn(AllShapes()),
	[](const ::testing::TestParamInfo<ShapeCase>& info) { return info.param.name; });

// --- Shapes: checks specific to one shape -----------------------------------

TEST(ShapeGeometry, UVSphereVerticesOnUnitSphere) {
	for (auto [lat, lon] : { std::pair<uint, uint>{ 2, 3 }, { 3, 4 }, { 10, 10 }, { 17, 31 } }) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		generateUVSphere(verts, faces, lat, lon);
		for (size_t i = 0; i < verts.size(); ++i)
			EXPECT_NEAR(verts[i].Length(), 1.0f, kEps) << lat << "x" << lon << " vertex " << i;
	}
}

// The icosphere is typed in by hand to ~6 digits, so it gets a looser epsilon.
TEST(ShapeGeometry, IcosphereVerticesOnUnitSphere) {
	for (size_t i = 0; i < ICOSPHERE_VERTICES.size(); ++i)
		EXPECT_NEAR(ICOSPHERE_VERTICES[i].Length(), 1.0f, kLooseEps) << "vertex " << i;
}

TEST(ShapeGeometry, IcosphereEdgesAllSameLength) {
	std::vector<float> lengths;
	for (const Vec4u& face : ICOSPHERE_FACES) {
		std::vector<uint> c = Corners(face);
		for (size_t i = 0; i < c.size(); ++i)
			lengths.push_back((ICOSPHERE_VERTICES[c[i]] - ICOSPHERE_VERTICES[c[(i + 1) % c.size()]]).Length());
	}
	for (float len : lengths) EXPECT_NEAR(len, lengths[0], 1e-3f);
}

TEST(ShapeGeometry, CylinderVerticesOnUnitCylinder) {
	for (uint n : { 3u, 4u, 12u, 64u }) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		generateUnitCylinder(verts, faces, n);
		for (size_t i = 0; i < verts.size(); ++i) {
			const Vec3f& p = verts[i];
			EXPECT_NEAR(p.x * p.x + p.z * p.z, 1.0f, kEps) << "n=" << n << " vertex " << i;
			EXPECT_EQ(std::abs(p.y), 1.0f) << "n=" << n << " vertex " << i;
		}
	}
}

TEST(ShapeGeometry, TorusVerticesOnTorusSurface) {
	const float R = 1.0f, r = 0.3f;
	std::vector<Vec3f> verts;
	std::vector<Vec4u> faces;
	generateUnitTorus(verts, faces, 40, 20, R, r);
	for (size_t i = 0; i < verts.size(); ++i) {
		const Vec3f& p = verts[i];
		float ring = std::sqrt(p.x * p.x + p.z * p.z) - R;
		EXPECT_NEAR(ring * ring + p.y * p.y, r * r, kEps) << "vertex " << i;
	}
}

TEST(ShapeGeometry, CylinderOverloadsAgree) {
	std::array<Vec3f, 24> arrVerts;
	std::array<Vec4u, 32> arrFaces;
	generateUnitCylinder(arrVerts, arrFaces, 12);

	std::vector<Vec3f> vecVerts;
	std::vector<Vec4u> vecFaces;
	generateUnitCylinder(vecVerts, vecFaces, 12);

	ASSERT_EQ(vecVerts.size(), arrVerts.size());
	ASSERT_EQ(vecFaces.size(), arrFaces.size());
	for (size_t i = 0; i < arrVerts.size(); ++i) EXPECT_TRUE(arrVerts[i] == vecVerts[i]) << "vertex " << i;
	for (size_t i = 0; i < arrFaces.size(); ++i) EXPECT_TRUE(arrFaces[i] == vecFaces[i]) << "face " << i;
}

// The UV sphere generators append, so a non-empty vector would shift every index.
// They assert instead. In a release build the assert is compiled out, which is why
// this is a DEBUG_DEATH: it only expects the crash where asserts exist.
TEST(ShapeGeometry, UVSphereRejectsNonEmptyVertices) {
	std::vector<Vec3f> verts = { Vec3f(0.0f) };
	std::vector<Vec4u> faces;
	EXPECT_DEBUG_DEATH(generateUVSphere(verts, faces, 3, 4), "");
}

TEST(ShapeGeometry, UVSphereRejectsNonEmptyFaces) {
	std::vector<Vec3f> verts;
	std::vector<Vec4u> faces = { Vec4u(0, 1, 2, MAX_UINT) };
	EXPECT_DEBUG_DEATH(generateUVSphere(verts, faces, 3, 4), "");
}

// --- Helpers ----------------------------------------------------------------

TEST(Helpers, DegreesRadiansKnownValues) {
	EXPECT_NEAR(degreesToRadians(180.0f), cy::Pi<float>(), kEps);
	EXPECT_NEAR(degreesToRadians(90.0f), cy::Pi<float>() / 2.0f, kEps);
	EXPECT_NEAR(degreesToRadians(-45.0f), -cy::Pi<float>() / 4.0f, kEps);
	EXPECT_NEAR(radiansToDegrees(cy::Pi<float>()), 180.0f, kLooseEps);
}

TEST(Helpers, DegreesRadiansRoundTrip) {
	for (float deg = -720.0f; deg <= 720.0f; deg += 7.5f)
		EXPECT_NEAR(radiansToDegrees(degreesToRadians(deg)), deg, kLooseEps);
}

TEST(Helpers, CompareFloat) {
	EXPECT_TRUE(compareFloat(1.0f, 1.05f, 0.1f));
	EXPECT_FALSE(compareFloat(1.0f, 1.2f, 0.1f));
	// Strict <: a distance exactly equal to epsilon counts as different, so a
	// value never equals itself with an epsilon of 0. Pinned so nobody trips on it.
	EXPECT_FALSE(compareFloat(1.0f, 1.0f, 0.0f));
	EXPECT_FALSE(compareFloat(0.0f, 0.5f, 0.5f));
}

TEST(Helpers, Clamp) {
	float below = -3.0f, above = 7.0f, inside = 0.5f, onLeft = 0.0f, onRight = 1.0f;
	clamp(below, 0.0f, 1.0f);
	clamp(above, 0.0f, 1.0f);
	clamp(inside, 0.0f, 1.0f);
	clamp(onLeft, 0.0f, 1.0f);
	clamp(onRight, 0.0f, 1.0f);
	EXPECT_EQ(below, 0.0f);
	EXPECT_EQ(above, 1.0f);
	EXPECT_EQ(inside, 0.5f);
	EXPECT_EQ(onLeft, 0.0f);
	EXPECT_EQ(onRight, 1.0f);
}

// The Wrap functions build next/prev links in mesh.cpp. Their argument orders differ:
//   WrapInc(x, upper, lower)       WrapDec(x, upper, lower)
//   WrapAdd(x, add, upper, lower)  WrapSub(x, sub, upper, lower)
//   WrapAdd(x, add, upper)         (lower is 0)
// These tests are the reference for which argument is which.
TEST(Helpers, WrapIncDecAtEnds) {
	EXPECT_EQ(WrapInc(1, 3), 2u);
	EXPECT_EQ(WrapInc(3, 3), 0u);
	EXPECT_EQ(WrapDec(2, 3), 1u);
	EXPECT_EQ(WrapDec(0, 3), 3u);

	// Range that doesn't start at 0, as mesh.cpp uses them.
	EXPECT_EQ(WrapInc(13, 13, 10), 10u);
	EXPECT_EQ(WrapInc(11, 13, 10), 12u);
	EXPECT_EQ(WrapDec(10, 13, 10), 13u);
	EXPECT_EQ(WrapDec(12, 13, 10), 11u);
}

TEST(Helpers, WrapAddSubStayInRangeAndInvert) {
	for (uint lower : { 0u, 10u }) {
		for (uint size = 1; size <= 8; ++size) {
			uint upper = lower + size - 1;
			for (uint x = lower; x <= upper; ++x) {
				for (uint k = 0; k <= 2 * size; ++k) {
					uint added = WrapAdd(x, k, upper, lower);
					uint subbed = WrapSub(x, k, upper, lower);

					EXPECT_GE(added, lower);
					EXPECT_LE(added, upper);
					EXPECT_GE(subbed, lower);
					EXPECT_LE(subbed, upper);
					EXPECT_EQ(WrapAdd(subbed, k, upper, lower), x) << "x=" << x << " k=" << k << " [" << lower << "," << upper << "]";
					EXPECT_EQ(WrapSub(added, k, upper, lower), x) << "x=" << x << " k=" << k << " [" << lower << "," << upper << "]";
					if (lower == 0) EXPECT_EQ(WrapAdd(x, k, upper), added);
				}
				EXPECT_EQ(WrapAdd(x, 1, upper, lower), WrapInc(x, upper, lower));
				EXPECT_EQ(WrapSub(x, 1, upper, lower), WrapDec(x, upper, lower));
			}
		}
	}
}

// coords = (angle around Y in degrees, angle from +Y in degrees, radius)
TEST(Helpers, SphericalToCartesianKnownValues) {
	EXPECT_TRUE(Near(sphericalToCartesian(Vec3f(37.0f, 0.0f, 2.0f)), Vec3f(0.0f, 2.0f, 0.0f), kEps));
	EXPECT_TRUE(Near(sphericalToCartesian(Vec3f(0.0f, 90.0f, 2.0f)), Vec3f(2.0f, 0.0f, 0.0f), kEps));
	EXPECT_TRUE(Near(sphericalToCartesian(Vec3f(90.0f, 90.0f, 2.0f)), Vec3f(0.0f, 0.0f, -2.0f), kEps));
	EXPECT_TRUE(Near(sphericalToCartesian(Vec3f(0.0f, 180.0f, 2.0f)), Vec3f(0.0f, -2.0f, 0.0f), kEps));
}

TEST(Helpers, SphericalToCartesianKeepsRadius) {
	for (float around = -180.0f; around <= 180.0f; around += 15.0f)
		for (float fromUp = 0.0f; fromUp <= 180.0f; fromUp += 15.0f)
			EXPECT_NEAR(sphericalToCartesian(Vec3f(around, fromUp, 5.0f)).Length(), 5.0f, kLooseEps);
}

TEST(Helpers, VectorProjSplitsIntoParallelAndPerpendicular) {
	std::mt19937 rng(1234);
	std::uniform_real_distribution<float> d(-5.0f, 5.0f);
	for (int i = 0; i < 100; ++i) {
		Vec3f v(d(rng), d(rng), d(rng));
		Vec3f u(d(rng), d(rng), d(rng));
		if (u.Length() < 0.1f) continue;

		Vec3f proj = vectorProj(v, u);
		EXPECT_NEAR(proj.Cross(u).Length(), 0.0f, kLooseEps * u.LengthSquared()) << "not parallel to u";
		EXPECT_NEAR((v - proj).Dot(u), 0.0f, kLooseEps * u.LengthSquared()) << "remainder not perpendicular to u";
	}
}

// Undecided behavior: projecting onto a zero vector currently divides by zero.
// Whatever the fix is (return zero, assert), the result must not be NaN.
TEST(Helpers, VectorProjOntoZeroIsFinite) {
	Vec3f proj = vectorProj(Vec3f(1.0f, 2.0f, 3.0f), Vec3f(0.0f));
	EXPECT_TRUE(proj.IsFinite()) << Str(proj);
}

TEST(Helpers, Vec3HashEqualVectorsHashEqual) {
	std::hash<Vec3f> h;
	EXPECT_EQ(h(Vec3f(1.0f, 2.0f, 3.0f)), h(Vec3f(1.0f, 2.0f, 3.0f)));
	// -0 == 0, so they must hash the same or a hash map would keep both.
	EXPECT_EQ(h(Vec3f(0.0f, 0.0f, 0.0f)), h(Vec3f(-0.0f, 0.0f, -0.0f)));
}

// --- Euler angles -------------------------------------------------------------
// Compare matrices, not angles: (180, 180-y, 180) and (0, y, 0) are the same
// rotation, and either answer is correct.

TEST(EulerAngles, RoundTripAwayFromPoles) {
	for (float x = -180.0f; x < 180.0f; x += 30.0f)
		for (float y = -85.0f; y <= 85.0f; y += 17.0f)
			for (float z = -180.0f; z < 180.0f; z += 30.0f) {
				Matrix3f m = EulerToMatrix(Vec3f(x, y, z));
				EXPECT_TRUE(Near(EulerToMatrix(matrixToEulerAngle(m)), m, kLooseEps))
					<< "euler (" << x << ", " << y << ", " << z << ")";
			}
}

// Gimbal lock: at y = +-90 none of matrixToEulerAngle's branches run, so x and z
// are never written.
TEST(EulerAngles, RoundTripAtPoles) {
	for (float y : { 90.0f, -90.0f }) {
		for (float x : { 0.0f, 30.0f }) {
			Matrix3f m = EulerToMatrix(Vec3f(x, y, 0.0f));
			Vec3f euler = matrixToEulerAngle(m);
			EXPECT_TRUE(euler.IsFinite()) << "y=" << y << " x=" << x << " gave " << Str(euler);
			EXPECT_TRUE(Near(EulerToMatrix(euler), m, kLooseEps)) << "y=" << y << " x=" << x;
		}
	}
}

// What the rotate gizmo does (viewport.cpp, rotation drag): read the Euler angles,
// rebuild the matrix, apply a small rotation, convert back. Driving it about Y takes
// it straight through the pole.
TEST(EulerAngles, GizmoDragThroughPole) {
	Vec3f euler(0.0f);
	for (int step = 0; step < 360; ++step) {
		Matrix3f rot;
		rot.SetRotation(WORLD_Y, degreesToRadians(1.0f));
		rot *= EulerToMatrix(euler);

		euler = matrixToEulerAngle(rot);
		ASSERT_TRUE(euler.IsFinite()) << "step " << step << " gave " << Str(euler);
		ASSERT_TRUE(Near(EulerToMatrix(euler), rot, kLooseEps)) << "step " << step;
	}
}

TEST(EulerAngles, AxisAngleMatchesSetRotation) {
	const Vec3f axes[] = { WORLD_X, WORLD_Y, WORLD_Z, Vec3f(1.0f, 1.0f, 0.0f).GetNormalized(), Vec3f(1.0f, -2.0f, 3.0f).GetNormalized() };
	for (const Vec3f& axis : axes) {
		for (float deg = -80.0f; deg <= 80.0f; deg += 20.0f) {
			Matrix3f expected;
			expected.SetRotation(axis, degreesToRadians(deg));
			EXPECT_TRUE(Near(EulerToMatrix(axisAngleToEulerAngle(axis, degreesToRadians(deg))), expected, kLooseEps))
				<< "axis (" << Str(axis) << ") angle " << deg;
		}
	}
}

// --- Transform ----------------------------------------------------------------

TEST(Transform, DefaultIsIdentity) {
	HalfEdgeMesh mesh;
	EXPECT_TRUE(mesh.getTransform3x3().IsIdentity());
	EXPECT_TRUE(mesh.getTransform().IsIdentity());
}

// The two are written out separately in mesh.cpp, so they can drift apart.
TEST(Transform, FourByFourAgreesWithThreeByThree) {
	HalfEdgeMesh mesh;
	mesh.Pos(Vec3f(1.0f, -2.0f, 3.0f));
	mesh.Rot(Vec3f(20.0f, -35.0f, 110.0f));
	mesh.Scale(Vec3f(0.5f, 2.0f, -3.0f));

	Matrix3f m3 = mesh.getTransform3x3();
	Matrix4f m4 = mesh.getTransform();

	for (int col = 0; col < 3; ++col)
		for (int row = 0; row < 3; ++row)
			EXPECT_FLOAT_EQ(m4.cell[col * 4 + row], m3.cell[col * 3 + row]) << "row " << row << " col " << col;

	EXPECT_FLOAT_EQ(m4.cell[12], 1.0f);
	EXPECT_FLOAT_EQ(m4.cell[13], -2.0f);
	EXPECT_FLOAT_EQ(m4.cell[14], 3.0f);
	EXPECT_FLOAT_EQ(m4.cell[3], 0.0f);
	EXPECT_FLOAT_EQ(m4.cell[7], 0.0f);
	EXPECT_FLOAT_EQ(m4.cell[11], 0.0f);
	EXPECT_FLOAT_EQ(m4.cell[15], 1.0f);
}

// Right-handed: a positive angle turns counter-clockwise looking down the axis.
TEST(Transform, SingleAxisRotations) {
	HalfEdgeMesh mesh;

	mesh.Rot(Vec3f(90.0f, 0.0f, 0.0f));
	EXPECT_TRUE(Near(mesh.getTransform3x3() * WORLD_Y, WORLD_Z, kEps));

	mesh.Rot(Vec3f(0.0f, 90.0f, 0.0f));
	EXPECT_TRUE(Near(mesh.getTransform3x3() * WORLD_X, -WORLD_Z, kEps));

	mesh.Rot(Vec3f(0.0f, 0.0f, 90.0f));
	EXPECT_TRUE(Near(mesh.getTransform3x3() * WORLD_X, WORLD_Y, kEps));
}

TEST(Transform, ColumnLengthsAreScale) {
	HalfEdgeMesh mesh;
	mesh.Rot(Vec3f(33.0f, -12.0f, 71.0f));
	mesh.Scale(Vec3f(1.0f, 2.0f, 3.0f));

	Matrix3f m = mesh.getTransform3x3();
	EXPECT_NEAR(m.Column(0).Length(), 1.0f, kEps);
	EXPECT_NEAR(m.Column(1).Length(), 2.0f, kEps);
	EXPECT_NEAR(m.Column(2).Length(), 3.0f, kEps);
}

// Scale first, then rotate, then translate.
TEST(Transform, OrderIsScaleRotateTranslate) {
	HalfEdgeMesh mesh;
	Vec3f pos(4.0f, 5.0f, -6.0f), rot(10.0f, 50.0f, -30.0f), scale(2.0f, 0.5f, 3.0f);
	mesh.Pos(pos);
	mesh.Rot(rot);
	mesh.Scale(scale);

	Vec3f p(1.0f, -1.0f, 2.0f);
	Vec3f expected = EulerToMatrix(rot) * (p * scale) + pos;
	Vec4f actual = mesh.getTransform() * p;
	EXPECT_TRUE(Near(Vec3f(actual.x, actual.y, actual.z), expected, kLooseEps));
}

TEST(Transform, WorldDirToLocalDirInvertsTransform) {
	std::mt19937 rng(42);
	std::uniform_real_distribution<float> angle(-180.0f, 180.0f);
	std::uniform_real_distribution<float> magnitude(0.2f, 4.0f);
	std::uniform_int_distribution<int> sign(0, 1);
	std::uniform_real_distribution<float> coord(-3.0f, 3.0f);

	HalfEdgeMesh mesh;
	for (int i = 0; i < 200; ++i) {
		auto s = [&] { return magnitude(rng) * (sign(rng) ? -1.0f : 1.0f); };
		mesh.Rot(Vec3f(angle(rng), angle(rng), angle(rng)));
		mesh.Scale(Vec3f(s(), s(), s()));
		Vec3f d(coord(rng), coord(rng), coord(rng));

		Matrix3f m = mesh.getTransform3x3();
		EXPECT_TRUE(Near(m * mesh.WorldDirToLocalDir(d), d, kLooseEps)) << "iteration " << i;
		EXPECT_TRUE(Near(mesh.WorldDirToLocalDir(m * d), d, kLooseEps)) << "iteration " << i;
	}
}

// Undecided behavior: a zero scale axis makes the matrix impossible to invert, and
// the NaN it produces gets added straight to vertex positions by TranslateVertex.
TEST(Transform, WorldDirToLocalDirWithZeroScaleIsFinite) {
	HalfEdgeMesh mesh;
	mesh.Scale(Vec3f(1.0f, 0.0f, 1.0f));
	Vec3f local = mesh.WorldDirToLocalDir(Vec3f(1.0f, 1.0f, 1.0f));
	EXPECT_TRUE(local.IsFinite()) << Str(local);
}
