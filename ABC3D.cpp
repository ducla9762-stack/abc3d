
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
    ui.topRightPanels->setCurrentIndex(2);
    
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

    QObject::connect(
        ui.actionImage,
        &QAction::triggered,
        this,
        &ABC3D::imageFileDialog
    );

    QObject::connect(ui.shaderComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        ui.viewport, &Viewport::SetMeshShader);

    // AI-generated code: Adds mutually exclusive UI actions for standard and simple rendering modes.
    // Menu button for standard mode, selected by default
    QAction* actionStandardMode = new QAction("Standard Mode", ui.menuBar);
    actionStandardMode->setCheckable(true);
    actionStandardMode->setChecked(true);
    ui.menuBar->addAction(actionStandardMode);
    QObject::connect(actionStandardMode, &QAction::triggered, this, &ABC3D::showStandardMode);

    // Menu button for simple mode
    QAction* actionSimpleMode = new QAction("Simple Mode", ui.menuBar);
    actionSimpleMode->setCheckable(true);
    ui.menuBar->addAction(actionSimpleMode);
    QObject::connect(actionSimpleMode, &QAction::triggered, this, &ABC3D::showSimpleMode);
    QActionGroup* rendererModes = new QActionGroup(this);
    rendererModes->setExclusive(true);
    rendererModes->addAction(actionStandardMode);
    rendererModes->addAction(actionSimpleMode);
    // AI-generated code: Reflects rejected or automatic viewport mode changes in the menu actions.
    QObject::connect(ui.viewport, &Viewport::simpleModeChanged, this,
        [actionStandardMode, actionSimpleMode](bool simpleMode) {
            if (simpleMode) actionSimpleMode->setChecked(true);
            else actionStandardMode->setChecked(true);
        });
    // AI-generated code: Reflects Simple Mode's automatic orbit-camera switch in the camera controls.
    QObject::connect(ui.viewport, &Viewport::orbitCameraActivated, this, [this]() {
        ui.orbitButton->setChecked(true);
        });

    // Search Bar for documents
    QLineEdit* searchBar = new QLineEdit();    
    searchBar->setPlaceholderText("Search Help Documents");
    searchBar->setFixedHeight(30);
    searchBar->setContentsMargins(5, 0, 5, 0);
    ui.verticalLayout_13->addWidget(searchBar);

    QStringList searchList;
    searchList << "Beveling" << "Camera Movement" << "Controls" << "Elements" << "Extrusion" 
        << "Knife Tool" << "Loop Cutting" << "Merging" << "Refinement" << "Scene" 
        << "Selection" << "Shapes" << "Simple Mode" << "Translation";
    QCompleter* searchCompleter = new QCompleter(searchList, this);

    searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    searchCompleter->setFilterMode(Qt::MatchContains);
    searchBar->setCompleter(searchCompleter);

    QObject::connect(searchBar, &QLineEdit::editingFinished, this, [=]() {
        updateSearchBar(searchBar->text());
        });

    // Connections based on key-presses
    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, [=]() {
        ui.viewport->deleteSelection();
        });

    QObject::connect(this, &ABC3D::ctrlKeyPressed, ui.viewport, [=]() {
        ui.viewport->setAdditiveSelection();
        sceneGroup->setExclusive(false);
    });
    QObject::connect(this, &ABC3D::ctrlKeyReleased, ui.viewport, [=]() {
        ui.viewport->setSingleSelection();
        sceneGroup->setExclusive(true);
    });

    QObject::connect(ui.knifeToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionKnife);
    QObject::connect(ui.loopCutToolButton, &QPushButton::released, ui.viewport, &Viewport::setActionLoopCut);
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

    QObject::connect(ui.loopCutToolButton, &QPushButton::customContextMenuRequested, ui.loopCutToolButton, [=]() {
        QMenu loopCutMenu(this);
        QAction* helpAction = loopCutMenu.addAction("Help");
        QObject::connect(helpAction, &QAction::triggered, this, [=]() {
            ui.topRightPanels->setCurrentIndex(0);
            ui.helpStack->setCurrentIndex(0);
            ui.helpBrowser->setSource(QUrl("assets/help documents/Loop Cut.html"));
            });
        loopCutMenu.exec(QCursor::pos());
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

    ui.sceneTree->header()->hide();
    ui.sceneTree->setDragEnabled(true);
    ui.sceneTree->setAcceptDrops(true);
    ui.sceneTree->setDropIndicatorShown(true);
    ui.sceneTree->setDragDropMode(QTreeView::InternalMove);

    sceneModel = new QStandardItemModel(this);
    sceneModel->setColumnCount(1);
    sceneModel->setHorizontalHeaderLabels(QStringList());
    sceneModel->setVerticalHeaderLabels(QStringList());

    ui.sceneTree->setModel(sceneModel);
    sceneGroup->setExclusive(true);

    for (int i = 0; i < ui.viewport->getMeshCount(); i++) {
        createSceneButton(i, "Cube");
    }

    // Undo / Redo. Added to the window too so the shortcuts work wherever focus is.
    ui.actionUndo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    ui.actionRedo->setShortcuts({ QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), QKeySequence(Qt::CTRL | Qt::Key_Y) });
    addAction(ui.actionUndo);
    addAction(ui.actionRedo);
    QObject::connect(ui.actionUndo, &QAction::triggered, ui.viewport, &Viewport::undo);
    QObject::connect(ui.actionRedo, &QAction::triggered, ui.viewport, &Viewport::redo);
    QObject::connect(ui.viewport, &Viewport::historyChanged, this, &ABC3D::refreshUndoActions);
    QObject::connect(ui.viewport, &Viewport::displaySettingsChanged, this, &ABC3D::refreshDisplayWidgets);
    QObject::connect(ui.viewport, &Viewport::toolReset, this, &ABC3D::showSelectTool);

    // The startup color setup above is not a user edit.
    ui.viewport->clearHistory();

    // Wait for the main application to process so it registers the controller
    for (int i = 0; i < 10 && controllerManager->connectedGamepads().isEmpty(); i++) {
        QCoreApplication::processEvents();
    }

    devices = controllerManager->connectedGamepads();
    analogTimer->setInterval(16);
    dPadTimer->setInterval(16);
 
    // Create the first controller on start
    if (!devices.isEmpty()) {
        createGamepad(devices.first()); 
    }

    // AI-assisted
    // Handles gamepads connected after start 
    connect(controllerManager, &QGamepadManager::gamepadConnected, this, [this](int id) {
        if (!controller) {
            createGamepad(id);
        }
    });

    // Handles gamepads being disconnected
    connect(controllerManager, &QGamepadManager::gamepadDisconnected, this, [this](int id) {
        if (controller) {
            QObject::disconnect(controller, nullptr, this, nullptr);
            controller->deleteLater();
            controller = nullptr;

            analogTimer->stop();
            dPadTimer->stop();
            L2pressed = false;
            R2pressed = false;
        }
    });
}

