#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "mesh.h"
#include "scene.h"
#include "selectionManager.h"
#include "sceneCommands.h"
#include "shapes.h"
#include "undoHistory.h"

namespace {
	// A minimal document: what the viewport owns, without Qt or GL.
	struct Doc {
		Scene scene;
		SelectionManager selection;
		DisplaySettings display;
		DocumentRefs refs{ scene, selection, display };
		UndoHistory history;
		DocumentTransaction step;

		HalfEdgeMesh* AddCube(const std::string& name = "Cube") {
			HalfEdgeMesh* mesh = scene.NewMesh(name);
			mesh->load(CUBE_VERTICES, CUBE_FACES);
			return mesh;
		}

		void Begin(const char* label = "Edit") { step.begin(refs, label); }
		bool Commit() {
			std::unique_ptr<Command> cmd = step.commit(refs);
			if (!cmd) { return false; }
			history.push(std::move(cmd));
			return true;
		}
		bool Undo() { return history.undo(refs); }
		bool Redo() { return history.redo(refs); }

		// Same as Viewport's recorded delete.
		void DeleteMesh(uint index) {
			auto detached = scene.DetachMesh(index);
			step.recordStructural(std::make_unique<DeleteMeshCommand>(index, std::move(detached.first), std::move(detached.second)));
		}
		HalfEdgeMesh* NewMesh(const std::string& name) {
			HalfEdgeMesh* mesh = scene.NewMesh(name);
			step.recordStructural(std::make_unique<AddMeshCommand>(scene.MeshCount() - 1));
			return mesh;
		}
	};

	::testing::AssertionResult IsValid(const HalfEdgeMesh& mesh) {
		std::string err;
		if (mesh.Validate(err)) return ::testing::AssertionSuccess();
		return ::testing::AssertionFailure() << err;
	}

	// Copies every mesh (with its name) so a later state can be compared exactly.
	struct SceneSnapshot {
		std::vector<HalfEdgeMesh> meshes;
		std::vector<std::string> names;
		std::vector<ReferenceImage> images;
		SelectionState selection;
		DisplaySettings display;
	};

	SceneSnapshot Snapshot(const Doc& doc) {
		SceneSnapshot s;
		for (uint i = 0; i < doc.scene.MeshCount(); ++i) { s.meshes.push_back(*doc.scene[i]); }
		s.names = doc.scene.Names();
		s.images = doc.scene.ReferenceImages();
		s.selection = doc.selection.getState();
		s.display = doc.display;
		return s;
	}

	::testing::AssertionResult Matches(const Doc& doc, const SceneSnapshot& s) {
		if (doc.scene.MeshCount() != s.meshes.size()) {
			return ::testing::AssertionFailure() << "mesh count " << doc.scene.MeshCount() << " != " << s.meshes.size();
		}
		for (uint i = 0; i < s.meshes.size(); ++i) {
			if (!doc.scene[i]->SameState(s.meshes[i])) { return ::testing::AssertionFailure() << "mesh " << i << " differs"; }
			if (doc.scene.Names()[i] != s.names[i]) { return ::testing::AssertionFailure() << "name " << i << " differs"; }
		}
		if (!SameReferenceImages(doc.scene.ReferenceImages(), s.images)) { return ::testing::AssertionFailure() << "reference images differ"; }
		if (doc.selection.getState() != s.selection) { return ::testing::AssertionFailure() << "selection differs"; }
		if (doc.display != s.display) { return ::testing::AssertionFailure() << "display settings differ"; }
		return ::testing::AssertionSuccess();
	}

	// A command whose size is set by the test, for the limit tests.
	struct FixedCommand : Command {
		size_t size;
		int* counter;
		explicit FixedCommand(size_t size, int* counter = nullptr) : size(size), counter(counter) {}
		void undo(DocumentRefs&) override { if (counter) --*counter; }
		void redo(DocumentRefs&) override { if (counter) ++*counter; }
		size_t byteSize() const override { return size; }
		const char* label() const override { return "Fixed"; }
	};
}

// ---------------------------------------------------------------------------------------------
// History semantics ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

TEST(UndoHistory, UndoRedoOrder) {
	Doc doc;
	int value = 0;
	for (int i = 0; i < 3; ++i) { ++value; doc.history.push(std::make_unique<FixedCommand>(10, &value)); }
	EXPECT_EQ(doc.history.undoCount(), 3u);

	EXPECT_TRUE(doc.Undo()); EXPECT_EQ(value, 2);
	EXPECT_TRUE(doc.Undo()); EXPECT_EQ(value, 1);
	EXPECT_TRUE(doc.Redo()); EXPECT_EQ(value, 2);
	EXPECT_EQ(doc.history.undoCount(), 2u);
	EXPECT_EQ(doc.history.redoCount(), 1u);
}

