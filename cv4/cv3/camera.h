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
#include <QThread>
#include <QQueue>
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
#include <sys/stat.h>
#include <linux/fb.h>
#include <malloc.h>
#include <signal.h>
#include <sys/select.h>		//fd_set struct
#include <bits/time.h> 		//timeval struct
#include <mxcfb.h>

#define TPASS 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define ipu_fourcc(a,b,c,d)\
    (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
#define IPU_PIX_FMT_RGB332  ipu_fourcc('R','G','B','1') /*!<  8  RGB-3-3-2     */
#define IPU_PIX_FMT_RGB555  ipu_fourcc('R','G','B','O') /*!< 16  RGB-5-5-5     */
#define IPU_PIX_FMT_RGB565  ipu_fourcc('R','G','B','P') /*!< 16  RGB-5-6-5     */
#define IPU_PIX_FMT_RGB666  ipu_fourcc('R','G','B','6') /*!< 18  RGB-6-6-6     */
#define IPU_PIX_FMT_BGR24   ipu_fourcc('B','G','R','3') /*!< 24  BGR-8-8-8     */
#define IPU_PIX_FMT_RGB24   ipu_fourcc('R','G','B','3') /*!< 24  RGB-8-8-8     */
#define IPU_PIX_FMT_BGR32   ipu_fourcc('B','G','R','4') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_BGRA32  ipu_fourcc('B','G','R','A') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_RGB32   ipu_fourcc('R','G','B','4') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_RGBA32  ipu_fourcc('R','G','B','A') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_ABGR32  ipu_fourcc('A','B','G','R') /*!< 32  ABGR-8-8-8-8  */


class camera : public QThread
{
    Q_OBJECT
private:
    explicit camera();
    static camera* m_instance;
    static void Destroy(){
        delete m_instance;
    }
public:
    static camera* getinstance(){
        if(!m_instance){
            static QMutex mutex;
            QMutexLocker locker(&mutex);
            if(!m_instance){
                m_instance = new camera;
                atexit(Destroy);
            }
        }
        return m_instance;
    }
private:
    void run();

public:
    //explicit camera(QObject *parent = 0);
    typedef struct{
        uint8_t   *start;
        size_t  length;

    }buffer_t;
    buffer_t* buffers;
    char v4l_device[100];
    char v4l_capture[100];
    int fd_v4l ;
    int g_sensor_width ;
    int g_sensor_height;
    int g_sensor_top;
    int g_sensor_left;
    int g_display_width;
    int g_display_height;
    int g_display_top ;
    int g_display_left;
    int g_rotate ;
    int g_timeout ;
    int g_display_lcd;
    int n_buffers ;
    int g_overlay ;
    int g_camera_color ;
    int g_camera_framerate ;
    int g_capture_mode ;
    int g_alpha_mode ;
    char *alpha_buf0 ;
    char *alpha_buf1 ;
    int alpha_fb_w , alpha_fb_h;
    unsigned long loc_alpha_phy_addr0;
    unsigned long loc_alpha_phy_addr1;
    int alpha_buf_size ;
    int ctrl_c_rev ;
    int g_fd_fb_fg ;

int sdj_cmd(int fd,fd_set fds1,int stv1,int utv2);
void print_pixelformat(char *prefix, int val);
void ctrl_c_handler(int signum, siginfo_t *info, void *myact);
void fill_alpha_buffer(char *alpha_buf, int left, int top,
                       int right, int bottom, char alpha_val);
int mxc_v4l_overlay_test(int timeout);
int process_cmdline(int argc, char **argv);
int mxc_v4l_overlay_setup(struct v4l2_format *fmt);
int minmax(int min, int v, int max);
uint8_t* yuyv2rgb(uint8_t* yuyv, uint32_t width, uint32_t height);
signals:

public slots:
};

#endif // CAMERA_H
