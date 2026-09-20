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

    void keyReleaseEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Delete) {
            event->accept();
            emit deleteKeyPressed();
            return;
        }
        // Close the program is escape is clicked
        else if (event->key() == Qt::Key_Escape) {
            event->accept();
            this->close();
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
            ui.viewport->openScene(file.toUtf8().constData());
        }
    }

signals:
    void deleteKeyPressed();
};

