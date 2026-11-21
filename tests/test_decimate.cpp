#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "mesh.h"
#include "MeshDecimator.h"
#include "shapes.h"

namespace {
	using Decimator = HalfEdgeMeshDecimator;

	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	HalfEdgeMesh Sphere(uint latitude, uint longitude) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		generateUVSphere(verts, faces, latitude, longitude);
		HalfEdgeMesh mesh;
		mesh.load(verts, faces);
		return mesh;
	}

	HalfEdgeMesh Torus(uint u, uint v) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		generateUnitTorus(verts, faces, u, v);
		HalfEdgeMesh mesh;
		mesh.load(verts, faces);
		return mesh;
	}

	HalfEdgeMesh Cube() {
		HalfEdgeMesh mesh;
		mesh.load(CUBE_VERTICES, CUBE_FACES);
		return mesh;
	}

	// An open n x n grid of quads in the xy plane, from (0, 0) to (n, n)
	HalfEdgeMesh Grid(uint n) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		for (uint j = 0; j <= n; ++j) {
			for (uint i = 0; i <= n; ++i) { verts.push_back(Vec3f(float(i), float(j), 0.0f)); }
		}
		for (uint j = 0; j < n; ++j) {
			for (uint i = 0; i < n; ++i) {
				const uint a = i + j * (n + 1);
				faces.push_back(Vec4u(a, a + 1, a + n + 2, a + n + 1));
			}
		}
		HalfEdgeMesh mesh;
		mesh.load(verts, faces);
		return mesh;
	}

	uint LiveFaces(const HalfEdgeMesh& mesh) {
		uint count = 0;
		for (uint f = 0; f < mesh.FaceCount(); ++f) { if (!mesh.FaceIsNull(f)) { ++count; } }
		return count;
	}

	bool AllTriangles(const HalfEdgeMesh& mesh) {
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			if (mesh.FaceIsNull(f)) { continue; }
			std::vector<uint> verts;
			mesh.GetFaceVertices(f, verts);
			if (verts.size() != 3) { return false; }
		}
		return true;
	}

	// True when every half-edge has a face (no open boundary)
	bool IsClosed(const HalfEdgeMesh& mesh) {
		for (uint e = 0; e < mesh.EdgeCount(); ++e) {
			if (!mesh.EdgeIsNull(e) && mesh.GetEdge(e).face == MAX_UINT) { return false; }
		}
		return true;
	}

	// V - E + F, for a closed mesh (every edge is two half-edges with faces)
	int EulerCharacteristic(const HalfEdgeMesh& mesh) {
		return int(mesh.VertexCount()) - int(mesh.EdgeCount() / 2) + int(LiveFaces(mesh));
	}

	Vec3f TriangleNormal(HalfEdgeMesh& mesh, uint f) {
		Vec3f p[3];
		mesh.GetTriVertices(f, p);
		return (p[1] - p[0]).Cross(p[2] - p[0]);
	}
}

TEST(Decimate, TriangleCountCountsNGonsAsFans) {
	EXPECT_EQ(Decimator::TriangleCount(Cube()), 12u);
	EXPECT_EQ(Decimator::TriangleCount(Grid(4)), 32u);
	EXPECT_EQ(Decimator::TriangleCount(Sphere(8, 8)), 2u * 8u * 7u);
}

