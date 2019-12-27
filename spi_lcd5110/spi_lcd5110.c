#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define SPI_LCD5110_DRV_NAME	"spi_lcd5110"
#define DRIVER_VERSION		    "1.0"

#define ID_LCD5110 0

#define RES   17
#define DC    27
#define gpio_pins_number 2

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#define LCD_C  0
#define LCD_D  1


//static unsigned char spi_lcd5110_mode=0;
//static unsigned char fduplex_mode=0;
unsigned char retval=0;
char *mtx, *mrx;

static unsigned char stbl_tmp;

//enum lcd5110_inversion_mode_t {NOT_INVERTED_MODE, INVERTED};
//enum lcd5110_data_mode_t {DATA_TEXT_MODE, DATA_IMAGE_MODE};

/* --- D R I V E R   S P E C I F I C --------------------------------------- */

void initLcdScreen(struct spi_device *spi);
void clearLcdScreen(struct spi_device *spi);
void sendByteToLcd(struct spi_device *spi, bool, unsigned char);
//void writeCharToLcd(char);
//void writeStringToLcd(char *);

//static int fdx_transfer(struct spi_device *spi, unsigned char *val) {
//    int ret;
//    struct spi_transfer t = {
//            .tx_buf		= mtx,
//            .rx_buf 	= mrx,
//            .len		= 1,
//    };
//    struct spi_message	m;
//
//    mtx[0]=*val;
//    mrx[0]=0;
//
//    spi_message_init(&m);
//    spi_message_add_tail(&t, &m);
//    if((ret=spi_sync(spi, &m))<0)
//        return ret;
//    retval=mrx[0];
//    return ret;
//}
//
//static ssize_t spi_led_store_val(struct device *dev,
//                                 struct device_attribute *attr,
//                                 const char *buf, size_t count)
//{
//    struct spi_device *spi = to_spi_device(dev);
//    unsigned char tmp;
//    unsigned long val;
//
//    if (strict_strtoul(buf, 10, &val) < 0)
//        return -EINVAL;
//    if (val > 255)
//        return -EINVAL;
//    switch(led_mode) {
//        case LED_MODE_L2R:
//        case LED_MODE_R2L:
//            tmp = led_progress(val);
//            break;
//        default:
//            tmp = (unsigned char)val;
//    }
//    stbl_tmp=tmp;
//    if(fduplex_mode)
//        fdx_transfer(spi, &tmp);
//    else
//        spi_write(spi, &tmp, sizeof(tmp));
//    return count;
//}
//
//static ssize_t spi_led_show_val(struct device *dev,
//                                struct device_attribute *attr,
//                                char *buf)
//{
//    unsigned char val;
//    struct spi_device *spi = to_spi_device(dev);
//    if(!fduplex_mode)
//        spi_read(spi, &val, sizeof(val));
//    return scnprintf(buf, PAGE_SIZE, "%d\n", fduplex_mode ? retval : val);
//}
//
//static ssize_t spi_led_store_mode(struct device *dev,
//                                  struct device_attribute *attr,
//                                  const char *buf, size_t count)
//{
//    unsigned long tmp;
//    if (strict_strtoul(buf, 10, &tmp) < 0)
//        return -EINVAL;
//    if(tmp>6)
//        return -EINVAL;
//    led_mode = (unsigned char)tmp&0x03;
//    fduplex_mode = ((unsigned char)tmp&0x04)>>2;
//    return count;
//}
//
//static ssize_t spi_led_show_mode(struct device *dev,
//                                 struct device_attribute *attr,
//                                 char *buf)
//{
//    return scnprintf(buf, PAGE_SIZE, "%d\n", led_mode);
//}
//
//static DEVICE_ATTR(value, S_IWUSR|S_IRUSR, spi_lcd5110_show_val, spi_lcd5110_store_val);
//static DEVICE_ATTR(mode, S_IWUSR|S_IRUSR, spi_lcd5110_show_mode, spi_lcd5110_store_mode);
//
static struct attribute *spi_lcd5110_attributes[] = {
//        &dev_attr_value.attr,
//        &dev_attr_mode.attr,
        NULL
};

static const struct attribute_group spi_lcd5110_attr_group = {
        .attrs = spi_lcd5110_attributes,
};

