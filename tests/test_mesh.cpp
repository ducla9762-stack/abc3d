#include <gtest/gtest.h>

#include "mesh.h"

// Test for the build wiring: abc3d_core links without FLTK or OpenGL
TEST(HalfEdgeMeshLoad, BoxHasEightVertices) {
	FaceList faceList;
	ASSERT_TRUE(faceList.LoadObj(ABC3D_MODELS_DIR "/box.obj"));

	// load() forwards to the private build(), passing Polygons() as the face
	// container and polyVertexCount as the vertex-count function.
	HalfEdgeMesh mesh;
	ASSERT_TRUE(mesh.load(faceList));

	EXPECT_EQ(mesh.VertexCount(), 8u);
}
