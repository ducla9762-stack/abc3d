
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

    // Menu button for standard mode, selected by default
    QAction* actionStandardMode = new QAction("Standard Mode", ui.menuBar);
    ui.menuBar->addAction(actionStandardMode);
    //QObject::connect(actionStandardMode, &QAction::triggered, this, &ABC3D::showStandardMode);

    // Menu button for simple mode
    QAction* actionSimpleMode = new QAction("Simple Mode", ui.menuBar);
    ui.menuBar->addAction(actionSimpleMode);
    //QObject::connect(actionSimpleMode, &QAction::triggered, this, &ABC3D::showSimpleMode);

    // Connections based on key-presses
    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, [=]() {
        ui.viewport->deleteSelection();
        //emit meshDeletedSignal();
        });
    QObject::connect(this, &ABC3D::ctrlKeyPressed, ui.viewport, &Viewport::setAdditiveSelection);
    QObject::connect(this, &ABC3D::ctrlKeyReleased, ui.viewport, &Viewport::setSingleSelection);

    QObject::connect(ui.knifeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionKnife);
    QObject::connect(ui.mergeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionMerge);

    // Pulls up subdivide panel
    QObject::connect(ui.subdivideToolButton, &QPushButton::released, this, [=]() { 
        ui.bottomRightPanels->setCurrentIndex(2);
        emit elementChanged(ui.viewport->getActiveElement());
        });

    // Pulls up shape panel
    QObject::connect(ui.shapeToolButton, &QPushButton::released, this, [=]() {
        ui.bottomRightPanels->setCurrentIndex(1);
        });

    // Pulls up merge panel
    QObject::connect(ui.mergeToolButton, &QPushButton::released, this, [=]() {
        ui.mergeModeButton->setChecked(true);
        ui.bottomRightPanels->setCurrentIndex(0);
        });

    // Emits signal for when to create shapes (with resolutions)
    QObject::connect(ui.torusButton_5, &QPushButton::released, this, [=]() {
            QDialog torusResolution(this, Qt::FramelessWindowHint);

            QGridLayout* torusLayout = new QGridLayout();
            QPushButton* create = new QPushButton("Create");
            QPushButton* cancel = new QPushButton("Cancel");
            QLabel* latitudeLabel = new QLabel("Set Latitude");
            QDoubleSpinBox* latitude = new QDoubleSpinBox();
            latitude->setMinimum(3);
            latitude->setValue(40);
            QLabel* longitudeLabel = new QLabel("Set Longitude");
            QDoubleSpinBox* longitude = new QDoubleSpinBox();
            longitude->setMinimum(3);
            longitude->setValue(20);
            QLabel* outsideRadiusLabel = new QLabel("Set Outer Radius");
            QDoubleSpinBox* outsideRadius = new QDoubleSpinBox();
            outsideRadius->setValue(1);
            QLabel* insideRadiosLabel = new QLabel("Set Inner Radius");
            QDoubleSpinBox* insideRadius = new QDoubleSpinBox();
            insideRadius->setValue(.3);

            connect(create, &QPushButton::released, &torusResolution, &QDialog::accept);
            connect(cancel, &QPushButton::released, &torusResolution, &QDialog::reject);

            torusLayout->addWidget(latitude, 0, 0);
            torusLayout->addWidget(latitudeLabel, 0, 1);
            torusLayout->addWidget(longitude, 1, 0);
            torusLayout->addWidget(longitudeLabel, 1, 1);
            torusLayout->addWidget(outsideRadius, 2, 0);
            torusLayout->addWidget(outsideRadiusLabel, 2, 1);
            torusLayout->addWidget(insideRadius, 3, 0);
            torusLayout->addWidget(insideRadiosLabel, 3, 1);
            torusLayout->addWidget(cancel, 4, 0);
            torusLayout->addWidget(create, 4, 1);

            torusResolution.setLayout(torusLayout);
            
            if (torusResolution.exec() == QDialog::Accepted) {
                ui.viewport->loadTorus(latitude->value(), longitude->value());
            }
        });

    QObject::connect(ui.cylinderButton_5, &QPushButton::released, this, [=]() {
        QDialog cylinderResolution(this, Qt::FramelessWindowHint);

        QGridLayout* cylinderLayout = new QGridLayout();
        QPushButton* create = new QPushButton("Create");
        QPushButton* cancel = new QPushButton("Cancel");
        QLabel* resolutionLabel = new QLabel("Set Resolution");
        QDoubleSpinBox* resolution = new QDoubleSpinBox();
        resolution->setMinimum(3);
        resolution->setValue(12);

        connect(create, &QPushButton::released, &cylinderResolution, &QDialog::accept);
        connect(cancel, &QPushButton::released, &cylinderResolution, &QDialog::reject);

        cylinderLayout->addWidget(resolution, 0, 0);
        cylinderLayout->addWidget(resolutionLabel, 0, 1);
        cylinderLayout->addWidget(cancel, 1, 0);
        cylinderLayout->addWidget(create, 1, 1);

        cylinderResolution.setLayout(cylinderLayout);

        if (cylinderResolution.exec() == QDialog::Accepted) {
            ui.viewport->loadCylinder(resolution->value());
        }
        });

    QObject::connect(ui.uvSphereButton_5, &QPushButton::released, this, [=]() {
        QDialog uvResolution(this, Qt::FramelessWindowHint);

        QGridLayout* uvLayout = new QGridLayout();
        QPushButton* create = new QPushButton("Create");
        QPushButton* cancel = new QPushButton("Cancel");
        QLabel* latitudeLabel = new QLabel("Set Latitude");
        QDoubleSpinBox* latitude = new QDoubleSpinBox();
        latitude->setMinimum(2);
        latitude->setValue(10);
        QDoubleSpinBox* longitude = new QDoubleSpinBox();
        QLabel* longitudeLabel = new QLabel("Set Longitude");
        longitude->setMinimum(3);
        longitude->setValue(10);


        connect(create, &QPushButton::released, &uvResolution, &QDialog::accept);
        connect(cancel, &QPushButton::released, &uvResolution, &QDialog::reject);

        uvLayout->addWidget(latitude, 0, 0);
        uvLayout->addWidget(latitudeLabel, 0, 1);
        uvLayout->addWidget(longitude, 1, 0);
        uvLayout->addWidget(longitudeLabel, 1, 1);
        uvLayout->addWidget(cancel, 2, 0);
        uvLayout->addWidget(create, 2, 1);

        uvResolution.setLayout(uvLayout);

        if (uvResolution.exec() == QDialog::Accepted) {
            ui.viewport->loadUvSphere(latitude->value(), longitude->value());
        }
        });

    // The main Move tool defaults to Free translation. Its toolbar button and
    // the translation-mode buttons live in separate exclusive groups, so keep
    // the visible mode selection synchronized when returning to Move.
    QObject::connect(ui.translateToolButton, &QPushButton::released, this, [=]() {
        ui.axisModeTranslationButton->setChecked(true);
        emit elementChanged(ui.viewport->getActiveElement());
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


    /* Creates & connects the right - click menus for all the tool buttons 
    * Each tool button has a help action in the menu that brings up their respective document*/
    QObject::connect(ui.selectToolButton, &QPushButton::customContextMenuRequested, ui.selectToolButton, [=]() {
        QMenu selectMenu(this);
        QAction* helpAction = selectMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Selection.html"));
        });
        selectMenu.exec(QCursor::pos());
    });

    QObject::connect(ui.cameraToolButton, &QPushButton::customContextMenuRequested, ui.cameraToolButton, [=]() {
        QMenu cameraMenu(this);
        QAction* helpAction = cameraMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Camera Movement.html"));
            });
        cameraMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.translateToolButton, &QPushButton::customContextMenuRequested, ui.translateToolButton, [=]() {
        QMenu translateMenu(this);
        QAction* helpAction = translateMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Translation.html"));
            });
        translateMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.scaleToolButton, &QPushButton::customContextMenuRequested, ui.scaleToolButton, [=]() {
        QMenu scaleMenu(this);
        QAction* helpAction = scaleMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Translation.html"));
            });
        scaleMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.rotateToolButton, &QPushButton::customContextMenuRequested, ui.rotateToolButton, [=]() {
        QMenu rotateMenu(this);
        QAction* helpAction = rotateMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Translation.html"));
            });
        rotateMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.shapeToolButton, &QPushButton::customContextMenuRequested, ui.shapeToolButton, [=]() {
        QMenu shapeMenu(this);
        QAction* helpAction = shapeMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Shapes.html"));
            });
        shapeMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.extrudeToolButton, &QPushButton::customContextMenuRequested, ui.extrudeToolButton, [=]() {
        QMenu extrudeMenu(this);
        QAction* helpAction = extrudeMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Extrusion.html"));
            });
        extrudeMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.subdivideToolButton, &QPushButton::customContextMenuRequested, ui.subdivideToolButton, [=]() {
        QMenu subdivideMenu(this);
        QAction* helpAction = subdivideMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Subdivision.html"));
            });
        subdivideMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.bevelToolButton, &QPushButton::customContextMenuRequested, ui.bevelToolButton, [=]() {
        QMenu bevelMenu(this);
        QAction* helpAction = bevelMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Beveling.html"));
            });
        bevelMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.knifeToolButton, &QPushButton::customContextMenuRequested, ui.knifeToolButton, [=]() {
        QMenu knifeMenu(this);
        QAction* helpAction = knifeMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Knife Tool.html"));
            });
        knifeMenu.exec(QCursor::pos());
        });

    QObject::connect(ui.mergeToolButton, &QPushButton::customContextMenuRequested, ui.mergeToolButton, [=]() {
        QMenu mergeMenu(this);
        QAction* helpAction = mergeMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Merging.html"));
            });
        mergeMenu.exec(QCursor::pos());
        });

    ui.viewport->SetFaceColor(ui.faceColorButton->GetColorF());
    ui.viewport->SetEdgeColor(ui.edgeColorButton->GetColorF());
    ui.viewport->SetVertexColor(ui.vertexColorButton->GetColorF());
    ui.viewport->SetBackgroundColor(ui.backgroundColorButton->GetColorF());
    ui.viewport->SetSelectColor(ui.selectColorButton->GetColorF());
    ui.viewport->SetHoverColor(ui.hoverColorButton->GetColorF());

    startingMeshes = ui.viewport->getMeshes();
    sceneButtons.resize(startingMeshes.size());
    for (int i = 0; i < startingMeshes.size(); i++) {
        QPushButton* sceneButton = new QPushButton(QString::number(i), this);
        sceneButtons[i] = sceneButton;
        ui.verticalLayout_10->addWidget(sceneButton);
    }
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
        //emit meshDeletedSignal();
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
    case Qt::Key_M:
        event->accept();
        ui.viewport->setActionMerge();
        ui.mergeModeButton->setChecked(true);
        ui.bottomRightPanels->setCurrentIndex(0);
        ui.mergeToolButton->setChecked(true);
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

void ABC3D::meshAdded(int meshID) {
    QPushButton* newButton = new QPushButton(QString::number(meshID), this);
    ui.verticalLayout_10->addWidget(newButton);
}

void ABC3D::meshRemoved(int meshID) {
    sceneButtons[meshID]->deleteLater();
}

void ABC3D::elementChanged(Element element) {
    if (ui.bottomRightPanels->currentIndex() == 2 || ui.bottomRightPanels->currentIndex() == 0) {
        switch (element) {
        case VERTEX:
            // Subdivision Panel
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(false);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.flipButton->setEnabled(false);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            break;
        case EDGE:
            // Subdivision Panel
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(false);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.flipButton->setEnabled(true);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            break;
        case FACE:
            // Subdivision panel
            ui.subdivisionButton->setEnabled(false);
            ui.subdivisionButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            ui.triangulationButton->setEnabled(true);
            ui.triangulationButton->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
            ui.flipButton->setEnabled(false);
            ui.flipButton->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
            break;
        case OBJECT:
            // Subdivision Panel
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
