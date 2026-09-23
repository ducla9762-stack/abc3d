#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include "ui_ABC3D.h"

class ABC3D : public QMainWindow
{
    Q_OBJECT

public:
    ABC3D(QWidget *parent = nullptr);
    ~ABC3D();
private:
    Ui::ABC3DClass ui;
    QByteArray projectFile;
    // Hotkeys

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Control) {
            event->accept();
            emit ctrlKeyPressed();
            return;
        }
    }

    void keyReleaseEvent(QKeyEvent* event) override {
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
            event->modifiers() &Qt::KeyboardModifier::ControlModifier;
            //ui.viewport->
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

