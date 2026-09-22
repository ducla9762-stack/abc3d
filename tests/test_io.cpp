//AI Documentation: AI wrote this entire file

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "mesh.h"
#include "shapes.h"

// Import/export tests. Two groups:
//
//   ImportContract - the fixtures in assets/models, including the deliberately broken
//                    ones. Each one's exact ImportReport is pinned in kFixtures below.
//   RoundTrip      - export a mesh, read it back, and check whatever the format
//                    actually promises to preserve. That promise differs per format,
//                    so it is driven by the flags on a Format descriptor rather than
//                    by the format's name.
namespace {

	// --- shared helpers ----------------------------------------------------

	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	std::string ModelPath(const char* name) {
		return std::string(ABC3D_MODELS_DIR "/") + name;
	}

	// Test output goes to a temp dir, never into assets/models: that directory is the
	// fixture meshes and must not accumulate artifacts.
	std::string TempPath(const std::string& name) {
		std::filesystem::path dir = std::filesystem::temp_directory_path() / "abc3d_io_tests";
		std::filesystem::create_directories(dir);
		return (dir / name).string();
	}

	const char* StatusName(ImportReport::Status s) {
		switch (s) {
		case ImportReport::Ok:              return "Ok";
		case ImportReport::FileError:       return "FileError";
		case ImportReport::InvalidTopology: return "InvalidTopology";
		}
		return "?";
	}

	// --- fixture contract table -------------------------------------------

	struct Fixture {
		const char* file;
		ImportReport::Status status;
		uint skippedFaces;   // A file can import Ok and still have lost face lines.
		uint meshCount;      // Only meaningful when status is Ok.
	};

	// Pinned from observed behaviour, cross-checked against the intent each fixture
	// documents in its own header comment. A change here is a change in import
	// behaviour, so it should be a deliberate edit rather than a test fix.
	const Fixture kFixtures[] = {
		// Deliberately broken inputs
		{ "bad.obj",                      ImportReport::InvalidTopology, 0, 0 }, // two vertex blocks, so faces 1-8 repeat a directed edge
		{ "degen.obj",                    ImportReport::InvalidTopology, 0, 0 }, // index-valid, zero-area geometry
		{ "oob.obj",                      ImportReport::InvalidTopology, 0, 0 }, // face references vertex 9 of 3
		{ "truncated.stl",                ImportReport::FileError,       0, 0 }, // claims 12 triangles, carries 6
		// Rejected during parse, so the file reports Ok with nothing in it. This is the
		// outcome a plain "did it fail" test cannot see, and the one negidx.obj exists
		// to pin: the '-' guard drops the face line, leaving 3 vertices and no faces,
		// and loadMeshes then has no part with a face to build.
		{ "negidx.obj",                   ImportReport::Ok,              1, 0 },
		// A valid binary STL body with one byte of trailing garbage. The size check is
		// exact equality, so the file falls through to the ASCII parser and is rejected
		// outright rather than read up to the garbage. Strict by choice, not by accident.
		{ "trailing.stl",                 ImportReport::FileError,       0, 0 },
		// Well-formed as a binary STL (84 + 50*768 bytes), so it imports; "bad" here is
		// about the geometry it carries, not about the container.
		{ "bad.stl",                      ImportReport::Ok,              0, 1 },
		// Known-good inputs, so a parser regression cannot hide behind the broken
		// fixtures all still failing, only for a new and wrong reason.
		{ "box.obj",                      ImportReport::Ok,              0, 1 },
		{ "dodecahedron.obj",             ImportReport::Ok,              0, 1 },
		{ "ico_sphere.obj",               ImportReport::Ok,              0, 1 },
		{ "Cube.stl",                     ImportReport::Ok,              0, 1 },
		{ "weld_signed_zero.stl",         ImportReport::Ok,              0, 1 },
		{ "weld_signed_zero_control.stl", ImportReport::Ok,              0, 1 },
	};

	// --- topology snapshots ------------------------------------------------

