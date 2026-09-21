#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "mesh.h"
#include "shapes.h"

namespace {
	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	HalfEdgeMesh Cube() {
		HalfEdgeMesh mesh;
		mesh.load(CUBE_VERTICES, CUBE_FACES);
		return mesh;
	}

	// Anything that has to handle a triangle gets tested against this, not the cube.
	HalfEdgeMesh Pyramid() {
		HalfEdgeMesh mesh;
		mesh.load(PYRAMID_VERTICES, PYRAMID_FACES);
		return mesh;
	}

	// Counts and validity in one place, so a test that drops a face fails even
	// though the connectivity it left behind is internally consistent.
	void ExpectMesh(const HalfEdgeMesh& mesh, uint verts, uint faces, uint edges) {
		EXPECT_TRUE(IsValid(mesh));
		EXPECT_EQ(mesh.VertexCount(), verts);
		EXPECT_EQ(mesh.FaceCount(), faces);
		EXPECT_EQ(mesh.EdgeCount(), edges);
	}
}

// Test for the build wiring: abc3d_core links without FLTK or OpenGL.
TEST(HalfEdgeMeshLoad, BoxHasEightVertices) {
	FaceList faceList;
	ASSERT_TRUE(faceList.LoadObj(ABC3D_MODELS_DIR "/box.obj"));

	HalfEdgeMesh mesh;
	ASSERT_TRUE(mesh.load(faceList));

	EXPECT_EQ(mesh.VertexCount(), 8u);
}

// --- Baselines -------------------------------------------------------------
// If either of these fails, the bug is in Validate, not in an operation.

TEST(MeshInvariants, CubeStartsValid) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 8u);
	EXPECT_EQ(mesh.FaceCount(), 6u);
	EXPECT_EQ(mesh.EdgeCount(), 24u);
}

TEST(MeshInvariants, PyramidStartsValid) {
	HalfEdgeMesh mesh = Pyramid();
	ASSERT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 5u);
	EXPECT_EQ(mesh.FaceCount(), 5u);
	EXPECT_EQ(mesh.EdgeCount(), 16u);
}

// --- Extrude ---------------------------------------------------------------

TEST(MeshInvariants, ExtrudeKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.Extrude(0);
	ExpectMesh(mesh, 12u, 10u, 40u);
}

TEST(MeshInvariants, ExtrudeTwiceKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.Extrude(0);
	mesh.Extrude(0);
	ExpectMesh(mesh, 16u, 14u, 56u);
}

TEST(MeshInvariants, ExtrudeQuadFaceKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.Extrude(0); // face 0 is the quad base
	ExpectMesh(mesh, 9u, 9u, 32u);
}

TEST(MeshInvariants, ExtrudeTriangleFaceKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.Extrude(1); // faces 1-4 are triangles
	ExpectMesh(mesh, 8u, 8u, 28u);
}

// --- Triangulation ---------------------------------------------------------

TEST(MeshInvariants, TriangulateFaceKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.TriangulateFace(0);
	ExpectMesh(mesh, 8u, 7u, 26u);
}

TEST(MeshInvariants, TriangulateKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.Triangulate();
	ExpectMesh(mesh, 8u, 12u, 36u);
}

// The cube is all quads, so there is no n-gon to split. Asserting the counts
// are unchanged pins that this is a no-op rather than quietly touching quads.
TEST(MeshInvariants, TriangulateAllNGonsLeavesAllQuadCubeAlone) {
	HalfEdgeMesh mesh = Cube();
	mesh.TriangulateAllNGons();
	ExpectMesh(mesh, 8u, 6u, 24u);
}

TEST(MeshInvariants, TriangulateKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.Triangulate(); // splits the quad base, leaves the four triangles alone
	ExpectMesh(mesh, 5u, 6u, 18u);
}

// --- refineEdge ------------------------------------------------------------

TEST(MeshInvariants, RefineEdgeKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.refineEdge(0);
	ExpectMesh(mesh, 9u, 6u, 26u);
}

