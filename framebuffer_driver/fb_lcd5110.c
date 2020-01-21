/*
 * Framebuffer driver definition.
 * main resource:
 *  http://www.esys.ir/Files/Ref_Books/Linux/esys.ir_Embedded.Linux.System.Design.and.Development.pdf

=   Fill up driver operations structure struct fb_ops.
   Fill up frame buffer fixed info struct fb_fix_screeninfo.
-   Fill up driver information structure struct fb_info.
-   Initialize hardware registers and video memory area.
-   Allocate and initialize color map struct fb_cmap, if necessary.7
-   Register the fb_info structure with driver framework using register_framebuffer.

 *  */

#include <fb.h>

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

int fb_lcd5110_major = 29;

static struct fb_info fb_info;

static struct fb_ops fb_lcd5110_ops = {
        .owner		= THIS_MODULE,
        .fb_read = fb_sys_read
//        .fb_open	= ...,
//        .fb_write	= ...,
//        .fb_release	= ...,
//        .fb_pan_display	= ...,
//        .fb_fillrect	= ...,
//        .fb_copyarea	= ...,
//        .fb_imageblit	= ...,
//        .fb_cursor	= ...,
};


static int __init  fb_lcd5110_init(void);
static int __init fb_lcd5110_probe(struct platform_device *device);
static void __exit fb_lcd5110_exit(void);


/* Config for GPU. */
static struct fb_var_screeninfo fb_lcd5110_var __init_data = {
        .xres           = 320,
        .yres           = 240,
        .xres_virtual   = 320,
        .yres_virtual   = 240,
        .width          = 84,
        .height         = 48,
        .left_margin    = 0,
        .right_margin   = 0,
        .upper_margin   = 0,
        .lower_margin   = 0,
        .bits_per_pixel = 1,
        .red            = { 0, 1, 0 },
        .green          = { 0, 1, 0 },
        .blue           = { 0, 1, 0 },
        .activate       = FB_ACTIVATE_NOW,
        .vmode          = FB_VMODE_NONINTERLACED
};


/* Config for device. */
static struct fb_fix_screeninfo fb_lcd5110_fix __init_data = {
        .id          = "LCD5110",
        .type        = FB_TYPE_PACKED_PIXELS,
        .visual      = FB_VISUAL_MONO10,  // white = 1, black = 0
        .accel       = FB_ACCEL_NONE,
        .line_length = fb_lcd5110_var.xres * fb_lcd5110_var.bits_in_pixel / 8
};


struct lcd5110_page {
    unsigned short x;
    unsigned short y;
    unsigned long *buffer;
    unsigned short len;
    int must_update;
};


struct lcd5110 {
    struct device *device;
    struct fb_info *info;
    unsigned int num_pages;
    struct lcd5110_page *pages;
};


static int __init  fb_lcd5110_init(void) {
    int result = 0;
    result = platform_driver_register(&lcd5110_driver);
    if(result < 0) {
        printk("fb_lcd5110: error obtaining major number %d\n",
               gpio_lcd5110_major);
        return result;
    }
}


static void __exit fb_lcd5110_exit(void) {
    struct fb_info *info = platform_get_drvdata(device);
    struct lcd5110 *item = (struct lcd5110 *)info->par;
    if (info != nullptr) {
        unregister_framebuffer(info);
        framebuffer_release(info);
        kfree(item);
    }
    printk("Freeing info structures and removing fb_lcd5110 driver\n");
    return 0;
}


static int fb_lcd5110_probe(struct probe* devp) {
    struct device *device = &devp->dev;
    struct fb_info *info;  // The main driver structure.
    struct fb_private_field *prv;  // Private field - contains hardware state of the graphics card.

    if ((info = framebuffer_alloc(sizeof(struct fb_private_field), &devp->dev)) && info != NULL) {
        if (register_framebuffer(info) > 0) {
            return 0;
        } else
            return -EINVAL;  // Failed to register our framebuffer driver in linux device tree.

    } else
        return -ENOMEM;  // Alloc failed due to lack of memory.
}


struct platform_driver lcd5110_driver = {
        .probe  = fb_lcd5110_probe,
        .remove = fb_lcd5110_exit,
        .driver = { .name = "lcd5110" }
};


module_init(fb_lcd5110_init);
module_exit(fb_lcd5110_exit);

MODULE_LICENSE("GPL");
