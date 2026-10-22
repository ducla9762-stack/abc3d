
#include "ABC3D.h"

ABC3D::ABC3D(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.transformModes->setVisible(false);
    ui.scaleModes->setVisible(false);
    ui.mergeModes->setVisible(false);
    ui.posInput->setEnabled(false);
    ui.scaleInput->setEnabled(false);
    ui.rotInput->setEnabled(false);
    ui.topRightPanels->setCurrentIndex(1);
    
    // Makes the model and tree used to view all the help documents
    QString helpPath = "assets/help documents";
    helpFileModel->setFilter(QDir::Files);
    helpFileModel->setRootPath(helpPath);
    ui.helpTree->setModel(helpFileModel);
    ui.helpTree->setRootIndex(helpFileModel->index(helpPath));
    ui.helpTree->setColumnHidden(1, true);
    ui.helpTree->setColumnHidden(2, true);
    ui.helpTree->setColumnHidden(3, true);
    ui.helpTree->header()->hide();
    ui.helpTree->show();
    ui.helpStack->setCurrentIndex(1);
    ui.helpBrowser->setReadOnly(true);
    QObject::connect(ui.helpTree, &QTreeView::pressed, this, &ABC3D::helpRowClicked);

    // Makes the model and tree used to view all the tutorial documents
    QString tutorialPath = "assets/tutorials";
    tutorialFileModel->setFilter(QDir::Files);
    tutorialFileModel->setRootPath(tutorialPath);
    ui.tutorialTree->setModel(tutorialFileModel);
    ui.tutorialTree->setRootIndex(tutorialFileModel->index(tutorialPath));
    ui.tutorialTree->setColumnHidden(1, true);
    ui.tutorialTree->setColumnHidden(2, true);
    ui.tutorialTree->setColumnHidden(3, true);
    ui.tutorialTree->header()->hide();
    ui.tutorialTree->show();
    ui.tutorialStack->setCurrentIndex(1);
    ui.tutorialBrowser->setReadOnly(true);
    QObject::connect(ui.tutorialTree, &QTreeView::pressed, this, &ABC3D::tutorialRowClicked);

    // Menu button for the camera panel - does the same thing as the toolbar
    QAction* actionCamera = new QAction("Camera", ui.menuBar);
    ui.menuBar->addAction(actionCamera);
    QObject::connect(actionCamera, &QAction::triggered, this, &ABC3D::showCameraPanel);

    // Menu button for standard mode, selected by default
    QAction* actionStandardMode = new QAction("Standard Mode", ui.menuBar);
    ui.menuBar->addAction(actionStandardMode);
    //QObject::connect(actionStandardMode, &QAction::triggered, this, &ABC3D::showStandardMode);

    // Menu button for simple mode
    QAction* actionSimpleMode = new QAction("Simple Mode", ui.menuBar);
    ui.menuBar->addAction(actionSimpleMode);
    //QObject::connect(actionSimpleMode, &QAction::triggered, this, &ABC3D::showSimpleMode);

    // Connections based on key-presses
    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, &Viewport::deleteSelection);
    QObject::connect(this, &ABC3D::ctrlKeyPressed, ui.viewport, &Viewport::setAdditiveSelection);
    QObject::connect(this, &ABC3D::ctrlKeyReleased, ui.viewport, &Viewport::setSingleSelection);

    QObject::connect(ui.knifeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionKnife);
    QObject::connect(ui.mergeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionMerge);

    // Pulls up subdivide panel
    QObject::connect(ui.subdivideToolButton, &QPushButton::released, this, [=]() { 
        ui.bottomRightPanels->setCurrentIndex(2); 
        });

    // Pulls up shape panel
    QObject::connect(ui.shapeToolButton, &QPushButton::released, this, [=]() {
        ui.bottomRightPanels->setCurrentIndex(1);
        });

    QObject::connect(ui.mergeToolButton, &QPushButton::released, this, [=]() {
        ui.bottomRightPanels->setCurrentIndex(0);
        });

    // Emits signal for when to create shapes (with resolutions)
    QObject::connect(ui.torusButton_5, &QPushButton::released, this, [=]() {
            ui.viewport->loadTorus(ui.torusSlider->value());
        });

    QObject::connect(ui.cylinderButton_5, &QPushButton::released, this, [=]() {
            ui.viewport->loadCylinder(ui.cylinderSlider->value());
        });

    QObject::connect(ui.uvSphereButton_5, &QPushButton::released, this, [=]() {
            ui.viewport->loadUvSphere(ui.uvSphereSlider->value());
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
        ui.tutorialStack->setCurrentIndex(1);
        });

    // Connects the "Load New Document" Button to the QTextBrowser
    // Opens the file explorer in the "tutorials" folder
    // Allows users to select text, HTML, or markdown file formats
    QObject::connect(ui.loadHelpButton, &QPushButton::released, this, [=]() {
        ui.helpStack->setCurrentIndex(1);
        });

    QObject::connect(ui.importTutorialButton, &QPushButton::released, this, [=]() {
        // Opens a file dialog and sets the browser source
        QString filePath = QFileDialog::getOpenFileName(this, tr("Import Tutorial"), "assets/tutorials", tr("Text Files(*.txt * .html * .markdown * .md"));
        if (filePath.isEmpty()) { return; }
        ui.tutorialBrowser->setSource(QUrl::fromLocalFile(filePath));

        // Copies the imported file and adds it to the list in the tutorials panel
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        QFile::copy(filePath, "assets/tutorials/" + fileName);
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

void ABC3D::tutorialRowClicked(const QModelIndex& index) {
    ui.tutorialStack->setCurrentIndex(0);
    QString file = tutorialFileModel->filePath(index);
    ui.tutorialBrowser->setSource(QUrl::fromLocalFile(file));
}

void ABC3D::helpRowClicked(const QModelIndex& index) {
    ui.helpStack->setCurrentIndex(0);
    QString file = helpFileModel->filePath(index);
    ui.helpBrowser->setSource(QUrl::fromLocalFile(file));
}

void ABC3D::elementChanged(Element element) {
    if (ui.bottomRightPanels->currentIndex() == 2) {
        switch (element) {
        case VERTEX:
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(false);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.flipButton->setEnabled(false);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            break;
        case EDGE:
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(false);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.flipButton->setEnabled(true);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            break;
        case FACE:
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(true);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            ui.flipButton->setEnabled(false);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            break;
        case OBJECT:
            ui.subdivisionButton->setEnabled(true);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            ui.triangulationButton->setEnabled(true);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            ui.flipButton->setEnabled(false);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            break;
        }
    }
}

ABC3D::~ABC3D()
{}
