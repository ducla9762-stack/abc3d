
#include "ABC3D.h"

ABC3D::ABC3D(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.transformModes->setVisible(false);
    ui.posInput->setEnabled(false);
    ui.scaleInput->setEnabled(false);
    ui.rotInput->setEnabled(false);
    //ui.topRightPanels->setCurrentIndex(3);


    QAction* actionStandardMode = new QAction("Standard Mode", ui.menuBar);
    ui.menuBar->addAction(actionStandardMode);
    //QObject::connect(actionStandardMode, &QAction::triggered, this, &ABC3D::showStandardMode);


    QAction* actionSimpleMode = new QAction("Simple Mode", ui.menuBar);
    ui.menuBar->addAction(actionSimpleMode);
    //QObject::connect(actionSimpleMode, &QAction::triggered, this, &ABC3D::showSimpleMode);

    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, &Viewport::deleteSelection);
    QObject::connect(this, &ABC3D::ctrlKeyPressed, ui.viewport, &Viewport::setAdditiveSelection);
    QObject::connect(this, &ABC3D::ctrlKeyReleased, ui.viewport, &Viewport::setSingleSelection);

    QObject::connect(ui.subdivideToolButton, &QPushButton::released, this, [=]() { 
        ui.bottomRightPanels->setCurrentIndex(2); 
        });

    QObject::connect(ui.shapeToolButton, &QPushButton::released, this, [=]() {
        ui.bottomRightPanels->setCurrentIndex(1);
        });

    // The main Move tool defaults to Free translation. Its toolbar button and
    // the translation-mode buttons live in separate exclusive groups, so keep
    // the visible mode selection synchronized when returning to Move.
    QObject::connect(ui.translateToolButton, &QPushButton::released, this, [=]() {
        ui.freeModeTranslationButton->setChecked(true);
        });
}

void ABC3D::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control) {
        event->accept();
        emit ctrlKeyPressed();
        return;
    }
}

void ABC3D::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
    // Close the program is escape is clicked
    case Qt::Key_Control:
        event->accept();
        emit ctrlKeyReleased();
        return;
    case Qt::Key_Escape:
        event->accept();
        this->close();
        return;
    // Delete Selction
    case Qt::Key_Delete:
        event->accept();
        emit deleteKeyPressed();
        return;
    // Duplicate Selection
    case Qt::Key_D:
        event->accept();
        event->modifiers()& Qt::KeyboardModifier::ControlModifier;
        ui.viewport->duplicateSelection();
        return;
    // Toggle Grid
    case Qt::Key_G:
        event->accept();
        //ui.gridVisibilityBox->toggle();
        return;

    // Toggle Vertices 
    case Qt::Key_P:
        event->accept();
        ui.vertexVisibilityBox->toggle();
        return;

    // Bottom toolbar hotkeys
    case Qt::Key_1:
        event->accept();
        ui.viewport->setElementVertex();
        return;
    case Qt::Key_2:
        event->accept();
        ui.viewport->setElementEdge();
        return;
    case Qt::Key_3:
        event->accept();
        ui.viewport->setElementFace();
        return;
    case Qt::Key_4:
        event->accept();
        ui.viewport->setElementObject();
        return;


    // Select
    case Qt::Key_V:
        event->accept();
        ui.viewport->setActionSelect();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.selectToolButton->setChecked(true);
        return;
    case Qt::Key_C:
        ui.viewport->setActionCamera();
        ui.bottomRightPanels->setCurrentIndex(3);
        ui.cameraToolButton->setChecked(true);
        return;
    // Translate
    case Qt::Key_T:
        event->accept();
        ui.freeModeTranslationButton->setChecked(true);
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.translateToolButton->setChecked(true);
        return;
    // Scale
    case Qt::Key_S:
        event->accept();
        ui.viewport->setActionScale();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.scaleToolButton->setChecked(true);
        return;
    // Rotate
    case Qt::Key_R:
        event->accept();
        ui.viewport->setActionRotate();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.rotateToolButton->setChecked(true);
        return;
    // Shape
    case Qt::Key_Q:
        event->accept();
        ui.bottomRightPanels->setCurrentIndex(1);
        ui.shapeToolButton->setChecked(true);
        return;
    // Extrude
    case Qt::Key_E:
        event->accept();
        ui.viewport->setActionExtrude();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.extrudeToolButton->setChecked(true);
        return;
    // Subdivide
    case Qt::Key_Z:
        event->accept();
        ui.bottomRightPanels->setCurrentIndex(2);
        ui.subdivideToolButton->setChecked(true);
        return;
    // Bevel
    case Qt::Key_B:
        event->accept();
        ui.viewport->setActionBevel();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.bevelToolButton->setChecked(true);
        return;
    default:
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    QMainWindow::keyReleaseEvent(event);
}

ABC3D::~ABC3D()
{}