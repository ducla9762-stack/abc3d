#include "Viewport.h"
#include "shapes.h"

Viewport::Viewport(QWidget *parent)
	: QOpenGLWidget(parent)
{
	QSurfaceFormat format;
	format.setVersion(4, 6);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setRedBufferSize(8);
	format.setBlueBufferSize(8);
	format.setGreenBufferSize(8);
	format.setAlphaBufferSize(8);
	format.setDepthBufferSize(24);
	setFormat(format);

	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
}

Viewport::~Viewport()
{}

