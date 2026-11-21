#include <gtest/gtest.h>

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "mesh.h"
#include "MeshLoopCut.h"
#include "shapes.h"

// Edge counts here are half-edge counts (HalfEdgeMesh::EdgeCount), which include the exterior twins of open
// boundaries. A closed mesh with E edges has 2E half-edges, and so does an open one.

namespace {
	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	void ExpectMesh(const HalfEdgeMesh& mesh, uint verts, uint faces, uint halfEdges) {
		EXPECT_TRUE(IsValid(mesh));
		EXPECT_EQ(mesh.VertexCount(), verts);
		EXPECT_EQ(mesh.FaceCount(), faces);
		EXPECT_EQ(mesh.EdgeCount(), halfEdges);
	}

	HalfEdgeMesh Cube() {
		HalfEdgeMesh mesh;
		mesh.load(CUBE_VERTICES, CUBE_FACES);
		return mesh;
	}

	HalfEdgeMesh Pyramid() {
		HalfEdgeMesh mesh;
		mesh.load(PYRAMID_VERTICES, PYRAMID_FACES);
		return mesh;
	}

	HalfEdgeMesh FromObj(const std::string& obj) {
		std::istringstream in(obj);
		FaceList faces;
		EXPECT_TRUE(faces.LoadObjStream(in));
		HalfEdgeMesh mesh;
		EXPECT_TRUE(mesh.load(faces));
		return mesh;
	}

	// An open n x n grid of quads in the xy plane. Vertex (i, j) is i + j * (n + 1).
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

	// A torus made of quads. Vertex (i, j) is i * v + j, i goes around the big circle, j around the tube.
	HalfEdgeMesh QuadTorus(uint u, uint v) {
		std::vector<Vec3f> verts;
		std::vector<Vec4u> faces;
		for (uint i = 0; i < u; ++i) {
			const float a = 6.28318530718f * i / u;
			for (uint j = 0; j < v; ++j) {
				const float b = 6.28318530718f * j / v;
				verts.push_back(Vec3f((1.0f + 0.3f * std::cos(b)) * std::cos(a), 0.3f * std::sin(b),
				                      (1.0f + 0.3f * std::cos(b)) * std::sin(a)));
			}
		}
		for (uint i = 0; i < u; ++i) {
			for (uint j = 0; j < v; ++j) {
				const uint ni = (i + 1) % u, nj = (j + 1) % v;
				faces.push_back(Vec4u(i * v + j, i * v + nj, ni * v + nj, ni * v + j));
			}
		}
		HalfEdgeMesh mesh;
		mesh.load(verts, faces);
		return mesh;
	}

	uint FindHalfEdge(const HalfEdgeMesh& mesh, uint orig, uint dest) {
		for (uint e = 0; e < mesh.EdgeCount(); ++e) {
			const HalfEdgeMesh::HalfEdge he = mesh.GetEdge(e);
			if (he.orig == orig && he.dest == dest) { return e; }
		}
		ADD_FAILURE() << "no half-edge " << orig << " -> " << dest;
		return MAX_UINT;
	}

	// A half-edge of a face with `sides` sides
	uint EdgeOfFaceWithSides(const HalfEdgeMesh& mesh, uint sides) {
		std::vector<uint> edges;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			edges.clear();
			mesh.GetFaceEdges(f, edges);
			if (edges.size() == sides) { return edges[0]; }
		}
		ADD_FAILURE() << "no face with " << sides << " sides";
		return MAX_UINT;
	}

	bool Cut(HalfEdgeMesh& mesh, uint halfEdge, uint cuts = 1) {
		HalfEdgeMeshLoopCutter cutter(mesh);
		const bool changed = cutter.Cut(halfEdge, cuts);
		if (changed) { EXPECT_EQ(cutter.error, ""); }
		return changed;
	}
}

TEST(LoopCut, CubeRingGoesAllTheWayAround) {
	HalfEdgeMesh mesh = Cube();
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(0);
	EXPECT_TRUE(ring.closed);
	EXPECT_EQ(ring.edges.size(), 4u);
	EXPECT_EQ(ring.faces.size(), 4u);
}

TEST(LoopCut, CubeSingleCut) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(Cut(mesh, 0));
	// V 8 -> 12, E 12 -> 20, F 6 -> 10
	ExpectMesh(mesh, 12, 10, 40);
}

TEST(LoopCut, CubeCutIsAtTheMiddleOfTheRing) {
	HalfEdgeMesh mesh = Cube();
	const HalfEdgeMesh::HalfEdge picked = mesh.GetEdge(0);
	const Vec3f a = CUBE_VERTICES[picked.orig];
	const Vec3f b = CUBE_VERTICES[picked.dest];

	ASSERT_TRUE(Cut(mesh, 0));

	// The cube spans -1..1, and the cut edges are parallel to the picked one, so every new vertex sits at 0 on
	// the axis the picked edge runs along.
	int axis = 0;
	for (int i = 1; i < 3; ++i) { if (std::abs(b[i] - a[i]) > std::abs(b[axis] - a[axis])) { axis = i; } }
	for (uint v = 8; v < mesh.VertexCount(); ++v) {
		EXPECT_NEAR(mesh.Vertices()[v][axis], 0.0f, 1e-5f) << "vertex " << v;
	}
}