TEST(UndoHistory, EmptyStacksDoNothing) {
	Doc doc;
	EXPECT_FALSE(doc.Undo());
	EXPECT_FALSE(doc.Redo());
	EXPECT_FALSE(doc.history.canUndo());
	EXPECT_FALSE(doc.history.canRedo());
}

TEST(UndoHistory, NewPushClearsRedo) {
	Doc doc;
	doc.history.push(std::make_unique<FixedCommand>(10));
	doc.history.push(std::make_unique<FixedCommand>(10));
	doc.Undo();
	ASSERT_TRUE(doc.history.canRedo());
	doc.history.push(std::make_unique<FixedCommand>(10));
	EXPECT_FALSE(doc.history.canRedo());
	EXPECT_EQ(doc.history.bytes(), 20u);
}

TEST(UndoHistory, StepLimitDropsOldest) {
	Doc doc;
	doc.history.setLimits(3, size_t(-1));
	int value = 0;
	for (int i = 0; i < 5; ++i) { ++value; doc.history.push(std::make_unique<FixedCommand>(1, &value)); }
	EXPECT_EQ(doc.history.undoCount(), 3u);
	while (doc.Undo()) {}
	EXPECT_EQ(value, 2); // the two oldest steps can no longer be undone
}

TEST(UndoHistory, ByteLimitDropsOldestButKeepsNewest) {
	Doc doc;
	doc.history.setLimits(100, 100);
	doc.history.push(std::make_unique<FixedCommand>(60));
	doc.history.push(std::make_unique<FixedCommand>(60));
	EXPECT_EQ(doc.history.undoCount(), 1u);
	EXPECT_EQ(doc.history.bytes(), 60u);

	doc.history.push(std::make_unique<FixedCommand>(500)); // over budget alone
	EXPECT_EQ(doc.history.undoCount(), 1u);
	EXPECT_EQ(doc.history.bytes(), 500u);
}

TEST(UndoHistory, BytesTrackSizeChangesAcrossStacks) {
	Doc doc;
	doc.AddCube();
	const size_t meshBytes = doc.scene[0]->MemoryBytes();

	doc.Begin("Add");
	doc.NewMesh("Cube")->load(CUBE_VERTICES, CUBE_FACES);
	ASSERT_TRUE(doc.Commit());
	const size_t live = doc.history.bytes();

	// After undo the add command holds the removed mesh, so the history grows by about one mesh.
	doc.Undo();
	EXPECT_GE(doc.history.bytes(), live + meshBytes - sizeof(HalfEdgeMesh));
	doc.Redo();
	EXPECT_EQ(doc.history.bytes(), live);

	doc.Undo();
	doc.history.clear();
	EXPECT_EQ(doc.history.bytes(), 0u);
}

// ---------------------------------------------------------------------------------------------
// Transactions --------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

TEST(DocumentTransaction, NoChangeRecordsNothing) {
	Doc doc;
	doc.AddCube();
	doc.selection.selectMesh(0);
	doc.Begin();
	doc.step.touchMesh(doc.refs, 0);
	doc.step.touchVertexPositions(doc.refs, 0);
	EXPECT_FALSE(doc.Commit());
	EXPECT_FALSE(doc.history.canUndo());
}

TEST(DocumentTransaction, SelectionOnlyStepIsLabeledSelect) {
	Doc doc;
	doc.AddCube();
	doc.Begin("Move");
	doc.selection.selectMesh(0);
	ASSERT_TRUE(doc.Commit());
	EXPECT_STREQ(doc.history.undoLabel(), "Select");

	doc.Undo();
	EXPECT_EQ(doc.selection.itemCount(), 0u);
	doc.Redo();
	EXPECT_TRUE(doc.selection.hasSelectedMesh(0));
}

