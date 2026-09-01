#ifndef _ABC3D_UI_H_
#define _ABC3D_UI_H_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Simple_Counter.H>
#include <FL/Fl_Radio_Button.H>
#undef max

#include "viewport.h"

inline const char* AXIS_LETTERS[3] = { "x", "y", "z" };

inline Viewport* viewport;

class ToolPanel : public Fl_Window {
public:
	virtual void updateValues() {}

	ToolPanel(int x, int y, int w, int h, const char* title = nullptr) : Fl_Window(x, y, w, h, title) {}
};

#define BASE_TRANSFORMATION_PANEL_CONSTRUCTOR(labelText, defaultVal, stepSize) \
	Fl_Box* labelBox = new Fl_Box(0, 0, w, 0);\
	labelBox->label(labelText);\
	int labelW, labelH;\
	labelBox->measure_label(labelW, labelH);\
	labelBox->size(w, labelH);\
	int yPos = labelH + 5;\
	for (int i = 0; i < 3; ++i) {\
		spinners[i] = new Fl_Simple_Counter(0, yPos, w, 20);\
		spinners[i]->value(defaultVal);\
		spinners[i]->step(stepSize);\
		spinners[i]->label(AXIS_LETTERS[i]);\
		spinners[i]->align(FL_ALIGN_LEFT);\
		spinners[i]->type(FL_FLOAT_INPUT);\
		spinners[i]->range(-1000000000, 1000000000);\
		yPos += 25;\
	}\
	spinners[0]->callback(spinnerCallback<0>);\
	spinners[1]->callback(spinnerCallback<1>);\
	spinners[2]->callback(spinnerCallback<2>);\

/// <summary> Subclass of ToolPanel that populates rotation spinners in the rotation panel
/// Works with the rotate panel by calling viewport </summary>
class RotateToolPanel : public ToolPanel {
	template <int component>
	static void spinnerCallback(Fl_Widget* spinner) {
		float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
		viewport->setRot(component, spinnerVal);
	}

	Fl_Simple_Counter* spinners[3] = { nullptr, nullptr, nullptr };

public:
	RotateToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h, title) {
		begin();
		BASE_TRANSFORMATION_PANEL_CONSTRUCTOR("Rotation", 0.0, 2.0)
		size(w, yPos);
		end();
	}

	void updateValues() override {
		const HalfEdgeMesh* mesh = viewport->primarySelectedMesh();
		if (!mesh) {
			hide();
			return;
		}
		show();
		for (int i = 0; i < 3; ++i) {
			spinners[i]->value(mesh->eulerRot[i]);
		}
	}
};

/// <summary> Subclass of ToolPanel that populates scale spinners in the scale panel
/// Works with the scale panel by calling viewport </summary>
class ScaleToolPanel : public ToolPanel {
	template <int component>
	static void spinnerCallback(Fl_Widget* spinner) {
		float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
		viewport->setScale(component, spinnerVal);
	}

	Fl_Simple_Counter* spinners[3] = { nullptr, nullptr, nullptr };
public:
	ScaleToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h, title) {
		begin();
		
BASE_TRANSFORMATION_PANEL_CONSTRUCTOR("Scale", 0.0, 0.2)
		size(w, yPos);
		end();
	}

	void updateValues() override {
		const HalfEdgeMesh* mesh = viewport->primarySelectedMesh();
		if (!mesh) {
			hide();
			return;
		}
		show();
		for (int i = 0; i < 3; ++i) {
			spinners[i]->value(mesh->scale[i]);
		}
	}
};

/// <summary> Subclass of ToolPanel that populates translation spinners in the translation panel
/// Works with the panel by calling viewport </summary>
class TranslationToolPanel : public ToolPanel {
	template <int component>
	static void spinnerCallback(Fl_Widget* spinner) {
		float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
		viewport->setPos(component, spinnerVal);
	}

	Fl_Radio_Button* buttons[2] = { nullptr, nullptr };

