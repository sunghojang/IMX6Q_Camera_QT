#include "camera.h"
camera *camera::m_instance = 0;
camera::camera()  : QThread()
{
    qDebug("/////////////////////////////////////////////////////////////////////// camera");

    sprintf(v4l_device,"/dev/video0");
    qDebug("/////////////////////////////////////////////////////////////////////// camera1-1");
    sprintf(v4l_capture,"/dev/mxc_v4l2");
    fd_v4l = 0;
    g_sensor_width = 640;
    g_sensor_height = 480;
    g_sensor_top = 0;
    g_sensor_left = 0;
    g_display_width = 240;
    g_display_height = 320;
    g_display_top = 0;
    g_display_left = 0;
    g_rotate = 0;
    g_timeout = 3600;
    g_display_lcd = 0;
    n_buffers =0;
    g_overlay = 0;
    g_camera_color = 0;
    g_camera_framerate = 30;
    g_capture_mode = 0;
    g_alpha_mode = 0;

    alpha_fb_w = 0, alpha_fb_h = 0;

    alpha_buf_size = 0;
    ctrl_c_rev = 0;
    g_fd_fb_fg = 0;

    sprintf(v4l_capture,"/dev/mxc_v4l2");
    fd_v4l = 0;


}

void camera::run(){
    qDebug("/////////////////////////////////////////////////////////////////////// camera2");
    struct v4l2_format fmt;
    struct v4l2_framebuffer fb_v4l2;
#ifdef BUILD_FOR_ANDROID
    char fb_device_0[100] = "/dev/graphics/fb0";
#else
    char fb_device_0[100] = "/dev/fb0";
#endif
    char *fb_device_fg;
    int fd_fb_0 = 0;
    struct fb_fix_screeninfo fb0_fix, fb_fg_fix;
    struct fb_var_screeninfo fb0_var, fb_fg_var;
    struct mxcfb_color_key color_key;
    struct mxcfb_gbl_alpha g_alpha;
    struct mxcfb_loc_alpha l_alpha;
    int ret = 0;
    struct sigaction act;
    struct v4l2_dbg_chip_ident chip;
    //sdjtei cmd try
#if 0
    fd_set fds1;
    struct timeval tv1;
    int fd;
    int select_ret;
    int cmd_len;
    char cmd[256];
#endif
    /* for ctrl-c */
//    sigemptyset(&act.sa_mask);
//    act.sa_flags = SA_SIGINFO;
//    act.sa_sigaction = ctrl_c_handler;
//    ///////////////////////////////////////////////////////////////////////sigact       DEFENCE CTRL-C
//    if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
//        printf("install sigal error\n");
//        return ;
//    }
    qDebug("/////////////////////////////////////////////////////////////////////// PARAMETER CHECK");
    /////////////////////////////////////////////////////////////////////// PARAMETER CHECK
    int argc = 12;
    char *temp[]= {"-iw", "2560", "-ih", "1920", "-ow", "800", "-oh", "600", "-r", "4", "-fr", "30"};
    qDebug("/////////////////////////////////////////////////////////////////////// PARAMETER CHECK1-1");
    if (process_cmdline(argc, temp) < 0) {
        return ;
    }
    qDebug("/////////////////////////////////////////////////////////////////////// dev/vidoe0  CAM OPEN");
    ///////////////////////////////////////////////////////////////////////dev/vidoe0  CAM OPEN
    if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
        printf("Unable to open %s\n", v4l_device);
        return ;
    }
    ///////////////////////////////////////////////////////////////////////iic after    GET CAM ADDR & NAME
    if (ioctl(fd_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
    {
        printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
        return ;
    }
    printf("sensor chip is %s\n", chip.match.name);

    ///////////////////////////////////////////////////////////////////////dev/fb0 open     FRAME BUFFER DEVICE OPEN
    if ((fd_fb_0 = open(fb_device_0, O_RDWR )) < 0)	{
        printf("Unable to open frame buffer 0\n");
        return ;
    }
    ///////////////////////////////////////////////////////////////////////FRAME BUFFER INFO GET
    if (ioctl(fd_fb_0, FBIOGET_VSCREENINFO, &fb0_var) < 0) {
        close(fd_fb_0);
        return ;
    }
    if (ioctl(fd_fb_0, FBIOGET_FSCREENINFO, &fb0_fix) < 0) {
        close(fd_fb_0);
        return ;
    }

    if (strcmp(fb0_fix.id, "DISP3 BG - DI1") == 0)
        g_display_lcd = 1;
    else if (strcmp(fb0_fix.id, "DISP3 BG") == 0)
        g_display_lcd = 0;
    else if (strcmp(fb0_fix.id, "DISP4 BG") == 0)
        g_display_lcd = 3;
    else if (strcmp(fb0_fix.id, "DISP4 BG - DI1") == 0)
        g_display_lcd = 4;
    ///////////////////////////////////////////////////////////////////////???
    if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_display_lcd) < 0)
    {
        printf("VIDIOC_S_OUTPUT failed\n");
        return ;
    }

    ///////////////////////////////////////////////////////////////////////V4L2 FOMAT SETTING
    fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    fmt.fmt.win.w.top=  g_display_top ;
    fmt.fmt.win.w.left= g_display_left;
    fmt.fmt.win.w.width=g_display_width;
    fmt.fmt.win.w.height=g_display_height;
    printf("sdjtei start mxc overlay setup\n");
    if (mxc_v4l_overlay_setup(&fmt) < 0) {
        printf("Setup overlay failed.\n");
        return ;
    }
    ///////////////////////////////////////////////////////////////////////fd_v4l fd_fb_0 = Camera Device, Frame buffer device Setting///////////////////////////////////////////////////////////////////////
    memset(&fb_v4l2, 0, sizeof(fb_v4l2));
    ///////////////////////////////////////////////////////////////////////android