void ABC3D::refreshUndoActions() {
    const bool canUndo = ui.viewport->canUndo();
    const bool canRedo = ui.viewport->canRedo();
    ui.actionUndo->setEnabled(canUndo);
    ui.actionRedo->setEnabled(canRedo);
    ui.actionUndo->setText(canUndo ? QString("Undo %1").arg(ui.viewport->undoLabel()) : QString("Undo"));
    ui.actionRedo->setText(canRedo ? QString("Redo %1").arg(ui.viewport->redoLabel()) : QString("Redo"));
}

void ABC3D::refreshDisplayWidgets() {
    // Show the viewport's settings without the widgets sending them back as new edits.
    const DisplaySettings& d = ui.viewport->getDisplaySettings();
    auto setColor = [](ColorButton* button, const Vec3f& clr) {
        const QSignalBlocker block(button);
        button->SetColor(QColor::fromRgbF(clr.x, clr.y, clr.z));
    };
    auto setChecked = [](QAbstractButton* box, bool checked) {
        const QSignalBlocker block(box);
        box->setChecked(checked);
    };
    auto setValue = [](QDoubleSpinBox* spinner, double value) {
        const QSignalBlocker block(spinner);
        spinner->setValue(value);
    };
	const QSignalBlocker shaderBlock(ui.shaderComboBox);

    setColor(ui.faceColorButton, d.faceColor);
    setColor(ui.edgeColorButton, d.edgeColor);
    setColor(ui.vertexColorButton, d.vertexColor);
    setColor(ui.backgroundColorButton, d.backgroundColor);
    setColor(ui.selectColorButton, d.selectColor);
    setColor(ui.hoverColorButton, d.hoverColor);
    setChecked(ui.checkBox, d.surfacesVisible);
    setChecked(ui.checkBox_2, d.edgesVisible);
    setChecked(ui.gridVisibilityBox, d.gridVisible);
    setChecked(ui.vertexVisibilityBox, d.verticesAlwaysVisible);
    setValue(ui.edgeRadiusSpinner, d.edgeRadius);
    setValue(ui.vertexScaleSpinner, d.vertexScale);
    ui.shaderComboBox->setCurrentIndex(static_cast<int>(d.meshShader));
}