	// Each face as a cycle, in face order, starting at that face's own start edge -
	// which is exactly the order appendObj writes it in. Recorded twice: by vertex
	// index, and by world position.
	//
	// The positions are the comparable form across an import. FaceList::SplitObjects
	// renumbers each part's vertices by order of first use in the face list, so indices
	// survive a round trip only up to that renumbering, while positions do not move.
	struct Topology {
		uint verts = 0, faces = 0, edges = 0;
		std::vector<std::vector<uint>> cycles;
		std::vector<std::vector<Vec3f>> posCycles;
		std::vector<Vec3f> world;
	};

	Topology Snapshot(HalfEdgeMesh& mesh) {
		Topology t;
		t.verts = mesh.VertexCount();
		t.faces = mesh.FaceCount();
		t.edges = mesh.EdgeCount();
		mesh.GetWorldVertexPositions(t.world);

		t.cycles.reserve(t.faces);
		t.posCycles.reserve(t.faces);
		for (uint f = 0; f < t.faces; ++f) {
			const uint first = mesh.GetTriEdges(f)[0]; // == faceEdges[f]
			std::vector<uint> cycle;
			std::vector<Vec3f> posCycle;
			uint e = first;
			do {
				const uint v = mesh.GetEdge(e).orig;
				cycle.push_back(v);
				posCycle.push_back(t.world[v]);
				e = mesh.GetEdge(e).next;
			} while (e != first && cycle.size() <= t.edges);

			t.cycles.push_back(std::move(cycle));
			t.posCycles.push_back(std::move(posCycle));
		}
		return t;
	}

	// The exporters stream floats at the default 6 significant digits, so positions
	// come back rounded rather than bit-exact. Relative so it holds at any magnitude.
	::testing::AssertionResult NearRel(const Vec3f& a, const Vec3f& b) {
		for (int i = 0; i < 3; ++i) {
			const float tol = 1e-5f * std::max(1.0f, std::fabs(a[i]));
			if (std::fabs(a[i] - b[i]) > tol) {
				return ::testing::AssertionFailure()
					<< "component " << i << ": " << a[i] << " vs " << b[i];
			}
		}
		return ::testing::AssertionSuccess();
	}

	// A face's start vertex is not guaranteed across a round trip, so cycles are
	// compared up to rotation. Winding order still has to match.
	template <typename T, typename Eq>
	bool SameCycleUpToRotation(const std::vector<T>& a, const std::vector<T>& b, Eq equal) {
		if (a.size() != b.size()) return false;
		if (a.empty()) return true;

		for (size_t shift = 0; shift < a.size(); ++shift) {
			bool same = true;
			for (size_t i = 0; i < a.size(); ++i) {
				if (!equal(a[i], b[(i + shift) % b.size()])) { same = false; break; }
			}
			if (same) return true;
		}
		return false;
	}

	bool SamePosCycle(const std::vector<Vec3f>& a, const std::vector<Vec3f>& b) {
		return SameCycleUpToRotation(a, b, [](const Vec3f& x, const Vec3f& y) {
			return static_cast<bool>(NearRel(x, y));
			});
	}

	// --- geometric oracles that survive triangulation ----------------------

