#ifndef _ABC3D_VIEWPORT_H_
#define _ABC3D_VIEWPORT_H_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>

#include <GL/glew.h>

#include <iostream>

#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"
#include "helper.h"
#include "selectionManager.h"
#undef max

inline bool initGlew = true;

enum Mode { MODEL, SIMPLE, RENDER };
enum Action { CAMERA, SELECT, TRANSLATE_OUT, TRANSLATE_FREE, TRANSLATE_AXIS, TRANSLATE_EDGE, SCALE, ROTATE, EXTRUDE, BEVEL, SHAPE };

class Viewport : public Fl_Gl_Window {

	SelectionManager selectionManager;

	Mode activeMode = MODEL;
	//Element activeElement = selectionManager.getActiveElement();
	Action activeAction = SELECT;

	ModelingRenderer modelingRenderer;

	Renderer* activeRenderer = &modelingRenderer;
	bool modeChanged = false;

	bool additiveSelection = false;

	Scene scene;

	void (*primarySelectionCallback)() = nullptr;

	Vec2f mouseDragStartPosUV = Vec2f(0.0f);
	Vec2f initialDragDirUV = Vec2f(0.0f);
	Vec2f initialDragDir = Vec2f(0.0f);
	Vec2f mouseDragPrevPos = Vec2f(0.0f);
	Vec2f mouseDragPrevPosUV = Vec2f(0.0f);
	Vec2f mouseDragStartPos = Vec2f(0.0f);
	//Vec3u selectionIds = Vec3u(uint(0));
	Vec3f actionDir = Vec3f(0.0f);
	uint actionEdge = MAX_UINT; 
	std::vector<Vec3f> dragActionDirs;
	Vec3f initialActionState = Vec3f(0.0f);
	bool firstDragCallback = false;

	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates

	uint primarySelection = MAX_UINT;

	/// <summary>
	/// Meshes export every selected or whole scene when nothing is selected
	/// </summary>
	/// <returns>Vector of meshes ids</returns>
	std::vector<uint> exportMeshIds(); 

	/// <summary>
	/// Draw loop required for an FLTK widget. Called by FLTK.
	/// </summary>
	void draw();

	/// <summary>
	/// Catches FLTK events; button-presses, etc.
	/// </summary>
	/// <param name="event"> event type. </param>
	/// <returns> callback function </returns>
	int handle(int event);

	/// <summary>
	/// Logic executed on a mouse drag event.
	/// 
	/// todo: fill this out.
	/// </summary>
	void mouseDrag();

	/// <summary>
	/// Translates the selected elements out to pos.
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="uv"></param>
	/// <returns> True if translation occurred. </returns>
	bool translateOutDrag(const Vec2f& pos, const Vec2f& uv);

	/// <summary>
	/// Translates selected elements along an edge to pos.
	/// </summary>
	/// <param name="pos"></param>
	/// <returns> True if translation occurred. </returns>
	bool translateAlongEdgeDrag(const Vec2f& pos); 

	/// <summary>
	/// Translates the selected vertices out.
	/// </summary>
	/// <param name="scale"></param>
	/// <returns> True if translation occurred. </returns>
	bool translateOutVertexDrag(float scale);

	/// <summary>
	/// Translates selected elements in free mode.
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="uv"></param>
	/// <returns> True if translation occurred. </returns>
	bool translateFreeDrag(const Vec2f& pos, const Vec2f& uv);

	/// <summary>
	/// Uniformly scales selected elements.
	/// </summary>
	/// <param name="pos"></param>
	/// <returns> True if scale occurred. </returns>
	bool uniformScaleDrag(const Vec2f& pos);

	/// <summary>
	/// Rotates selected elements.
	/// </summary>
	/// <param name="pos"></param>
	/// <returns> True if rotation occurred. </returns>
	bool rotateDrag(const Vec2f& pos);
	
	/// <summary>
	/// Determines what to do when an arrow is dragged.
	/// </summary>
	/// <param name="ids"> The arrow selection ids. </param>
	/// <returns> True if the arrows were handled. </returns>
	bool arrowDrag(Vec2u ids);

	/// <summary>
	/// Logic executed on a mouse button press event.
	/// </summary>
	/// <returns></returns>
	int mousePush();

	/// <summary>
	/// Logic executed on a mouse wheel event.
	/// </summary>
	void mouseWheel();

	/// <summary>
	/// Logic executed on a key press event.
	/// </summary>
	void keyDown();

	/// <summary>
	/// Logic executed on a key release event.
	/// </summary>
	void keyUp();

	/// <summary>
	/// Constructs a selection item given the raw ids from the renderer.
	/// Intended to be sent to the selection manager for processing.
	/// </summary>
	/// <param name="ids"> Raw IDs from the renderer. </param>
	/// <returns> Valid selection object. </returns>
	std::optional<SelectionItem> createSelectionItem(Vec3u ids);

public:
	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="x"> Viewport x-offset, top left = 0,0 </param>
	/// <param name="y"> Viewport y-offset, top left = 0,0 </param>
	/// <param name="w"> Viewport width. </param>
	/// <param name="h"> Viewport height. </param>
	/// <param name="label"></param>
	Viewport(int x, int y, int w, int h, const char* label = nullptr);

