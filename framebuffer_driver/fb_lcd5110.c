/*
 * Framebuffer driver definition.
 * resources:
 *  https://habr.com/ru/post/213775/
 *  http://www.esys.ir/Files/Ref_Books/Linux/esys.ir_Embedded.Linux.System.Design.and.Development.pdf.
 *  */

#include <fb.h>

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

int fb_lcd5110_major = 29;


static void __exit fb_lcd5110_exit(void);
static int __init  fb_lcd5110_init(void);


static struct fb_var_screeninfo fb_lcd5110_var __init_data = {
        .xres           = 320,
        .yres           = 240,
        .xres_virtual   = 320,
        .yres_virtual   = 240,
        .width          = 84,
        .height         = 48,
        .bits_per_pixel = 1,
        .red            = { 0, 1, 0 },
        .green          = { 0, 1, 0 },
        .blue           = { 0, 1, 0 },
        .activate       = FB_ACTIVATE_NOW,
        .vmode          = FB_VMODE_NONINTERLACED,
};


static struct fb_fix_screeninfo fb_lcd5110_fix __init_data = {
        .id          = "LCD5110",
        .type        = FB_TYPE_PACKED_PIXELS,
        .visual      = FB_VISUAL_MONO10,  // white = 1, black = 0
        .accel       = FB_ACCEL_NONE,
        .line_length = fb_lcd5110_var.xres * fb_lcd5110_var.bits_in_pixel / 8,
};


static struct fb_ops fb_lcd5110_ops = {
        .fb_read      = fb_sys_read,
        // TODO fb_write
        // TODO fb_copyarea
};


struct lcd5110 {
    struct device *device;
    struct fb_info *info;
    unsigned int num_pages;
    struct lcd5110_page *pages;
};

struct lcd5110_page {
    unsigned short x;
    unsigned short y;
    unsigned long *buffer;
    unsigned short len;
    int must_update;
};


static void ssd1963_update(struct fb_info *info, struct list_head *pagelist)
{
    struct ssd1963 *item = (struct ssd1963 *)info->par;
    struct page *page;
    int i;

    list_for_each_entry(page, pagelist, lru) {
        item->pages[page->index].must_update=1;
    }

    //Copy changed pages.
    for (i=0; i<item->pages_count; i++) {
        if (item->pages[i].must_update) {
            item->pages[i].must_update=0;
            ssd1963_copy(item, i);
        }
    }
}

static struct fb_deferred_io lcd5110_defio = {
        .delay          = HZ / 20,
        .deferred_io    = &lcd5110_update,
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


static int __exit fb_lcd5110_remove(struct platform_device *device) {
    /* Free internal structures. */
    struct fb_info *info = platform_get_drvdata(device);
    struct ssd1963 *item = (struct lcd5110 *)info->par;
    if (info) {
        unregister_framebuffer(info);
        ssd1963_pages_free(item);
        ssd1963_video_free(item);
        framebuffer_release(info);
        kfree(item);
    }
    return 0;
}


static void __exit fb_lcd5110_exit(void) {
    struct fb_info *info = platform_get_drvdata(device);
    struct ssd1963 *item = (struct ssd1963 *)info->par;
    if (info != nullptr) {
        unregister_framebuffer(info);
        ssd1963_pages_free(item);
        ssd1963_video_free(item);
        framebuffer_release(info);
        kfree(item);
    }
    printk("Freeing info structures and removing fb_lcd5110 driver\n");
    return 0;
}


struct platform_driver lcd5110_driver = {
        .probe = lcd5110_probe, // ??
        .remove = lcd5110_remove,  // done
        .driver = { .name = "lcd5110" }
};


module_init(fb_lcd5110_init);
module_exit(fb_lcd5110_exit);

MODULE_LICENSE("GPL v2");
