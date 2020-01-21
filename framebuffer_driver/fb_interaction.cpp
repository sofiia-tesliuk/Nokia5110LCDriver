#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>


struct fb_var_screeninfo vinfo;

int main(int argc, char *argv[]) {

    char* fb_name = "/dev/fb_lcd5110";
    int fb_device_file_descriptor, fb_size, i;
    unsigned char *fb_buf;

    if ((fb_device_file_descriptor = open(fb_name, O_RDWR)) < 0) {
        std::cout << ("Failed to open " << fb_name var2) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (ioctl(fb_device_file_descriptor, FBIOGET_VSCREENINFO, &vinfo)) {
        std::cout << "Failed calling vscreeninfo ioctl." << std::endl;
        exit(EXIT_FAILURE);
    }

    fb_size = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);

    if ((fb_buf = mmap(0, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb_device_file_descriptor, 0)) == (void *) - 1){
        exit(EXIT_FAILURE);
    }

    for (i=0; i<fb_size; i++) {
        *(fb_buf+i) = 0x0;
    }
    msleep(10000);
    for (i=0; i<fb_size; i++) {
        *(fb_buf+i) = 0x1;
    }

    // clean-up
    munmap(fb_buf, fb_size);
    close(fb_device_file_descriptor);
    exit(EXIT_SUCCESS);
}