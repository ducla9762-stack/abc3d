#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
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

    // Hotkeys
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void showEvent(QShowEvent* event) override;

    void showStart() {
        StartMenu startMenu(this);
        startMenu.exec();
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

signals:
    void deleteKeyPressed();
    void ctrlKeyPressed();
    void ctrlKeyReleased();
};

