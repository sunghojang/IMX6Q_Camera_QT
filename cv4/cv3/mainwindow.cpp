#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/background_segm.hpp>

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug("start!!");
    camera::getinstance()->start();

}

MainWindow::~MainWindow()
{

    delete ui;
}
void MainWindow::fb_setup(void)
{

}
void MainWindow::on_pushButton_clicked()
{





}

void MainWindow::on_pushButton_2_clicked()
{

}



void MainWindow::on_toolButton_clicked()
{


//    QTimer *timerDsptime;
//    timerDsptime = new QTimer(this);
//    QObject::connect(timerDsptime,SIGNAL(timeout()),this,SLOT(slot_showQlabelFrame()),Qt::DirectConnection);
//    timerDsptime->start(33);
}
void MainWindow::slot_showQlabelFrame(){
#if 0 //OpenCV Color -> GrayScale -> Binary low high -> Hough Circle
    cam->camera_frame(mcamera, timeout);
    unsigned char* rgb =cam->yuyv2rgb(mcamera->head.start, 2560, 1920);


    Mat src(Size(2560,1920),CV_8UC3,rgb);
    Mat src_gray;
    Mat dist_range;
    cvtColor(src,src_gray,CV_RGB2GRAY);
    Scalar lowerb(100,100,100);
    Scalar upperb(200,200,200);
    inRange(src_gray,lowerb, upperb, dist_range);
    vector<Vec3f> circles;
    HoughCircles(dist_range,circles,CV_HOUGH_GRADIENT,2,dist_range.rows/4,200,100);
    for( size_t i = 0; i < circles.size(); i++ )
    {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        // circle center
        circle( dist_range, center, 1, Scalar(0,255,0),-1,8,0);
        // circle outline
        int radius = c[2];
        circle( dist_range, center, radius, Scalar(0,0,255),3,8,0);
    }
    QImage *myImage = new QImage( dist_range.ptr<uchar>(0,0), 2560, 1920, QImage::Format_Indexed8);
    ui->label->setPixmap(QPixmap::fromImage(*myImage));
    QImage *myImage2 = new QImage( src.ptr<uchar>(0,0), 2560, 1920, QImage::Format_RGB888);
    ui->label_2->setPixmap(QPixmap::fromImage(*myImage2));

    free(rgb);
#endif


}































