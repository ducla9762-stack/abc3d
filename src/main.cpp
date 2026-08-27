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
//#include <FL/Fl_Flex.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_ask.H>
#undef max

#include <GL/glew.h>

#include <iostream>

#include "customSlider.h"
#include "viewport.h"
#include "ui.h"

Fl_Window* topRightPanel; 
Fl_Window* rightPanel;
customSlider* slider;

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

const int VIEWPORT_W = 900;
const int VIEWPORT_H = 720;
const int RIGHT_PANEL_W = 1080 - VIEWPORT_W;
const int RIGHT_PANEL_INPUTS_W = RIGHT_PANEL_W * 0.9f;
const int RIGHT_PANEL_LABELS_W = RIGHT_PANEL_W - RIGHT_PANEL_INPUTS_W;
const int LEFT_TOOLBAR_W = 50;
const int LEFT_TOOLBAR_H = 480;
const int BOTTOM_TOOLBAR_H = 50;
const int BOTTOM_TOOLBAR_W = 200;

const int TOP_TOOLBAR_H = 40;
const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

void primarySelectionCallback() {
	toolPanels[activeTool]->updateValues();
}

/* Top Menu Callbacks */
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

void helpCallback(Fl_Widget* w, void* v) {

}

void standardModeCallback(Fl_Widget* w, void*) {

}

void simpleModeCallback(Fl_Widget* w, void*) {

}

void renderModeCallback(Fl_Widget* w, void*) {

}

void tutorialModeCallback(Fl_Widget* w, void*) {

}

void cameraModeCallback(Fl_Widget* w, void*) {
	
}

void lessonEditorCallback(Fl_Widget* w, void* v) {

}


/* Left Toolbar Callbacks */
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