	float TotalArea(const HalfEdgeMesh& mesh) {
		std::vector<Vec3f> world;
		mesh.GetWorldVertexPositions(world);

		std::vector<Vec3u> tris;
		double area = 0.0;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			tris.clear();
			mesh.GetTriangulation(f, tris);
			for (const Vec3u& t : tris) {
				const Vec3f e0 = world[t[1]] - world[t[0]];
				const Vec3f e1 = world[t[2]] - world[t[0]];
				area += 0.5 * static_cast<double>(e0.Cross(e1).Length());
			}
		}
		return static_cast<float>(area);
	}

	void BoundingBox(const HalfEdgeMesh& mesh, Vec3f& lo, Vec3f& hi) {
		std::vector<Vec3f> world;
		mesh.GetWorldVertexPositions(world);
		ASSERT_FALSE(world.empty());

		lo = hi = world[0];
		for (const Vec3f& v : world) {
			for (int i = 0; i < 3; ++i) {
				lo[i] = std::min(lo[i], v[i]);
				hi[i] = std::max(hi[i], v[i]);
			}
		}
	}

	// The triangles the exporter will actually emit: the triangulation minus the
	// degenerate ones appendStlTriangles drops.
	uint ExportedTriangleCount(const HalfEdgeMesh& mesh) {
		std::vector<Triangle> tris;
		mesh.appendStlTriangles(tris);
		return static_cast<uint>(tris.size());
	}

	// What welding on re-import should collapse that triangle soup back down to.
	uint UniqueWeldedVertexCount(const HalfEdgeMesh& mesh) {
		struct Hash {
			size_t operator()(const Vec3f& v) const {
				auto bits = [](float f) {
					uint32_t i; std::memcpy(&i, &f, 4);
					return (i == 0x80000000u) ? 0u : i; // -0.0f welds with 0.0f
					};
				size_t h = std::hash<uint32_t>{}(bits(v.x));
				h ^= std::hash<uint32_t>{}(bits(v.y)) + 0x9e3779b9u + (h << 6) + (h >> 2);
				h ^= std::hash<uint32_t>{}(bits(v.z)) + 0x9e3779b9u + (h << 6) + (h >> 2);
				return h;
			}
		};
		struct Eq {
			bool operator()(const Vec3f& a, const Vec3f& b) const {
				return a.x == b.x && a.y == b.y && a.z == b.z;
			}
		};

		std::vector<Triangle> tris;
		mesh.appendStlTriangles(tris);

		std::unordered_set<Vec3f, Hash, Eq> seen;
		for (const Triangle& t : tris) { seen.insert(t.v0); seen.insert(t.v1); seen.insert(t.v2); }
		return static_cast<uint>(seen.size());
	}

	// --- format descriptors ------------------------------------------------

	// Adding a format is one row in kFormats. The flags, not the name, decide which
	// assertions run, so a new format inherits the right subset automatically.
	struct Format {
		const char* name;
		const char* ext;
		bool (*write)(const HalfEdgeMesh&, const std::string& path);

		bool triangulates;   // n-gons are flattened, so topology is not preserved
		bool welds;          // vertex identity is rebuilt from positions on import
		bool bakesTransform; // pos/rot/scale are folded into the written vertices
	};

	bool WriteObj(const HalfEdgeMesh& m, const std::string& p) { return m.exportObj(p.c_str()); }
	bool WriteStlBin(const HalfEdgeMesh& m, const std::string& p) { return m.exportStl(p.c_str(), true); }
	bool WriteStlAscii(const HalfEdgeMesh& m, const std::string& p) { return m.exportStl(p.c_str(), false); }

	const Format kFormats[] = {
		{ "obj",       ".obj", &WriteObj,      false, false, true },
		{ "stl-bin",   ".stl", &WriteStlBin,   true,  true,  true },
		{ "stl-ascii", ".stl", &WriteStlAscii, true,  true,  true },
	};

	// --- mesh corpus -------------------------------------------------------

	struct NamedMesh {
		std::string name;
		HalfEdgeMesh mesh;
	};

	HalfEdgeMesh Cube() {
		HalfEdgeMesh m; m.load(CUBE_VERTICES, CUBE_FACES); return m;
	}
	HalfEdgeMesh Pyramid() {
		HalfEdgeMesh m; m.load(PYRAMID_VERTICES, PYRAMID_FACES); return m;
	}
	HalfEdgeMesh Torus() {
		std::vector<Vec3f> v; std::vector<Vec4u> f;
		generateUnitTorus(v, f, 8, 6);
		HalfEdgeMesh m; m.load(v, f); return m;
	}
	HalfEdgeMesh Sphere() {
		std::vector<Vec3f> v; std::vector<Vec4u> f;
		generateUVSphere(v, f, 6, 8);
		HalfEdgeMesh m; m.load(v, f); return m;
	}
	HalfEdgeMesh FromFile(const char* file) {
		std::vector<HalfEdgeMesh> meshes;
		const ImportReport r = loadMeshes(ModelPath(file).c_str(), meshes);
		EXPECT_TRUE(r.ok()) << file << ": " << StatusName(r.status);
		EXPECT_EQ(meshes.size(), 1u) << file;
		return meshes.empty() ? HalfEdgeMesh() : std::move(meshes[0]);
	}

	// Adding a mesh is one row here, and it is exercised against every format.
	std::vector<NamedMesh> Corpus() {
		std::vector<NamedMesh> corpus;
		corpus.push_back({ "cube",         Cube() });
		corpus.push_back({ "pyramid",      Pyramid() });
		corpus.push_back({ "torus",        Torus() });
		corpus.push_back({ "uv_sphere",    Sphere() });
		corpus.push_back({ "dodecahedron", FromFile("dodecahedron.obj") });
		corpus.push_back({ "cube_stl",     FromFile("Cube.stl") });
		return corpus;
	}

	// Writes `mesh` in `fmt` and reads it back as a single mesh.
	::testing::AssertionResult RoundTrip(const Format& fmt, const HalfEdgeMesh& mesh,
		const std::string& tag, HalfEdgeMesh& out) {
		const std::string path = TempPath(tag + "_" + fmt.name + fmt.ext);
		if (!fmt.write(mesh, path))
			return ::testing::AssertionFailure() << "write failed: " << path;

		std::vector<HalfEdgeMesh> meshes;
		const ImportReport report = loadMeshes(path.c_str(), meshes);
		if (!report.ok())
			return ::testing::AssertionFailure() << "re-import: " << StatusName(report.status);
		if (report.skippedFaces != 0)
			return ::testing::AssertionFailure() << "re-import skipped " << report.skippedFaces << " faces";
		if (meshes.size() != 1)
			return ::testing::AssertionFailure() << "re-import produced " << meshes.size() << " meshes";

		out = std::move(meshes[0]);
		return ::testing::AssertionSuccess();
	}

	class RoundTripTest : public ::testing::TestWithParam<Format> {};

	std::string FormatName(const ::testing::TestParamInfo<Format>& info) {
		std::string n = info.param.name;
		std::replace(n.begin(), n.end(), '-', '_');
		return n;
	}
}

