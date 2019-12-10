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

MODULE_LICENSE("Dual BSD/GPL");

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
static char message[BUF_LEN];
static char *msg_p;

//buffer to store data
char * memory_buffer;
/* --- D R I V E R   S P E C I F I C --------------------------------------- */


static int  lcd5110_open(struct inode *inode, struct file *filp);
static int  lcd5110_release(struct inode *inode, struct file *filp);
static ssize_t  lcd5110_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t lcd5110_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static void __exit lcd5110_exit(void);
static int __init  lcd5110_init(void);

/* Structure that declares the common */
/* file access functions */
struct file_operations lcd5110_fops = {
        .read =     lcd5110_read,
        .write =        lcd5110_write,
        .open =     lcd5110_open,
        .release =   lcd5110_release
};

struct my_device_data {
    struct cdev cdev;
    /* my data starts here */
    //...
};

struct my_device_data devs[MY_MAX_MINORS];

/* Driver global variables */
/* Major number */
int lcd5110_major = 61;


/* --- L C D   S P E C I F I C --------------------------------------------- */

static const unsigned short ASCII[][5] =
        {
                 {0x00, 0x00, 0x00, 0x00, 0x00} // 20
                ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
                ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
                ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
                ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
                ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
                ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
                ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
                ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
                ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
                ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
                ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
                ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
                ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
                ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
                ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
                ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
                ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
                ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
                ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
                ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
                ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
                ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
                ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
                ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
                ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
                ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
                ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
                ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
                ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
                ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
                ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
                ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
                ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
                ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
                ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
                ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
                ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
                ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
                ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
                ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
                ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
                ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
                ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
                ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
                ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
                ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
                ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
                ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
                ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
                ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
                ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
                ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
                ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
                ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
                ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
                ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
                ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
                ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
                ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
                ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c ¥
                ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
                ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
                ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
                ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
                ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
                ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
                ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
                ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
                ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
                ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
                ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
                ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
                ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
                ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j
                ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
                ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
                ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
                ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
                ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
                ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
                ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
                ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
                ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
                ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
                ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
                ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
                ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
                ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
                ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
                ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
                ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
                ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
                ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
                ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e .
                ,{0x00, 0x06, 0x09, 0x09, 0x06} // 7f .
        };




void initLcdScreen(void);
void clearLcdScreen(void);
void sendByteToLcd(bool, unsigned char);
void writeCharToLcd(char);
void writeStringToLcd(char *);

module_init(lcd5110_init);
module_exit(lcd5110_exit);

static int __init  lcd5110_init(void) {
    int result;

    // register char device
    result = register_chrdev(lcd5110_major, "lcd5110",
                             &lcd5110_fops);
    if(result < 0) {
        printk("lcd5110: error obtaining major number %d\n",
               lcd5110_major);
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
    writeStringToLcd("LCD initialized");

    /* Allocating memory for the buffer */
    memory_buffer = kmalloc(1, GFP_KERNEL);
    if (!memory_buffer) {
        result = -ENOMEM;
        goto fail;
    }
    memset(memory_buffer, 0, 1);
    printk("Inserting lcd5110 module\n");
    return 0;
    fail:
    lcd5110_exit();
    return result;
}

static void __exit lcd5110_exit(void) {
    unregister_chrdev(lcd5110_major, "lcd5110");

    int pins[5] = {RES, SCE, DC, SDIN, SCLK};
    int g;
    for (g=0; g<6; g++) {
        gpio_free(pins[g]);
    }

    /* Freeing buffer memory */
    if (memory_buffer) {
        kfree(memory_buffer);
    }

    printk("Removing lcd5110 module\n");
}

static int lcd5110_open(struct inode *inode, struct file *file){
    struct my_device_data *my_data;
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    file->private_data = my_data;
    return 0;
}

static int lcd5110_release(struct inode *inode, struct file *filp) {
    /* Success */
    return 0;
}

static ssize_t lcd5110_read(struct file *file, char * buf,size_t size, loff_t *f_pos) {
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

static ssize_t lcd5110_write( struct file *filp, const char *ubuf, size_t count, loff_t *f_pos) {
    /* Buffer writing to the device */
    char *kbuf = kcalloc((count + 1), sizeof(char), GFP_KERNEL);

    if(copy_from_user(kbuf, ubuf, count) != 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    kbuf[count-1] = 0;

    clearLcdScreen();
    writeStringToLcd(kbuf);
    kfree(kbuf);

    return (ssize_t)count;
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
    while(*data)
        writeCharToLcd(*data++);
}