	Fl_Simple_Counter* spinners[3] = { nullptr, nullptr, nullptr };
	Fl_Window* spinnerContainer = nullptr;

public:
	TranslationToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h, title) {
		begin();

		spinnerContainer = new Fl_Window(0, 45, w, 0);
		BASE_TRANSFORMATION_PANEL_CONSTRUCTOR("Position", 1.0, 0.2)
		spinnerContainer->end();
		spinnerContainer->size(w, yPos);

		// Outward Translation Button
		buttons[0] = new Fl_Radio_Button(0, 0, w, 20, "Outward");
		buttons[0]->selection_color(10);
		buttons[0]->box(FL_FLAT_BOX);
		//buttons[0]->callback(actionChangedCallback<TRANSLATE_OUT>);
		buttons[0]->align(FL_ALIGN_INSIDE);

		// Free Translation Button
		buttons[1] = new Fl_Radio_Button(0, 20, w, 20, "Free");
		buttons[1]->selection_color(10);
		buttons[1]->box(FL_UP_BOX);
		//buttons[1]->callback(actionChangedCallback<TRANSLATE_FREE>);
		buttons[1]->align(FL_ALIGN_INSIDE);
		buttons[1]->value(1);

		resize(x, y, w, yPos + 45);

		end();
	}

	void updateValues() override {
		const HalfEdgeMesh* mesh = viewport->primarySelectedMesh();
		if (!mesh) {
			spinnerContainer->hide();
			return;
		}
		spinnerContainer->show();
		for (int i = 0; i < 3; ++i) {
			spinners[i]->value(mesh->pos[i]);
		}
	}

	void show() override
	{
		ToolPanel::show();
		buttons[0]->value(0);
		buttons[1]->value(1);
	}

	void hideButtons() {
		buttons[0]->hide();
		buttons[1]->hide();
	}

	void showButtons() {
		buttons[0]->show();
		buttons[1]->show();
	}

	template <typename T>
	void buttonCallback(int button, T callback) {
		buttons[button]->callback(callback);
	}
};

/// <summary> Subclass of ToolPanel that populates translation, scale, and rotation spinners in the select panel
/// Works with the select panel by calling viewport </summary>
class SelectToolPanel : public ToolPanel {
	TranslationToolPanel* posPanel = nullptr;
	ScaleToolPanel* scalePanel = nullptr;
	RotateToolPanel* rotPanel = nullptr;
	Vec2i posPanelPos = Vec2i(0);
	Vec2i scalePanelPos = Vec2i(0);
	Vec2i rotPanelPos = Vec2i(0);
	bool canShow = true;
public:
	SelectToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, 0, 0) { end(); }

	void updateValues() override {
		if (posPanel) { posPanel->updateValues(); }
		if (scalePanel) { scalePanel->updateValues(); }
		if (rotPanel) { rotPanel->updateValues(); }
	}

	void setPositionPanel(TranslationToolPanel* posPanel) {
		this->posPanel = posPanel;
	}

	void setScalePanel(ScaleToolPanel* scalePanel) {
		this->scalePanel = scalePanel;
	}

	void setRotatePanel(RotateToolPanel* rotPanel) {
		this->rotPanel = rotPanel;
	}

	void show() override {
		if (!canShow) { return; }

		int yPos = y();
		if (posPanel) {
			posPanelPos.Set(posPanel->x(), posPanel->y());
			posPanel->position(x(), yPos);
			posPanel->hideButtons();
			posPanel->show();
			yPos += posPanel->h();
			
		}
		if (scalePanel) {
			scalePanelPos.Set(scalePanel->x(), scalePanel->y());
			scalePanel->position(x(), yPos);
			scalePanel->show();
			yPos += scalePanel->h();
		}
		if (rotPanel) {
			rotPanelPos.Set(rotPanel->x(), rotPanel->y());
			//std::cout << "hi" << rotPanelPos << std::endl;
			rotPanel->position(x(), yPos);
			rotPanel->show();
			//std::cout << "la" << posPanel->x() << ", " << posPanel->y() << std::endl;
		}

		canShow = false;
	}

	void hide() override {
		if (posPanel) {
			posPanel->hide();
			posPanel->showButtons();
			posPanel->position(posPanelPos.x, posPanelPos.y);
		}
		if (scalePanel) {
			scalePanel->hide();
			scalePanel->position(scalePanelPos.x, scalePanelPos.y);
		}
		if (rotPanel) {
			rotPanel->hide();
			rotPanel->position(rotPanelPos.x, rotPanelPos.y);
		}
		canShow = true;
	}
};

/// <summary> Subclass of ToolPanel that populates buttons for each shape in the shape panel
/// Works with the select panel by calling viewport </summary>
class ShapeToolPanel : public ToolPanel {
	// Callbacks from within the Shape button
	static void cubeCallback(Fl_Widget* w, void* data) {
		viewport->displayCube();
	}

	static void UVSphereCallback(Fl_Widget* w, void* data) {
		viewport->displayUVSphere();
	}

