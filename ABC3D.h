#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QTreeView>
#include <QSaveFile>
#include "ui_ABC3D.h"
#include "StartMenu.h"

class ABC3D : public QMainWindow
{
    Q_OBJECT

public:
    ABC3D(QWidget *parent = nullptr);
    ~ABC3D();

    bool firstShow = true;

    QFileSystemModel* tutorialFileModel = new QFileSystemModel;
    QFileSystemModel* helpFileModel = new QFileSystemModel;

private:
    Ui::ABC3DClass ui;
    QByteArray projectFile;

    // Hotkeys
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void showEvent(QShowEvent* event) override;

    /* Creates a start menu and checks what integer is returned by the exec function
    */
    void showStart() {
        StartMenu startMenu(this);
        int execResult = startMenu.exec();
        if (execResult == 4) { modelFileDialog(); }
        else if (execResult == 5) { sceneFileDialog(); }
        //else if (execResult = 6) {} -- add once we have a website
    }

private slots:
    void showPropertiesPanel() { ui.bottomRightPanels->setCurrentIndex(0); }
    void showCameraPanel() { ui.bottomRightPanels->setCurrentIndex(3); }

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

signals:
    void deleteKeyPressed();
    void ctrlKeyPressed();
    void ctrlKeyReleased();
};