TEST(Decimate, SphereReachesTargetAndStaysClosed) {
	HalfEdgeMesh mesh = Sphere(32, 32);
	const uint before = Decimator::TriangleCount(mesh);
	const int euler = EulerCharacteristic(mesh);

	Decimator decimator(mesh);
	Decimator::Options options;
	options.targetTriangles = before / 4;
	Decimator::Result result;
	ASSERT_TRUE(decimator.Decimate(options, &result)) << decimator.error;

	EXPECT_TRUE(IsValid(mesh));
	EXPECT_TRUE(AllTriangles(mesh));
	EXPECT_TRUE(IsClosed(mesh));
	EXPECT_EQ(EulerCharacteristic(mesh), euler);
	EXPECT_LE(LiveFaces(mesh), options.targetTriangles);
	EXPECT_GE(LiveFaces(mesh), options.targetTriangles - 1);
	EXPECT_EQ(result.trianglesBefore, before);
	EXPECT_EQ(result.trianglesAfter, LiveFaces(mesh));
	EXPECT_EQ(result.verticesAfter, mesh.VertexCount());

	// The shape is kept: every vertex stays close to the unit sphere
	for (const Vec3f& p : mesh.Vertices()) {
		EXPECT_NEAR(p.Length(), 1.0f, 0.05f);
	}
}

TEST(Decimate, SphereNormalsStayOutward) {
	HalfEdgeMesh mesh = Sphere(24, 24);
	Decimator decimator(mesh);
	ASSERT_TRUE(decimator.DecimateToRatio(0.1f)) << decimator.error;
	ASSERT_TRUE(IsValid(mesh));

	for (uint f = 0; f < mesh.FaceCount(); ++f) {
		if (mesh.FaceIsNull(f)) { continue; }
		Vec3f p[3];
		mesh.GetTriVertices(f, p);
		const Vec3f centroid = (p[0] + p[1] + p[2]) / 3.0f;
		EXPECT_GT(TriangleNormal(mesh, f).Dot(centroid), 0.0f) << "face " << f << " turned inward";
	}
}

TEST(Decimate, TorusKeepsItsHole) {
	HalfEdgeMesh mesh = Torus(32, 16);
	ASSERT_TRUE(IsValid(mesh));
	ASSERT_EQ(EulerCharacteristic(mesh), 0);

	Decimator decimator(mesh);
	ASSERT_TRUE(decimator.DecimateToRatio(0.2f)) << decimator.error;
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_TRUE(IsClosed(mesh));
	EXPECT_EQ(EulerCharacteristic(mesh), 0);
}

TEST(Decimate, FlatGridKeepsPlaneOutlineAndOrientation) {
	const uint n = 20;
	HalfEdgeMesh mesh = Grid(n);

	Decimator decimator(mesh);
	Decimator::Options options;
	options.targetTriangles = 2 * n * n / 10;
	ASSERT_TRUE(decimator.Decimate(options)) << decimator.error;
	ASSERT_TRUE(IsValid(mesh));
	EXPECT_TRUE(AllTriangles(mesh));
	EXPECT_LT(LiveFaces(mesh), 2 * n * n);

	const float size = float(n);
	bool corner[4] = {};
	for (uint v = 0; v < mesh.VertexCount(); ++v) {
		const Vec3f& p = mesh.Vertices()[v];
		EXPECT_FLOAT_EQ(p.z, 0.0f);
		EXPECT_GE(p.x, -1e-4f); EXPECT_LE(p.x, size + 1e-4f);
		EXPECT_GE(p.y, -1e-4f); EXPECT_LE(p.y, size + 1e-4f);
		auto near = [](float a, float b) { return std::abs(a - b) < 1e-4f; };
		if (near(p.x, 0) && near(p.y, 0)) corner[0] = true;
		if (near(p.x, size) && near(p.y, 0)) corner[1] = true;
		if (near(p.x, 0) && near(p.y, size)) corner[2] = true;
		if (near(p.x, size) && near(p.y, size)) corner[3] = true;
	}
	for (bool c : corner) { EXPECT_TRUE(c) << "a corner of the grid moved"; }

	// No face turned over, and the covered area is unchanged
	float area = 0.0f;
	for (uint f = 0; f < mesh.FaceCount(); ++f) {
		if (mesh.FaceIsNull(f)) { continue; }
		const Vec3f normal = TriangleNormal(mesh, f);
		EXPECT_GT(normal.z, 0.0f) << "face " << f << " flipped";
		area += 0.5f * normal.z;
	}
	EXPECT_NEAR(area, size * size, 1e-2f);

	// Boundary vertices stay on the outline
	for (uint e = 0; e < mesh.EdgeCount(); ++e) {
		const HalfEdgeMesh::HalfEdge edge = mesh.GetEdge(e);
		if (edge.face != MAX_UINT) { continue; }
		const Vec3f& p = mesh.Vertices()[edge.orig];
		const bool onOutline = std::abs(p.x) < 1e-4f || std::abs(p.y) < 1e-4f ||
			std::abs(p.x - size) < 1e-4f || std::abs(p.y - size) < 1e-4f;
		EXPECT_TRUE(onOutline) << "boundary vertex left the outline";
	}
}

