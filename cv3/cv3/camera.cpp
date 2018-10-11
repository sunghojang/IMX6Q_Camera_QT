#include "camera.h"
#include <mxcfb.h>
camera::camera(QObject *parent) : QObject(parent)
{

    //      dev_camera = camera_open("/dev/video0", 1920, 1080);
    //      camera_init(dev_camera);
    //      camera_start(dev_camera);

    //      struct timeval timeout;
    //      timeout.tv_sec = 1;
    //      timeout.tv_usec = 0;
    //      /* skip 5 frames for booting a cam */
    //      for (int i = 0; i < 5; i++) {
    //        camera_frame(dev_camera, timeout);
    //      }
    //      camera_frame(dev_camera, timeout);

    //      unsigned char* rgb =
    //        yuyv2rgb(dev_camera->head.start, dev_camera->width, dev_camera->height);
    //      FILE* out = fopen("result.jpg", "w");

    //      fclose(out);
    //      free(rgb);

    //      camera_stop(dev_camera);
    //      camera_finish(dev_camera);
    //      camera_close(dev_camera);

}
/*
 * capturing from UVC cam
 * requires: libjpeg-dev
 * build: gcc -std=c99 capture.c -ljpeg -o capture
 */




void camera::quit(const char * msg)
{
    fprintf(stderr, "[%s] %d: %s\n", msg, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int camera::xioctl(int fd, int request, void* arg)
{
    for (int i = 0; i < 100; i++) {
        int r = ioctl(fd, request, arg);
        if (r != -1 || errno != EINTR) return r;
    }
    return -1;
}

camera_t* camera::camera_open(const char * device, uint32_t width, uint32_t height)
{
    int fd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) quit("open");
    camera_t* camera =(camera_t*) malloc(sizeof (camera_t));
    camera->fd = fd;
    camera->width = width;
    camera->height = height;
    camera->buffer_count = 0;
    camera->buffers = NULL;
    camera->head.length = 0;
    camera->head.start = NULL;
    return camera;
}


void camera::camera_init(camera_t* camera) {
    struct v4l2_capability cap;
    if (xioctl(camera->fd, VIDIOC_QUERYCAP, &cap) == -1) quit("VIDIOC_QUERYCAP");
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) quit("no capture");
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) quit("no streaming");
#if 1 //Confirm Chip Ident from ioctl
    struct v4l2_dbg_chip_ident chip;
    if (ioctl(camera->fd, VIDIOC_DBG_G_CHIP_IDENT, &chip))
    {
        printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
        return ;
    }
    printf("sensor chip is %s\n", chip.match.name);
#endif
#if 1 //frmsizeenum Frame SIZE GET
    struct v4l2_frmsizeenum fsize;
    printf("sensor supported frame size:\n");
    fsize.index = 0;
    while (ioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0) {
        printf(" %dx%d\n", fsize.discrete.width,
               fsize.discrete.height);
        fsize.index++;
    }
#endif
#if 1 //Frame Format enum GET
    struct v4l2_fmtdesc ffmt;
    ffmt.index = 0;
    while (ioctl(camera->fd, VIDIOC_ENUM_FMT, &ffmt) >= 0) {
        print_pixelformat("sensor frame format", ffmt.pixelformat);
        ffmt.index++;
    }
#endif
#if 1  //set STREAM PARMETER
    struct v4l2_streamparm parm;
    ///////////////////////////////////////////////////////////////////////SET V4L2 STREAM PARAMETER SETTING & FRAME RATE ex)1 / 30 = 30fps
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 30;
    parm.parm.capture.capturemode = 1;//g_capture_mode
    if (ioctl(camera->fd, VIDIOC_S_PARM, &parm) < 0)
    {
        printf("VIDIOC_S_PARM failed\n");
        return ;
    }
    ///////////////////////////////////////////////////////////////////////GET V4L2 STREAM PARAMETER SETTING & FRAME RATE ex)1 / 30 = 30fps
    parm.parm.capture.timeperframe.numerator = 0;
    parm.parm.capture.timeperframe.denominator = 0;
    if (ioctl(camera->fd, VIDIOC_G_PARM, &parm) < 0)
    {
        printf("get frame rate failed\n");
        return ;
    }
    printf("frame_rate is %d\n readbuffers %d\n capability %d/%d\n",
           parm.parm.capture.timeperframe.denominator,
           parm.parm.capture.readbuffers,
           parm.parm.capture.capturemode,
           parm.parm.capture.capability);
