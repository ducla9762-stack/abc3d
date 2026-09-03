#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Simple_Counter.H> // Use later?
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Radio_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_ask.H>
#include <Fl/Fl_PNG_Image.H>
#include <FL/Fl_Tooltip.H>
#include <print>
#undef max

#include <GL/glew.h>

#include <iostream>

#include "customSlider.h"
#include "viewport.h"
#include "ui.h"
#include "customWindow.h"

customWindow* tutorialPanel;
customWindow* rightPanel;
customSlider* slider;
customWindow* window;

// Enum that describes which tool is selected by the user
enum Tool { SELECT_TOOL, TRANSLATE_TOOL, SCALE_TOOL, ROTATE_TOOL, SUBDIVIDE_TOOL, SHAPE_TOOL, EXTRUDE_TOOL, BEVEL_TOOL };
constexpr int TOOL_COUNT = 8;

enum ElementSelect { VERTEX_SELECT, EDGE_SELECT, FACE_SELECT, OBJECT_SELECT};
constexpr int SELECT_MODES = 4;

RotateToolPanel* rotateToolPanel;
ScaleToolPanel* scaleToolPanel;
TranslationToolPanel* translateToolPanel;
SubdivideToolPanel* subdivideToolPanel;
ShapeToolPanel* shapeToolPanel;
SelectToolPanel* selectToolPanel;
ExtrudeToolPanel* extrudeToolPanel;
BevelToolPanel* bevelToolPanel;

ToolPanel* toolPanels[TOOL_COUNT];
Fl_PNG_Image* toolIcons[TOOL_COUNT];
Fl_PNG_Image* selectionIcons[SELECT_MODES];
Tool activeTool = SELECT_TOOL;

void primarySelectionCallback() {
	toolPanels[activeTool]->updateValues();
}

/* Top Menu Callbacks
* Includes import model, import stl, export obj, save, edit, help, standard mode, simple mode, tutorial mode, and camera mode */
void importModelCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() != 0) { return; } // Cancelled, or the chooser errored

	const char* filepath = fileChooser.filename();
	ImportReport report = viewport->loadFile(filepath);

	switch (report.status) {
	case ImportReport::FileError:
		fl_alert("Could not read \"%s\".\n\n"
			"The file could not be opened, or it is not a supported .obj or .stl file.",
			filepath);
		break;

	case ImportReport::InvalidTopology:
		fl_alert("Could not import \"%s\".\n\n"
			"The model is not a valid mesh: a face refers to a vertex that does not exist, "
			"repeats the same vertex twice, or an edge is shared by more than two faces.\n\n"
			"Nothing was added to the scene.",
			filepath);
		break;

	case ImportReport::Ok:
		// The mesh loaded, but the parser had to discard geometry to get there.
		if (report.skippedFaces > 0) {
			fl_alert("Imported \"%s\", but %u face(s) were skipped.\n\n"
				"They use negative (relative) vertex indices, which are not supported. "
				"The model may look incomplete.",
				filepath, report.skippedFaces);
		}
		break;
	}
}

void importStlCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->exportStl(fileChooser.filename());
	}
}

void exportObjCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->exportObj(fileChooser.filename());
	}
}

void saveCallback(Fl_Widget* w, void*) {

}

void editCallback(Fl_Widget* w, void*) {

}

void settingsCallback(Fl_Widget* w, void*) {

}

void exitCallback(Fl_Widget* w, void*) {
	window->hide();
}

void helpCallback(Fl_Widget* w, void* v) {

}

void deleteCallback(Fl_Widget* w, void* v) {
	viewport->deleteElement();
}

void duplicateCallback(Fl_Widget* w, void* v) {
	viewport->duplicateElement();
}

void undoCallback(Fl_Widget* w, void* v) {

}

void redoCallback(Fl_Widget* w, void* v) {

}

void standardModeCallback(Fl_Widget* w, void*) {

}

void simpleModeCallback(Fl_Widget* w, void*) {

}

