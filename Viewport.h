#pragma once

#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QMouseEvent>

#include "render.h"
#include "helper.h"
#include "selectionManager.h"

class Viewport  : public QOpenGLWidget
{
	Q_OBJECT

	SelectionManager selectionManager;
	ModelingRenderer renderer;
	Scene scene;
	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates

	void initializeGL() override {
		GLenum error = glewInit();
		if (error != GLEW_OK) {
			std::cout << "Failed to initialize glew: " << glewGetErrorString(error) << std::endl;
			return;
		}

		const GLubyte* version = glGetString(GL_VERSION);
		std::cout << "OpenGL Version: " << version << std::endl;

		makeCurrent();
		bool success = renderer.Initialize(defaultFramebufferObject());
		if (!success) { 
			std::cout << "Failed to initialize renderer" << std::endl;
			return; 
		}

		renderer.SetCamera(sphericalToCartesian(camPos));
		renderer.UpdateRenderBuffers(scene);
		renderer.UpdateSelection(selectionManager.getSelectedItems());
	}

	void paintGL() override {
		renderer.Draw(defaultFramebufferObject());
	}

	void resizeGL(int w, int h) override {
		renderer.Resize(devicePixelRatio() * w, devicePixelRatio() * h, defaultFramebufferObject());
	}

	void mousePressEvent(QMouseEvent* event) override {
		event->accept();
		makeCurrent();
		Vec3u ids = renderer.SelectionIds(devicePixelRatio() * event->position().x(), devicePixelRatio() * (this->height() - 1 - event->position().y()));
		std::cout << ids << std::endl;
	}

public:
	Viewport(QWidget *parent);
	~Viewport();
};

