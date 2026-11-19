#ifndef _ABC3D_UNDO_HISTORY_H_
#define _ABC3D_UNDO_HISTORY_H_

#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "scene.h"
#include "selectionManager.h"

// ---------------------------------------------------------------------------------------------
// Display settings ----------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>
/// Single source of truth for the undoable display settings. The renderer only has setters for
/// most of these, so the viewport keeps this copy to know what a setting was before a change.
/// Defaults match the renderer's and the panel widgets' initial values.
/// </summary>
struct DisplaySettings {
	Vec3f faceColor = Vec3f(0.0f);
	Vec3f edgeColor = Vec3f(0.0f);
	Vec3f vertexColor = Vec3f(0.0f);
	Vec3f backgroundColor = Vec3f(0.0f);
	Vec3f selectColor = Vec3f(0.0f);
	Vec3f hoverColor = Vec3f(0.0f);
	float edgeRadius = 0.01f;
	float vertexScale = 0.1f;
	bool surfacesVisible = true;
	bool edgesVisible = true;
	bool gridVisible = true;
	bool verticesAlwaysVisible = false;

	bool operator==(const DisplaySettings& o) const {
		return faceColor == o.faceColor && edgeColor == o.edgeColor && vertexColor == o.vertexColor &&
			backgroundColor == o.backgroundColor && selectColor == o.selectColor && hoverColor == o.hoverColor &&
			edgeRadius == o.edgeRadius && vertexScale == o.vertexScale &&
			surfacesVisible == o.surfacesVisible && edgesVisible == o.edgesVisible &&
			gridVisible == o.gridVisible && verticesAlwaysVisible == o.verticesAlwaysVisible;
	}
	bool operator!=(const DisplaySettings& o) const { return !(*this == o); }
};

/// <summary>
/// Everything a command may read or change. Camera and tool state are deliberately absent.
/// </summary>
struct DocumentRefs {
	Scene& scene;
	SelectionManager& selection;
	DisplaySettings& display;
};

// ---------------------------------------------------------------------------------------------
// Command -------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>
/// One recorded change. Commands are created after the edit already happened, holding whatever
/// is needed to put the document back. Most store the "other" state and swap it with the live
/// state, so undo and redo are the same operation and each step holds one copy, not two.
/// </summary>
struct Command {
	virtual ~Command() = default;
	virtual void undo(DocumentRefs& doc) = 0;
	virtual void redo(DocumentRefs& doc) = 0;
	/// <returns>Approximate heap memory the command holds right now. May change after undo/redo.</returns>
	virtual size_t byteSize() const = 0;
	virtual const char* label() const = 0;
};

/// <summary>
/// Ordered children. Redo runs them in order, undo in reverse.
/// </summary>
class CompositeCommand : public Command {
	std::string name;
	std::vector<std::unique_ptr<Command>> children;

public:
	explicit CompositeCommand(std::string label) : name(std::move(label)) {}

	void add(std::unique_ptr<Command> child) { if (child) { children.push_back(std::move(child)); } }
	bool empty() const { return children.empty(); }
	size_t childCount() const { return children.size(); }
	void setLabel(std::string label) { name = std::move(label); }

	void undo(DocumentRefs& doc) override {
		for (auto it = children.rbegin(); it != children.rend(); ++it) { (*it)->undo(doc); }
	}
	void redo(DocumentRefs& doc) override {
		for (auto& child : children) { child->redo(doc); }
	}
	size_t byteSize() const override {
		size_t total = sizeof(*this);
		for (const auto& child : children) { total += child->byteSize(); }
		return total;
	}
	const char* label() const override { return name.c_str(); }
};

// ---------------------------------------------------------------------------------------------
// History -------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>
/// Two stacks. A new step clears the redo stack, because the undone future no longer follows
/// from the current document. The oldest steps are dropped once either limit is exceeded; the
/// newest step is always kept, even when it alone is over the byte budget.
/// </summary>
class UndoHistory {
	std::deque<std::unique_ptr<Command>> undoStack;   // oldest at front, newest at back
	std::vector<std::unique_ptr<Command>> redoStack;  // most recently undone at back
	size_t totalBytes = 0;
	size_t maxSteps;
	size_t maxBytes;

	void enforceLimits() {
		while (undoStack.size() > 1 && (undoStack.size() > maxSteps || totalBytes > maxBytes)) {
			totalBytes -= undoStack.front()->byteSize();
			undoStack.pop_front();
		}
	}

	void clearRedo() {
		for (const auto& cmd : redoStack) { totalBytes -= cmd->byteSize(); }
		redoStack.clear();
	}

public:
	static constexpr size_t DEFAULT_MAX_STEPS = 100;
	static constexpr size_t DEFAULT_MAX_BYTES = size_t(512) << 20; // 512 MB

	explicit UndoHistory(size_t maxSteps = DEFAULT_MAX_STEPS, size_t maxBytes = DEFAULT_MAX_BYTES)
		: maxSteps(maxSteps == 0 ? 1 : maxSteps), maxBytes(maxBytes) {}

	void setLimits(size_t steps, size_t bytes) {
		maxSteps = steps == 0 ? 1 : steps;
		maxBytes = bytes;
		enforceLimits();
	}

	void push(std::unique_ptr<Command> cmd) {
		if (!cmd) { return; }
		clearRedo();
		totalBytes += cmd->byteSize();
		undoStack.push_back(std::move(cmd));
		enforceLimits();
	}

	bool undo(DocumentRefs& doc) {
		if (undoStack.empty()) { return false; }
		std::unique_ptr<Command> cmd = std::move(undoStack.back());
		undoStack.pop_back();
		totalBytes -= cmd->byteSize();
		cmd->undo(doc);
		totalBytes += cmd->byteSize();
		redoStack.push_back(std::move(cmd));
		return true;
	}

	bool redo(DocumentRefs& doc) {
		if (redoStack.empty()) { return false; }
		std::unique_ptr<Command> cmd = std::move(redoStack.back());
		redoStack.pop_back();
		totalBytes -= cmd->byteSize();
		cmd->redo(doc);
		totalBytes += cmd->byteSize();
		undoStack.push_back(std::move(cmd));
		enforceLimits();
		return true;
	}

	void clear() {
		undoStack.clear();
		redoStack.clear();
		totalBytes = 0;
	}

	bool canUndo() const { return !undoStack.empty(); }
	bool canRedo() const { return !redoStack.empty(); }
	size_t undoCount() const { return undoStack.size(); }
	size_t redoCount() const { return redoStack.size(); }
	size_t bytes() const { return totalBytes; }
	const char* undoLabel() const { return undoStack.empty() ? "" : undoStack.back()->label(); }
	const char* redoLabel() const { return redoStack.empty() ? "" : redoStack.back()->label(); }
};

#endif
