/* Necessary includes for drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include "ascii.h"

MODULE_LICENSE("Dual BSD/GPL");

bool active_mode = false;
bool text_mode = true;

#define MY_MAX_MINORS  5
#define RES   17 // 0
#define SCE   18 // 1
#define DC    27 // 2
#define SDIN  22 // 3
#define SCLK  23 // 4

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#define LCD_C  0
#define LCD_D  1
#define BUF_LEN 16

//buffer to store data
char * memory_buffer;


/* --- A T T R I B U T E S --------------------------------------- */
void initLcdScreen(void);
void clearLcdScreen(void);
void sendByteToLcd(bool, unsigned char);
void writeCharToLcd(char);
void writeStringToLcd(char *);
void showImage(char *);

static ssize_t gpio_lcd5110_show_state_mode(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    if (active_mode)
        return sprintf(buf, "active\n");
    else
        return sprintf(buf, "sleep\n");
}

static ssize_t gpio_lcd5110_store_state_mode(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    if (strncmp(buf, "a", count-1)==0) { active_mode = true;  }
    else if (strncmp(buf,"s",count-1)==0) { active_mode = false; clearLcdScreen(); }
    else{
        printk("[gpio_lcd5110] - Invalid mode.\n");
    }
    return count;
}

static ssize_t gpio_lcd5110_show_data_mode(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    if (text_mode)
        return sprintf(buf, "text\n");
    else
        return sprintf(buf, "image\n");
}

static ssize_t gpio_lcd5110_store_data_mode(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    if (strncmp(buf, "t", count-1)==0) { text_mode = true;  }
    else if (strncmp(buf,"i",count-1)==0) { text_mode = false;}
    else{
        printk("[gpio_lcd5110] - Invalid mode.\n");
    }
    return count;
}


static struct kobj_attribute state_mode_attr = __ATTR(state_mode, 0660, gpio_lcd5110_show_state_mode, gpio_lcd5110_store_state_mode);
static struct kobj_attribute data_mode_attr = __ATTR(data_mode, 0660, gpio_lcd5110_show_data_mode, gpio_lcd5110_store_data_mode);


static struct attribute *gpio_lcd5110_attrs[] = {
        &state_mode_attr.attr,
        &data_mode_attr.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .name  = "gpio_lcd5110",                        // The name is generated in ebbLED_init()
        .attrs = gpio_lcd5110_attrs,                      // The attributes array defined just above
};

static struct kobject *gpio_lcd5110_kobj;


/* --- D R I V E R   S P E C I F I C --------------------------------------- */

static int  gpio_lcd5110_open(struct inode *inode, struct file *filp);
static int  gpio_lcd5110_release(struct inode *inode, struct file *filp);
static ssize_t  gpio_lcd5110_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t gpio_lcd5110_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

struct file_operations gpio_lcd5110_fops = {
        .read =     gpio_lcd5110_read,
        .write =    gpio_lcd5110_write,
        .open =     gpio_lcd5110_open,
        .release =  gpio_lcd5110_release
};

struct my_device_data {
    struct cdev cdev;
    /* my data starts here */
    //...
};

struct my_device_data devs[MY_MAX_MINORS];

static int __init  gpio_lcd5110_init(void);
static void __exit gpio_lcd5110_exit(void);

/* Driver global variables */
/* Major number */
int gpio_lcd5110_major = 61;

static int gpio_lcd5110_open(struct inode *inode, struct file *file){
    struct my_device_data *my_data;
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    file->private_data = my_data;
    return 0;
}

static int gpio_lcd5110_release(struct inode *inode, struct file *filp) {
    /* Success */
    return 0;
}

static ssize_t gpio_lcd5110_read(struct file *file, char * buf,size_t size, loff_t *f_pos) {
    /* Transfering data to user space */
    copy_to_user(buf, memory_buffer,1);
    /* Changing reading position as best suits */
    if (*f_pos == 0) {
        *f_pos+=1;
        return 1;
    } else {
        return 0;
    }
}