#endif
#if 1 ///////////////////////////////////////////////////////////////////////SET V4L2 Crop left-top width-height cut
    struct v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(camera->fd, VIDIOC_CROPCAP, &cropcap) == 0) {
        struct v4l2_crop crop;
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c.left = 0 ;
        crop.c.top  = 0 ;
        crop.c.width = camera->width;
        crop.c.height = camera->height;
        qDebug("[[[[[[[[[[[[[[camera->width : %d, camera->height : %d",camera->width,camera->height);
        //crop.c = cropcap.defrect;
        if (xioctl(camera->fd, VIDIOC_S_CROP, &crop) == -1) {
            // cropping not supported
            qDebug("cropping not supported");
        }
    }
#endif
#if 1 ///////////////////////////////////////////////////////////////////////SET V4L2 FORMAT
    struct v4l2_format format;
    memset(&format, 0, sizeof format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = camera->width;
    format.fmt.pix.height = camera->height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    if (xioctl(camera->fd, VIDIOC_S_FMT, &format) == -1) quit("VIDIOC_S_FMT");
    if (xioctl(camera->fd, VIDIOC_G_FMT, &format) == -1) quit("VIDIOC_G_FMT");
#endif

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
#if 1 //CAPTURE
    if (xioctl(camera->fd, VIDIOC_REQBUFS, &req) == -1) quit("VIDIOC_REQBUFS");
    camera->buffer_count = req.count;
    camera->buffers =(buffer_t*) calloc(req.count, sizeof (buffer_t));

    size_t buf_max = 0;
    for (size_t i = 0; i < camera->buffer_count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(camera->fd, VIDIOC_QUERYBUF, &buf) == -1)
            quit("VIDIOC_QUERYBUF");
        if (buf.length > buf_max) buf_max = buf.length;
        camera->buffers[i].length = buf.length;
        camera->buffers[i].start =
                (uint8_t*)  mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                 camera->fd, buf.m.offset);
        if (camera->buffers[i].start == MAP_FAILED) quit("mmap");
    }
    camera->head.start =(uint8_t*) malloc(buf_max);
#endif
#if 0 //OVERLAP
    if (xioctl(camera->fd, VIDIOC_QUERYBUF, &req) == -1) quit("VIDIOC_REQBUFS");
    camera->buffer_count = req.count;
    camera->buffers =(buffer_t*) calloc(req.count, sizeof (buffer_t));

    size_t buf_max = 0;
    for (size_t i = 0; i < camera->buffer_count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(camera->fd, VIDIOC_QUERYBUF, &buf) == -1)
            quit("VIDIOC_QUERYBUF");
        if (buf.length > buf_max) buf_max = buf.length;
        camera->buffers[i].length = buf.length;
        camera->buffers[i].start =
                (uint8_t*)  mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 camera->fd, buf.m.offset);
        if (camera->buffers[i].start == MAP_FAILED) quit("mmap");
    }
    camera->head.start =(uint8_t*) malloc(buf_max);
#endif




#if 1 //FlASH FRAME
    struct v4l2_control ctl;
    for (int i = 0; i < 3 ; i++) {
        // flash a frame
        ctl.id = V4L2_CID_PRIVATE_BASE + 1;
        if (ioctl(camera->fd, VIDIOC_S_CTRL, &ctl) < 0)
        {
            printf("set ctl failed\n");
            return ;
        }
        qDebug("FLASH FRAME");
        sleep(1);
    }