void ABC3D::showSelectTool() {
    ui.bottomRightPanels->setCurrentIndex(0);
    ui.selectToolButton->setChecked(true);
}

void ABC3D::createGamepad(int id) {
    controller = new QGamepad(id, this);

    // Analog Sticks
    // updateAxis AI-assisted
    auto updateAxis = [this](double& axisMember, double rawValue) {
        axisMember = (std::abs(rawValue) < 0.15) ? 0.0 : rawValue;

        bool anyActive = (leftX != 0.0 || leftY != 0.0 || rightX != 0.0 || rightY != 0.0);
        if (anyActive) {
            if (!analogTimer->isActive()) {
                analogTimer->start();
            }
        }
        else {
            analogTimer->stop();
        }
    };

    connect(controller, &QGamepad::axisLeftXChanged, this, [=](double value) { updateAxis(leftX, value); });
    connect(controller, &QGamepad::axisLeftYChanged, this, [=](double value) { updateAxis(leftY, value); });
    connect(controller, &QGamepad::axisRightXChanged, this, [=](double value) { updateAxis(rightX, value); });
    connect(controller, &QGamepad::axisRightYChanged, this, [=](double value) { updateAxis(rightY, value); });

    // AI-assisted
    // Timer for holding down the analog sticks
    connect(analogTimer, &QTimer::timeout, this, [this]() {
        if (leftX != 0.0) {

        }

        if (leftY != 0.0) {
            if (ui.viewport->getCameraType() == ORBIT) {
                if (leftY > 0) { emit cameraKeyPressed(Axis::Z_NEG); }
                else { emit cameraKeyPressed(Axis::Z); }
            }
        }

        if (rightX != 0.0) {
            if (ui.viewport->getCameraType() == ORBIT) {
                if (rightX > 0) { emit cameraKeyPressed(Axis::X); }
                else { emit cameraKeyPressed(Axis::X_NEG); }
            }
            else {
                if (rightX > 0) { emit cameraKeyPressed(Axis::X_NEG); }
                else { emit cameraKeyPressed(Axis::X); }
            }
        }

        if (rightY != 0.0) {
            if (ui.viewport->getCameraType() == ORBIT) {
                if (rightY > 0) { emit cameraKeyPressed(Axis::Y); }
                else { emit cameraKeyPressed(Axis::Y_NEG); }
            }
            else {
                if (rightY > 0) { emit cameraKeyPressed(Axis::Z_NEG); }
                else { emit cameraKeyPressed(Axis::Z); }
            }
        }
    });

    // AI-assisted
    auto updateDPad = [this](bool& buttonState, bool pressed) {
        buttonState = pressed;

        bool anyPressed = (dPadUp || dPadDown || dPadLeft || dPadRight);
        if (anyPressed) {
            if (!dPadTimer->isActive()) {
                dPadTimer->start();
            }
        }
        else {
            dPadTimer->stop();
        }
    };

    connect(controller, &QGamepad::buttonUpChanged, this, [=](bool pressed) { updateDPad(dPadUp, pressed); });
    connect(controller, &QGamepad::buttonDownChanged, this, [=](bool pressed) { updateDPad(dPadDown, pressed); });
    connect(controller, &QGamepad::buttonLeftChanged, this, [=](bool pressed) { updateDPad(dPadLeft, pressed); });
    connect(controller, &QGamepad::buttonRightChanged, this, [=](bool pressed) { updateDPad(dPadRight, pressed); });

    // AI-assisted
    connect(dPadTimer, &QTimer::timeout, this, [this]() {
        int dx = 0;
        int dy = 0;
        const int step = 10;

        if (dPadUp)    dy -= step;
        if (dPadDown)  dy += step;
        if (dPadLeft)  dx -= step;
        if (dPadRight) dx += step;

        if (dx != 0 || dy != 0) {
            QCursor::setPos(QCursor::pos() + QPoint(dx, dy));
        }
    });


    // Controller Buttons
    // A button: selects the button, mesh, or whatever else it's hovering over
    connect(controller, &QGamepad::buttonAChanged, this, [this](bool pressed) {
        QWidget* target = QApplication::widgetAt(QCursor::pos());
        QPoint localPosition = target->mapFromGlobal(QCursor::pos());
        if (pressed) {
            QMouseEvent* pressEvent = new QMouseEvent(QEvent::MouseButtonPress, localPosition, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::postEvent(target, pressEvent);
        }

        else {
            QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, localPosition, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::postEvent(target, releaseEvent);
        }
    });

    // B button: deselects current selection
    connect(controller, &QGamepad::buttonBChanged, this, [this](bool pressed) {
        QWidget* target = QApplication::widgetAt(QCursor::pos());
        QPoint localPosition = target->mapFromGlobal(QCursor::pos());
        if (pressed) {
            QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonDblClick, localPosition, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            ui.viewport->controllerToMouseRelease(event);
            QApplication::postEvent(target, event);
        }
    });

    // X Button: triggers redo
    connect(controller, &QGamepad::buttonXChanged, this, [this](bool pressed) {
        if (pressed) {
            ui.actionUndo->trigger();
        }
    });

    // Y Button: triggers undo
    connect(controller, &QGamepad::buttonYChanged, this, [this](bool pressed) {
        if (pressed) {
            ui.actionRedo->trigger();
        }
    });

    // Shoulder bumpers and triggers
    // Moves the selected left-toolbar button backward one
    connect(controller, &QGamepad::buttonL1Changed, this, [this](bool pressed) {
        if (pressed) {
            for (int i = 0; i < ui.verticalLayout->count() - 1; ++i) {
                QPushButton* btn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(i)->widget());
                if (btn->isChecked()) {
                    // Selects the last button if the first was selected
                    if (i == 0) {
                        QPushButton* newBtn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(ui.verticalLayout->count() - 2)->widget());
                        newBtn->setChecked(true);
                        newBtn->released();
                    }
                    // Selects the previous button otherwise
                    else {
                        QPushButton* newBtn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(i - 1)->widget());
                        newBtn->setChecked(true);
                        newBtn->released();
                    }
                    return;
                }
            }
        }
    });

    // Moves the selected left-toolbar button forward one
    connect(controller, &QGamepad::buttonR1Changed, this, [this](bool pressed) {
        if (pressed) {
            for (int i = 0; i < ui.verticalLayout->count() - 1; i++) {
                QPushButton* btn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(i)->widget());
                if (btn->isChecked()) {
                    // Selects the first button if it reaches the end
                    if (i == ui.verticalLayout->count() - 2) {
                        QPushButton* newBtn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(0)->widget());
                        newBtn->setChecked(true);
                        newBtn->released();
                    }
                    // Selects the next button otherwise
                    else {
                        QPushButton* newBtn = qobject_cast<QPushButton*>(ui.verticalLayout->itemAt(i + 1)->widget());
                        newBtn->setChecked(true);
                        newBtn->released();
                    }
                    return;
                }
            }
        }
    });

    // Moves the selected element backward one
    connect(controller, &QGamepad::buttonL2Changed, this, [this](bool pressed) {
        if (pressed == L2pressed) { return; }
        L2pressed = pressed;

        if (pressed) { ui.viewport->cyclePrev(); }
        });


    // Moves the selected element forward one
    connect(controller, &QGamepad::buttonR2Changed, this, [this](bool pressed) {
        if (pressed == R2pressed) { return; }
        R2pressed = pressed;

        if (pressed) { ui.viewport->cycleNext(); }
    });

    // Maps to right click and pulls up the corresponding menu
    connect(controller, &QGamepad::buttonR3Changed, this, [this](bool pressed) {
        if (pressed) {
            QContextMenuEvent* event = new QContextMenuEvent(QContextMenuEvent::Mouse, QCursor::pos(), QCursor::pos(), Qt::NoModifier);
            ui.viewport->controllerToContextMenu(event);
        }
    });

    // Home, Select, & Start
    connect(controller, &QGamepad::buttonCenterChanged, this, [this](bool pressed) {
        if (pressed) { showStart(); }
    });

    connect(controller, &QGamepad::buttonStartChanged, this, [this](bool pressed) {
        if (pressed) { saveProject(); }
    });

    connect(controller, &QGamepad::buttonSelectChanged, this, [this](bool pressed) {
        if (pressed) { this->close(); }
    });
}