static ssize_t gpio_lcd5110_write( struct file *filp, const char *ubuf, size_t count, loff_t *f_pos) {
    /* Buffer writing to the device */
    char *kbuf = kcalloc((count + 1), sizeof(char), GFP_KERNEL);

    if(copy_from_user(kbuf, ubuf, count) != 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    kbuf[count-1] = 0;

    clearLcdScreen();

    // Reading as image
    if (text_mode){
        writeStringToLcd(kbuf);
    } else{
        int image_length = LCD_WIDTH * LCD_HEIGHT / 8;
        if (count < image_length){
            printk("[gpio_lcd5110]: Too few values.\n");
            return;
        }

        int j = 0;
        int i = 0;
        for(i=0; i < count; i++){
            sendByteToLcd(LCD_D, kbuf[i]);
            j++;
            if (i >= LCD_WIDTH * LCD_HEIGHT / 8){
                j = 0;
                msleep(200);
            }
        }
    }

    kfree(kbuf);

    return (ssize_t)count;
}


/* --- L C D   S P E C I F I C --------------------------------------------- */
module_init(gpio_lcd5110_init);
module_exit(gpio_lcd5110_exit);

static int __init  gpio_lcd5110_init(void) {
    int result;

    // register char device
    result = register_chrdev(gpio_lcd5110_major, "gpio_lcd5110",
                             &gpio_lcd5110_fops);

    if(result < 0) {
        printk("[gpio_lcd5110]: error obtaining major number %d\n",
               gpio_lcd5110_major);
        return result;
    }

    gpio_lcd5110_kobj = kobject_create_and_add("gpio_lcd5110", kernel_kobj->parent);

    if(!gpio_lcd5110_kobj) {
        printk("[gpio_lcd5110]: error creating kobj.\n");
        return -ENOMEM;
    }

    result = sysfs_create_group(gpio_lcd5110_kobj, &attr_group);
    if(result) {
        printk(KERN_ALERT "[gpio_lcd5110]: failed to create sysfs group.\n");
        kobject_put(gpio_lcd5110_kobj);                // clean up -- remove the kobject sysfs entry
        return result;
    }

    if(result < 0) {
        printk("[gpio_lcd5110]: error obtaining major number %d\n",
               gpio_lcd5110_major);
        return result;
    }

    // request and set GPIO ports for lcd display to output
    int pins[5] = {RES, SCE, DC, SDIN, SCLK};
    char *pins_name = { "RES", "SCE", "DC", "SDIN", "SCLK" };
    int g;
    for (g = 0; g < 6; g++) {
        gpio_request(pins[g], &pins_name[g]);
    }
    for (g = 0; g < 6; g++) {
        gpio_direction_output(pins[g], 0);
    }

    initLcdScreen();
    clearLcdScreen();
    static const unsigned short unicorn[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x6, 0xc, 0x1c, 0x78, 0xf0, 0xf0, 0xe0, 0xe0, 0xc0, 0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xf8, 0xf8, 0xf0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x7f, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xf8, 0xf8, 0xf4, 0xe2, 0xf3, 0xf9, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xfd, 0xf9, 0xfb, 0xf3, 0xe7, 0xcf, 0x9f, 0x3f, 0x7e, 0xfe, 0xfc, 0xf8, 0xf8, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xe7, 0xe7, 0xef, 0xe7, 0xe7, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xf8, 0xf3, 0xc7, 0x1f, 0x7f, 0xfe, 0xf8, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xc0, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf1, 0xff, 0x3f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x3, 0xf, 0x1f, 0x1f, 0x1f, 0x1f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f, 0xf, 0xf, 0xf, 0x7, 0x7, 0x3, 0x3, 0x3, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x3, 0xf, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x3f, 0x1f, 0xc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xf, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    int length = LCD_WIDTH * LCD_HEIGHT / 8;
    for(g = 0; g < length; g++){
        sendByteToLcd(LCD_D, unicorn[g]);
    }

    printk("[gpio_lcd5110] - Inserting module\n");
    return 0;
}

static void __exit gpio_lcd5110_exit(void) {
    kobject_put(gpio_lcd5110_kobj);
    unregister_chrdev(gpio_lcd5110_major, "gpio_lcd5110");

    int pins[5] = {RES, SCE, DC, SDIN, SCLK};
    int g;
    for (g=0; g<6; g++) {
        gpio_free(pins[g]);
    }

    /* Freeing buffer memory */
    if (memory_buffer) {
        kfree(memory_buffer);
    }

    printk("[gpio_lcd5110] - Removing module\n");
}


/* --- L C D   S T A R T S   H E R E --------------------------------------- */

void initLcdScreen() {
    // set GPIOs
    gpio_set_value(SCE, false);
    gpio_set_value(SCLK, false);
    gpio_set_value(RES, false);
    udelay(2);
    gpio_set_value(RES, true);

    // init LCD
    sendByteToLcd(LCD_C, 0x21);  // LCD Extended Commands
    sendByteToLcd(LCD_C, 0xb1);  // Set LCD Cop (Contrast).  //0xb1
    sendByteToLcd(LCD_C, 0x04);  // Set Temp coefficent.    //0x04
    sendByteToLcd(LCD_C, 0x14);  // LCD bias mode 1:48.     //0x13
    sendByteToLcd(LCD_C, 0x0c);  // LCD in normal mode. 0x0d inverse mode
    sendByteToLcd(LCD_C, 0x20);
    sendByteToLcd(LCD_C, 0x0c);

    clearLcdScreen();
}

void sendByteToLcd(bool cd, unsigned char data) {
    if(cd)
        gpio_set_value(DC, true);
    else
        gpio_set_value(DC, false);

    unsigned char pattern = 0b10000000;
    int i;
    for(i=0; i < 8; i++) {
        gpio_set_value(SCLK, false);
        if(data & pattern)
            gpio_set_value(SDIN, true);
        else
            gpio_set_value(SDIN, false);

        udelay(1);
        gpio_set_value(SCLK, true);
        udelay(1);
        pattern >>= 1;
    }
}

void clearLcdScreen() {
    int i;
    for(i=0; i < LCD_WIDTH * LCD_HEIGHT / 8; i++)
        sendByteToLcd(LCD_D, 0x00);

    sendByteToLcd(LCD_C, 0x80 | 0); // set x coordinate to 0
    sendByteToLcd(LCD_C, 0x40 | 0); // set y coordinate to 0
}

void writeCharToLcd(char data) {
    sendByteToLcd(LCD_D, 0x00);
    int i;
    for(i=0; i < 5; i++)
        sendByteToLcd(LCD_D, ASCII[data-0x20][i]);
    sendByteToLcd(LCD_D, 0x00);
}


void writeStringToLcd(char *data) {
    int i = 0;
    char *curr;
    while(*data){
        writeCharToLcd(*data++);
        i++;
        if (i > 11){
            i = 0;
            msleep(1000);
        }
    }
}