#ifdef BUILD_FOR_ANDROID
    fb_device_fg = "/dev/graphics/fb1";
#else
    fb_device_fg = "/dev/fb1";
#endif
    ///////////////////////////////////////////////////////////////////////fb1 OPEN
    if ((g_fd_fb_fg = open(fb_device_fg, O_RDWR)) < 0) {
        printf("Unable to open frame buffer 1\n");
        close(fd_fb_0);
        return ;
    }

    if (ioctl(g_fd_fb_fg, FBIOGET_FSCREENINFO, &fb_fg_fix) < 0) {
        printf("Get fix of fb1 failed\n");
        close(g_fd_fb_fg);
        close(fd_fb_0);
        return ;
    }

    if (((strcmp(fb_fg_fix.id, "DISP3 FG") != 0) && g_display_lcd < 3)//not entered
            || ((strcmp(fb_fg_fix.id, "DISP4 FG") != 0) && g_display_lcd > 2)) {
        close(g_fd_fb_fg);
#ifdef BUILD_FOR_ANDROID
        fb_device_fg = "/dev/graphics/fb2";
#else
        fb_device_fg = "/dev/fb2";
#endif
        ///////////////////////////////////////////////////////////////////////dev/fb2 open
        printf("test point play frame start check0 \n");
        sleep(10);
        if ((g_fd_fb_fg = open(fb_device_fg, O_RDWR)) < 0) {
            printf("Unable to open frame buffer 2\n");
            close(fd_fb_0);
            return ;
        }

        if (ioctl(g_fd_fb_fg, FBIOGET_FSCREENINFO, &fb_fg_fix) < 0) {
            printf("Get fix of fb2 failed\n");
            close(g_fd_fb_fg);
            close(fd_fb_0);
            return ;
        }
        if (((strcmp(fb_fg_fix.id, "DISP3 FG") != 0) && g_display_lcd < 3)
                || ((strcmp(fb_fg_fix.id, "DISP4 FG") != 0) && g_display_lcd > 2)) {
            printf("Cannot find overlay frame buffer\n");
            close(g_fd_fb_fg);
            close(fd_fb_0);
            return ;
        }
    }
    ///////////////////////////////////////////////////////////////////end
    printf("test point play frame start check70 \n");

    if (!g_overlay) {
        printf("test point play frame start check900 \n");

        g_alpha.alpha = 255;
        g_alpha.enable = 1;
        ///////////////////////////////////////////////////////////////////////fb0 ALPHA SET
        if (ioctl(fd_fb_0, MXCFB_SET_GBL_ALPHA, &g_alpha) < 0) {
            printf("Set global alpha failed\n");
            close(fd_fb_0);
            close(g_fd_fb_fg);
            return ;
        }

        if (g_alpha_mode) {//not entered
            printf("test point play frame start check90 \n");
            l_alpha.enable = 1;
            l_alpha.alpha_phy_addr0 = 0;
            l_alpha.alpha_phy_addr1 = 0;
            if (ioctl(fd_fb_0, MXCFB_SET_LOC_ALPHA,
                      &l_alpha) < 0) {
                printf("Set local alpha failed\n");
                close(fd_fb_0);
                close(g_fd_fb_fg);
                return ;
            }
            loc_alpha_phy_addr0 =
                    (unsigned long)(l_alpha.alpha_phy_addr0);
            loc_alpha_phy_addr1 =
                    (unsigned long)(l_alpha.alpha_phy_addr1);

            alpha_buf_size = fb0_var.xres * fb0_var.yres;

            alpha_buf0 = (char *)mmap(0, alpha_buf_size,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd_fb_0,
                                      loc_alpha_phy_addr0);
            if ((int)alpha_buf0 == -1) {
                ///////////////////////////////////////////////////////////////////////alpha buffer 0
                printf("\nError: failed to map alpha buffer 0"
                       " to memory.\n");
                close(fd_fb_0);
                close(g_fd_fb_fg);										//
                return ;
            }
            alpha_buf1 = (char *)mmap(0, alpha_buf_size,
                                      PROT_READ | PROT_WRITE,						//
                                      MAP_SHARED, fd_fb_0,
                                      loc_alpha_phy_addr1);
            if ((int)alpha_buf1 == -1) {
                ///////////////////////////////////////////////////////////////////////alpha buffer 1
                printf("\nError: failed to map alpha buffer 1"
                       " to memory.\n");
                munmap((void *)alpha_buf0, alpha_buf_size);				//
                close(fd_fb_0);
                close(g_fd_fb_fg);										//
                return ;
            }
            printf("test point play frame start check110 \n");
            fb_fg_var.xres = fb0_var.xres;
            fb_fg_var.yres = fb0_var.yres;
            fb_fg_var.xres_virtual = fb0_var.xres;
            fb_fg_var.yres_virtual = fb0_var.yres*2;
            if (ioctl(g_fd_fb_fg, FBIOPUT_VSCREENINFO,
                      &fb_fg_var) < 0) {
                printf("Put overlay frame buffer var info failed\n");
                close(fd_fb_0);
                close(g_fd_fb_fg);
                return ;
            }

            if (ioctl(g_fd_fb_fg, FBIOBLANK, FB_BLANK_UNBLANK) < 0) {
                printf("Unblank overlay frame buffer failed\n");
                close(fd_fb_0);
                close(g_fd_fb_fg);
                return ;
            }
        }
        /////////////////////////////////////////////////////////////////end
        printf("test point play frame start check890 \n");
        /////////////////////////////////////////////////////////////////////// sturct v4l2_framebuffer setting << Frame Buffer device info
        fb_v4l2.fmt.width = fb0_var.xres;
        fb_v4l2.fmt.height = fb0_var.yres;
        printf("[[[[[[[[[ fb bits per pixel %d \n",fb0_var.bits_per_pixel);
        if (fb0_var.bits_per_pixel == 32) {
            fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR32;
            fb_v4l2.fmt.bytesperline = 4 * fb_v4l2.fmt.width;
        }
        else if (fb0_var.bits_per_pixel == 24) {
            fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR24;
            fb_v4l2.fmt.bytesperline = 3 * fb_v4l2.fmt.width;
        }
        else if (fb0_var.bits_per_pixel == 16) {
            fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_RGB565;
            fb_v4l2.fmt.bytesperline = 2 * fb_v4l2.fmt.width;
        }
        printf("[[[[[[[[[[[width: %d\n", fb_v4l2.fmt.width);

        fb_v4l2.flags = V4L2_FBUF_FLAG_PRIMARY;
        fb_v4l2.base = (void *) fb0_fix.smem_start +
                fb0_fix.line_length*fb0_var.yoffset;
        printf("[[[[[[[[fb base : %x \n",fb_v4l2.base);
    } else {//not entered
        printf("test point play frame start check80 \n");
        g_alpha.alpha = 0;
        g_alpha.enable = 1;
        if (ioctl(fd_fb_0, MXCFB_SET_GBL_ALPHA, &g_alpha) < 0) {
            printf("Set global alpha failed\n");
            close(fd_fb_0);
            close(g_fd_fb_fg);
            return ;
        }

        color_key.color_key = 0x00080808;
        color_key.enable = 0;
        if (ioctl(fd_fb_0, MXCFB_SET_CLR_KEY, &color_key) < 0) {
            printf("Set color key failed\n");
            close(fd_fb_0);
            close(g_fd_fb_fg);
            return ;
        }

        if (g_alpha_mode) {
            fb_fg_var.xres = g_display_width;
            fb_fg_var.yres = g_display_height;
            fb_fg_var.xres_virtual = g_display_width;
            fb_fg_var.yres_virtual = g_display_height*2;
            if (ioctl(g_fd_fb_fg, FBIOPUT_VSCREENINFO,
                      &fb_fg_var) < 0) {
                printf("Put var of overlay fb failed\n");
                close(g_fd_fb_fg);
                close(fd_fb_0);
                return ;
            }

            if (ioctl(g_fd_fb_fg, FBIOGET_VSCREENINFO,
                      &fb_fg_var) < 0) {
                printf("Get var of overlay fb failed\n");
                close(g_fd_fb_fg);
                close(fd_fb_0);
                return ;
            }

            l_alpha.enable = 1;
            l_alpha.alpha_phy_addr0 = 0;
            l_alpha.alpha_phy_addr1 = 0;
            if (ioctl(g_fd_fb_fg, MXCFB_SET_LOC_ALPHA,
                      &l_alpha) < 0) {
                printf("Set local alpha failed\n");
                close(g_fd_fb_fg);
                close(fd_fb_0);
                return ;
            }
            ///////////////////////////////////////////////////////////////////////alpha phy 0 , 1
            printf("test point play frame start check \n");
            sleep(10);
            loc_alpha_phy_addr0 =
                    (unsigned long)(l_alpha.alpha_phy_addr0);
            loc_alpha_phy_addr1 =
                    (unsigned long)(l_alpha.alpha_phy_addr1);

            alpha_buf_size = fb_fg_var.xres * fb_fg_var.yres;

            alpha_buf0 = (char *)mmap(0, alpha_buf_size,
                                      PROT_READ | PROT_WRITE,						//
                                      MAP_SHARED, g_fd_fb_fg,
                                      loc_alpha_phy_addr0);
            if ((int)alpha_buf0 == -1) {
                printf("\nError: failed to map alpha buffer 0"
                       " to memory.\n");
                close(g_fd_fb_fg);
                close(fd_fb_0);
                return ;
            }
            alpha_buf1 = (char *)mmap(0, alpha_buf_size,				//
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, g_fd_fb_fg,
                                      loc_alpha_phy_addr1);
            if ((int)alpha_buf1 == -1) {
                printf("\nError: failed to map alpha buffer 1"
                       " to memory.\n");
                munmap((void *)alpha_buf0, alpha_buf_size);
                close(g_fd_fb_fg);
                close(fd_fb_0);
                return ;
            }
        }
        ///////////////////////////////////////////////////////////////////////GET frame buffer
        printf("test point play frame start check1 \n");
        sleep(10);

        if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {			//
            printf("Get framebuffer failed\n");
            close(fd_fb_0);
            close(g_fd_fb_fg);
            return ;
        }
        fb_v4l2.flags = V4L2_FBUF_FLAG_OVERLAY;					//

    }
    //
    ///////////////////////////////////////////////////////////////////////end0
    /// ///////////////////////////////////////////////////////////////////////Frame buffer device Close??? fb0
    close(fd_fb_0);

    ///////////////////////////////////////////////////////////////////////Set buffer
    printf("test point play frame start check2 \n");
    sleep(1);
    //////////////////////////////////////////////////////////////////////////dev/video0 device SET << frame buffer sturct v4l2_framebuffer
    if (ioctl(fd_v4l, VIDIOC_S_FBUF, &fb_v4l2) < 0)
    {
        printf("set framebuffer failed\n");
        return ;
    }
    /// ///////////////////////////////////////////////////////////////////////dev/video0 device GET << frame buffer sturct v4l2_framebuffer
    if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
        printf("set framebuffer failed\n");
        return ;
    }
    printf("get frame test point\n");
    sleep(10);


    printf("\n frame buffer width %d, height %d, bytesperline %d, sizeimage %d\n",
           fb_v4l2.fmt.width, fb_v4l2.fmt.height, fb_v4l2.fmt.bytesperline ,fb_v4l2.fmt.sizeimage);
    //test printf
    //printf("test point 1 \n");



    ///////////////////////////////////////////////////////////////////////My add source for cmd input

    //sdjtei
    //printf("testpoint 1\n");
    ret = mxc_v4l_overlay_test(g_timeout);
    //printf("testpoint 3\n");
    //printf("close fd_v4l\n");
    ///////////////////////////////////////////////////////////////////////dev/video0 close
    close(fd_v4l);
}

