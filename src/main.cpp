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
#include <Fl/Fl_Image.H>
#include <Fl/Fl_SVG_Image.H>
#include <Fl/Fl_PNG_Image.H>
#include <Fl/Fl_Tooltip.H>
#undef max

#include <GL/glew.h>

#include <iostream>
#include <cmath>

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
Fl_SVG_Image* toolIcons[TOOL_COUNT];
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
	if (fileChooser.show() == 0) {
		viewport->loadFile(fileChooser.filename());
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

	toolIcons[activeTool]->color_average(FL_BLACK, 0.0f);
	toolIcons[SELECT_TOOL]->color_average(FL_WHITE, 0.0f);
	w->redraw();

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

//void toolButtonCallback(Fl_Widget* w, void* data) {
//	IconButton* button = static_cast<IconButton*>(w);
//}

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
	viewport->setActiveAction(TRANSLATE_FREE);
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
	viewport->setActiveAction(TRANSLATE_FREE);
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
	viewport->setActiveAction(TRANSLATE_FREE);
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
	viewport->resizable(nullptr);
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
	viewport->begin();

	Fl_Window* sideToolbar = new Fl_Window(20, 70, 64, 64 * 8);
	sideToolbar->resizable(nullptr);
	sideToolbar->size_range(64, 64 * 8, 64, 64 * 8);

	//sideToolbar->color(7);

	Fl_Tooltip::delay(0.2f);

	float screenScale = Fl::screen_scale(window->screen_num());

	Fl_Radio_Button* selectButton = new Fl_Radio_Button(0, 0, 64, 64);
	selectButton->value(1);
	selectButton->selection_color(0);
	selectButton->callback(selectCallback);
	toolIcons[SELECT_TOOL] = new Fl_SVG_Image("../assets/images/select_tool.svg");
	//toolIcons[SELECT_TOOL]->scale(std::round(64 / screenScale), std::round(64 / screenScale));
	//toolIcons[SELECT_TOOL]->scale(std::ceil(64 / screenScale), std::ceil(64 / screenScale));
	//toolIcons[SELECT_TOOL]->scale(64, 64);
	//toolIcons[SELECT_TOOL]->RGB_scaling(FL_RGB_SCALING_BILINEAR);
	//selectImage->scale(64, 64);
	toolIcons[SELECT_TOOL]->color_average(FL_WHITE, 0.0f);
	//selectImage->scale(64, 64);
	selectButton->image(toolIcons[SELECT_TOOL]);
	//selectButton->labeltype(_FL_IMAGE_LABEL);
	selectButton->align(FL_ALIGN_CENTER);
	selectButton->tooltip("Select Tool");

	Fl_Radio_Button* translationButton = new Fl_Radio_Button(0, 64, 64, 64);
	translationButton->selection_color(0);
	translationButton->callback(translationCallback);
	toolIcons[TRANSLATE_TOOL] = new Fl_SVG_Image("../assets/images/move_tool.svg");
	translationButton->image(toolIcons[TRANSLATE_TOOL]);
	translationButton->tooltip("Move Tool");
	
	Fl_Radio_Button* rotateButton = new Fl_Radio_Button(0, 2 * 64, 64, 64);
	rotateButton->selection_color(0);
	rotateButton->callback(rotateCallback);
	toolIcons[ROTATE_TOOL] = new Fl_SVG_Image("../assets/images/rotate_tool.svg");
	rotateButton->image(toolIcons[ROTATE_TOOL]);
	rotateButton->tooltip("Rotate Tool");
	rotateButton->box(Fl_Boxtype::_FL_GLEAM_THIN_UP_BOX);

	Fl_Radio_Button* scaleButton = new Fl_Radio_Button(0, 3 * 64, 64, 64);
	scaleButton->selection_color(0);
	scaleButton->callback(scaleCallback);
	toolIcons[SCALE_TOOL] = new Fl_SVG_Image("../assets/images/scale_tool.svg");
	scaleButton->image(toolIcons[SCALE_TOOL]);
	scaleButton->tooltip("Scale Tool");

	Fl_Radio_Button* shapeButton = new Fl_Radio_Button(0, 4 * 64, 64, 64);
	shapeButton->selection_color(0);
	shapeButton->callback(shapeCallback);
	toolIcons[SHAPE_TOOL] = new Fl_SVG_Image("../assets/images/shape_tool.svg");
	shapeButton->image(toolIcons[SHAPE_TOOL]);
	shapeButton->tooltip("Shape Tool");
	// buttons for resolutions?

	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * 64, 64, 64);
	extrudeButton->selection_color(0);
	extrudeButton->callback(extrudeCallback, rightPanel);
	toolIcons[EXTRUDE_TOOL] = new Fl_SVG_Image("../assets/images/extrude_tool.svg");
	extrudeButton->image(toolIcons[EXTRUDE_TOOL]);
	extrudeButton->tooltip("Extrude Tool");

	// Subdivision button
	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * 64, 64, 64);
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionPanelCallback);
	toolIcons[SUBDIVIDE_TOOL] = new Fl_SVG_Image("../assets/images/subdiv_tool.svg");
	subdivisionButton->image(toolIcons[SUBDIVIDE_TOOL]);
	subdivisionButton->tooltip("Subdivision Tool");
		
	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * 64, 64, 64);
	bevelButton->selection_color(0);
	bevelButton->callback(bevelCallback);
	toolIcons[BEVEL_TOOL] = new Fl_SVG_Image("../assets/images/bevel_tool.svg");
	bevelButton->bind_image(toolIcons[BEVEL_TOOL]);
	bevelButton->tooltip("Bevel Tool");

	//sideToolbar->resizable(sideToolbar);
	sideToolbar->end();
	viewport->end();

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
	sideToolbar->show();
	//window->fullscreen();

	return Fl::run();
}