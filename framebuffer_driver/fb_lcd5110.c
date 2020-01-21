/*
 * Framebuffer driver for Nokia lcd-5110 display connected.
 *
 * main resources:
 *  http://www.linux-fbdev.org/HOWTO/4.html official Linux tutorial
 *  https://github.com/spotify/linux/blob/master/drivers/video/fbmem.c example from the tutorial
 *  http://www.esys.ir/Files/Ref_Books/Linux/esys.ir_Embedded.Linux.System.Design.and.Development.pdf
 *  */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/fb.h>
#include <linux/init.h>

#define VIDEOMEMSIZE	(1*1024*1024)	/* 1 MB */

// ---- CONFIGURATIONS -----

static void *videomemory;
static u_long videomemorysize = VIDEOMEMSIZE;


static struct fb_var_screeninfo fb_lcd5110_var = {
        .xres           = 84,
        .yres           = 48,
        .xres_virtual   = 84,
        .yres_virtual   = 48,
        .width          = 50,
        .height         = 50,
        .bits_per_pixel = 1,
        .red            = { 0, 1, 0 },
        .green          = { 0, 1, 0 },
        .blue           = { 0, 1, 0 },
        .activate       = FB_ACTIVATE_NOW,
        .vmode          = FB_VMODE_NONINTERLACED
};


static struct fb_fix_screeninfo fb_lcd5110_fix = {
        .id          = "LCD5110",
        .type        = FB_TYPE_PACKED_PIXELS,
        .visual      = FB_VISUAL_MONO10,
        .accel       = FB_ACCEL_NONE,
        .line_length = 84
};

// -------------------------

static int fb_lcd5110_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int fb_lcd5110_set_par(struct fb_info *info);
static int fb_lcd5110_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
static int fb_lcd5110_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);



static u_long get_line_length(int xres_virtual, int bpp) {
    // from github.com/torvalds/linux/blob/master/drivers/video/fbdev/vfb.c#L98
    u_long length;

    length = xres_virtual * bpp;
    length = (length + 31) & ~31;
    length >>= 3;
    return (length);
}


static int fb_lcd5110_set_par(struct fb_info *info) {
    info->fix.visual = FB_VISUAL_MONO01;
    info->fix.line_length = get_line_length(info->var.xres_virtual,
                                            info->var.bits_per_pixel);
    return 0;
}

static struct fb_ops fb_lcd5110_ops = {};

//        .fb_read        = fb_sys_read,
//        .fb_write       = fb_sys_write,
//        .fb_fillrect	  = sys_fillrect,
//        .fb_copyarea	  = sys_copyarea,
//        .fb_imageblit	  = sys_imageblit,
//        .fb_check_var	  = fb_lcd5110_check_var,
//        .fb_set_par	  = fb_lcd5110_set_par,
//        .fb_setcolreg	  = fb_lcd5110_setcolreg,
//        .fb_pan_display = fb_lcd5110_pan_display,


// ----- DRIVER CODE -------
// github.com/torvalds/linux/blob/master/drivers/video/fbdev/vfb.c

static int fb_lcd5110_probe(struct platform_device *dev) {
    struct fb_info *info;
    unsigned int size = PAGE_ALIGN(videomemorysize);
    int retval = -ENOMEM;

    if (!(videomemory = vmalloc_32_user(size)))
        return retval;

    info = framebuffer_alloc(sizeof(u32) * 256, &dev->dev);
    if (!info)
        goto err;

    info->screen_base = (char __iomem *)videomemory;
    info->fbops = &fb_lcd5110_ops;

    fb_lcd5110_fix.smem_start = (unsigned long) videomemory;
    fb_lcd5110_fix.smem_len = videomemorysize;
    info->fix = fb_lcd5110_fix;
    info->pseudo_palette = info->par;
    info->par = NULL;
    info->flags = FBINFO_FLAG_DEFAULT;

    retval = fb_alloc_cmap(&info->cmap, 256, 0);
    if (retval < 0)
        goto err1;

    retval = register_framebuffer(info);
    if (retval < 0)
        goto err2;
    platform_set_drvdata(dev, info);

    fb_lcd5110_set_par(info);

    fb_info(info, "Virtual frame buffer device, using %ldK of video memory\n",
            videomemorysize >> 10);
    return 0;
    err2:
    fb_dealloc_cmap(&info->cmap);
    err1:
    framebuffer_release(info);
    err:
    vfree(videomemory);
    return retval;
}

static int fb_lcd5110_remove(struct platform_device *dev)
{
    struct fb_info *info = platform_get_drvdata(dev);

    if (info) {
        unregister_framebuffer(info);
        vfree(videomemory);
        fb_dealloc_cmap(&info->cmap);
        framebuffer_release(info);
    }
    return 0;
}


// Initialize driver structure.
static struct platform_driver fb_lcd5110_driver = {
        .probe	= fb_lcd5110_probe,
        .remove = fb_lcd5110_remove,
        .driver = {
                .name	= "fb_lcd5110",
        },
};

// Initialize device structure for the driver above.
static struct platform_device *fb_lcd5110_device;

// -------------------------


static int __init fb_lcd5110_init(void) {
    int ret = 0;
    ret = platform_driver_register(&fb_lcd5110_driver);
    if (!ret) {
        fb_lcd5110_device = platform_device_alloc("fb_lcd5110", 0);
        if (fb_lcd5110_device)
            ret = platform_device_add(fb_lcd5110_device);
        else
            ret = -ENOMEM;
        if (ret) {
            platform_device_put(fb_lcd5110_device);
            platform_driver_unregister(&fb_lcd5110_driver);
        }
    }
    return ret;
}

module_init(fb_lcd5110_init);

static void __exit fb_lcd5110_exit(void)
{
    platform_device_unregister(fb_lcd5110_device);
    platform_driver_unregister(&fb_lcd5110_driver);
}
module_exit(fb_lcd5110_exit);

MODULE_LICENSE("GPL");