	/// <summary>
	/// Sets the scale of the component by scale.
	/// </summary>
	/// <param name="component"> x, y, or z component. </param>
	/// <param name="scale"> Amount </param>
	void setScale(int component, float scale);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="component"></param>
	/// <param name="scale"></param>
	/// <returns></returns>
	float getScale(int component, float scale);

	/// <summary>
	/// Subdivides all selected meshes.
	/// 
	/// This function should be modified to allow for specificity in the level of subdivision.
	/// </summary>
	void subdivision();

	/// <summary>
	/// Tesselates all selected meshes.
	/// 
	/// This function should be modified to allow for specificity in the level of subdivision.
	/// </summary>
	void tesselate();

	/// <summary>
	/// todo
	/// </summary>
	void bevel();
	
	/// <summary>
	/// Extrude a face with a drag.
	/// </summary>
	/// <param name="pos"></param>
	bool extrudeDrag(const Vec2f& pos, const Vec2f& uv);

	/// <summary>
	/// Adds a new cube mesh to the scene list.
	/// </summary>
	void displayCube();

	/// <summary>
	/// Adds a new pyramid mesh to the scene list.
	/// </summary>
	void displayPyramid();

	/// <summary>
	/// Adds a new cylinder mesh to the scene list.
	/// </summary>
	void displayCylinder();

	/// <summary>
	/// Adds a new UV sphere mesh to the scene list.
	/// </summary>
	void displayUVSphere();

	/// <summary>
	/// Adds a new icosphere mesh to the scene list.
	/// </summary>
	void displayIcoSphere();

	/// <summary>
	/// Adds a new torus mesh to the scene list.
	/// </summary>
	void displayTorus();

	/// <summary>
	/// todo
	/// </summary>
	/// <param name="compontent"> x, y, or z component. </param>
	/// <param name="angle"> </param>
	/// <returns></returns>
	float getRot(int compontent, float angle);

	/// <summary>
	/// Delete all selected meshes from the scene list.
	/// </summary>
	void deleteElement();

	/// <summary>
	/// Duplicates all selected meshes.
	/// </summary>
	void duplicateElement();

	/// <summary>
	/// Set the rotation of component of mesh to angle.
	/// </summary>
	/// <param name="component"> x, y, or z component. </param>
	/// <param name="angle"> Angle. </param>
	void setRot(int component, float angle);

	/// <summary>
	/// todo
	/// </summary>
	/// <param name="compontent"></param>
	/// <param name="angle"></param>
	/// <returns></returns>
	float getPos(int compontent, float angle);

	/// <summary>
	/// Set the position of component of mesh.
	/// </summary>
	/// <param name="compontent"> x, y, or z component. </param>
	/// <param name="pos"> Position. </param>
	/// <returns></returns>
	void setPos(int component, float pos);

	/// <summary>
	/// Loads the file at filepath as a mesh.
	/// </summary>
	/// <param name="filepath"></param>
	/// <returns></returns>
	ImportReport loadFile(const char* filepath);

	/// <summary>
	/// Each mesh in exportMeshIds is written in world space so pos, rot, and scale save
	/// </summary>
	/// <param name="filepath"></param>
	/// <returns>success or fail when export</returns>
	bool exportObj(const char* filepath);

	/// <summary>
	/// STL has no idea for separate objects. We will store them as a soup of triangles for all objects.
	/// </summary>
	/// <param name="filepath"></param>
	/// <returns>success or fail of export</returns>
	bool exportStl(const char* filepath);

	/// <summary>
	/// Returns the active mode: model, simple, or render.
	/// </summary>
	/// <returns> Active mode. </returns>
	Mode getActiveMode();

	/// <summary>
	/// Returns the active element: vertex, edge, face, or object.
	/// </summary>
	/// <returns> Active element. </returns>
	Element getActiveElement();

	/// <summary>
	/// Returns the active action, i.e. the current tool being used.
	/// </summary>
	/// <returns> Active action. </returns>
	Action getActiveAction();

	/// <summary>
	/// Sets the active mode.
	/// </summary>
	/// <param name="mode"> Mode to change to. </param>
	void setActiveMode(Mode mode);

	/// <summary>
	/// Sets the active element.
	/// </summary>
	/// <param name="element"> Element to change to. </param>
	void setActiveElement(Element element);

	/// <summary>
	/// Sets the active action.
	/// </summary>
	/// <param name="action"> Action to change to. </param>
	void setActiveAction(Action action);

	/// <summary>
	/// Returns the non-mutable primary selected mesh, which is the last selected item.
	/// </summary>
	/// <returns> Pointer to the primary selected mesh. </returns>
	const HalfEdgeMesh* primarySelectedItem();

	/// <summary>
	/// Sets the callback for what is called when the primary selected object is selected.
	/// </summary>
	/// <param name="callback"></param>
	void setPrimarySelectionCallback(void (*callback)());
};

#endif