// --- 1. Import contract ----------------------------------------------------
// The assertion is the exact report, not just "did it fail": a file can import Ok
// and still have dropped faces, and that outcome is the one worth catching.

TEST(ImportContract, FixturesReportExpectedStatus) {
	for (const Fixture& fx : kFixtures) {
		SCOPED_TRACE(fx.file);

		std::vector<HalfEdgeMesh> meshes;
		const ImportReport report = loadMeshes(ModelPath(fx.file).c_str(), meshes);

		EXPECT_EQ(report.status, fx.status)
			<< "got " << StatusName(report.status) << ", want " << StatusName(fx.status);
		EXPECT_EQ(report.skippedFaces, fx.skippedFaces);

		if (fx.status == ImportReport::Ok) {
			EXPECT_EQ(meshes.size(), fx.meshCount);
			for (const HalfEdgeMesh& m : meshes) EXPECT_TRUE(IsValid(m));
		}
	}
}

// loadMeshes documents all-or-nothing: a rejected file must leave the caller's vector
// exactly as it was, so a failed import cannot leave half a model behind.
TEST(ImportContract, FailedImportLeavesOutputUntouched) {
	for (const Fixture& fx : kFixtures) {
		if (fx.status == ImportReport::Ok) continue;
		SCOPED_TRACE(fx.file);

		std::vector<HalfEdgeMesh> meshes;
		meshes.push_back(Cube()); // A sentinel the failed import must not disturb

		const ImportReport report = loadMeshes(ModelPath(fx.file).c_str(), meshes);
		ASSERT_FALSE(report.ok());

		ASSERT_EQ(meshes.size(), 1u);
		EXPECT_EQ(meshes[0].VertexCount(), 8u);
		EXPECT_EQ(meshes[0].FaceCount(), 6u);
		EXPECT_TRUE(IsValid(meshes[0]));
	}
}

TEST(ImportContract, MissingFileIsFileError) {
	std::vector<HalfEdgeMesh> meshes;
	EXPECT_EQ(loadMeshes(ModelPath("does_not_exist.obj").c_str(), meshes).status,
		ImportReport::FileError);
	EXPECT_TRUE(meshes.empty());
}

TEST(ImportContract, UnsupportedExtensionIsFileError) {
	std::vector<HalfEdgeMesh> meshes;
	EXPECT_EQ(loadMeshes(ModelPath("make_weld_fixtures.py").c_str(), meshes).status,
		ImportReport::FileError);
	EXPECT_TRUE(meshes.empty());
}