TEST(LoopCut, EitherHalfEdgeGivesTheSameCut) {
	HalfEdgeMesh a = Cube();
	HalfEdgeMesh b = Cube();
	ASSERT_TRUE(Cut(a, 0));
	ASSERT_TRUE(Cut(b, b.GetEdge(0).twin));
	ExpectMesh(a, 12, 10, 40);
	ExpectMesh(b, 12, 10, 40);
}

TEST(LoopCut, MultipleCutsAreEvenlySpaced) {
	HalfEdgeMesh mesh = Cube();
	const HalfEdgeMesh::HalfEdge picked = mesh.GetEdge(0);
	const Vec3f a = CUBE_VERTICES[picked.orig];
	const Vec3f b = CUBE_VERTICES[picked.dest];
	int axis = 0;
	for (int i = 1; i < 3; ++i) { if (std::abs(b[i] - a[i]) > std::abs(b[axis] - a[axis])) { axis = i; } }

	ASSERT_TRUE(Cut(mesh, 0, 3));
	// 4 ring edges x 3 cuts = 12 new vertices; each of the 4 faces is split into 4; E 12 + 12 + 12 = 36
	ExpectMesh(mesh, 20, 18, 72);

	// Along the cut axis the new vertices sit at -0.5, 0 and 0.5, four of each
	int counts[3] = { 0, 0, 0 };
	for (uint v = 8; v < mesh.VertexCount(); ++v) {
		const float x = mesh.Vertices()[v][axis];
		for (int k = 0; k < 3; ++k) { if (std::abs(x - (-0.5f + 0.5f * k)) < 1e-5f) { ++counts[k]; } }
	}
	EXPECT_EQ(counts[0], 4);
	EXPECT_EQ(counts[1], 4);
	EXPECT_EQ(counts[2], 4);
}

TEST(LoopCut, CutsCanBeRepeated) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(Cut(mesh, 0));
	// A second cut across the new edges, through an edge the first cut made
	ASSERT_TRUE(Cut(mesh, mesh.EdgeCount() - 1));
	EXPECT_TRUE(IsValid(mesh));
}

TEST(LoopCut, SinglePlaneQuadAddsOneEdge) {
	HalfEdgeMesh mesh = Grid(1);
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(FindHalfEdge(mesh, 0, 1));
	EXPECT_FALSE(ring.closed);
	EXPECT_EQ(ring.faces.size(), 1u);
	EXPECT_EQ(ring.edges.size(), 2u);

	ASSERT_TRUE(Cut(mesh, FindHalfEdge(mesh, 0, 1)));
	// 4 + 2 vertices, 2 faces, 4 + 2 + 1 = 7 edges
	ExpectMesh(mesh, 6, 2, 14);
}

TEST(LoopCut, PickingTheExteriorHalfEdgeOfABoundaryWorks) {
	HalfEdgeMesh mesh = Grid(1);
	// 1 -> 0 has no face: it is the exterior twin of the boundary edge 0 -> 1
	ASSERT_TRUE(Cut(mesh, FindHalfEdge(mesh, 1, 0)));
	ExpectMesh(mesh, 6, 2, 14);
}

TEST(LoopCut, OpenGridStripStopsAtTheBoundary) {
	HalfEdgeMesh mesh = Grid(3);
	// A horizontal edge in the middle column: the strip runs up the column, across 3 faces and 4 edges
	const uint e = FindHalfEdge(mesh, 5, 6);
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(e);
	EXPECT_FALSE(ring.closed);
	EXPECT_EQ(ring.faces.size(), 3u);
	EXPECT_EQ(ring.edges.size(), 4u);

	ASSERT_TRUE(Cut(mesh, e));
	// 16 + 4 vertices, 9 + 3 faces, 24 + 4 + 3 = 31 edges
	ExpectMesh(mesh, 20, 12, 62);

	// The new vertices are the midpoints of the column, at x = 1.5
	for (uint v = 16; v < mesh.VertexCount(); ++v) { EXPECT_NEAR(mesh.Vertices()[v].x, 1.5f, 1e-5f); }
}

TEST(LoopCut, TorusRingClosesAroundTheTube) {
	HalfEdgeMesh mesh = QuadTorus(8, 6);
	// An edge around the tube (j direction): the ring follows the big circle, through all 8 faces of one band
	const uint e = FindHalfEdge(mesh, 0, 1);
	HalfEdgeMeshLoopCutter cutter(mesh);
	EXPECT_TRUE(cutter.FindRing(e).closed);
	EXPECT_EQ(cutter.FindRing(e).faces.size(), 8u);

	ASSERT_TRUE(Cut(mesh, e));
	// 48 vertices, 48 faces, 96 edges. The ring has 8 edges and 8 faces.
	ExpectMesh(mesh, 56, 56, 2 * (96 + 8 + 8));
}

