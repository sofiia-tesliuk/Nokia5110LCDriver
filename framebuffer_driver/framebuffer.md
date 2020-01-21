```
struct fb_info {
     kdev_t node;
     int flags;
     int open;
     struct fb_var_screeninfo var;
     struct fb_fix_screeninfo fix;
     struct fb_monspecs monspecs;
     struct fbcursor cursor;
     struct fb_cmap cmap;
     struct fb_ops *fbops;
     struct pm_dev *pm_fb;
     char *screen_base;
     wait_queue_head_t wait;
     devfs_handle_t devfs_handle;
     devfs_handle_t devfs_lhandle;
     void *pseudo_palette;
  #ifdef CONFIG_MTRR
     int mtrr_handle;
  #endif
     void *par;
  }               
```