// The two weld fixtures differ only in the sign of a zero coordinate, so they must
// weld to the same topology. If -0.0f ever stops hashing with 0.0f, this splits.
TEST(ImportContract, SignedZeroWeldsLikeItsControl) {
	const HalfEdgeMesh a = FromFile("weld_signed_zero.stl");
	const HalfEdgeMesh b = FromFile("weld_signed_zero_control.stl");

	ASSERT_TRUE(IsValid(a));
	ASSERT_TRUE(IsValid(b));

	// 200 rim points plus the one shared apex, per make_weld_fixtures.py. Before the
	// weld fix the mixed-sign file came back with 202.
	EXPECT_EQ(a.VertexCount(), 201u);
	EXPECT_EQ(a.VertexCount(), b.VertexCount());
	EXPECT_EQ(a.FaceCount(), b.FaceCount());
	EXPECT_EQ(a.EdgeCount(), b.EdgeCount());
}

// --- 2. Round trips --------------------------------------------------------

// Holds for every format regardless of what it loses: the file re-imports, and the
// mesh it produces is internally consistent and the same solid geometrically.
TEST_P(RoundTripTest, ReimportsIntoAValidMesh) {
	const Format& fmt = GetParam();

	for (NamedMesh& nm : Corpus()) {
		SCOPED_TRACE(nm.name);
		ASSERT_TRUE(IsValid(nm.mesh));

		HalfEdgeMesh back;
		ASSERT_TRUE(RoundTrip(fmt, nm.mesh, nm.name, back));
		EXPECT_TRUE(IsValid(back));

		const float area = TotalArea(nm.mesh);
		EXPECT_NEAR(TotalArea(back), area, 1e-3f * area);

		Vec3f lo0, hi0, lo1, hi1;
		BoundingBox(nm.mesh, lo0, hi0);
		BoundingBox(back, lo1, hi1);
		EXPECT_TRUE(NearRel(lo0, lo1));
		EXPECT_TRUE(NearRel(hi0, hi1));
	}
}

// A format that loses nothing must come back face for face: same counts, same face
// order, same winding, each face spanning the same points. Compared by position
// rather than by index, because SplitObjects renumbers the vertices on import.
TEST_P(RoundTripTest, LosslessFormatsPreserveTopologyExactly) {
	const Format& fmt = GetParam();
	if (fmt.triangulates || fmt.welds) GTEST_SKIP() << fmt.name << " is a lossy format";

	for (NamedMesh& nm : Corpus()) {
		SCOPED_TRACE(nm.name);

		HalfEdgeMesh back;
		ASSERT_TRUE(RoundTrip(fmt, nm.mesh, nm.name, back));

		const Topology a = Snapshot(nm.mesh);
		const Topology b = Snapshot(back);

		ASSERT_EQ(b.verts, a.verts);
		ASSERT_EQ(b.faces, a.faces);
		EXPECT_EQ(b.edges, a.edges);

		for (uint f = 0; f < a.faces; ++f) {
			SCOPED_TRACE(f);
			EXPECT_TRUE(SamePosCycle(a.posCycles[f], b.posCycles[f]));
		}
	}
}

// SplitObjects only carries the vertices a part's own faces use, so a vertex no face
// references is dropped on import. That is why the check above compares counts: it
// only holds because nothing in the corpus has an orphan vertex.
TEST(ImportContract, UnreferencedVertexIsDropped) {
	const std::string path = TempPath("orphan_vertex.obj");
	{
		std::ofstream out(path);
		out << "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 5 5 5\nf 1 2 3\n"; // vertex 4 is unused
	}

	std::vector<HalfEdgeMesh> meshes;
	ASSERT_TRUE(loadMeshes(path.c_str(), meshes).ok());
	ASSERT_EQ(meshes.size(), 1u);

	EXPECT_EQ(meshes[0].VertexCount(), 3u);
	EXPECT_EQ(meshes[0].FaceCount(), 1u);
	EXPECT_TRUE(IsValid(meshes[0]));
}