TEST(Decimate, QuadMeshComesOutTriangulated) {
	HalfEdgeMesh mesh = Cube();
	mesh.subdivide();
	mesh.subdivide();
	ASSERT_FALSE(AllTriangles(mesh));
	const uint before = Decimator::TriangleCount(mesh);

	Decimator decimator(mesh);
	ASSERT_TRUE(decimator.DecimateToRatio(0.5f)) << decimator.error;
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_TRUE(AllTriangles(mesh));
	EXPECT_TRUE(IsClosed(mesh));
	EXPECT_EQ(EulerCharacteristic(mesh), 2);
	EXPECT_LE(LiveFaces(mesh), (before + 1) / 2);
}

TEST(Decimate, CubeCannotCollapsePastATetrahedron) {
	HalfEdgeMesh mesh = Cube();
	Decimator decimator(mesh);
	Decimator::Options options;
	options.targetTriangles = 0;
	ASSERT_TRUE(decimator.Decimate(options)) << decimator.error;
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_TRUE(IsClosed(mesh));
	EXPECT_EQ(EulerCharacteristic(mesh), 2);
	EXPECT_GE(LiveFaces(mesh), 4u);
}

TEST(Decimate, NothingToDoLeavesMeshUntouched) {
	HalfEdgeMesh mesh = Sphere(8, 8);
	const HalfEdgeMesh before = mesh;

	Decimator decimator(mesh);
	Decimator::Options options;
	options.targetTriangles = Decimator::TriangleCount(mesh);
	EXPECT_FALSE(decimator.Decimate(options));
	EXPECT_FALSE(decimator.error.empty());
	EXPECT_TRUE(mesh.SameState(before));
}

TEST(Decimate, ErrorLimitStopsCollapses) {
	// Every collapse on a curved surface costs something, so a zero budget allows none
	HalfEdgeMesh mesh = Sphere(16, 16);
	const HalfEdgeMesh before = mesh;

	Decimator decimator(mesh);
	Decimator::Options options;
	options.targetTriangles = 10;
	options.maxError = 0.0;
	EXPECT_FALSE(decimator.Decimate(options));
	EXPECT_TRUE(mesh.SameState(before));

	// A flat region collapses for free, so the same budget still decimates a plane
	HalfEdgeMesh grid = Grid(8);
	Decimator gridDecimator(grid);
	Decimator::Result result;
	ASSERT_TRUE(gridDecimator.Decimate(options, &result)) << gridDecimator.error;
	EXPECT_LT(result.trianglesAfter, result.trianglesBefore);
	EXPECT_LE(result.maxCollapseError, 1e-9);
	EXPECT_TRUE(IsValid(grid));
}

TEST(Decimate, SingleTriangleIsRejected) {
	HalfEdgeMesh mesh;
	mesh.load(std::vector<Vec3f>{ Vec3f(0, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 1, 0) },
	          std::vector<Vec4u>{ Vec4u(0, 1, 2, MAX_UINT) });
	const HalfEdgeMesh before = mesh;

	Decimator decimator(mesh);
	EXPECT_FALSE(decimator.DecimateToRatio(0.0f));
	EXPECT_TRUE(mesh.SameState(before));
}

