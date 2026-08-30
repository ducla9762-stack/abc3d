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
//#include <FL/Fl_Flex.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_ask.H>
#undef max

#include <GL/glew.h>

#include <iostream>

#include "customSlider.h"
#include "viewport.h"
#include "ui.h"

Fl_Window* tutorialPanel;
Fl_Window* rightPanel;
customSlider* slider;
Fl_Window* window;

// Enum that describes which tool is selected by the user
enum Tool { SELECT_TOOL, TRANSLATE_TOOL, SCALE_TOOL, ROTATE_TOOL, SUBDIVIDE_TOOL, SHAPE_TOOL, EXTRUDE_TOOL, BEVEL_TOOL };
constexpr int TOOL_COUNT = 8;

RotateToolPanel* rotateToolPanel;
ScaleToolPanel* scaleToolPanel;
TranslationToolPanel* translateToolPanel;
SubdivideToolPanel* subdivideToolPanel;
ShapeToolPanel* shapeToolPanel;
SelectToolPanel* selectToolPanel;
ExtrudeToolPanel* extrudeToolPanel;
BevelToolPanel* bevelToolPanel;

ToolPanel* toolPanels[TOOL_COUNT];
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

	selectToolPanel->show();
	selectToolPanel->updateValues();

	activeTool = SELECT_TOOL;
	viewport->setActiveAction(SELECT);
}

void translationCallback(Fl_Widget* w, void*) {
	if (activeTool == TRANSLATE_TOOL) {
		return;
	}

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

	toolPanels[activeTool]->hide();
	shapeToolPanel->show();

	activeTool = SHAPE_TOOL;
	viewport->setActiveAction(TRANSLATE_FREE);
}

void extrudeCallback(Fl_Widget* w, void* data) {
	if (activeTool == EXTRUDE_TOOL) {
		return;
	}

	toolPanels[activeTool]->hide();
	extrudeToolPanel->show();

	activeTool = EXTRUDE_TOOL;
	viewport->setActiveAction(TRANSLATE_FREE);
}

void subdivisionPanelCallback(Fl_Widget* w, void*) {
	if (activeTool == SUBDIVIDE_TOOL) {
		return;
	}

	toolPanels[activeTool]->hide();
	subdivideToolPanel->show();

	activeTool = SUBDIVIDE_TOOL;
	viewport->setActiveAction(TRANSLATE_FREE);
}

