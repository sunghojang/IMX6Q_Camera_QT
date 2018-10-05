#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QImage>
#include <QBitmap>
#include <QBuffer>
#include <QDebug>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QImage>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
typedef struct {
  uint8_t* start;
  size_t length;
} buffer_t;

typedef struct {
  int fd;
  uint32_t width;
  uint32_t height;
  size_t buffer_count;
  buffer_t* buffers;
  buffer_t head;
} camera_t;
class camera : public QObject
{
    Q_OBJECT
public:
    explicit camera(QObject *parent = 0);

    int xioctl(int fd, int request, void* arg);
    void quit(const char * msg);
    camera_t* camera_open(const char * device, uint32_t width, uint32_t height);
    void camera_init(camera_t* camera);
    void camera_start(camera_t* camera);
    void camera_stop(camera_t* camera);
    void camera_finish(camera_t* camera);
    void camera_close(camera_t* camera);
    int camera_capture(camera_t* camera);
    int camera_frame(camera_t* camera, struct timeval timeout);
    int minmax(int min, int v, int max);
    uint8_t* yuyv2rgb(uint8_t* yuyv, uint32_t width, uint32_t height);
signals:

public slots:
};

#endif // CAMERA_H
