#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug("start!!");

    cam =new camera(this);
    //loadFromData
    mcamera = cam->camera_open("/dev/video0", 640, 480);
    cam->camera_init(mcamera);
    cam->camera_start(mcamera);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    /* skip 5 frames for booting a cam */
    for (int i = 0; i < 10; i++) {
        qDebug("ddd");
        cam->camera_frame(mcamera, timeout);
    }

//    unsigned char* rgb =cam->yuyv2rgb(mcamera->head.start, mcamera->width, mcamera->height);
//    QImage tmpImage(rgb, mcamera->width,mcamera->height, QImage::Format_RGB888 ); //image.format=RGB888
//    if (tmpImage.save("test.bmp"), "BMP"){
//        qDebug() << "image.bmp save the image";
//    }

//    QSize Overlabsizeimage2(800,450);
//    QImage m_qimgCamDspSrc2(Overlabsizeimage2, QImage::Format_ARGB32_Premultiplied);
//    m_qimgCamDspSrc2.load("test.bmp");
//    m_qimgCamDspSrc2.scaled(Overlabsizeimage2, Qt::IgnoreAspectRatio);
//    ui->toolButton->setIcon(QPixmap::fromImage(m_qimgCamDspSrc2));

}

MainWindow::~MainWindow()
{
    cam->camera_stop(mcamera);
    cam->camera_finish(mcamera);
    cam->camera_close(mcamera);
    delete ui;
}
void MainWindow::fb_setup(void)
{
    //        struct mxcfb_gbl_alpha alpha;
    //    int fd;

    //#ifdef BUILD_FOR_ANDROID
    //    fd = open("/dev/graphics/fb0",O_RDWR);
    //#else
    //    fd = open("/dev/fb0",O_RDWR);
    //#endif

    //        alpha.alpha = 0;
    //        alpha.enable = 1;
    //        if (ioctl(fd, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
    //        printf("set alpha %d failed for fb0\n",  alpha.alpha);
    //        }

    // close(fd);
}
void MainWindow::on_pushButton_clicked()
{





}

void MainWindow::on_pushButton_2_clicked()
{

}



void MainWindow::on_toolButton_clicked()
{
    cam->camera_frame(mcamera, timeout);
    unsigned char* rgb =cam->yuyv2rgb(mcamera->head.start, mcamera->width, mcamera->height);
    QImage *myImage = new QImage( rgb, mcamera->width, mcamera->height, QImage::Format_RGB888);
    ui->label->setPixmap(QPixmap::fromImage(*myImage));
    free(rgb);

    //    QTimer *timerDsptime;
    //    timerDsptime = new QTimer(this);
    //    QObject::connect(timerDsptime,SIGNAL(timeout()),this,SLOT(on_pushButton_clicked()),Qt::DirectConnection);
    //    timerDsptime->start(33);
}
void ______________mxc_fb_overlay_____________();
