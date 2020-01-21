#include <external_files.c>

void sys_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
    u32 dx = area->dx, dy = area->dy, sx = area->sx, sy = area->sy;
    u32 height = area->height, width = area->width;
    unsigned long const bits_per_line = p->fix.line_length*8u;
    unsigned long *base = NULL;
    int bits = BITS_PER_LONG, bytes = bits >> 3;
    unsigned dst_idx = 0, src_idx = 0, rev_copy = 0;

    if (p->state != FBINFO_STATE_RUNNING)
        return;

    /* if the beginning of the target area might overlap with the end of
    the source area, be have to copy the area reverse. */
    if ((dy == sy && dx > sx) || (dy > sy)) {
        dy += height;
        sy += height;
        rev_copy = 1;
    }

    /* split the base of the framebuffer into a long-aligned address and
       the index of the first bit */
    base = (unsigned long *)((unsigned long)p->screen_base & ~(bytes-1));
    dst_idx = src_idx = 8*((unsigned long)p->screen_base & (bytes-1));
    /* add offset of source and target area */
    dst_idx += dy*bits_per_line + dx*p->var.bits_per_pixel;
    src_idx += sy*bits_per_line + sx*p->var.bits_per_pixel;

    if (p->fbops->fb_sync)
        p->fbops->fb_sync(p);

    if (rev_copy) {
        while (height--) {
            dst_idx -= bits_per_line;
            src_idx -= bits_per_line;
            bitcpy_rev(p, base + (dst_idx / bits), dst_idx % bits,
                       base + (src_idx / bits), src_idx % bits, bits,
                       width*p->var.bits_per_pixel);
        }
    } else {
        while (height--) {
            bitcpy(p, base + (dst_idx / bits), dst_idx % bits,
                   base + (src_idx / bits), src_idx % bits, bits,
                   width*p->var.bits_per_pixel);
            dst_idx += bits_per_line;
            src_idx += bits_per_line;
        }
    }
}