// A lossy format is a projection, so what it promises is the shape of the result:
// one face per emitted triangle, and vertices welded back down by position.
TEST_P(RoundTripTest, LossyFormatsProjectToTheExpectedShape) {
	const Format& fmt = GetParam();
	if (!fmt.triangulates) GTEST_SKIP() << fmt.name << " is lossless";

	for (NamedMesh& nm : Corpus()) {
		SCOPED_TRACE(nm.name);

		HalfEdgeMesh back;
		ASSERT_TRUE(RoundTrip(fmt, nm.mesh, nm.name, back));

		EXPECT_EQ(back.FaceCount(), ExportedTriangleCount(nm.mesh));

		const Topology t = Snapshot(back);
		for (uint f = 0; f < t.faces; ++f)
			EXPECT_EQ(t.cycles[f].size(), 3u) << "face " << f << " is not a triangle";

		if (fmt.welds) EXPECT_EQ(back.VertexCount(), UniqueWeldedVertexCount(nm.mesh));
	}
}

// The invariant that generalises to every format, lossy or not: one pass through the
// format normalises the mesh, so a second pass must change nothing at all. Positions
// compare bit-exact here, because re-rounding an already rounded value is stable.
TEST_P(RoundTripTest, SecondRoundTripIsAFixedPoint) {
	const Format& fmt = GetParam();

	for (NamedMesh& nm : Corpus()) {
		SCOPED_TRACE(nm.name);

		HalfEdgeMesh once, twice;
		ASSERT_TRUE(RoundTrip(fmt, nm.mesh, nm.name, once));
		ASSERT_TRUE(RoundTrip(fmt, once, nm.name + "_2", twice));

		const Topology a = Snapshot(once);
		const Topology b = Snapshot(twice);

		ASSERT_EQ(b.verts, a.verts);
		ASSERT_EQ(b.faces, a.faces);
		EXPECT_EQ(b.edges, a.edges);
		EXPECT_EQ(b.cycles, a.cycles);

		for (uint v = 0; v < a.verts; ++v) {
			SCOPED_TRACE(v);
			EXPECT_EQ(a.world[v].x, b.world[v].x);
			EXPECT_EQ(a.world[v].y, b.world[v].y);
			EXPECT_EQ(a.world[v].z, b.world[v].z);
		}
	}
}

// Every exporter writes world space, so rotation and scale are baked into the
// vertices and cannot come back. Position does come back, but not as zero: build()
// ends by re-centering the vertices on the mesh's area-weighted centroid and moving
// that offset into pos, so the transform is reset to "translated to where the
// geometry already is" rather than to the identity.
TEST_P(RoundTripTest, TransformIsBakedIntoTheVertices) {
	const Format& fmt = GetParam();
	if (!fmt.bakesTransform) GTEST_SKIP() << fmt.name << " preserves transforms";

	HalfEdgeMesh mesh = Cube();
	mesh.Pos(Vec3f(3.0f, -1.5f, 0.25f));
	mesh.Rot(Vec3f(0.0f, 30.0f, 0.0f));
	mesh.Scale(Vec3f(2.0f, 0.5f, 1.0f));

	HalfEdgeMesh back;
	ASSERT_TRUE(RoundTrip(fmt, mesh, "transformed_cube", back));
	ASSERT_TRUE(IsValid(back));

	EXPECT_EQ(back.Rot(), Vec3f(0.0f));
	EXPECT_EQ(back.Scale(), Vec3f(1.0f));

	// The world shape is unchanged by any of that.
	Vec3f lo0, hi0, lo1, hi1;
	BoundingBox(mesh, lo0, hi0);
	BoundingBox(back, lo1, hi1);
	EXPECT_TRUE(NearRel(lo0, lo1));
	EXPECT_TRUE(NearRel(hi0, hi1));

	// A box stays symmetric under an affine transform, so its area-weighted centroid
	// is the centre of its bounding box - which is where pos should have landed.
	EXPECT_TRUE(NearRel(back.Pos(), (lo1 + hi1) * 0.5f));
}

INSTANTIATE_TEST_SUITE_P(Formats, RoundTripTest, ::testing::ValuesIn(kFormats), FormatName);

// ensureExtension appends rather than replaces, so exporting to a path that already
// ends in the other format's extension produces "name.stl.obj". Pinned because the
// round-trip helper above depends on knowing where the file actually lands.
TEST(Export, ExtensionIsAppendedNotReplaced) {
	const std::string path = TempPath("ext_check.stl");
	ASSERT_TRUE(Cube().exportObj(path.c_str()));

	EXPECT_TRUE(std::filesystem::exists(path + ".obj"));
	EXPECT_FALSE(std::filesystem::exists(path));
}
