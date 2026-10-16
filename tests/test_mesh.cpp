#include <gtest/gtest.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "mesh.h"
#include "MeshBeveler.h"
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

	void ExpectVec3Near(const Vec3f& actual, const Vec3f& expected, float tolerance = 1e-5f) {
		EXPECT_NEAR(actual.x, expected.x, tolerance);
		EXPECT_NEAR(actual.y, expected.y, tolerance);
		EXPECT_NEAR(actual.z, expected.z, tolerance);
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

TEST(HalfEdgeMeshMerge, RemovesSourceVertexAndKeepsTargetPosition) {
	HalfEdgeMesh mesh;
	mesh.load(
		std::vector<Vec3f>{
			Vec3f(0.0f, 0.0f, 0.0f),
			Vec3f(1.0f, 0.0f, 0.0f),
			Vec3f(0.0f, 1.0f, 0.0f),
			Vec3f(2.0f, 2.0f, 0.0f)
		},
		std::vector<Vec4u>{ Vec4u(0, 1, 2, MAX_UINT) });

	ASSERT_TRUE(IsValid(mesh));
	ASSERT_TRUE(mesh.MergeVertices(3, 1));
	EXPECT_EQ(mesh.VertexCount(), 3u);
	ExpectVec3Near(mesh.GetVertexPosition(1), Vec3f(1.0f, 0.0f, 0.0f));
	EXPECT_TRUE(IsValid(mesh));
}

TEST(HalfEdgeMeshMerge, CollapsesAdjacentCubeEdge) {
	HalfEdgeMesh mesh = Cube();

	EXPECT_TRUE(mesh.MergeVertices(0, 4));
	EXPECT_EQ(mesh.VertexCount(), 7u);
	EXPECT_EQ(mesh.FaceCount(), 6u);
	EXPECT_EQ(mesh.EdgeCount(), 22u);
	EXPECT_TRUE(IsValid(mesh));
}

TEST(HalfEdgeMeshMerge, AllowsClosedFaceReductionWhenResultStaysClosed) {
	HalfEdgeMesh mesh = Cube();

	// This merge removes a face, but the rebuilt surface remains closed.
	EXPECT_TRUE(mesh.MergeVertices(0, 3));
	EXPECT_EQ(mesh.VertexCount(), 7u);
	EXPECT_EQ(mesh.FaceCount(), 5u);
	EXPECT_EQ(mesh.EdgeCount(), 20u);
	EXPECT_TRUE(IsValid(mesh));
	for (uint edgeId = 0; edgeId < mesh.EdgeCount(); ++edgeId) {
		EXPECT_NE(mesh.GetEdge(edgeId).face, MAX_UINT);
	}
}

TEST(HalfEdgeMeshMerge, RejectsSameVertexWithoutChangingMesh) {
	HalfEdgeMesh mesh = Cube();
	const uint vertexCount = mesh.VertexCount();
	const uint edgeCount = mesh.EdgeCount();
	const Vec3f original = mesh.GetVertexPosition(0);

	EXPECT_FALSE(mesh.MergeVertices(0, 0));
	EXPECT_EQ(mesh.VertexCount(), vertexCount);
	EXPECT_EQ(mesh.EdgeCount(), edgeCount);
	ExpectVec3Near(mesh.GetVertexPosition(0), original);
	EXPECT_TRUE(IsValid(mesh));
}

TEST(HalfEdgeMeshMerge, RejectsCollapseOfLastFace) {
	HalfEdgeMesh mesh;
	mesh.load(
		std::vector<Vec3f>{
			Vec3f(0.0f, 0.0f, 0.0f),
			Vec3f(1.0f, 0.0f, 0.0f),
			Vec3f(0.0f, 1.0f, 0.0f)
		},
		std::vector<Vec4u>{ Vec4u(0, 1, 2, MAX_UINT) });

	const uint vertexCount = mesh.VertexCount();
	const uint faceCount = mesh.FaceCount();
	const uint edgeCount = mesh.EdgeCount();

	EXPECT_FALSE(mesh.MergeVertices(0, 1));
	EXPECT_EQ(mesh.VertexCount(), vertexCount);
	EXPECT_EQ(mesh.FaceCount(), faceCount);
	EXPECT_EQ(mesh.EdgeCount(), edgeCount);
	EXPECT_TRUE(IsValid(mesh));
}

// --- Component scaling ----------------------------------------------------
// AI-Generated Tests of scaling faces and edges.

TEST(HalfEdgeMeshScale, EdgeScalesAroundItsMidpoint) {
	HalfEdgeMesh mesh = Cube();
	const HalfEdgeMesh::HalfEdge edge = mesh.GetEdge(0);
	const Vec3f center = mesh.GetEdgeCenter(0);
	const Vec3f origBefore = mesh.GetVertexPosition(edge.orig);
	const Vec3f destBefore = mesh.GetVertexPosition(edge.dest);
	const Vec3f factors(2.0f, 0.5f, 3.0f);

	mesh.ScaleEdge(0, factors);

	ExpectVec3Near(mesh.GetVertexPosition(edge.orig), center + (origBefore - center) * factors);
	ExpectVec3Near(mesh.GetVertexPosition(edge.dest), center + (destBefore - center) * factors);
	EXPECT_TRUE(IsValid(mesh));
}

TEST(HalfEdgeMeshScale, FaceScalesOnlyItsSharedBoundaryVertices) {
	HalfEdgeMesh mesh = Cube();
	std::vector<Vec3f> before;
	before.reserve(mesh.VertexCount());
	for (uint vertexId = 0; vertexId < mesh.VertexCount(); ++vertexId) {
		before.push_back(mesh.GetVertexPosition(vertexId));
	}

	std::vector<uint> faceVertices;
	mesh.GetFaceVertices(0, faceVertices);
	const std::unordered_set<uint> scaledVertices(faceVertices.begin(), faceVertices.end());
	const Vec3f center = mesh.faceCentroid(0);

	mesh.ScaleFace(0, Vec3f(2.0f));

	for (uint vertexId = 0; vertexId < mesh.VertexCount(); ++vertexId) {
		const Vec3f expected = scaledVertices.count(vertexId)
			? center + (before[vertexId] - center) * 2.0f
			: before[vertexId];
		ExpectVec3Near(mesh.GetVertexPosition(vertexId), expected);
	}
	EXPECT_TRUE(IsValid(mesh));
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

//AI Documentation: AI wrote all the tests below after I wrote the initial tests and scaffolding

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

//AI's original test was wrong as tesselate() is hybrid (I had to fix this myself)
//Catmull-Clark for quads (1 -> 4)
//Loop (1 -> 4 triangles)
//So the pyramid should actually have 20 faces instead of 16
TEST(MeshInvariants, TesselateKeepsPyramidValid) {
	HalfEdgeMesh mesh = Pyramid();
	mesh.tesselate();
	ExpectMesh(mesh, 14u, 20u, 64u); 
}

//This test was also wrong based on the above example (also had to fix)
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

// refineEdge leaves a degree-2 midpoint. Beveling an edge that ends there used to
// leave that edge's side of the strip unset, and the strip took edge 0 instead.
TEST(MeshInvariants, BevelAtDegreeTwoVertexKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.refineEdge(0);
	ASSERT_EQ(mesh.GetEdge(24).orig, 8u) << "edge 24 should be the new half out of midpoint 8";

	HalfEdgeMeshBeveler beveler(mesh);
	beveler.StartEdgeBevel(std::vector<uint>{ 24 });
	beveler.AdjustBevel(0.5f);
	beveler.EndBevel();

	ExpectMesh(mesh, 11u, 7u, 32u);
}

// Both halves of a refined edge: the midpoint has two beveled edges and nothing else.
TEST(MeshInvariants, BevelBothEdgesAtDegreeTwoVertexKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube();
	mesh.refineEdge(0);
	ASSERT_EQ(mesh.GetEdge(0).dest, 8u);
	ASSERT_EQ(mesh.GetEdge(24).orig, 8u);

	HalfEdgeMeshBeveler beveler(mesh);
	beveler.StartEdgeBevel(std::vector<uint>{ 0, 24 });
	beveler.AdjustBevel(0.5f);
	beveler.EndBevel();

	ExpectMesh(mesh, 12u, 8u, 36u);
}