TEST(MeshInvariants, RefineTwoEdgesKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.refineEdge(0);
	mesh.refineEdge(1);
	ExpectMesh(mesh, 10u, 6u, 28u);
}

TEST(MeshInvariants, RefineEdgeKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.refineEdge(0);
	ExpectMesh(mesh, 6u, 5u, 18u);
}

// --- ClipCorner ------------------------------------------------------------

TEST(MeshInvariants, ClipCornerKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	uint innerClip = MAX_UINT, outerClip = MAX_UINT, cornerFace = MAX_UINT;
	mesh.ClipCorner(0, innerClip, outerClip, cornerFace);

	ExpectMesh(mesh, 8u, 7u, 26u);
	// The ids handed back have to address the mesh we ended up with.
	EXPECT_LT(innerClip, mesh.EdgeCount());
	EXPECT_LT(outerClip, mesh.EdgeCount());
	EXPECT_LT(cornerFace, mesh.FaceCount());
}

TEST(MeshInvariants, ClipCornerKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	uint innerClip = MAX_UINT, outerClip = MAX_UINT, cornerFace = MAX_UINT;
	mesh.ClipCorner(0, innerClip, outerClip, cornerFace);
	ExpectMesh(mesh, 5u, 6u, 18u);
}

// --- tesselate / subdivide -------------------------------------------------
// subdivide() is tesselate() followed by deform() (src/mesh.cpp:1809), so these
// cover deform() too -- it is private and has no other public entry point.

TEST(MeshInvariants, TesselateKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.tesselate();
	ExpectMesh(mesh, 26u, 24u, 96u);
}

TEST(MeshInvariants, SubdivideKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.subdivide();
	ExpectMesh(mesh, 26u, 24u, 96u);
}

TEST(MeshInvariants, SubdivideTwiceKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.subdivide();
	mesh.subdivide();
	ExpectMesh(mesh, 98u, 96u, 384u);
}

//My original test was wrong as tesselate() is hybrid
//Catmull-Clark for quads (1 -> 4)
//Loop (1 -> 4 triangles)
//So the pyramid should actually have 20 faces instead of 16
TEST(MeshInvariants, TesselateKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.tesselate();
	ExpectMesh(mesh, 14u, 20u, 64u); 
}

//This test was also wrong based on the above example
TEST(MeshInvariants, SubdivideKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.subdivide();
	ExpectMesh(mesh, 14u, 20u, 64u); 
}

// --- Beveling --------------------------------------------------------------
// BevelEdge is private; the reachable path is HalfEdgeMeshBeveler.

TEST(MeshInvariants, BevelOneEdgeKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	HalfEdgeMeshBeveler beveler(mesh);

	beveler.StartEdgeBevel(std::vector<uint>{ 0 });
	EXPECT_TRUE(IsValid(mesh)) << "after StartEdgeBevel";

	// AdjustBevel only moves vertex positions, so connectivity must survive
	// both ends of the range as well as the middle.
	beveler.AdjustBevel(0.0f);
	EXPECT_TRUE(IsValid(mesh)) << "after AdjustBevel(0.0)";
	beveler.AdjustBevel(0.5f);
	EXPECT_TRUE(IsValid(mesh)) << "after AdjustBevel(0.5)";
	beveler.AdjustBevel(1.0f);
	EXPECT_TRUE(IsValid(mesh)) << "after AdjustBevel(1.0)";

	beveler.EndBevel();
	ExpectMesh(mesh, 10u, 7u, 30u);
}

TEST(MeshInvariants, BevelTwoEdgesKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	HalfEdgeMeshBeveler beveler(mesh);

	beveler.StartEdgeBevel(std::vector<uint>{ 0, 2 });
	beveler.AdjustBevel(0.5f);
	beveler.EndBevel();

	ExpectMesh(mesh, 12u, 8u, 36u);
}
