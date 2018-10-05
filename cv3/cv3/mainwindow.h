#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QThread>
#include <QToolButton>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QToolButton>
#include "camera.h"

#define TFAIL -1
#define TPASS 0
#define MAX_BUFFER_NUM 16


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public:
    camera *cam;
    camera_t* mcamera;
    struct timeval timeout;
public:
    void fb_setup(void);
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_toolButton_clicked();

private:
    Ui::MainWindow *ui;
};

class MyThread1 : public QThread {
protected:
    virtual void run ();
};





#endif // MAINWINDOW_H