TEST(DocumentTransaction, TransformRoundTrip) {
	Doc doc;
	doc.AddCube();
	doc.AddCube();
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Move");
	doc.scene[1]->TranslatePos(Vec3f(1.0f, 2.0f, 3.0f));
	doc.scene[1]->Rot(Vec3f(10.0f, 0.0f, 0.0f));
	doc.scene[1]->Scale(Vec3f(2.0f));
	ASSERT_TRUE(doc.Commit());
	const SceneSnapshot after = Snapshot(doc);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DocumentTransaction, TopologyRoundTripRestoresSelectionIds) {
	Doc doc;
	doc.AddCube();
	doc.selection.setActiveElement(FACE, doc.scene);
	doc.selection.addSelection({ 0, 2 });
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Extrude");
	doc.step.touchMesh(doc.refs, 0);
	doc.scene[0]->Extrude(2);
	doc.scene[0]->subdivide();
	doc.selection.clearSelection();
	doc.selection.addSelection({ 0, doc.scene[0]->FaceCount() - 1 });
	ASSERT_TRUE(doc.Commit());
	ASSERT_TRUE(IsValid(*doc.scene[0]));
	const SceneSnapshot after = Snapshot(doc);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	EXPECT_TRUE(IsValid(*doc.scene[0]));
	EXPECT_TRUE(doc.selection.isSelected(0, 2));
	EXPECT_LT(doc.selection.getSelectedItems()[0].componentId, doc.scene[0]->FaceCount());

	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DocumentTransaction, VertexPositionsRoundTrip) {
	Doc doc;
	doc.AddCube();
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Move");
	doc.step.touchVertexPositions(doc.refs, 0);
	doc.scene[0]->TranslateFace(0, Vec3f(0.0f, 0.5f, 0.0f));
	ASSERT_TRUE(doc.Commit());
	const SceneSnapshot after = Snapshot(doc);

	// Smaller than a full mesh copy.
	EXPECT_LT(doc.history.bytes(), doc.scene[0]->MemoryBytes());

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DocumentTransaction, ReferenceImagesAndDisplayRoundTrip) {
	Doc doc;
	ReferenceImage* image = doc.scene.NewReferenceImage("a.png");
	image->opacity = 0.5f;
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Change");
	doc.scene.FindReferenceImage(0)->position = Vec3f(1.0f);
	doc.scene.NewReferenceImage("b.png");
	doc.display.edgeColor = Vec3f(1.0f, 0.0f, 0.0f);
	doc.display.gridVisible = false;
	doc.display.meshShader = 1;
	ASSERT_TRUE(doc.Commit());
	const SceneSnapshot after = Snapshot(doc);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DocumentTransaction, AddMeshRoundTrip) {
	Doc doc;
	doc.AddCube("A");
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Add");
	doc.NewMesh("B")->load(PYRAMID_VERTICES, PYRAMID_FACES);
	*doc.NewMesh("C") = *doc.scene[0]; // duplicate
	ASSERT_TRUE(doc.Commit());
	const SceneSnapshot after = Snapshot(doc);
	ASSERT_EQ(doc.scene.MeshCount(), 3u);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DocumentTransaction, RollbackRestoresEverything) {
	Doc doc;
	doc.AddCube("A");
	doc.AddCube("B");
	doc.selection.selectMesh(1);
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Extrude");
	doc.step.touchMesh(doc.refs, 0);
	doc.scene[0]->Extrude(0);
	doc.scene[1]->TranslatePos(Vec3f(1.0f));
	doc.NewMesh("C");
	doc.DeleteMesh(1);
	doc.selection.clearSelection();
	doc.display.vertexScale = 3.0f;
	doc.step.rollback(doc.refs);

	EXPECT_TRUE(Matches(doc, before));
	EXPECT_FALSE(doc.step.isActive());
	EXPECT_FALSE(doc.history.canUndo());
}

// ---------------------------------------------------------------------------------------------
// Deleting meshes -----------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

namespace {
	// Five distinguishable meshes: each has a different position.
	void FillScene(Doc& doc, uint count) {
		for (uint i = 0; i < count; ++i) {
			doc.AddCube("Mesh" + std::to_string(i))->Pos(Vec3f(float(i), 0.0f, 0.0f));
		}
	}
}

TEST(DeleteMesh, DetachThenInsertIsExactInverse) {
	for (uint target = 0; target < 5; ++target) {
		Doc doc;
		FillScene(doc, 5);
		const SceneSnapshot before = Snapshot(doc);
		auto detached = doc.scene.DetachMesh(target);
		doc.scene.InsertMeshAt(target, std::move(detached.first), std::move(detached.second));
		EXPECT_TRUE(Matches(doc, before)) << "target " << target;
	}
}

