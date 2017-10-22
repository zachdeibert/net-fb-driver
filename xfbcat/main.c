#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

Display *dis;
int screen;
Window win;
GC gc;
Colormap colors;
int fd;
struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
char *fb;

void paint() {
    XImage image = {
        .width = vinfo.xres,
        .height = vinfo.yres,
        .xoffset = vinfo.xoffset,
        .format = ZPixmap,
        .data = fb,
        .byte_order = LSBFirst,
        .bitmap_unit = 8,
        .bitmap_bit_order = LSBFirst,
        .bitmap_pad = 8,
        .depth = 24,
        .bytes_per_line = 0,
        .bits_per_pixel = vinfo.bits_per_pixel,
        .red_mask = 0,
        .green_mask = 0,
        .blue_mask = 0
    };
    XInitImage(&image);
    XPutImage(dis, win, gc, &image, 0, 0, 0, 0, image.width, image.height);
    XFlush(dis);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (*argv[argc - 1] == '/') {
            fd = open(argv[argc - 1], O_RDONLY);
        } else {
            char buffer[FILENAME_MAX + 1];
            sprintf(buffer, "/dev/fb%s", argv[argc - 1]);
            fd = open(buffer, O_RDONLY);
        }
    } else {
        fd = open("/dev/fb0", O_RDONLY);
    }
    if (fd < 0) {
        fputs("Unable to open framebuffer file.", stderr);
        return EXIT_FAILURE;
    }
    ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
    long screenSize = vinfo.yres_virtual * finfo.line_length;
    fb = mmap(0, screenSize, PROT_READ, MAP_SHARED, fd, (off_t) 0);
    if (fb < 0) {
        fputs("Unable to map framebuffer.", stderr);
        close(fd);
        return EXIT_FAILURE;
    }
    dis = XOpenDisplay((char *) 0);
    screen = DefaultScreen(dis);
    unsigned long black = BlackPixel(dis, screen);
    unsigned long white = WhitePixel(dis, screen);
    win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, vinfo.xres, vinfo.yres, 5, white, black);
    XSetStandardProperties(dis, win, "FrameBuffer Viewer", "", None, argv, argc, NULL);
    XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);
    gc = XCreateGC(dis, win, 0, 0);
    XSetBackground(dis, gc, white);
    XMapRaised(dis, win);
    XSync(dis, False);
    colors = DefaultColormap(dis, DefaultScreen(dis));
    XFlush(dis);
    XEvent event;
    while (1) {
        XNextEvent(dis, &event);
        if (event.type == Expose && event.xexpose.count == 0) {
            paint();
        }
    }
    XFreeGC(dis, gc);
    XDestroyWindow(dis, win);
    XCloseDisplay(dis);
    close(fd);
    return EXIT_SUCCESS;
}