// Linking driver to device
static int spi_lcd5110_probe(struct spi_device *spi) {
    int ret;

    spi->bits_per_word = 8;
    spi->mode = SPI_MODE_3;
    spi->max_speed_hz = 35000;
    ret = spi_setup(spi);
    if(ret < 0){
        printk("Failed to probe lcd5110 module\n");
        return ret;
    }

    ret = sysfs_create_group(&spi->dev.kobj, &spi_lcd5110_attr_group);
    if (ret < 0){
        printk("Failed to create group of attributes lcd5110 module\n");
        return ret;
    }

    initLcdScreen(spi);
    clearLcdScreen(spi);
    static const unsigned short unicorn[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x6, 0xc, 0x1c, 0x78, 0xf0, 0xf0, 0xe0, 0xe0, 0xc0, 0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xf8, 0xf8, 0xf0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x7f, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xf8, 0xf8, 0xf4, 0xe2, 0xf3, 0xf9, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xfd, 0xf9, 0xfb, 0xf3, 0xe7, 0xcf, 0x9f, 0x3f, 0x7e, 0xfe, 0xfc, 0xf8, 0xf8, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xe7, 0xe7, 0xef, 0xe7, 0xe7, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xf8, 0xf3, 0xc7, 0x1f, 0x7f, 0xfe, 0xf8, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xc0, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf1, 0xff, 0x3f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x3, 0xf, 0x1f, 0x1f, 0x1f, 0x1f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f, 0xf, 0xf, 0xf, 0x7, 0x7, 0x3, 0x3, 0x3, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x3, 0xf, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x3f, 0x1f, 0xc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xf, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    int length = LCD_WIDTH * LCD_HEIGHT / 8;
    int g;
    for(g = 0; g < length; g++){
        sendByteToLcd(spi, LCD_D, unicorn[g]);
    }

    printk("Succeed to probe lcd5110 module\n");
    return ret;
}

// Unlinking device
static int spi_lcd5110_remove(struct spi_device *spi) {
    sysfs_remove_group(&spi->dev.kobj, &spi_lcd5110_attr_group);
    return 0;
}

struct spi_device_id spi_lcd5110_id_table[] = {
        {"lcd5110", ID_LCD5110},
        { }
};

//static const struct of_device_id lcdbar_of_match[] = {
//        { .compatible = "lcd,spi_lcd5110" },
//        {}
//};

static struct spi_driver spi_lcd5110_driver = {
        .driver = {
                .name	= SPI_LCD5110_DRV_NAME,
//                .of_match_table = of_match_ptr(lcdbar_of_match),
                .owner	= THIS_MODULE,
        },
        .id_table	= spi_lcd5110_id_table,
        .probe	= spi_lcd5110_probe,
        .remove	= spi_lcd5110_remove,
};

static int __init spi_lcd5110_init(void) {
    mtx=kzalloc(1, GFP_KERNEL);
    mrx=kzalloc(1, GFP_KERNEL);

    // Initialize GPIO pins
    int pins[gpio_pins_number] = {RES, DC};
    char *pins_name = { "RES", "DC"};

    int g;
    for (g = 0; g < gpio_pins_number; g++) {
        gpio_request(pins[g], &pins_name[g]);
    }
    for (g = 0; g < gpio_pins_number; g++) {
        gpio_direction_output(pins[g], 0);
    }

    printk("Initializing lcd5110 module\n");
//     If succeed to register -> goes to probe.
    return spi_register_driver(&spi_lcd5110_driver);
}

static void __exit spi_lcd5110_exit(void) {
    kfree(mtx);
    kfree(mrx);

    int pins[gpio_pins_number] = {RES, DC};
    int g;
    for (g = 0; g < gpio_pins_number; g++) {
        gpio_free(pins[g]);
    }

    printk("Removing lcd5110 module\n");

    spi_unregister_driver(&spi_lcd5110_driver);
}

module_init(spi_lcd5110_init);
module_exit(spi_lcd5110_exit);

MODULE_DEVICE_TABLE(spi, spi_lcd5110_id_table);
//MODULE_DEVICE_TABLE(of, lcdbar_of_match);

MODULE_AUTHOR("CS_UCU_2019");
MODULE_DESCRIPTION("spi_lcd5110");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);



/* --- L C D   S T A R T S   H E R E --------------------------------------- */

void initLcdScreen(struct spi_device *spi) {
    // set GPIOs
    gpio_set_value(RES, false);
    udelay(2);
    gpio_set_value(RES, true);

    // init LCD
    sendByteToLcd(spi, LCD_C, 0x21);  // LCD Extended Commands
    sendByteToLcd(spi, LCD_C, 0xb1);  // Set LCD Cop (Contrast).  //0xb1
    sendByteToLcd(spi, LCD_C, 0x04);  // Set Temp coefficent.    //0x04
    sendByteToLcd(spi, LCD_C, 0x14);  // LCD bias mode 1:48.     //0x13
    sendByteToLcd(spi, LCD_C, 0x0c);  // LCD in normal mode. 0x0d inverse mode
    sendByteToLcd(spi, LCD_C, 0x20);
    sendByteToLcd(spi, LCD_C, 0x0c);

    clearLcdScreen(spi);
}

void sendByteToLcd(struct spi_device *spi, bool cd, unsigned char data) {
    if(cd)
        gpio_set_value(DC, true);
    else
        gpio_set_value(DC, false);

    spi_write(spi, &data, sizeof(data));
}

void clearLcdScreen(struct spi_device *spi) {
    int i;
    for(i=0; i < LCD_WIDTH * LCD_HEIGHT / 8; i++)
        sendByteToLcd(spi, LCD_D, 0x00);

    sendByteToLcd(spi, LCD_C, 0x80 | 0); // set x coordinate to 0
    sendByteToLcd(spi, LCD_C, 0x40 | 0); // set y coordinate to 0
}

//void writeCharToLcd(char data) {
//    sendByteToLcd(LCD_D, 0x00);
//    int i;
//    for(i=0; i < 5; i++)
//        sendByteToLcd(LCD_D, ASCII[data-0x20][i]);
//    sendByteToLcd(LCD_D, 0x00);
//}
//
//
//void writeStringToLcd(char *data) {
//    while(*data)
//        writeCharToLcd(*data++);
//}