TEST(DeleteMesh, SingleDeleteRestoresOrder) {
	for (uint target = 0; target < 5; ++target) { // includes the last mesh
		Doc doc;
		FillScene(doc, 5);
		doc.selection.selectMesh(target);
		const SceneSnapshot before = Snapshot(doc);

		doc.Begin("Delete");
		doc.DeleteMesh(target);
		doc.selection.clearSelection();
		ASSERT_TRUE(doc.Commit());
		const SceneSnapshot after = Snapshot(doc);

		doc.Undo();
		EXPECT_TRUE(Matches(doc, before)) << "target " << target;
		doc.Redo();
		EXPECT_TRUE(Matches(doc, after)) << "target " << target;
	}
}

TEST(DeleteMesh, MultiDeleteHighestFirstRestoresOrder) {
	Doc doc;
	FillScene(doc, 7);
	const std::vector<uint> targets = { 6, 4, 2, 0 }; // deleteSelection sorts highest first
	const SceneSnapshot before = Snapshot(doc);

	doc.Begin("Delete");
	for (uint target : targets) { doc.DeleteMesh(target); }
	ASSERT_TRUE(doc.Commit());
	ASSERT_EQ(doc.scene.MeshCount(), 3u);
	const SceneSnapshot after = Snapshot(doc);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

TEST(DeleteMesh, TransformOfMovedMeshSurvivesDeleteInSameStep) {
	Doc doc;
	FillScene(doc, 4);
	const SceneSnapshot before = Snapshot(doc);

	// Mesh 3 moves into slot 1 when mesh 1 is deleted; its transform change must still undo.
	doc.Begin("Edit");
	doc.scene[3]->TranslatePos(Vec3f(0.0f, 5.0f, 0.0f));
	doc.DeleteMesh(1);
	doc.scene[1]->TranslatePos(Vec3f(0.0f, 0.0f, 5.0f)); // the same mesh, at its new index
	ASSERT_TRUE(doc.Commit());
	const SceneSnapshot after = Snapshot(doc);

	doc.Undo();
	EXPECT_TRUE(Matches(doc, before));
	doc.Redo();
	EXPECT_TRUE(Matches(doc, after));
}

// ---------------------------------------------------------------------------------------------
// Fuzz ----------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

TEST(UndoFuzz, RandomStepsUndoBackToStart) {
	std::mt19937 rng(1234);
	for (int run = 0; run < 20; ++run) {
		Doc doc;
		FillScene(doc, 2);
		std::vector<SceneSnapshot> states{ Snapshot(doc) };

		for (int stepIndex = 0; stepIndex < 12; ++stepIndex) {
			doc.Begin("Random");
			const uint meshCount = doc.scene.MeshCount();
			const uint op = std::uniform_int_distribution<uint>(0, 5)(rng);
			const uint mesh = meshCount ? std::uniform_int_distribution<uint>(0, meshCount - 1)(rng) : 0;

			if (op == 0 && meshCount) { // topology
				doc.step.touchMesh(doc.refs, mesh);
				const uint face = std::uniform_int_distribution<uint>(0, doc.scene[mesh]->FaceCount() - 1)(rng);
				doc.scene[mesh]->Extrude(face);
			}
			else if (op == 1 && meshCount) { // vertex move
				doc.step.touchVertexPositions(doc.refs, mesh);
				doc.scene[mesh]->TranslateVertex(0, Vec3f(0.1f, 0.2f, 0.3f));
			}
			else if (op == 2 && meshCount) { // transform
				doc.scene[mesh]->TranslatePos(Vec3f(1.0f, 0.0f, 0.0f));
			}
			else if (op == 3) { // add
				doc.NewMesh("New")->load(PYRAMID_VERTICES, PYRAMID_FACES);
			}
			else if (op == 4 && meshCount > 1) { // delete
				doc.DeleteMesh(mesh);
			}
			else { // selection
				doc.selection.clearSelection();
				if (meshCount) { doc.selection.selectMesh(mesh); }
			}

			if (doc.Commit()) { states.push_back(Snapshot(doc)); }
		}

		// Undo all the way, checking every intermediate state, then redo all the way.
		for (size_t i = states.size() - 1; i > 0; --i) {
			ASSERT_TRUE(doc.Undo());
			ASSERT_TRUE(Matches(doc, states[i - 1])) << "run " << run << " undo to state " << i - 1;
		}
		EXPECT_FALSE(doc.Undo());
		for (size_t i = 1; i < states.size(); ++i) {
			ASSERT_TRUE(doc.Redo());
			ASSERT_TRUE(Matches(doc, states[i])) << "run " << run << " redo to state " << i;
		}
		for (uint i = 0; i < doc.scene.MeshCount(); ++i) { EXPECT_TRUE(IsValid(*doc.scene[i])); }
	}
}
