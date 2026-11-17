#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QStandardItemModel>
#include <QTreeView>
#include <QCursor>
#include <QButtonGroup>
#include <QActionGroup>
#include <QSaveFile>
#include <QLineEdit>
#include <QWidgetAction>
#include <QToolBar>
#include <QCompleter>
#include <QtGamepadLegacy/QGamepad>
#include <QtGamepadLegacy/QGamepadManager>
#include "ui_ABC3D.h"
#include "StartMenu.h"

class ABC3D : public QMainWindow
{
    Q_OBJECT

public:
    ABC3D(QWidget *parent = nullptr);
    ~ABC3D();

    bool firstShow = true;

private:
    Ui::ABC3DClass ui;
    QByteArray projectFile;
    QFileSystemModel* tutorialFileModel = new QFileSystemModel;
    QFileSystemModel* helpFileModel = new QFileSystemModel;
    std::vector<QPushButton*> sceneButtons;
    QButtonGroup* sceneGroup = new QButtonGroup(this);
    QStandardItemModel* sceneModel;
    QModelIndex sceneModelIndex;

    QGamepadManager* controllerManager = QGamepadManager::instance();
  
    // Hotkeys
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void showEvent(QShowEvent* event) override;

    /* Creates a start menu and checks what integer is returned by the exec function */
    void showStart() {
        StartMenu startMenu(this);
        int execResult = startMenu.exec();
        if (execResult == 4) { modelFileDialog(); }
        else if (execResult == 5) { sceneFileDialog(); }
        else if (execResult == 6) { QDesktopServices::openUrl(QUrl("https://abc3d.me")); }
    }

private slots:
    void showPropertiesPanel() { ui.bottomRightPanels->setCurrentIndex(0); }
    void showCameraPanel() { ui.bottomRightPanels->setCurrentIndex(3); }

    // AI-generated code: Routes the rendering-mode actions to the viewport.
    void showStandardMode() { ui.viewport->setSimpleMode(false); }
    void showSimpleMode() { ui.viewport->setSimpleMode(true); }

    void updateSearchBar(QString text) {
        QModelIndex helpPath = helpFileModel->index("assets/help documents");
        for (int i = 0; i < helpFileModel->rowCount(helpPath); i++) {
            QModelIndex index = helpFileModel->index(i, 0, helpPath);
            if (helpFileModel->fileName(index).toUpper().startsWith(text.toUpper())) {
                helpRowClicked(index);
            }
        }
    }

    void imageFileDialog() {
        QString file = QFileDialog::getOpenFileName(
            this,
            "Import Image",
            "assets/images/",
            "Images (*.png *.jpg *.jpeg)"
        );

        const bool supportedImage =
            file.endsWith(".png", Qt::CaseInsensitive) ||
            file.endsWith(".jpg", Qt::CaseInsensitive) ||
            file.endsWith(".jpeg", Qt::CaseInsensitive);

        if (!file.isEmpty() && supportedImage) {
            ui.viewport->loadReferenceImage(file);
        }
    }

    void modelFileDialog() {
        QStringList files = QFileDialog::getOpenFileNames(
                                this,
                                "Select one or more models",
                                "assets/models/",
                                "3D Models (*.obj *.stl)"
                            );

        for (QString& file : files) {
            ui.viewport->loadFile(file.toUtf8().constData());
        }
    }

    void sceneFileDialog() {
        QString file = QFileDialog::getOpenFileName(
                            this,
                            "Select a project",
                            "assets/",
                            "ABC3D Projects (*.abc3d)"
                        );

        if (!file.isNull()) {
            projectFile = file.toUtf8();
            ui.viewport->openScene(projectFile.constData());
        }
    }

    void exportObj() {
        QString file = QFileDialog::getSaveFileName(
            this,
            "Export Obj",
            "assets/"
        );

        if (!file.isNull() && !file.isEmpty()) {
            ui.viewport->exportObj(file.toUtf8().constData());
        }
    }

    void exportStl() {
        QString file = QFileDialog::getSaveFileName(
            this,
            "Export Stl",
            "assets/"
        );

        if (!file.isNull() && !file.isEmpty()) {
            ui.viewport->exportStl(file.toUtf8().constData());
        }
    }

    void saveAsProject() {
        QString file = QFileDialog::getSaveFileName(
            this,
            "Save Project",
            "assets/"
        );

        if (!file.isNull() && !file.isEmpty()) {
            projectFile = file.toUtf8();
            ui.viewport->saveScene(projectFile.constData());
        }
    }

    void saveProject() {
        if (projectFile.isEmpty()) {
            saveAsProject();
            return;
        }

        ui.viewport->saveScene(projectFile.constData());
    }

    void tutorialRowClicked(const QModelIndex& index);
    void helpRowClicked(const QModelIndex& index);
    void elementChanged(Element element);
    void meshCountChanged(uint meshCount);
    void createSceneButton(uint index, QString name);
    void selectionChanged(int index);

signals:
    void deleteKeyPressed();
    void ctrlKeyPressed();
    void ctrlKeyReleased();
    void cameraKeyPressed(uint8_t axis);
};

