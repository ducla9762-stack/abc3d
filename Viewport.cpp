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
	format.setStencilBufferSize(0);
	format.setSamples(0);
	setFormat(format);

	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);

	selectionManager.setActiveElement(FACE);
}

Viewport::~Viewport()
{}

std::optional<SelectionItem> Viewport::createSelectionItem(Vec3u ids)
{
	switch (selectionManager.getActiveElement()) {
	case OBJECT:
		// Normal hit.
		if (ids.x != 0 && ids.x != MAX_UINT) {
			return SelectionItem{
				OBJECT,
				ids.x - 1,
				MAX_UINT
			};
		}
		// Vertex hit belonging to object.
		if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				OBJECT,
				ids.y - 1,
				MAX_UINT
			};
		}

		break;

	case VERTEX:
		if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				VERTEX,
				ids.y - 1,
				ids.z
			};
		}

		break;

	case FACE:
		if (ids.x != 0 && ids.x != MAX_UINT) {
			return SelectionItem{
				FACE,
				ids.x - 1,
				ids.y
			};
		}

		break;

	case EDGE:
		if (ids.x != 0 && ids.x != MAX_UINT && ids.z != MAX_UINT) {
			return SelectionItem{
				EDGE,
				ids.x - 1,
				ids.z
			};
		}

		break;
	}

	// Null if background hit.
	return std::nullopt;
}

