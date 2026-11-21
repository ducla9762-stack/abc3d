#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

#include "mesh.h"
#include "MeshKnife.h"
#include "shapes.h"

// The knife works on a screen-space path, so these tests look at the +z face of the default cube (it spans
// -1..1) from (0, 0, 3) with a 90 degree lens. At that distance the face covers -0.5..0.5 in NDC.

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

	KnifeCamera CameraAt(const Vec3f& eye) {
		Matrix4f proj, view;
		proj.SetPerspective(degreesToRadians(90.0f), 1.0f, 0.1f, 1000.0f);
		view.SetView(eye, Vec3f(0.0f), Vec3f(0.0f, 1.0f, 0.0f));
		KnifeCamera cam;
		cam.viewProj = proj * view;
		cam.position = eye;
		return cam;
	}

	// Cuts `mesh` as the only thing in the scene
	bool Cut(HalfEdgeMesh& mesh, const std::vector<Vec2f>& path, bool cutThrough, const Vec3f& eye = Vec3f(0.0f, 0.0f, 3.0f)) {
		KnifeOccluders occluders;
		occluders.Add(mesh);
		HalfEdgeMeshKnife knife(mesh);
		const bool changed = knife.Cut(CameraAt(eye), path, cutThrough, occluders);
		EXPECT_EQ(knife.error, "");
		return changed;
	}
}

TEST(Knife, StraightCutSplitsTheVisibleFace) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f) }, false));

	// Two new vertices on the front face's top and bottom edges, and the face is in two.
	// The faces behind it are hidden.
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 10u);
	EXPECT_EQ(mesh.FaceCount(), 7u);
}

TEST(Knife, CutThroughGoesAllTheWayAround) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f) }, true));

	// A ring around the cube: front, top, back and bottom are all split
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 12u);
	EXPECT_EQ(mesh.FaceCount(), 10u);
}

TEST(Knife, MissingTheMeshChangesNothing) {
	HalfEdgeMesh mesh = Cube();
	EXPECT_FALSE(Cut(mesh, { Vec2f(0.7f, -0.9f), Vec2f(0.7f, 0.9f) }, false));
	EXPECT_FALSE(Cut(mesh, { Vec2f(0.7f, -0.9f), Vec2f(0.7f, 0.9f) }, true));
	EXPECT_EQ(mesh.VertexCount(), 8u);
	EXPECT_EQ(mesh.FaceCount(), 6u);
}