template<Action action>
void actionChangedCallback(Fl_Widget* w) {
	viewport->setActiveAction(action);
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

template<Element element>
void elementSelectCallback(Fl_Widget* w) {
	viewport->setActiveElement(element);
}

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 40, VIEWPORT_W, VIEWPORT_H);
	viewport->setPrimarySelectionCallback(primarySelectionCallback);
	viewport->end();

	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W, TOP_TOOLBAR_H);
	topToolbar->color(8);

	// Menu within the top toolbar
	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(-2, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H);
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
	topMenu->add("&File/&Exit");

	// Edit Menu Button
	topMenu->add("&Edit/&Delete");
	topMenu->add("&Edit/&Duplicate");
	topMenu->add("&Edit/&Undo");
	topMenu->add("&Edit/&Redo");


	Fl_Toggle_Button* helpButton = new Fl_Toggle_Button(VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Help");
	helpButton->color(8);
	helpButton->selection_color(10);
	helpButton->box(FL_FLAT_BOX);
	helpButton->callback(helpCallback);
	helpButton->align(FL_ALIGN_CENTER);
	helpButton->type(FL_RADIO_BUTTON);

	// Standard Mode Menu Button
	Fl_Toggle_Button* standardButton = new Fl_Toggle_Button(2 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Standard Mode");
	standardButton->color(8);
	standardButton->selection_color(10);
	standardButton->box(FL_FLAT_BOX);
	standardButton->callback(standardModeCallback);
	standardButton->align(FL_ALIGN_INSIDE);
	standardButton->type(FL_RADIO_BUTTON);

	// Simple Mode Menu Button
	Fl_Toggle_Button* simpleButton = new Fl_Toggle_Button(3 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Simple Mode");
	simpleButton->color(8);
	simpleButton->selection_color(10);
	simpleButton->box(FL_FLAT_BOX);
	simpleButton->callback(simpleModeCallback);
	simpleButton->align(FL_ALIGN_INSIDE);
	simpleButton->type(FL_RADIO_BUTTON);

	// Render Mode Menu Button
	Fl_Toggle_Button* renderButton = new Fl_Toggle_Button(4 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Render");
	renderButton->color(8);
	renderButton->selection_color(10);
	renderButton->box(FL_FLAT_BOX);
	renderButton->callback(renderModeCallback);
	renderButton->align(FL_ALIGN_INSIDE);
	renderButton->type(FL_RADIO_BUTTON);

	// Tutorial View Menu Button
	Fl_Toggle_Button* tutorialButton = new Fl_Toggle_Button(5 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Tutorials");
	tutorialButton->color(8);
	tutorialButton->selection_color(10);
	tutorialButton->box(FL_FLAT_BOX);
	tutorialButton->callback(tutorialModeCallback);
	tutorialButton->align(FL_ALIGN_INSIDE);
	tutorialButton->type(FL_RADIO_BUTTON);

	// Camera Modes Menu Button
	Fl_Toggle_Button* cameraButton = new Fl_Toggle_Button(6 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Camera");
	cameraButton->color(8);
	cameraButton->selection_color(10);
	cameraButton->box(FL_FLAT_BOX);
	cameraButton->callback(cameraModeCallback);
	cameraButton->align(FL_ALIGN_INSIDE);
	cameraButton->type(FL_RADIO_BUTTON);

	// Lesson Editor Menu Button
	Fl_Toggle_Button* lessonEditorButton = new Fl_Toggle_Button(7 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Lesson Editor");
	lessonEditorButton->color(8);
	lessonEditorButton->selection_color(10);
	lessonEditorButton->box(FL_FLAT_BOX);
	lessonEditorButton->callback(lessonEditorCallback);
	lessonEditorButton->align(FL_ALIGN_INSIDE);
	lessonEditorButton->type(FL_RADIO_BUTTON);

	topToolbar->end();

	// Left Toolbar
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

	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Extrude");
	extrudeButton->selection_color(0);
	extrudeButton->callback(extrudeCallback, rightPanel);

	// Subdivision button
	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Subdivide");
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionPanelCallback);
		
	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Bevel");
	bevelButton->selection_color(0);
	bevelButton->callback(bevelCallback);

	sideToolbar->resizable(sideToolbar);
	sideToolbar->end();


	/* Top Right panel */
	topRightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, VIEWPORT_H / 3);
	topRightPanel->resizable(nullptr);
	topRightPanel->color(FL_DARK_RED);
	topRightPanel->begin();
	topRightPanel->end();

	/* Right panel */
	rightPanel = new Fl_Window(VIEWPORT_W, VIEWPORT_H / 3 , RIGHT_PANEL_W, 650);
	rightPanel->resizable(nullptr);
	rightPanel->begin();

	rotateToolPanel = new RotateToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[ROTATE_TOOL] = rotateToolPanel;

	scaleToolPanel = new ScaleToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[SCALE_TOOL] = scaleToolPanel;

	translateToolPanel = new TranslationToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[TRANSLATE_TOOL] = translateToolPanel;
	translateToolPanel->buttonCallback(0, actionChangedCallback<TRANSLATE_OUT>);
	translateToolPanel->buttonCallback(1, actionChangedCallback<TRANSLATE_FREE>);

	subdivideToolPanel = new SubdivideToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[SUBDIVIDE_TOOL] = subdivideToolPanel;

	shapeToolPanel = new ShapeToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[SHAPE_TOOL] = shapeToolPanel;

	extrudeToolPanel = new ExtrudeToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[EXTRUDE_TOOL] = extrudeToolPanel;

	bevelToolPanel = new BevelToolPanel(0, 0, RIGHT_PANEL_W, 650);
	toolPanels[BEVEL_TOOL] = bevelToolPanel;

	selectToolPanel = new SelectToolPanel(0, 0, RIGHT_PANEL_W, 650);
	selectToolPanel->setPositionPanel(translateToolPanel);
	selectToolPanel->setRotatePanel(rotateToolPanel);
	selectToolPanel->setScalePanel(scaleToolPanel);
	toolPanels[SELECT_TOOL] = selectToolPanel;

	for (int i = 0; i < 8; ++i) { toolPanels[i]->hide(); }

	selectToolPanel->show();
	selectToolPanel->updateValues();

	rightPanel->end();

	/* Bottom Toolbar */
	Fl_Window* bottomToolbar = new Fl_Window(150, VIEWPORT_H - 70, BOTTOM_TOOLBAR_W, BOTTOM_TOOLBAR_H);
	bottomToolbar->color(12);

	Fl_Radio_Button* vertexButton = new Fl_Radio_Button(0, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Vertex");
	vertexButton->callback(elementSelectCallback<VERTEX>);
	vertexButton->selection_color(0);

	Fl_Radio_Button* edgeButton = new Fl_Radio_Button(50, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Edge");
	edgeButton->callback(elementSelectCallback<EDGE>);
	edgeButton->selection_color(0);

	Fl_Radio_Button* faceButton = new Fl_Radio_Button(100, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Face");
	faceButton->callback(elementSelectCallback<FACE>);
	faceButton->selection_color(0);

	Fl_Radio_Button* objectButton = new Fl_Radio_Button(150, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Object");
	objectButton->callback(elementSelectCallback<OBJECT>);
	objectButton->selection_color(0);
	objectButton->value(1);

	bottomToolbar->resizable(bottomToolbar);
	bottomToolbar->end();

	// Window 
	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	window->fullscreen();

	return Fl::run();
}