	static void icoSphereCallback(Fl_Widget* w, void* data) {
		viewport->displayIcoSphere();
	}

	static void pyramidCallback(Fl_Widget* w, void* data) {
		viewport->displayPyramid();
	}

	static void cylinderCallback(Fl_Widget* w, void* data) {
		viewport->displayCylinder();
	}

	static void torusCallback(Fl_Widget* w, void* data) {
		viewport->displayTorus();
	}
public:
	ShapeToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h) {
		begin();

		int yPos = 0;
		int interval = h / 9;

		// Cube Shape Button
		Fl_Button* cubeButton = new Fl_Button(0, yPos, w, interval, "Cube");
		cubeButton->selection_color(10);
		cubeButton->box(FL_UP_BOX);
		cubeButton->callback(cubeCallback);
		cubeButton->align(FL_ALIGN_INSIDE);
		yPos += interval;

		// UV Sphere Shape Button
		Fl_Button* UVSphereButton = new Fl_Button(0, yPos, w, interval, "UV Sphere");
		UVSphereButton->selection_color(10);
		UVSphereButton->box(FL_UP_BOX);
		UVSphereButton->callback(UVSphereCallback);
		UVSphereButton->align(FL_ALIGN_INSIDE);
		yPos += interval;

		// ICO Sphere Shape Button
		Fl_Button* icoSphereButton = new Fl_Button(0, yPos, w, interval, "Ico Sphere");
		icoSphereButton->selection_color(10);
		icoSphereButton->box(FL_UP_BOX);
		icoSphereButton->callback(icoSphereCallback);
		icoSphereButton->align(FL_ALIGN_INSIDE);
		yPos += interval;

		// Pyramid Shape Button
		Fl_Button* pyramidButton = new Fl_Button(0, yPos, w, interval, "Pyramid");
		pyramidButton->selection_color(10);
		pyramidButton->box(FL_UP_BOX);
		pyramidButton->callback(pyramidCallback);
		pyramidButton->align(FL_ALIGN_INSIDE);
		yPos += interval;

		// Cylinder Button
		Fl_Button* cylinderButton = new Fl_Button(0, yPos, w, interval, "Cylinder");
		cylinderButton->selection_color(10);
		cylinderButton->box(FL_UP_BOX);
		cylinderButton->callback(cylinderCallback);
		cylinderButton->align(FL_ALIGN_INSIDE);
		yPos += interval;

		// Torus Shape Button
		Fl_Button* torusButton = new Fl_Button(0, yPos, w, interval, "Torus");
		torusButton->selection_color(10);
		torusButton->box(FL_UP_BOX);
		torusButton->callback(torusCallback);
		torusButton->align(FL_ALIGN_INSIDE);
		
		end();
	}
};

/// <summary> Subclass of ToolPanel that populates subdivision and tesselation buttons in the subdivision panel
/// Works with the select panel by calling viewport </summary>
class SubdivideToolPanel : public ToolPanel {
	static void subdivisionCallback(Fl_Widget* w, void*) {
		viewport->subdivision();
	}

	static void tesselationCallback(Fl_Widget*, void*) {
		viewport->tesselate();
	}
public:
	SubdivideToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h) {
		begin();

		int buttonH = h / 7;

		// Subdivide Button
		Fl_Button* subdivideButton = new Fl_Button(0, 0, w, buttonH, "Subdivide");
		subdivideButton->selection_color(10);
		subdivideButton->box(FL_UP_BOX);
		subdivideButton->callback(subdivisionCallback);
		subdivideButton->align(FL_ALIGN_INSIDE);

		// Tesselate Button
		Fl_Button* tesselateButton = new Fl_Button(0, buttonH, w, buttonH, "Tesselation");
		tesselateButton->selection_color(10);
		tesselateButton->box(FL_UP_BOX);
		tesselateButton->callback(tesselationCallback);
		tesselateButton->align(FL_ALIGN_INSIDE);

		end();
	}
};

/// <summary> Subclass of ToolPanel that populates ______
/// Works with the select panel by calling viewport </summary>
// TODO 
class BevelToolPanel : public ToolPanel {
public:
	BevelToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h) { end(); }
};

/// <summary> Subclass of ToolPanel that populates _____
/// Works with the select panel, not the buttons themselves, by calling viewport </summary>
// TODO
class ExtrudeToolPanel : public ToolPanel {
public:
	ExtrudeToolPanel(int x, int y, int w, int h, const char* title = nullptr) : ToolPanel(x, y, w, h) { end(); }
};

#endif
