
#include "ABC3D.h"

ABC3D::ABC3D(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.transformModes->setVisible(false);
    ui.scaleModes->setVisible(false);
    ui.posInput->setEnabled(false);
    ui.scaleInput->setEnabled(false);
    ui.rotInput->setEnabled(false);
    ui.topRightPanels->setCurrentIndex(1);
    ui.tutorialBrowser->setReadOnly(true);
    ui.tutorialBrowser->setSource(QUrl::fromLocalFile(QString("assets/tutorials/Welcome.html")));

    QAction* actionCamera = new QAction("Camera", ui.menuBar);
    ui.menuBar->addAction(actionCamera);
    QObject::connect(actionCamera, &QAction::triggered, this, &ABC3D::showCameraPanel);

    QAction* actionStandardMode = new QAction("Standard Mode", ui.menuBar);
    ui.menuBar->addAction(actionStandardMode);
    //QObject::connect(actionStandardMode, &QAction::triggered, this, &ABC3D::showStandardMode);

    QAction* actionSimpleMode = new QAction("Simple Mode", ui.menuBar);
    ui.menuBar->addAction(actionSimpleMode);
    //QObject::connect(actionSimpleMode, &QAction::triggered, this, &ABC3D::showSimpleMode);

    QObject::connect(ui.knifeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionKnife);
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
        ui.axisModeTranslationButton->setChecked(true);
        });

    // Connects the "Load Tutorial" Button to the QTextBrowser
    // Opens the file explorer in the "tutorials" folder
    // Allows users to select text, HTML, or markdown file formats
    QObject::connect(ui.loadTutorialButton, &QPushButton::released, this, [=]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Open Tutorial"), "assets/tutorials", tr("Text Files(*.txt * .html * .markdown * .md"));
        ui.tutorialBrowser->setSource(QUrl::fromLocalFile(file));
        });
}

void ABC3D::keyPressEvent(QKeyEvent* event) {
    bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    switch (event->key()) {
    case Qt::Key_Control:
            event->accept();
            emit ctrlKeyPressed();
            break;
    // Duplicate - CTRL + D
    case Qt::Key_D:
        if (ctrl) { ui.viewport->duplicateSelection(); }
        break;
    // Escape - Cancels the knife cut, or leaves the knife. Otherwise closes program
    case Qt::Key_Escape:
        event->accept();
        if (ui.viewport->knifeActive()) {
            if (!ui.viewport->knifeCancel()) {
                ui.viewport->setActionSelect();
                ui.bottomRightPanels->setCurrentIndex(0);
                ui.selectToolButton->setChecked(true);
            }
            break;
        }
        this->close();
        break;
    // Enter - Confirms the knife cut
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (ui.viewport->knifeActive()) {
            event->accept();
            ui.viewport->knifeConfirm();
        }
        break;
    // Knife
    case Qt::Key_K:
        event->accept();
        ui.viewport->setActionKnife();
        ui.knifeToolButton->setChecked(true);
        break;
    // Knife cut through
    case Qt::Key_X:
        if (ui.viewport->knifeActive()) {
            event->accept();
            ui.viewport->toggleKnifeCutThrough();
        }
        break;
    // Delete Selction
    case Qt::Key_Delete:
        event->accept();
        emit deleteKeyPressed();
        break;
    // Toggle Grid
    case Qt::Key_G:
        event->accept();
        ui.gridVisibilityBox->toggle();
        break;
    // Toggle Vertices 
    case Qt::Key_P:
        event->accept();
        ui.vertexVisibilityBox->toggle();
        break;

    // ---------------------------------------------------------------------------------------------
    // Bottom Element Hotkeys  ---------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
    case Qt::Key_1:
        event->accept();
        ui.viewport->setElementVertex();
        break;
    case Qt::Key_2:
        event->accept();
        ui.viewport->setElementEdge();
        break;
    case Qt::Key_3:
        event->accept();
        ui.viewport->setElementFace();
        break;
    case Qt::Key_4:
        event->accept();
        ui.viewport->setElementObject();
        break;

    //----------------------------------------------------------------------------------------------
    // Left Toolbar Hotkeys ------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
    // Select
    case Qt::Key_V:
        event->accept();
        ui.viewport->setActionSelect();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.selectToolButton->setChecked(true);
        break;
    // Camera
    case Qt::Key_C:
        ui.viewport->setActionCamera();
        ui.bottomRightPanels->setCurrentIndex(3);
        ui.cameraToolButton->setChecked(true);
        break;
    // Translate
    case Qt::Key_T:
        event->accept();
        ui.axisModeTranslationButton->setChecked(true);
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.translateToolButton->setChecked(true);
        break;
    // Scale
    case Qt::Key_S:
        event->accept();
        // Save - CTRL + S
        if (ctrl) { saveProject(); }
        else {
            ui.viewport->setActionScale();
            ui.bottomRightPanels->setCurrentIndex(0);
            ui.scaleToolButton->setChecked(true);
        }
        break;
    // Rotate
    case Qt::Key_R:
        event->accept();
        ui.viewport->setActionRotate();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.rotateToolButton->setChecked(true);
        break;
    // Shape
    case Qt::Key_Q:
        event->accept();
        ui.bottomRightPanels->setCurrentIndex(1);
        ui.shapeToolButton->setChecked(true);
        break;
    // Extrude
    case Qt::Key_E:
        event->accept();
        ui.viewport->setActionExtrude();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.extrudeToolButton->setChecked(true);
        break;
    // Subdivide
    case Qt::Key_Z:
        event->accept();
        ui.bottomRightPanels->setCurrentIndex(2);
        ui.subdivideToolButton->setChecked(true);
        break;
    // Bevel
    case Qt::Key_B:
        event->accept();
        ui.viewport->setActionBevel();
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.bevelToolButton->setChecked(true);
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void ABC3D::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Control:
        event->accept();
        emit ctrlKeyReleased();
        break;
    }
    QMainWindow::keyReleaseEvent(event);
}

void ABC3D::showEvent(QShowEvent* event){
    QMainWindow::showEvent(event);
    if (firstShow) {
        QMetaObject::invokeMethod(this, &ABC3D::showStart, Qt::QueuedConnection);
        firstShow = false;
    }
}

ABC3D::~ABC3D()
{}