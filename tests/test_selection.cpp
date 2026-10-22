#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "selectionManager.h"

namespace {
	std::optional<SelectionItem> Obj(uint meshId) {
		return SelectionItem{ OBJECT, meshId, MAX_UINT };
	}

	std::optional<SelectionItem> Face(uint meshId, uint faceId) {
		return SelectionItem{ FACE, meshId, faceId };
	}

	// processSelection takes the optional by non-const reference, so every call
	// needs an lvalue. This wraps that so the tests read as one line per click.
	bool Click(SelectionManager& sm, std::optional<SelectionItem> item, bool additive) {
		return sm.processSelection(item, additive);
	}

	void ExpectSelection(const SelectionManager& sm, const std::vector<SelectionItem>& want) {
		const std::vector<SelectionItem>& got = sm.getSelectedItems();
		ASSERT_EQ(got.size(), want.size());
		for (size_t i = 0; i < want.size(); ++i) {
			SCOPED_TRACE(i);
			EXPECT_TRUE(got[i] == want[i]);
		}
	}

	// Fills the selection to MAX_SELECTION_ITEMS with meshes 0..63.
	void FillToCap(SelectionManager& sm) {
		for (uint i = 0; i < MAX_SELECTION_ITEMS; ++i)
			ASSERT_TRUE(Click(sm, Obj(i), true));
	}
}

// --- Element mode ----------------------------------------------------------

TEST(SelectionManager, DefaultsToObjectWithNothingSelected) {
	SelectionManager sm;
	EXPECT_EQ(sm.getActiveElement(), OBJECT);
	EXPECT_TRUE(sm.getSelectedItems().empty());
}

TEST(SelectionManager, ConstructorSetsActiveElement) {
	SelectionManager sm(FACE);
	EXPECT_EQ(sm.getActiveElement(), FACE);
}

TEST(SelectionManager, ChangingElementClearsSelection) {
	SelectionManager sm;
	Click(sm, Obj(0), true);
	Click(sm, Obj(1), true);

	sm.setActiveElement(FACE);
	EXPECT_EQ(sm.getActiveElement(), FACE);
	EXPECT_TRUE(sm.getSelectedItems().empty());
}

// Setting the same element is still a clear. Pinned so a "skip if unchanged"
// optimisation can't sneak in and change what the mode buttons do.
TEST(SelectionManager, SettingSameElementStillClears) {
	SelectionManager sm;
	Click(sm, Obj(0), false);
	sm.setActiveElement(OBJECT);
	EXPECT_TRUE(sm.getSelectedItems().empty());
}

// --- processSelection: background and wrong type ---------------------------

TEST(SelectionManager, BackgroundClickClearsSelection) {
	for (bool additive : { false, true }) {
		SCOPED_TRACE(additive);
		SelectionManager sm;
		Click(sm, Obj(0), true);
		Click(sm, Obj(1), true);

		EXPECT_FALSE(Click(sm, std::nullopt, additive));
		EXPECT_TRUE(sm.getSelectedItems().empty());
	}
}

TEST(SelectionManager, WrongElementTypeIsIgnored) {
	SelectionManager sm;
	Click(sm, Obj(0), false);

	EXPECT_FALSE(Click(sm, Face(0, 3), false));
	EXPECT_FALSE(Click(sm, Face(0, 3), true));
	ExpectSelection(sm, { *Obj(0) });
}

// --- processSelection: single (non-additive) -------------------------------

TEST(SelectionManager, SingleClickSelectsItem) {
	SelectionManager sm;
	EXPECT_TRUE(Click(sm, Obj(4), false));
	ExpectSelection(sm, { *Obj(4) });
}

TEST(SelectionManager, SingleClickReplacesPreviousItem) {
	SelectionManager sm;
	Click(sm, Obj(4), false);
	EXPECT_TRUE(Click(sm, Obj(5), false));
	ExpectSelection(sm, { *Obj(5) });
}

TEST(SelectionManager, SingleClickOnSoleSelectedItemTogglesItOff) {
	SelectionManager sm;
	Click(sm, Obj(4), false);
	EXPECT_FALSE(Click(sm, Obj(4), false));
	EXPECT_TRUE(sm.getSelectedItems().empty());
}

// With several selected, a plain click on one of them collapses to just that
// one instead of toggling it off.
TEST(SelectionManager, SingleClickOnOneOfManyCollapsesToIt) {
	SelectionManager sm;
	Click(sm, Obj(1), true);
	Click(sm, Obj(2), true);
	Click(sm, Obj(3), true);

	EXPECT_TRUE(Click(sm, Obj(2), false));
	ExpectSelection(sm, { *Obj(2) });
}

// --- processSelection: additive --------------------------------------------

TEST(SelectionManager, AdditiveClicksAppendInOrder) {
	SelectionManager sm;
	EXPECT_TRUE(Click(sm, Obj(3), true));
	EXPECT_TRUE(Click(sm, Obj(1), true));
	EXPECT_TRUE(Click(sm, Obj(2), true));
	ExpectSelection(sm, { *Obj(3), *Obj(1), *Obj(2) });
}