void bevelCallback(Fl_Widget* w, void*) {
	if (activeTool == BEVEL_TOOL) {
		return;
	}

	toolPanels[activeTool]->hide();
	bevelToolPanel->show();


	activeTool = BEVEL_TOOL;
	viewport->setActiveAction(TRANSLATE_FREE);
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
	const int LEFT_TOOLBAR_W = VIEWPORT_W * 1 / 20;
	const int LEFT_TOOLBAR_H = VIEWPORT_H * 1 / 2;
	const int BOTTOM_TOOLBAR_H = VIEWPORT_W * 1 / 20;
	const int BOTTOM_TOOLBAR_W = VIEWPORT_W * 1 / 6;

	const int TOP_TOOLBAR_H = 40;
	const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

	window = new Fl_Window(0, 0, W, H, "ABC3D");

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
	topMenu->add("&File/&Export/&Model/&.OBJ", 0, exportObjCallback); //Temporary - will later be a popup menu
	topMenu->add("&File/&Export/&Scene", 0, importStlCallback);
	topMenu->add("&File/&Export/&Lesson");

	topMenu->add("&File/&Save", "^s", saveCallback);
	topMenu->add("&File/&Settings");
	topMenu->add("&File/&Exit", 0, exitCallback);

	// Edit Menu Button
	topMenu->add("&Edit/&Delete");
	topMenu->add("&Edit/&Duplicate");
	topMenu->add("&Edit/&Undo");
	topMenu->add("&Edit/&Redo");

	topMenu->add("Help", "^h", helpCallback);

	topMenu->add("Standard Mode", 0, standardModeCallback);

	topMenu->add("Simple Mode", 0, simpleModeCallback);

	topMenu->add("Tutorial", 0, tutorialModeCallback);

	topMenu->add("Camera", 0, cameraModeCallback);

	/* Left Toolbar that's always present. Creates a window and fills it with radio buttons
	* Includes buttons for select, translate, rotate, scale, shape, extrude, subdivision, and bevel */
	Fl_Window* sideToolbar = new Fl_Window(20, 70, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H);
	sideToolbar->color(7);

	Fl_Radio_Button* selectButton = new Fl_Radio_Button(0, 0, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Select");
	selectButton->selection_color(0);
	selectButton->callback(selectCallback);

	Fl_Radio_Button* translationButton = new Fl_Radio_Button(0, LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Move");
	translationButton->selection_color(0);
	translationButton->callback(translationCallback);

	Fl_Radio_Button* rotateButton = new Fl_Radio_Button(0, 2 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Rotate");
	rotateButton->selection_color(0);
	rotateButton->callback(rotateCallback);

	Fl_Radio_Button* scaleButton = new Fl_Radio_Button(0, 3 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Scale");
	scaleButton->selection_color(0);
	scaleButton->callback(scaleCallback);

	Fl_Radio_Button* shapeButton = new Fl_Radio_Button(0, 4 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Shape");
	shapeButton->selection_color(0);
	shapeButton->callback(shapeCallback);
	// buttons for resolutions?

	//Fl_SVG_Image* extrudeImg = new Fl_SVG_Image("extrude.svg");
	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "extrude");
	extrudeButton->selection_color(0);
	extrudeButton->callback(extrudeCallback, rightPanel);

	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Subdivide");
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionPanelCallback);

	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Bevel");
	bevelButton->selection_color(0);
	bevelButton->callback(bevelCallback);

	sideToolbar->resizable(sideToolbar);
	//Fl_Tile* sideTile = new Fl_Tile(20, 70, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H);
	sideToolbar->end();


	/* Top Right panel for the tutorials */
	tutorialPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, TUTORIAL_PANEL_H);
	tutorialPanel->resizable(nullptr);
	tutorialPanel->color(FL_DARK_RED);
	tutorialPanel->begin();
	tutorialPanel->end();

	/* Right panel tools that pop up when a button is selected. Mapping tools and panels
	* Creates a new window and fills it with options relevant to the selected left toolbar button
	* Includes rotation, scale, translate, subdivide, shape, extrude, bevel, and select */
	rightPanel = new Fl_Window(VIEWPORT_W, VIEWPORT_H / 3, RIGHT_PANEL_W, RIGHT_PANEL_H);
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
	Fl_Window* bottomToolbar = new Fl_Window(150, VIEWPORT_H - 110, BOTTOM_TOOLBAR_W, BOTTOM_TOOLBAR_H);
	bottomToolbar->color(12);

	Fl_Radio_Button* vertexButton = new Fl_Radio_Button(0, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Vertex");
	vertexButton->callback(elementSelectCallback<VERTEX>);
	vertexButton->selection_color(0);

	Fl_Radio_Button* edgeButton = new Fl_Radio_Button(BOTTOM_TOOLBAR_W / 4, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Edge");
	edgeButton->callback(elementSelectCallback<EDGE>);
	edgeButton->selection_color(0);

	Fl_Radio_Button* faceButton = new Fl_Radio_Button(2 * BOTTOM_TOOLBAR_W / 4, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Face");
	faceButton->callback(elementSelectCallback<FACE>);
	faceButton->selection_color(0);

	Fl_Radio_Button* objectButton = new Fl_Radio_Button(3 * BOTTOM_TOOLBAR_W / 4, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Object");
	objectButton->callback(elementSelectCallback<OBJECT>);
	objectButton->selection_color(0);
	objectButton->value(1);

	bottomToolbar->resizable(bottomToolbar);
	bottomToolbar->end();

	// Window 
	window->end();
	window->resize(X, Y, W, H);
	window->resizable(window);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();

	return Fl::run();
}