TEST(Decimate, KeepsObjectTransform) {
	HalfEdgeMesh mesh = Sphere(12, 12);
	mesh.Pos(Vec3f(1, 2, 3));
	mesh.Rot(Vec3f(10, 20, 30));
	mesh.Scale(Vec3f(2, 2, 2));

	Decimator decimator(mesh);
	ASSERT_TRUE(decimator.DecimateToRatio(0.5f)) << decimator.error;
	EXPECT_EQ(mesh.Pos(), Vec3f(1, 2, 3));
	EXPECT_EQ(mesh.Rot(), Vec3f(10, 20, 30));
	EXPECT_EQ(mesh.Scale(), Vec3f(2, 2, 2));
}

TEST(Decimate, DeletedFacesAreCompactedAway) {
	HalfEdgeMesh mesh = Sphere(16, 16);
	ASSERT_TRUE(mesh.DeleteFace(0));
	ASSERT_TRUE(mesh.DeleteFace(5));
	const uint before = Decimator::TriangleCount(mesh);

	Decimator decimator(mesh);
	ASSERT_TRUE(decimator.DecimateToRatio(0.5f)) << decimator.error;
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(LiveFaces(mesh), mesh.FaceCount()); // no deleted slots left
	EXPECT_LE(LiveFaces(mesh), (before + 1) / 2);
}

TEST(Decimate, TeapotModel) {
	// A real model with open boundaries (the lid and spout openings)
	std::vector<HalfEdgeMesh> meshes;
	ASSERT_TRUE(loadMeshes(ABC3D_MODELS_DIR "/utahTeapotBlinn.obj", meshes).ok());
	for (HalfEdgeMesh& mesh : meshes) {
		const uint before = Decimator::TriangleCount(mesh);
		Decimator decimator(mesh);
		Decimator::Result result;
		ASSERT_TRUE(decimator.DecimateToRatio(0.1f, &result)) << decimator.error;
		EXPECT_TRUE(IsValid(mesh));
		EXPECT_TRUE(AllTriangles(mesh));
		EXPECT_LE(LiveFaces(mesh), before / 10 + 1);
	}
}

TEST(Decimate, EstimateWorkGrowsWithSizeAndReduction) {
	EXPECT_EQ(Decimator::EstimateWork(1000, 1.0f), 0.0); // nothing to remove
	EXPECT_EQ(Decimator::EstimateWork(1, 0.5f), 0.0);
	EXPECT_GT(Decimator::EstimateWork(1000, 0.1f), Decimator::EstimateWork(1000, 0.5f));
	EXPECT_GT(Decimator::EstimateWork(2000, 0.5f), 2.0 * Decimator::EstimateWork(1000, 0.5f)); // n log n
	EXPECT_EQ(Decimator::EstimateWork(1000, -1.0f), Decimator::EstimateWork(1000, 0.0f)); // ratio is clamped
}

// Times Decimate on growing spheres so Decimator::SecondsPerWorkUnit can be recalibrated.
// Run with --gtest_also_run_disabled_tests --gtest_filter=Decimate.DISABLED_Timing (use a Release build).
TEST(Decimate, DISABLED_Timing) {
	for (uint n : { 64u, 128u, 256u, 512u }) {
		for (float ratio : { 0.5f, 0.1f }) {
			HalfEdgeMesh mesh = Sphere(n, n);
			const uint triangles = Decimator::TriangleCount(mesh);
			const auto start = std::chrono::steady_clock::now();
			Decimator decimator(mesh);
			ASSERT_TRUE(decimator.DecimateToRatio(ratio)) << decimator.error;
			const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
			std::cout << triangles << " triangles to " << ratio << ": " << seconds << " s, "
			          << 1e9 * seconds / Decimator::EstimateWork(triangles, ratio) << " ns per work unit" << std::endl;
		}
	}
}