#endif
#if 1//V4L2 FORMAT GET
    if (ioctl(camera->fd, VIDIOC_G_FMT, &format) < 0)
    {
        printf("get format failed\n");
        return ;
    }
    qDebug("fmt.fmt.pix.width [%d] ",format.fmt.pix.width);
    qDebug("fmt.fmt.pix.height [%d] ",format.fmt.pix.height);
    print_pixelformat("]]]]][[[[[sensor frame format", format.fmt.pix.pixelformat);
    qDebug("fmt.fmt.pix.bytesperline [%d] ",format.fmt.pix.bytesperline);
    qDebug("fmt.fmt.pix.sizeimage [%d] ",format.fmt.pix.sizeimage);
#endif
}
#if 1 //Frame Format enum GET
void camera::print_pixelformat(char *prefix, int val)
{
    printf("%s: %c%c%c%c\n", prefix ? prefix : "pixelformat",
           val & 0xff,
           (val >> 8) & 0xff,
           (val >> 16) & 0xff,
           (val >> 24) & 0xff);
}
#endif


void camera::camera_start(camera_t* camera)
{
    for (size_t i = 0; i < camera->buffer_count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1) quit("VIDIOC_QBUF");
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(camera->fd, VIDIOC_STREAMON, &type) == -1)
        quit("VIDIOC_STREAMON");
}

void camera::camera_stop(camera_t* camera)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(camera->fd, VIDIOC_STREAMOFF, &type) == -1)
        quit("VIDIOC_STREAMOFF");
}

void camera::camera_finish(camera_t* camera)
{
    for (size_t i = 0; i < camera->buffer_count; i++) {
        munmap(camera->buffers[i].start, camera->buffers[i].length);
    }
    free(camera->buffers);
    camera->buffer_count = 0;
    camera->buffers = NULL;
    free(camera->head.start);
    camera->head.length = 0;
    camera->head.start = NULL;
}

void camera::camera_close(camera_t* camera)
{
    if (close(camera->fd) == -1) quit("close");
    free(camera);
}


int camera::camera_capture(camera_t* camera)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(camera->fd, VIDIOC_DQBUF, &buf) == -1) return FALSE;
    memcpy(camera->head.start, camera->buffers[buf.index].start, buf.bytesused);
    camera->head.length = buf.bytesused;
    if (xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1) return FALSE;
    return TRUE;
}

int camera::camera_frame(camera_t* camera, struct timeval timeout) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(camera->fd, &fds);
    int r = select(camera->fd + 1, &fds, 0, 0, &timeout);
    if (r == -1) quit("select");
    if (r == 0) return FALSE;
    return camera_capture(camera);
}




int camera::minmax(int min, int v, int max)
{
    return (v < min) ? min : (max < v) ? max : v;
}

uint8_t* camera::yuyv2rgb(uint8_t* yuyv, uint32_t width, uint32_t height)
{
    uint8_t* rgb =(uint8_t*) calloc(width * height * 3, sizeof (uint8_t));
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j += 2) {
            size_t index = i * width + j;
            int y0 = yuyv[index * 2 + 0] << 8;
            int u = yuyv[index * 2 + 1] - 128;
            int y1 = yuyv[index * 2 + 2] << 8;
            int v = yuyv[index * 2 + 3] - 128;
            rgb[index * 3 + 0] = minmax(0, (y0 + 359 * v) >> 8, 255);
            rgb[index * 3 + 1] = minmax(0, (y0 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 2] = minmax(0, (y0 + 454 * u) >> 8, 255);
            rgb[index * 3 + 3] = minmax(0, (y1 + 359 * v) >> 8, 255);
            rgb[index * 3 + 4] = minmax(0, (y1 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 5] = minmax(0, (y1 + 454 * u) >> 8, 255);
        }
    }
    return rgb;
}