void ABC3D::createSceneButton(uint i, QString name) {
    sceneModel->insertRow(i);
    sceneModelIndex = sceneModel->index(i, 0);
    sceneModel->setData(sceneModelIndex, name);

    QPushButton* initButton = new QPushButton(QString::fromStdString(ui.viewport->getNames()[i]));
    initButton->setStyleSheet("QPushButton:checked { background-color: red; }");
    initButton->setCheckable(true);
    sceneGroup->addButton(initButton);
    sceneButtons.push_back(initButton);
    QObject::connect(sceneButtons.back(), &QPushButton::released, this, [this, i]() { 
        if (sceneGroup->exclusive()) {
            for (int i = 0; i < sceneButtons.size(); i++) {
                sceneButtons[i]->setChecked(false);
            }
            this->ui.viewport->setSelection(i);
        }

        else {
            this->ui.viewport->addSelection(i);
        }
        sceneButtons[i]->setChecked(true);
    });
    
    ui.sceneTree->setIndexWidget(sceneModelIndex, initButton);
}

void ABC3D::keyPressEvent(QKeyEvent* event) {
    bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    switch (event->key()) {
    case Qt::Key_Control:
        event->accept();
        emit ctrlKeyPressed();
        break;
    // WASD & Arrow keys
    case Qt::Key_W:
        event->accept();
        emit cameraKeyPressed(Axis::Z);
        break;
    case Qt::Key_S:
        event->accept();
        // Save - CTRL + S
        if (ctrl && shift) { saveAsProject(); }
        if (ctrl) { saveProject(); }

        emit cameraKeyPressed(Axis::Z_NEG);
        break;
    case Qt::Key_A:
        event->accept();
        emit cameraKeyPressed(Axis::X);
        break;
    // Duplicate - CTRL + D
    case Qt::Key_D:
        event->accept();
        if (ctrl) { 
            ui.viewport->duplicateSelection();
        }

        emit cameraKeyPressed(Axis::X_NEG);
        break;

    case Qt::Key_Up:
        event->accept();
        emit cameraKeyPressed(Axis::Z);
        break;
    case Qt::Key_Down:
        event->accept();
        emit cameraKeyPressed(Axis::Z_NEG);
        break;
    case Qt::Key_Left:
        event->accept();
        emit cameraKeyPressed(Axis::X);
        break;
    case Qt::Key_Right:
        event->accept();
        emit cameraKeyPressed(Axis::X_NEG);
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
        if (ui.viewport->loopCutActive()) {
            ui.viewport->setActionSelect();
            ui.bottomRightPanels->setCurrentIndex(0);
            ui.selectToolButton->setChecked(true);
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
    // Loop Cut
    case Qt::Key_L:
        event->accept();
        ui.viewport->setActionLoopCut();
        ui.loopCutToolButton->setChecked(true);
        break;
    // Loop cut: more or fewer parallel cuts
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (ui.viewport->loopCutActive()) {
            event->accept();
            ui.viewport->changeLoopCutCount(1);
        }
        // Scale if Loop Cut isn't active
        else {
            ui.viewport->setActionScale();
            ui.bottomRightPanels->setCurrentIndex(0);
            ui.scaleToolButton->setChecked(true);
        }
        break;
    case Qt::Key_Minus:
        if (ui.viewport->loopCutActive()) {
            event->accept();
            ui.viewport->changeLoopCutCount(-1);
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
        if (ctrl) { break; } // Ctrl+Z is Undo (handled by the Edit menu action)
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

void ABC3D::meshCountChanged(uint meshCount) {
    uint btnCount = uint(sceneButtons.size());

    // Removes scene buttons if the number of meshes is less than the number of buttons
    if (meshCount < btnCount) {
        // iterate btnCount - meshCount times
        for (uint i = meshCount; i < btnCount; ++i) {
            QPushButton* button = sceneButtons.back();
            sceneButtons.pop_back();
            sceneGroup->removeButton(button);
            button->deleteLater(); // will be removed from layout and de-allocated by qt safely
            sceneModel->removeRow(sceneModel->rowCount() - 1);
            selectionChanged(-1);
        }

        // You will need to update all names (ids shifted)
        for (uint i = 0; i < meshCount; ++i) {
            QString updatedName = QString::fromStdString(ui.viewport->getNames()[i]);
            sceneButtons[i]->setText(updatedName);
            sceneModel->setData(sceneModel->index(i, 0), updatedName);
        }
    }

    // Creates scene buttons if there's a mismatch between mesh and button count
    else if (meshCount != btnCount) {
        // Undoing a delete puts meshes back in the middle, so existing ids can change names too
        for (uint i = 0; i < btnCount; ++i) {
            QString updatedName = QString::fromStdString(ui.viewport->getNames()[i]);
            sceneButtons[i]->setText(updatedName);
            sceneModel->setData(sceneModel->index(i, 0), updatedName);
        }
        for (uint i = btnCount; i < meshCount; ++i) {
            //ui.viewport->setName(i, ui.viewport->getNames()[i]);
            createSceneButton(i, QString::fromStdString(ui.viewport->getNames()[i]));
        }
    }
}

/// <summary>
/// Slot for when the selection is changed in the viewport
/// Sent -1 as the index if the selection is cleared/nothing is selected
/// </summary>
/// <param name="index"> index of the button to select, corresponding to the mesh that was selected </param>
void ABC3D::selectionChanged(int index) {
    std::cout << " selection Changed" << std::endl;
    if (index == -1) {
        sceneGroup->setExclusive(false);
        for (int i = 0; i < sceneButtons.size(); i++) {
            sceneButtons[i]->setChecked(false);
        }
        sceneGroup->setExclusive(true);
    }
    else {
        sceneButtons[index]->setChecked(true);
    }
    return;
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