TEST(LoopCut, PyramidBaseStopsAtTheTriangles) {
	HalfEdgeMesh mesh = Pyramid();
	const uint e = EdgeOfFaceWithSides(mesh, 4);
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(e);
	EXPECT_FALSE(ring.closed);
	EXPECT_EQ(ring.faces.size(), 1u);
	EXPECT_EQ(ring.edges.size(), 2u);

	ASSERT_TRUE(Cut(mesh, e));
	// The base is split, and each triangle beside it gets a vertex on its side. 5 + 2 vertices, 8 + 2 + 1 edges.
	ExpectMesh(mesh, 7, 6, 22);
}

TEST(LoopCut, AllTriangleEdgeIsOnlySplit) {
	HalfEdgeMesh mesh;
	mesh.load(ICOSPHERE_VERTICES, ICOSPHERE_FACES);
	const uint verts = mesh.VertexCount(), faces = mesh.FaceCount(), halfEdges = mesh.EdgeCount();

	HalfEdgeMeshLoopCutter cutter(mesh);
	EXPECT_TRUE(cutter.FindRing(0).faces.empty());

	ASSERT_TRUE(Cut(mesh, 0));
	ExpectMesh(mesh, verts + 1, faces, halfEdges + 2);
}

TEST(LoopCut, HexagonIsCutHalfwayAround) {
	HalfEdgeMesh mesh = FromObj(
		"v 1 0 0\nv 0.5 0.87 0\nv -0.5 0.87 0\nv -1 0 0\nv -0.5 -0.87 0\nv 0.5 -0.87 0\n"
		"f 1 2 3 4 5 6\n");
	const uint e = FindHalfEdge(mesh, 0, 1);
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(e);
	ASSERT_EQ(ring.faces.size(), 1u);
	// Three steps along from 0 -> 1 is 3 -> 4
	EXPECT_EQ(mesh.GetEdge(ring.faces[0].exit).orig, 3u);
	EXPECT_EQ(mesh.GetEdge(ring.faces[0].exit).dest, 4u);

	ASSERT_TRUE(Cut(mesh, e));
	// Two pentagons
	ExpectMesh(mesh, 8, 2, 2 * 9);
	std::vector<uint> edges;
	for (uint f = 0; f < mesh.FaceCount(); ++f) {
		edges.clear();
		mesh.GetFaceEdges(f, edges);
		EXPECT_EQ(edges.size(), 5u);
	}
}

TEST(LoopCut, PentagonStopsTheStrip) {
	HalfEdgeMesh mesh = FromObj(
		"v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv 2 0.5 0\nv 2 1.5 0\nv 1 2 0\n"
		"f 1 2 3 4\n"
		"f 2 5 6 7 3\n");
	// Across the square toward the pentagon: the strip enters the square through 3 -> 0 and leaves through the
	// shared edge 1 -> 2, into the pentagon, where it stops.
	const uint e = FindHalfEdge(mesh, 3, 0);
	HalfEdgeMeshLoopCutter cutter(mesh);
	const HalfEdgeMeshLoopCutter::Ring ring = cutter.FindRing(e);
	EXPECT_EQ(ring.faces.size(), 1u);
	EXPECT_EQ(ring.edges.size(), 2u);

	ASSERT_TRUE(Cut(mesh, e));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.FaceCount(), 3u);
	EXPECT_EQ(mesh.VertexCount(), 9u);
}

TEST(LoopCut, PreviewMatchesTheCut) {
	HalfEdgeMesh mesh = Cube();
	HalfEdgeMeshLoopCutter cutter(mesh);
	std::vector<std::pair<Vec3f, Vec3f>> segments;
	std::vector<Vec3f> points;
	cutter.Preview(0, 2, segments, points);
	EXPECT_EQ(segments.size(), 8u);
	EXPECT_EQ(points.size(), 8u);

	// The preview doesn't change the mesh
	ExpectMesh(mesh, 8, 6, 24);

	// Each segment joins two of the points, and runs parallel to the picked edge's faces (it has no length
	// along the picked edge's axis)
	const HalfEdgeMesh::HalfEdge picked = mesh.GetEdge(0);
	const Vec3f dir = CUBE_VERTICES[picked.dest] - CUBE_VERTICES[picked.orig];
	for (const auto& s : segments) { EXPECT_NEAR((s.second - s.first).Dot(dir), 0.0f, 1e-5f); }
}

TEST(LoopCut, BadInputChangesNothing) {
	HalfEdgeMesh mesh = Cube();
	HalfEdgeMeshLoopCutter cutter(mesh);
	EXPECT_FALSE(cutter.Cut(9999));
	EXPECT_FALSE(cutter.Cut(0, 0));
	ExpectMesh(mesh, 8, 6, 24);
}