TEST(Knife, LineEndingInsideTheFaceLeavesADanglingEdge) {
	HalfEdgeMesh mesh = Cube();
	// Enters from the left and stops in the middle of the face
	ASSERT_TRUE(Cut(mesh, { Vec2f(-0.9f, 0.0f), Vec2f(0.0f, 0.0f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 10u); // one on the left edge, one inside
	EXPECT_EQ(mesh.FaceCount(), 6u);
}

TEST(Knife, LineStartingInsideTheFaceExitsTheEdge) {
	HalfEdgeMesh mesh = Cube();
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.0f, 0.0f), Vec2f(0.9f, 0.0f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 10u);
	EXPECT_EQ(mesh.FaceCount(), 6u);
}

TEST(Knife, LineInsideAFaceThatNeverTouchesAnEdgeIsDropped) {
	HalfEdgeMesh mesh = Cube();
	EXPECT_FALSE(Cut(mesh, { Vec2f(-0.2f, 0.0f), Vec2f(0.2f, 0.0f) }, false));
	EXPECT_EQ(mesh.VertexCount(), 8u);
}

TEST(Knife, BendingPathSplitsTheFaceOnce) {
	HalfEdgeMesh mesh = Cube();
	// Left edge -> inside -> bottom edge. One click lands inside the face and becomes a vertex.
	ASSERT_TRUE(Cut(mesh, { Vec2f(-0.9f, 0.0f), Vec2f(0.0f, 0.0f), Vec2f(0.0f, -0.9f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 11u);
	EXPECT_EQ(mesh.FaceCount(), 7u);
}

TEST(Knife, SelfCrossingPathMakesAVertexAtTheCrossing) {
	HalfEdgeMesh mesh = Cube();
	// A loop: runs in from the left along y = 0, turns up and back over itself, and leaves through the bottom.
	// It crosses itself at (0.1, 0), which is inside the face.
	const std::vector<Vec2f> path = {
		Vec2f(-0.8f, 0.0f), Vec2f(0.3f, 0.0f), Vec2f(0.3f, 0.3f), Vec2f(0.1f, 0.3f), Vec2f(0.1f, -0.8f)
	};
	ASSERT_TRUE(Cut(mesh, path, false));
	EXPECT_TRUE(IsValid(mesh));

	// 2 on the boundary, 1 crossing, 3 clicks inside the face. The loop is a face of its own.
	EXPECT_EQ(mesh.VertexCount(), 14u);
	EXPECT_EQ(mesh.FaceCount(), 8u);
}

TEST(Knife, HiddenFacesAreNotCutUnlessCuttingThrough) {
	HalfEdgeMesh mesh = Cube();
	// Viewed from the back, the same line must cut the -z face and not the +z face
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f) }, false, Vec3f(0.0f, 0.0f, -3.0f)));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.FaceCount(), 7u);

	bool backSplit = false;
	std::vector<Vec3f> world;
	mesh.GetWorldVertexPositions(world);
	for (const Vec3f& v : world) { if (v.z < 0.0f && std::abs(v.x) < 0.99f && std::abs(v.y) > 0.99f) backSplit = true; }
	EXPECT_TRUE(backSplit);
	for (const Vec3f& v : world) { EXPECT_FALSE(v.z > 0.0f && std::abs(v.x) < 0.99f && std::abs(v.y) > 0.99f); }
}

TEST(Knife, AnotherMeshInFrontHidesTheCut) {
	HalfEdgeMesh mesh = Cube();

	// A bigger cube between the camera and the first one hides it completely
	HalfEdgeMesh blocker = Cube();
	blocker.Scale(Vec3f(3.0f));
	blocker.Pos(Vec3f(0.0f, 0.0f, 0.0f));

	KnifeOccluders occluders;
	occluders.Add(blocker);

	HalfEdgeMeshKnife knife(mesh);
	EXPECT_FALSE(knife.Cut(CameraAt(Vec3f(0.0f, 0.0f, 8.0f)), { Vec2f(0.05f, -0.9f), Vec2f(0.05f, 0.9f) }, false, occluders));
	EXPECT_EQ(mesh.VertexCount(), 8u);
}

TEST(Knife, RespectsTheMeshTransform) {
	HalfEdgeMesh mesh = Cube();
	mesh.Scale(Vec3f(2.0f, 1.0f, 1.0f));
	mesh.Pos(Vec3f(0.5f, 0.0f, 0.0f));
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 10u);
	EXPECT_EQ(mesh.FaceCount(), 7u);

	// The new vertices sit on the top and bottom edges
	std::vector<Vec3f> world;
	mesh.GetWorldVertexPositions(world);
	int onTop = 0, onBottom = 0;
	for (uint v = 8; v < world.size(); ++v) {
		if (std::abs(world[v].y - 1.0f) < 1e-4f) ++onTop;
		if (std::abs(world[v].y + 1.0f) < 1e-4f) ++onBottom;
	}
	EXPECT_EQ(onTop, 1);
	EXPECT_EQ(onBottom, 1);
}