/*
 * capturing from UVC cam
 * requires: libjpeg-dev
 * build: gcc -std=c99 capture.c -ljpeg -o capture
 */

void camera::print_pixelformat(char *prefix, int val)
{
    printf("%s: %c%c%c%c\n", prefix ? prefix : "pixelformat",
           val & 0xff,
           (val >> 8) & 0xff,
           (val >> 16) & 0xff,
           (val >> 24) & 0xff);
}
void camera::ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
    ctrl_c_rev = 1;
    return;
}
/* fill in alpha value to a part of alpha buffer */
/////////////////////////////////////////////////////////alpha_buf indirect init all buf 0x80 = alpha_val 1000 0000b
void camera::fill_alpha_buffer(char *alpha_buf, int left, int top,
                       int right, int bottom, char alpha_val)
{
    printf("fill_alpha_buffer fill_alpha_buffer fill_alpha_buffer fill_alpha_buffer");
    char *pPointAlphaValue;
    int x, y;

    for (y = top; y < bottom; y++) {
        for (x = left; x < right; x++) {
            pPointAlphaValue = (char *)(alpha_buf +
                                        alpha_fb_w * y + x);
            *pPointAlphaValue = alpha_val;
        }
    }
}
int camera::mxc_v4l_overlay_test(int timeout)
{
    int i;
    int overlay = 1;
    int retval = 0;
    struct v4l2_control ctl;
#ifdef BUILD_FOR_ANDROID
    char fb_device_0[100] = "/dev/graphics/fb0";
#else
    char fb_device_0[100] = "/dev/fb0";
#endif
    int fd_graphic_fb = 0;
    struct fb_var_screeninfo fb0_var;
    struct mxcfb_loc_alpha l_alpha;

    //sdjtei cmd try
    fd_set fds1;
    int cmd_return=1;

    printf("mxc v4l2 overlay test func start!!\n");

    //////////////////////////////////////////////////////////////////////////dev/video0 OVERLAY SET
    if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
        printf("VIDIOC_OVERLAY start failed\n");
        retval = -1;
        goto out1;
    }

    printf("g_alpha_mode = %d \n",g_alpha_mode);
    if (g_alpha_mode) {//not entered
        if (g_overlay) {
            fd_graphic_fb = g_fd_fb_fg;

            alpha_fb_w = g_display_width;
            alpha_fb_h = g_display_height;
        } else {
            //////////////////////////////////////////////////////////////////////////dev/fb0 REOPEN
            if ((fd_graphic_fb = open(fb_device_0, O_RDWR)) < 0) {
                printf("Unable to open frame buffer 0\n");
                retval = -1;
                goto out2;
            }
            //////////////////////////////////////////////////////////////////////////dev/fb0 GET INFO
            if (ioctl(fd_graphic_fb, FBIOGET_VSCREENINFO, &fb0_var) < 0) {
                printf("Get frame buffer0 var info failed\n");

                retval = -1;
                goto out2;
            }

            alpha_fb_w = fb0_var.xres;
            alpha_fb_h = fb0_var.yres;
        }
        ////////////////////////////////////////////////////////////////////////// alpha_buf0 alpha_buf1 init 0x80
        /* The window shows graphics and video planes. */
        fill_alpha_buffer(alpha_buf0, 0, 0,
                          alpha_fb_w, alpha_fb_h, 0x80);
        fill_alpha_buffer(alpha_buf1, 0, 0,
                          alpha_fb_w, alpha_fb_h, 0x80);
        ////////////////////////////////////////////////////////////////////////// dev/fb0 device ...loc_alpha_phy_addr0? what???
        if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr0) < 0) {
            printf("Set local alpha buf failed\n");
            retval = -1;
            goto out2;
        }
        sleep(5);
        if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr1) < 0) {
            printf("Set local alpha buf failed\n");
            retval = -1;
            goto out2;
        }

        ////////////////////////////////////////////////////////////////////////// while loop before CTRL-C input
        printf("testpoint 6\n");
        printf("]]]]]]]]]]]]]]]ctrl_c_rev = %d\n ",ctrl_c_rev);
        while (ctrl_c_rev == 0) {
            /*
                         * The middle quarter window shows
                         * video plane only.
                         */
            printf("[[[[[[[[[[[[[[[[[[[[[[[ctrl_c_rev = %d\n ",ctrl_c_rev);


            ////////////////////////////////////////////////////////////////////////// alpha_buf0 alpha_buf1 init 0x00
            fill_alpha_buffer(alpha_buf0,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0x00);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr0) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }

            /*
                         * The middle quarter window shows
                         * graphics plane only.
                         */
            fill_alpha_buffer(alpha_buf1,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0xFF);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr1) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }

            /*
                         * The middle quarter window shows
                         * graphics and video planes.
                         */
            fill_alpha_buffer(alpha_buf0,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0x80);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr0) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }

            /*
                         * The middle quarter window shows
                         * video plane only.
                         */
            fill_alpha_buffer(alpha_buf1,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0x00);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr1) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }

            /*
                         * The middle quarter window shows
                         * graphics plane only.
                         */
            fill_alpha_buffer(alpha_buf0,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0xFF);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr0) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }

            /*
                         * The middle quarter window shows
                         * graphics and video planes.
                         */
            fill_alpha_buffer(alpha_buf1,
                              alpha_fb_w/4,
                              alpha_fb_h/4,
                              3*alpha_fb_w/4,
                              3*alpha_fb_h/4,
                              0x80);
            sleep(5);

            if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALP_BUF, &loc_alpha_phy_addr1) < 0) {
                printf("Set local alpha buf failed\n");
                retval = -1;
                goto out2;
            }
            printf("testpoint 5\n");
        }
        printf("testpoint 4\n");
        goto out2;
    }
    ///////////////////////////////////////////////////end

    //////////////////////////////////////////////////////////////////////////flash a frame
    printf("preview start testpoint 8\n");
    for (i = 0; i < 3 ; i++) {
        // flash a frame
        ctl.id = V4L2_CID_PRIVATE_BASE + 1;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
        {
            printf("set ctl failed\n");
            retval = -1;
            goto out2;
        }
        sleep(1);
    }

    printf("g_camera_color:%d \n",g_camera_color);
    if (g_camera_color == 1) {//if (g_camera_color == 1) {
        ctl.id = V4L2_CID_BRIGHTNESS;
        for (i = 0; i < 0xff; i+=0x20) {
            ctl.value = i;
            printf("change the brightness %d\n", i);
            ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
            sleep(1);
        }
        printf("testpoint 21\n");
        //getchar();
    }
    else if (g_camera_color == 2) {
        ctl.id = V4L2_CID_SATURATION;
        for (i = 25; i < 150; i+= 25) {
            ctl.value = i;
            printf("change the color saturation %d\n", i);
            ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
            sleep(5);
        }
    }
    else if (g_camera_color == 3) {
        ctl.id = V4L2_CID_RED_BALANCE;
        for (i = 0; i < 0xff; i+=0x20) {
            ctl.value = i;
            printf("change the red balance %d\n", i);
            ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
            sleep(1);
        }
    }
    else if (g_camera_color == 4) {
        ctl.id = V4L2_CID_BLUE_BALANCE;
        for (i = 0; i < 0xff; i+=0x20) {
            ctl.value = i;
            printf("change the blue balance %d\n", i);
            ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
            sleep(1);
        }
    }
    else if (g_camera_color == 5) {
        ctl.id = V4L2_CID_BLACK_LEVEL;
        for (i = 0; i < 4; i++) {
            ctl.value = i;
            printf("change the black balance %d\n", i);
            ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
            sleep(5);
        }
    }
    //else {
    //sdjtei cmd try
    //orign sleep(timeout);
    printf("testpoint 13 \n");

    while(1){
        cmd_return = sdj_cmd(0,fds1,timeout,0);
        if(cmd_return ==0){
            //sleep(timeout);
            goto out2;
        }
        //printf("replay cmd \n");
        //sleep(timeout);
    }

    printf("testpoint 14 \n");
    //}
    printf("testpoint 12 \n");

