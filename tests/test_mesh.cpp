#include <gtest/gtest.h>

#include <string>

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
}

// Test for the build wiring: abc3d_core links without FLTK or OpenGL.
TEST(HalfEdgeMeshLoad, BoxHasEightVertices) {
	FaceList faceList;
	ASSERT_TRUE(faceList.LoadObj(ABC3D_MODELS_DIR "/box.obj"));

	HalfEdgeMesh mesh;
	ASSERT_TRUE(mesh.load(faceList));

	EXPECT_EQ(mesh.VertexCount(), 8u);
}

TEST(MeshInvariants, CubeStartsValid) {
	HalfEdgeMesh mesh = Cube(); 
	ASSERT_TRUE(IsValid(mesh)); 
	EXPECT_EQ(mesh.VertexCount(), 8u);
	EXPECT_EQ(mesh.FaceCount(), 6u);
	EXPECT_EQ(mesh.EdgeCount(), 24u);
}

TEST(MeshInvariants, ExtrudeKeepsCubeValid) {
	HalfEdgeMesh mesh = Cube(); 
	mesh.Extrude(0); 
	EXPECT_TRUE(IsValid(mesh)); 
}