TEST(Knife, CutsAnOpenMeshBoundary) {
	// A single quad: the knife has to split edges that have no neighbor face
	HalfEdgeMesh mesh;
	std::vector<Vec3f> verts = { Vec3f(-1, -1, 0), Vec3f(1, -1, 0), Vec3f(1, 1, 0), Vec3f(-1, 1, 0) };
	std::vector<Vec4u> faces = { Vec4u(0, 1, 2, 3) };
	mesh.load(verts, faces);
	ASSERT_TRUE(Cut(mesh, { Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.FaceCount(), 2u);
	EXPECT_EQ(mesh.VertexCount(), 6u);
}

TEST(Knife, EdgeIsSplitAtEquallySpacedHitsWhenCutTwice) {
	HalfEdgeMesh mesh = Cube();
	// Two parallel strokes in one path (zig zag) cross the top and bottom edges twice each
	ASSERT_TRUE(Cut(mesh, { Vec2f(-0.2f, -0.9f), Vec2f(-0.2f, 0.9f), Vec2f(0.2f, 0.9f), Vec2f(0.2f, -0.9f) }, false));
	EXPECT_TRUE(IsValid(mesh));
	EXPECT_EQ(mesh.VertexCount(), 12u);
	EXPECT_EQ(mesh.FaceCount(), 8u);
}

TEST(Refine, SplitsAtAParameter) {
	HalfEdgeMesh mesh = Cube();
	const uint before = mesh.VertexCount();
	const uint v = mesh.refineEdge(0, 0.25f);
	EXPECT_EQ(v, before);
	EXPECT_TRUE(IsValid(mesh));

	const HalfEdgeMesh::HalfEdge e = mesh.GetEdge(0);
	EXPECT_EQ(e.dest, v);
}

// Random paths, with random camera angles, over a few shapes. The knife either changes nothing or produces a mesh
// that validates (it reports an error when its own result was invalid, so an empty error is what matters).
namespace {
	struct FuzzShape { const char* name; std::vector<Vec3f> verts; std::vector<Vec4u> faces; };

	std::vector<FuzzShape> FuzzShapes() {
		std::vector<FuzzShape> shapes;
		FuzzShape cube{ "cube", std::vector<Vec3f>(CUBE_VERTICES.begin(), CUBE_VERTICES.end()), std::vector<Vec4u>(CUBE_FACES.begin(), CUBE_FACES.end()) };
		shapes.push_back(cube);
		FuzzShape sphere{ "sphere", {}, {} };
		generateUVSphere(sphere.verts, sphere.faces, 8, 12);
		shapes.push_back(sphere);
		FuzzShape torus{ "torus", {}, {} };
		generateUnitTorus(torus.verts, torus.faces, 12, 8);
		shapes.push_back(torus);
		FuzzShape cylinder{ "cylinder", {}, {} };
		generateUnitCylinder(cylinder.verts, cylinder.faces, 10);
		shapes.push_back(cylinder);
		return shapes;
	}
}

TEST(Knife, RandomPathsAlwaysLeaveAValidMesh) {
	std::mt19937 rng(12345);
	std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
	std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
	std::uniform_int_distribution<int> points(2, 7);

	int changedCount = 0;
	for (const FuzzShape& shape : FuzzShapes()) {
		for (int trial = 0; trial < 150; ++trial) {
			HalfEdgeMesh mesh;
			mesh.load(shape.verts, shape.faces);

			const float az = angle(rng), el = unit(rng) * 1.2f, dist = 3.0f + std::abs(unit(rng)) * 2.0f;
			const Vec3f eye(dist * std::cos(el) * std::sin(az), dist * std::sin(el), dist * std::cos(el) * std::cos(az));

			std::vector<Vec2f> path;
			for (int i = 0, n = points(rng); i < n; ++i) { path.push_back(Vec2f(unit(rng) * 0.8f, unit(rng) * 0.8f)); }

			KnifeOccluders occluders;
			occluders.Add(mesh);
			HalfEdgeMeshKnife knife(mesh);
			const bool through = trial % 3 == 0;
			const bool changed = knife.Cut(CameraAt(eye), path, through, occluders);

			ASSERT_EQ(knife.error, "") << shape.name << " trial " << trial;
			ASSERT_TRUE(IsValid(mesh)) << shape.name << " trial " << trial;
			if (changed) { ++changedCount; }
		}
	}

	// Make sure the test is cutting things, not just missing
	EXPECT_GT(changedCount, 100);
}

TEST(Knife, ReportsWhichSegmentsCut) {
	HalfEdgeMesh mesh = Cube();
	KnifeOccluders occluders;
	occluders.Add(mesh);

	// Down the front face, then off to the side where there is nothing
	std::vector<char> segments;
	HalfEdgeMeshKnife knife(mesh);
	ASSERT_TRUE(knife.Cut(CameraAt(Vec3f(0.0f, 0.0f, 3.0f)),
		{ Vec2f(0.1f, -0.9f), Vec2f(0.1f, 0.9f), Vec2f(0.9f, 0.9f) }, false, occluders, &segments));
	EXPECT_EQ(segments, std::vector<char>({ 1, 0 }));
}

TEST(Knife, SegmentReportIsSharedBetweenMeshes) {
	// Two cubes side by side, each path segment over a different one
	HalfEdgeMesh left = Cube(), right = Cube();
	left.Pos(Vec3f(-3.0f, 0.0f, 0.0f));
	right.Pos(Vec3f(3.0f, 0.0f, 0.0f));
	KnifeOccluders occluders;
	occluders.Add(left);
	occluders.Add(right);

	const KnifeCamera cam = CameraAt(Vec3f(0.0f, 0.0f, 10.0f));
	const std::vector<Vec2f> path = { Vec2f(-0.3f, -0.9f), Vec2f(-0.3f, 0.9f), Vec2f(0.3f, 0.9f), Vec2f(0.3f, -0.9f) };

	std::vector<char> segments;
	HalfEdgeMeshKnife(left).Cut(cam, path, false, occluders, &segments);
	EXPECT_EQ(segments, std::vector<char>({ 1, 0, 0 }));
	HalfEdgeMeshKnife(right).Cut(cam, path, false, occluders, &segments);
	EXPECT_EQ(segments, std::vector<char>({ 1, 0, 1 })); // adds to what the left cube reported
}

TEST(KnifeScreenCover, TellsWhenASegmentIsOverTheMesh) {
	KnifeScreenCover cover;
	cover.Add(Cube());
	cover.Project(CameraAt(Vec3f(0.0f, 0.0f, 3.0f)).viewProj);

	EXPECT_TRUE(cover.Crosses(Vec2f(-0.9f, 0.0f), Vec2f(0.9f, 0.0f)));   // across the face
	EXPECT_TRUE(cover.Crosses(Vec2f(-0.1f, 0.1f), Vec2f(0.1f, -0.1f)));  // inside the face, touching no edge
	EXPECT_FALSE(cover.Crosses(Vec2f(0.7f, -0.9f), Vec2f(0.7f, 0.9f)));  // beside it
}

TEST(KnifeScreenCover, CountsTrianglesThatPassBehindTheCamera) {
	// A wide floor under the camera that runs past it
	HalfEdgeMesh floor = Cube();
	floor.Scale(Vec3f(100.0f, 1.0f, 100.0f));
	floor.Pos(Vec3f(0.0f, -2.0f, 0.0f));

	KnifeScreenCover cover;
	cover.Add(floor);
	cover.Project(CameraAt(Vec3f(0.0f, 0.0f, 3.0f)).viewProj);

	EXPECT_TRUE(cover.Crosses(Vec2f(-0.1f, -0.9f), Vec2f(0.1f, -0.9f)));
	EXPECT_FALSE(cover.Crosses(Vec2f(-0.1f, 0.9f), Vec2f(0.1f, 0.9f)));
}

TEST(KnifeScreenCover, EmptyCoversNothing) {
	KnifeScreenCover cover;
	EXPECT_TRUE(cover.Empty());
	cover.Project(CameraAt(Vec3f(0.0f, 0.0f, 3.0f)).viewProj);
	EXPECT_FALSE(cover.Crosses(Vec2f(-1.0f, -1.0f), Vec2f(1.0f, 1.0f)));
}