TEST(SelectionManager, AdditiveClickOnSelectedItemRemovesOnlyIt) {
	SelectionManager sm;
	Click(sm, Obj(1), true);
	Click(sm, Obj(2), true);
	Click(sm, Obj(3), true);

	EXPECT_FALSE(Click(sm, Obj(2), true));
	ExpectSelection(sm, { *Obj(1), *Obj(3) });
}

// Items are equal only if type, mesh and component all match, so two faces
// on the same mesh are separate selections.
TEST(SelectionManager, ComponentsOnSameMeshAreDistinct) {
	SelectionManager sm(FACE);
	EXPECT_TRUE(Click(sm, Face(0, 1), true));
	EXPECT_TRUE(Click(sm, Face(0, 2), true));
	EXPECT_TRUE(Click(sm, Face(1, 1), true));
	ExpectSelection(sm, { *Face(0, 1), *Face(0, 2), *Face(1, 1) });

	EXPECT_FALSE(Click(sm, Face(0, 2), true));
	ExpectSelection(sm, { *Face(0, 1), *Face(1, 1) });
}

// --- Selection cap ---------------------------------------------------------
// MAX_SELECTION_ITEMS is the size of the GPU upload, so going past it would
// overrun the buffer on the render side.

TEST(SelectionManager, AdditiveStopsAtCap) {
	SelectionManager sm;
	FillToCap(sm);

	EXPECT_FALSE(Click(sm, Obj(MAX_SELECTION_ITEMS), true));
	EXPECT_EQ(sm.getSelectedItems().size(), MAX_SELECTION_ITEMS);
}

TEST(SelectionManager, AtCapCanStillDeselect) {
	SelectionManager sm;
	FillToCap(sm);

	EXPECT_FALSE(Click(sm, Obj(10), true));
	EXPECT_EQ(sm.getSelectedItems().size(), MAX_SELECTION_ITEMS - 1);
	EXPECT_FALSE(sm.hasSelectedMesh(10));
}

TEST(SelectionManager, AtCapSingleClickStillReplaces) {
	SelectionManager sm;
	FillToCap(sm);

	EXPECT_TRUE(Click(sm, Obj(MAX_SELECTION_ITEMS), false));
	ExpectSelection(sm, { *Obj(MAX_SELECTION_ITEMS) });
}

// --- selectMesh (scene restore path) ---------------------------------------

TEST(SelectionManager, SelectMeshAddsObjectItem) {
	SelectionManager sm;
	EXPECT_TRUE(sm.selectMesh(0));
	EXPECT_TRUE(sm.selectMesh(2));
	ExpectSelection(sm, { *Obj(0), *Obj(2) });
}

// A mesh restored by selectMesh has to be the same item a click produces,
// or clicking it after loading a scene would add a duplicate instead of
// deselecting it.
TEST(SelectionManager, SelectMeshMatchesClickedItem) {
	SelectionManager sm;
	sm.selectMesh(7);
	EXPECT_FALSE(Click(sm, Obj(7), true));
	EXPECT_TRUE(sm.getSelectedItems().empty());
}

TEST(SelectionManager, SelectMeshRefusedOutsideObjectMode) {
	for (Element e : { FACE, EDGE, VERTEX }) {
		SCOPED_TRACE(e);
		SelectionManager sm(e);
		EXPECT_FALSE(sm.selectMesh(0));
		EXPECT_TRUE(sm.getSelectedItems().empty());
	}
}

TEST(SelectionManager, SelectMeshStopsAtCap) {
	SelectionManager sm;
	for (uint i = 0; i < MAX_SELECTION_ITEMS; ++i)
		ASSERT_TRUE(sm.selectMesh(i));

	EXPECT_FALSE(sm.selectMesh(MAX_SELECTION_ITEMS));
	EXPECT_EQ(sm.getSelectedItems().size(), MAX_SELECTION_ITEMS);
}

// --- hasSelectedMesh -------------------------------------------------------

TEST(SelectionManager, HasSelectedMeshForObjects) {
	SelectionManager sm;
	Click(sm, Obj(1), true);
	Click(sm, Obj(3), true);

	EXPECT_FALSE(sm.hasSelectedMesh(0));
	EXPECT_TRUE(sm.hasSelectedMesh(1));
	EXPECT_FALSE(sm.hasSelectedMesh(2));
	EXPECT_TRUE(sm.hasSelectedMesh(3));
}

// A selected face counts as its mesh being selected.
TEST(SelectionManager, HasSelectedMeshForComponents) {
	SelectionManager sm(FACE);
	Click(sm, Face(2, 5), false);

	EXPECT_TRUE(sm.hasSelectedMesh(2));
	EXPECT_FALSE(sm.hasSelectedMesh(5));
}