out2:
    printf("testpoint 15 \n");
    overlay = 0;
    if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0)
    {
        printf("VIDIOC_OVERLAY stop failed\n");
        retval = -1;
        goto out1;
    }
out1:
    printf("testpoint 16 \n");
    if (g_alpha_mode) {
        munmap((void *)alpha_buf0, alpha_buf_size);
        munmap((void *)alpha_buf1, alpha_buf_size);

        /*
                 * Disable DP local alpha function, otherwise,
                 * the alpha channel will be enabled even if we
                 * use DP global alpha next time and this will
                 * cause display issue.
                 */
        l_alpha.enable = 0;
        l_alpha.alpha_phy_addr0 = 0;
        l_alpha.alpha_phy_addr1 = 0;
        if (ioctl(fd_graphic_fb, MXCFB_SET_LOC_ALPHA, &l_alpha) < 0) {
            printf("Set local alpha failed\n");
            retval = -1;
            goto out0;
        }
out0:
        printf("testpoint 17 \n");
        if (!g_overlay) {
            if (ioctl(g_fd_fb_fg, FBIOBLANK, FB_BLANK_POWERDOWN) < 0) {
                printf("Unblank overlay frame buffer failed\n");
                close(g_fd_fb_fg);
                return -1;
            }
            close(g_fd_fb_fg);
        }
    }

    //printf("testpoint 10 \n");
    //	sleep(20);
    close(fd_graphic_fb);
    //printf("testpoint 11 \n");
    return retval;
}
int
camera::mxc_v4l_overlay_setup(struct v4l2_format *fmt)
{
    struct v4l2_streamparm parm;
    v4l2_std_id id;
    struct v4l2_control ctl;
    struct v4l2_crop crop;
    struct v4l2_frmsizeenum fsize;
    struct v4l2_fmtdesc ffmt;

    ///////////////////////////////////////////////////////////////////////SHOW SUPPORTED RESOLUTION SIZE
    printf("sensor supported frame size:\n");
    fsize.index = 0;
    while (ioctl(fd_v4l, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0) {
        printf(" %dx%d\n", fsize.discrete.width,
               fsize.discrete.height);
        fsize.index++;
    }
    ///////////////////////////////////////////////////////////////////////SHOW SUPPORTED IMAGE RAW FORMAT
    ffmt.index = 0;
    while (ioctl(fd_v4l, VIDIOC_ENUM_FMT, &ffmt) >= 0) {
        print_pixelformat("sensor frame format", ffmt.pixelformat);
        ffmt.index++;
    }
    ///////////////////////////////////////////////////////////////////////SET V4L2 STREAM PARAMETER SETTING & FRAME RATE ex)1 / 30 = 30fps
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = g_camera_framerate;
    parm.parm.capture.capturemode = g_capture_mode;
    if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
    {
        printf("VIDIOC_S_PARM failed\n");
        return -1;
    }
    ///////////////////////////////////////////////////////////////////////GET V4L2 STREAM PARAMETER SETTING & FRAME RATE ex)1 / 30 = 30fps
    parm.parm.capture.timeperframe.numerator = 0;
    parm.parm.capture.timeperframe.denominator = 0;
    if (ioctl(fd_v4l, VIDIOC_G_PARM, &parm) < 0)
    {
        printf("get frame rate failed\n");
        return -1;
    }
    printf("frame_rate is %d\n readbuffers %d\n",
           parm.parm.capture.timeperframe.denominator,parm.parm.capture.readbuffers);

    ///////////////////////////////////////////////////////////////////////V4L2_CID_PRIVATE_BASE=0x08000000 reselved +2 is Rotate
    ctl.id = V4L2_CID_PRIVATE_BASE + 2;
    ctl.value = g_rotate;
    if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
    {
        printf("set control failed\n");
        return -1;
    }
    ///////////////////////////////////////////////////////////////////////V4L2 Crop left-top width-height cut
    crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    crop.c.left = g_sensor_left;
    crop.c.top = g_sensor_top;
    crop.c.width = g_sensor_width;
    crop.c.height = g_sensor_height;
    printf("[[[[[[[[[[[[[[[[g_sensor_width : %d , g_sensor_height : %d \n", g_sensor_width,g_sensor_height);
    if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
    {
        printf("set cropping failed\n");
        return -1;
    }

    if (ioctl(fd_v4l, VIDIOC_S_FMT, fmt) < 0)
    {
        printf("set format failed\n");
        return -1;
    }

    if (ioctl(fd_v4l, VIDIOC_G_FMT, fmt) < 0)
    {
        printf("get format failed\n");
        return -1;
    }

    if (ioctl(fd_v4l, VIDIOC_G_STD, &id) < 0)
    {
        printf("VIDIOC_G_STD failed\n");
        return -1;
    }

    return TPASS;
}

int camera::process_cmdline(int argc, char **argv)
{
    int i;
    for (i = 0; i < argc; i++) {

        if (strcmp(argv[i], "-iw") == 0) {
            g_sensor_width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-ih") == 0) {
            g_sensor_height = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-it") == 0) {
            g_sensor_top = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-il") == 0) {
            g_sensor_left = atoi(argv[++i]);
        }
        if (strcmp(argv[i], "-ow") == 0) {
            g_display_width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-oh") == 0) {
            g_display_height = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-ot") == 0) {
            g_display_top = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-ol") == 0) {
            g_display_left = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-r") == 0) {
            g_rotate = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0) {
            g_timeout = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-do") == 0) {
            g_display_lcd = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-di") == 0) {
            strcpy(v4l_device, argv[++i]);
        }
        else if (strcmp(argv[i], "-fg") == 0) {
            g_overlay = 1;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            g_camera_color = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-fr") == 0) {
            g_camera_framerate = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-m") == 0) {
            g_capture_mode = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-a") == 0) {
            g_alpha_mode = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-help") == 0) {
            printf("MXC Video4Linux overlay Device Test\n\n" \
                   " -iw <input width>\n -ih <input height>\n" \
                   " -it <input top>\n -il <input left>\n"	\
                   " -ow <display width>\n -oh <display height>\n" \
                   " -ot <display top>\n -ol <display left>\n"	\
                   " -r <rotate mode>\n -t <timeout>\n" \
                   " -do <output display> \n"	\
                   " -di <camera select, /dev/video0, /dev/video1> \n" \
                   " -v <camera color> 1-brightness 2-saturation"
                   " 3-red 4-blue 5-black balance\n"\
                   " -m <capture mode> 0-low resolution 1-high resolution\n" \
                   " -a <alpha mode> 0-global alpha blending 1-local alpha blending\n" \
                   " -fr <frame rate> 30fps by default\n" \
                   " -fg foreground mode when -fg specified,"
                   " otherwise go to frame buffer\n");
            return -1;
        }
    }

    printf("g_display_width = %d, g_display_height = %d\n", g_display_width, g_display_height);
    printf("g_display_top = %d, g_display_left = %d\n", g_display_top, g_display_left);

    if ((g_display_width == 0) || (g_display_height == 0)) {
        return -1;
    }
    return 0;
}
int camera::sdj_cmd(int fd,fd_set fds1,int stv1,int utv2){
    struct timeval tv1;
    struct v4l2_control ctl;
    int select_ret;
    int cmd_len=0;
    int fcp =0;
    int rcp =0;
    char cmd[256];
    int temp=5;
    printf("--please insert commands--\n");
    fd=0;
    tv1.tv_sec=stv1;
    tv1.tv_usec=utv2;
    FD_ZERO(&fds1);	//fds is fd_set struct. sd_set save all fds
    FD_SET(fd,&fds1);//fd is '0' , '0' is mean stdio file discriptor
    //fd is Set 1,
    select_ret = select(fd+1,&fds1,0,0,&tv1);
    //1parameter used fd number ex)stdio use -> in 1
    if(select_ret == -1){
        printf("select error\n");
    }
    else if(select_ret == 0){
        printf("select timeout\n");
    }
    else{
        temp=FD_ISSET(fd,&fds1);
        //check fd if insert any commands, then select func is set fd on 1
        //therefor FD_ISSET funs is SET 1
        //	printf("temp check : %d\n",temp);
        if(temp==1){
            //		 printf("check FD set is OK\n");
            memset(cmd,0,sizeof(cmd));
            cmd_len = read(fd,cmd,256);
            //read func just reads fd(sdtio) and store cmd[256]
            cmd[cmd_len-1]='\0';
        }
        else{
            //		printf("FD is not SET select_ret = %d\n",select_ret);
            FD_CLR(fd,&fds1);
            //	sleep(3600);
        }
    }
    //printf("check input cmd : <%s>\n",cmd);

    if(strcmp(cmd,"sdjtei")==0){
        printf("cmp check cmd : <%s>\n",cmd);
    }
    else if(strcmp(cmd,"exit")==0){
        printf("end camera app\n");
        return 0;
    }
    else if(strcmp(cmd,"delay")==0){
        printf("delay(sleep) 20 seconds\n");
        sleep(20);
    }
    else if(strcmp(cmd,"ftest")==0){
        printf("file read test\n");
        fcp=open(v4l_capture, O_RDWR, 0);
        if(fcp<0)
            printf("fcp open error\n");
        rcp = read(fcp,cmd,256);
        if(rcp<0)
            printf("rcp read error\n");
    }
    else if(strcmp(cmd,"i2cb")==0){
        printf("APP_i2c test burst mode\n");
        ctl.id = V4L2_CID_BRIGHTNESS;
        ctl.value = 1;
        ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
    }
    else{
        printf("! plase insert defined commands \nexample:\nexit,delay,i2cb\n");
        //sleep(3600);
    }
    return 1;
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


