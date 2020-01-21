#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdlib.h>


struct fb_var_screeninfo vinfo;

int main(int argc, char *argv[])

{

    int fbfd, fbsize, i;

    unsigned char *fbbuf;



    /* Открываем видеопамять */

    if ((fbfd = open("/dev/fb0", O_RDWR)) < 0) {

        exit(1);

    }



    /* Получаем изменяемые параметры изображения */

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {

        printf("Bad vscreeninfo ioctl\n");

        exit(2);

    }



    /* Размер кадрового буфера =

        (разрешение по X * разрешение по Y * байты на пиксель) */

    fbsize = vinfo.xres*vinfo.yres*(vinfo.bits_per_pixel/8);



    /* Отображаем видеопамять */

    if ((fbbuf = mmap(0, fbsize, PROT_READ|PROT_WRITE,

                      MAP_SHARED, fbfd, 0)) == (void *) -1){

        exit(3);

    }



    /* Очищаем экран */

    for (i=0; i<fbsize; i++) {

        *(fbbuf+i) = 0x0;

    }



    munmap(fbbuf, fbsize);

    close(fbfd);

}