void tutorialModeCallback(Fl_Widget* w, void*) {

}

void cameraModeCallback(Fl_Widget* w, void*) {

}


/// <summary> Template for an action changed callback from the RIGHT panel, which sends the action to the VIEWPORT </summary>
/// <param name="w"> Widget of the changed action </param>
template<Action action>
void actionChangedCallback(Fl_Widget* w) {
	viewport->setActiveAction(action);
}

/* Left Toolbar Callbacks that set the active tool enum, and the active action using the template and sending it to the viewport
* Brings up the menu for the new tool, hiding the previous
* Includes select, translation, rotate, scale, shape, extrude, subdivide, and bevel */
void selectCallback(Fl_Widget* w, void*) {
	if (activeTool == SELECT_TOOL) {
		return;
	}

	toolPanels[activeTool]->hide();

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[SELECT_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	selectToolPanel->show();
	selectToolPanel->updateValues();

	activeTool = SELECT_TOOL;
	viewport->setActiveAction(SELECT);
}

void translationCallback(Fl_Widget* w, void*) {
	if (activeTool == TRANSLATE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[TRANSLATE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();

	translateToolPanel->show();
	translateToolPanel->updateValues();

	activeTool = TRANSLATE_TOOL;
	viewport->setActiveAction(TRANSLATE_FREE);
}

void rotateCallback(Fl_Widget* w, void*) {
	if (activeTool == ROTATE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[ROTATE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();

	activeTool = ROTATE_TOOL;
	viewport->setActiveAction(ROTATE);

	rotateToolPanel->show();
	rotateToolPanel->updateValues();
}

void scaleCallback(Fl_Widget* w, void*) {
	if (activeTool == SCALE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[SCALE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();

	activeTool = SCALE_TOOL;
	viewport->setActiveAction(SCALE);

	scaleToolPanel->show();
	scaleToolPanel->updateValues();
	scaleToolPanel->redraw();
}

void shapeCallback(Fl_Widget* w, void*) {
	if (activeTool == SHAPE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[SHAPE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();
	shapeToolPanel->show();

	activeTool = SHAPE_TOOL;
	viewport->setActiveAction(SHAPE);
}

void extrudeCallback(Fl_Widget* w, void* data) {
	if (activeTool == EXTRUDE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[EXTRUDE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();
	extrudeToolPanel->show();

	activeTool = EXTRUDE_TOOL;
	viewport->setActiveAction(EXTRUDE);
}

void subdivisionPanelCallback(Fl_Widget* w, void*) {
	if (activeTool == SUBDIVIDE_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[SUBDIVIDE_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();
	subdivideToolPanel->show();

	activeTool = SUBDIVIDE_TOOL;
	viewport->setActiveAction(SUBDIVIDE);
}

void bevelCallback(Fl_Widget* w, void*) {
	if (activeTool == BEVEL_TOOL) {
		return;
	}

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[BEVEL_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

	toolPanels[activeTool]->hide();
	bevelToolPanel->show();

	activeTool = BEVEL_TOOL;
	viewport->setActiveAction(BEVEL);
}

/// <summary> Template for different element selects in the bottom toolbar, telling the viewport </summary>
/// <param name="w"> Widget of selected element </param>
template<Element element>
void elementSelectCallback(Fl_Widget* w) {
	viewport->setActiveElement(element);
}



/// <summary> Creates the actual buttons, viewport, panels, windows, toolbars, and menus </summary>
/// Does this through callbacks, 
/// <returns></returns>
int main(int argc, char* argv[]) {
	int X, Y, W, H;
	Fl::screen_work_area(X, Y, W, H);

	// Defines the size of the viewport, right panel, and toolbars
	const int VIEWPORT_W = W * 5 / 6;
	const int VIEWPORT_H = H;
	const int TUTORIAL_PANEL_H = VIEWPORT_H / 3;
	const int RIGHT_PANEL_W = W * 1 / 6;
	const int RIGHT_PANEL_H = VIEWPORT_H * 2 / 3;
	const int RIGHT_PANEL_INPUTS_W = RIGHT_PANEL_W * 0.9f;
	const int RIGHT_PANEL_LABELS_W = RIGHT_PANEL_W - RIGHT_PANEL_INPUTS_W;
	const int LEFT_TOOLBAR_W = 64;
	const int LEFT_TOOLBAR_H = 512;
	const int BOTTOM_TOOLBAR_W = 256;
	const int BOTTOM_TOOLBAR_H = 64;

	const int TOP_TOOLBAR_H = 40;
	const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

	window = new customWindow(0, 0, W, H, "ABC3D");


	viewport = new Viewport(0, 38, VIEWPORT_W, VIEWPORT_H - 38);
	viewport->setPrimarySelectionCallback(primarySelectionCallback);
	viewport->resizable();
	viewport->end();

	/* Top toolbar that has all the different modes, options, and menus.
	* Includes file, edit, help, standard mode, simple mode, tutorial view, and camera view */
	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(0, -2, VIEWPORT_W, TOP_TOOLBAR_H);
	topMenu->selection_color(10);
	topMenu->color(8);

	// File -> Import Menu Button
	topMenu->add("&File/&Import/&Model", "^o", importModelCallback);
	topMenu->add("&File/&Import/&Scene");
	topMenu->add("&File/&Import/&Lesson");

	// File -> Export Menu Button
	topMenu->add("&File/&Export/&Model/&.STL", 0, importStlCallback);
	topMenu->add("&File/&Export/&Model/&.OBJ", 0, exportObjCallback);
	topMenu->add("&File/&Export/&Scene", 0, importStlCallback);
	topMenu->add("&File/&Export/&Lesson");

	topMenu->add("&File/&Save", FL_CTRL + 's', saveCallback);
	topMenu->add("&File/&Settings");
	topMenu->add("&File/&Exit", 27, exitCallback);

	// Edit Menu Button
	topMenu->add("&Edit/&Delete", FL_Delete, deleteCallback);
	topMenu->add("&Edit/&Duplicate", FL_CTRL + 'c', duplicateCallback);
	topMenu->add("&Edit/&Undo", FL_CTRL + 'z', undoCallback);
	topMenu->add("&Edit/&Redo", FL_CTRL + 'r', redoCallback);

	topMenu->add("Help", FL_CTRL + "h", helpCallback);

	topMenu->add("Standard Mode", 0, standardModeCallback);

	topMenu->add("Simple Mode", 0, simpleModeCallback);

	topMenu->add("Tutorial", 0, tutorialModeCallback);

	topMenu->add("Camera", 0, cameraModeCallback);

	/* Left Toolbar that's always present. Creates a window and fills it with radio buttons
	* Includes buttons for select, translate, rotate, scale, shape, extrude, subdivision, and bevel */
	viewport->begin();
	customWindow* sideToolbar = new customWindow(20, 70, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H, 0);

	Fl_Radio_Button* selectButton = new Fl_Radio_Button(0, 0, 64, 64, "Select");
	selectButton->value(1);
	selectButton->selection_color(0);
	selectButton->callback(selectCallback);
	toolIcons[SELECT_TOOL] = new Fl_PNG_Image("../assets/images/select_tool.png");
	toolIcons[SELECT_TOOL]->color_average(FL_WHITE, 0.0f);
	selectButton->image(toolIcons[SELECT_TOOL]);
	selectButton->align(FL_ALIGN_CENTER);
	selectButton->tooltip("Select Tool");
	selectButton->shortcut();

	Fl_Radio_Button* translationButton = new Fl_Radio_Button(0, 64, 64, 64, "Translation");
	translationButton->selection_color(0);
	translationButton->callback(translationCallback);
	toolIcons[TRANSLATE_TOOL] = new Fl_PNG_Image("../assets/images/move_tool.png");
	translationButton->image(toolIcons[TRANSLATE_TOOL]);
	translationButton->align(FL_ALIGN_CENTER);
	translationButton->tooltip("Move Tool");
	translationButton->shortcut();

	Fl_Radio_Button* rotateButton = new Fl_Radio_Button(0, 2 * 64, 64, 64, "Rotate");
	rotateButton->selection_color(0);
	rotateButton->callback(rotateCallback);
	toolIcons[ROTATE_TOOL] = new Fl_PNG_Image("../assets/images/rotate_tool.png");
	rotateButton->image(toolIcons[ROTATE_TOOL]);
	rotateButton->tooltip("Rotate Tool");
	rotateButton->image(toolIcons[ROTATE_TOOL]);
	rotateButton->shortcut('r');

	Fl_Radio_Button* scaleButton = new Fl_Radio_Button(0, 3 * 64, 64, 64, "Scale");
	scaleButton->selection_color(0);
	scaleButton->callback(scaleCallback);
	toolIcons[SCALE_TOOL] = new Fl_PNG_Image("../assets/images/scale_tool.png");
	scaleButton->image(toolIcons[SCALE_TOOL]);
	scaleButton->tooltip("Scale Tool");
	scaleButton->shortcut();

	Fl_Radio_Button* shapeButton = new Fl_Radio_Button(0, 4 * 64, 64, 64, "Shape");
	shapeButton->selection_color(0);
	shapeButton->callback(shapeCallback);
	toolIcons[SHAPE_TOOL] = new Fl_PNG_Image("../assets/images/shape_tool.png");
	shapeButton->image(toolIcons[SHAPE_TOOL]);
	shapeButton->tooltip("Shape Tool");
	shapeButton->shortcut();
	// buttons for resolutions?

	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * 64, 64, 64, "Extrude");
	extrudeButton->selection_color(0);
	extrudeButton->callback(extrudeCallback, rightPanel);
	toolIcons[EXTRUDE_TOOL] = new Fl_PNG_Image("../assets/images/extrude_tool.png");
	extrudeButton->image(toolIcons[EXTRUDE_TOOL]);
	extrudeButton->tooltip("Extrude Tool");
	extrudeButton->shortcut('e');

	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * 64, 64, 64, "Subdivide");
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionPanelCallback);
	toolIcons[SUBDIVIDE_TOOL] = new Fl_PNG_Image("../assets/images/subdiv_tool.png");
	subdivisionButton->image(toolIcons[SUBDIVIDE_TOOL]);
	subdivisionButton->tooltip("Subdivision Tool");
	subdivisionButton->shortcut('s');

	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * 64, 64, 64, "Bevel");
	bevelButton->selection_color(0);
	bevelButton->callback(bevelCallback);
	toolIcons[BEVEL_TOOL] = new Fl_PNG_Image("../assets/images/bevel_tool.png");
	bevelButton->image(toolIcons[BEVEL_TOOL]);
	bevelButton->tooltip("Bevel Tool");
	bevelButton->shortcut('b');

	//sideToolbar->resizable(sideToolbar);
	sideToolbar->end();
	viewport->end();


	/* Top Right panel for the tutorials */
	tutorialPanel = new customWindow(VIEWPORT_W, 0, RIGHT_PANEL_W, TUTORIAL_PANEL_H, 0);
	tutorialPanel->resizable(nullptr);
	tutorialPanel->color(FL_DARK_RED);
	tutorialPanel->begin();
	tutorialPanel->end();

	/* Right panel tools that pop up when a button is selected. Mapping tools and panels
	* Creates a new window and fills it with options relevant to the selected left toolbar button
	* Includes rotation, scale, translate, subdivide, shape, extrude, bevel, and select */
	rightPanel = new customWindow(VIEWPORT_W, VIEWPORT_H / 3, RIGHT_PANEL_W, RIGHT_PANEL_H, 0);
	rightPanel->resizable();
	rightPanel->begin();

	rotateToolPanel = new RotateToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[ROTATE_TOOL] = rotateToolPanel;

	scaleToolPanel = new ScaleToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[SCALE_TOOL] = scaleToolPanel;

	// actionChangedCallback is only used here, determining whether to use translate out or translate free
	translateToolPanel = new TranslationToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[TRANSLATE_TOOL] = translateToolPanel;
	translateToolPanel->buttonCallback(0, actionChangedCallback<TRANSLATE_OUT>);
	translateToolPanel->buttonCallback(1, actionChangedCallback<TRANSLATE_FREE>);

	subdivideToolPanel = new SubdivideToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[SUBDIVIDE_TOOL] = subdivideToolPanel;

	shapeToolPanel = new ShapeToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[SHAPE_TOOL] = shapeToolPanel;

	extrudeToolPanel = new ExtrudeToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[EXTRUDE_TOOL] = extrudeToolPanel;

	bevelToolPanel = new BevelToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	toolPanels[BEVEL_TOOL] = bevelToolPanel;

	selectToolPanel = new SelectToolPanel(0, 0, RIGHT_PANEL_W, RIGHT_PANEL_H);
	selectToolPanel->setPositionPanel(translateToolPanel);
	selectToolPanel->setRotatePanel(rotateToolPanel);
	selectToolPanel->setScalePanel(scaleToolPanel);
	toolPanels[SELECT_TOOL] = selectToolPanel;

	for (int i = 0; i < 8; ++i) { toolPanels[i]->hide(); }

	selectToolPanel->show();
	selectToolPanel->updateValues();

	rightPanel->end();

	/* Bottom Toolbar with options for vertex, edge, face, and object selections
	* Uses the elementSelect callback template
	* Creates a new window and fills it with radio buttons */
	customWindow* bottomToolbar = new customWindow(150, VIEWPORT_H - 110, BOTTOM_TOOLBAR_W, BOTTOM_TOOLBAR_H, 0);
	bottomToolbar->color(12);

	Fl_Radio_Button* vertexButton = new Fl_Radio_Button(0, 0, 64, 64, "Vertex");
	vertexButton->callback(elementSelectCallback<VERTEX>);
	vertexButton->selection_color(0);
	selectionIcons[VERTEX_SELECT] = new Fl_PNG_Image("../assets/images/vertex_select.png");
	vertexButton->tooltip("Vertex Selection");
	vertexButton->shortcut('v');

	Fl_Radio_Button* edgeButton = new Fl_Radio_Button(64, 0, 64, 64, "Edge");
	edgeButton->callback(elementSelectCallback<EDGE>);
	edgeButton->selection_color(0);
	selectionIcons[EDGE_SELECT] = new Fl_PNG_Image("../assets/images/edge_select.png");
	edgeButton->tooltip("Edge Selection");
	edgeButton->shortcut('e');

	Fl_Radio_Button* faceButton = new Fl_Radio_Button(2 * 64, 0, 64, 64, "Face");
	faceButton->callback(elementSelectCallback<FACE>);
	faceButton->selection_color(0);
	selectionIcons[FACE_SELECT] = new Fl_PNG_Image("../assets/images/face_select.png");
	faceButton->tooltip("Face Selection");
	faceButton->shortcut('f');

	Fl_Radio_Button* objectButton = new Fl_Radio_Button(3 * 64, 0, 64, 64, "Object");
	objectButton->callback(elementSelectCallback<OBJECT>);
	objectButton->selection_color(0);
	objectButton->value(1);
	selectionIcons[FACE_SELECT] = new Fl_PNG_Image("../assets/images/face_select.png");
	objectButton->tooltip("Object Selection");
	objectButton->shortcut('o');

	bottomToolbar->resizable(bottomToolbar);
	bottomToolbar->end();

	// Window 
	window->end();
	window->resize(X, Y, W, H);
	window->resizable(window);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();

	sideToolbar->show();

	return Fl::run();
}