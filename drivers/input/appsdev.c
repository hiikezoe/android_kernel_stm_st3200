#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/version.h>
#include "appsdev.h"

#define DRIVER_NAME         "appsdev"
#define AUTHOR              "Appside Co., Ltd."
#define DESCRIPTION         "Appside Vector Device Driver"
#define LICENSE             "GPL"

#define DEFAULT_MAJOR_NUM   APPS_DEV_NODE_MAJOR

static struct input_dev *apps_input_dev = NULL;

static int appsdev_input_open(struct input_dev *dev)
{
    return 0;
}

static void appsdev_input_close(struct input_dev *dev)
{
    input_unregister_device(apps_input_dev);
}

static int appsdev_gpio_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);
    
    filp->private_data = (void *)minor;

    return 0;
}

static int appsdev_gpio_close(struct inode *inode, struct file *filp)
{
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int appsdev_gpio_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int appsdev_gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    int ret = 0;

    switch (cmd) {
    case APPS_IOC_CMD_SET_CS:
        gpio_set_value(APPS_GPIO_PIN_CS, arg);
        //printk(KERN_INFO DRIVER_NAME ": CMD_SET_CS: pin=%d val=%d\n", APPS_GPIO_PIN_CS, ret);
        break;
    case APPS_IOC_CMD_GET_WU:
        ret = gpio_get_value(APPS_GPIO_PIN_WU);
        //printk(KERN_INFO DRIVER_NAME ": CMD_GET_WU: pin=%d val=%d\n", APPS_GPIO_PIN_WU, ret);
        break;
    case APPS_IOC_CMD_GET_SW:
        ret = gpio_get_value(arg);
        //printk(KERN_INFO DRIVER_NAME ": CMD_GET_SW: pin=%ld val=%d\n", arg, ret);
        break;
    case APPS_IOC_CMD_GPIO_HIGH:
        gpio_set_value(arg, 1);
        //printk(KERN_INFO DRIVER_NAME ": CMD_GPIO_HIGH: pin=%ld\n", arg);
        break;
    case APPS_IOC_CMD_GPIO_LOW:
        gpio_set_value(arg, 0);
        //printk(KERN_INFO DRIVER_NAME ": CMD_GPIO_LOW: pin=%ld\n", arg);
        break;
    case APPS_IOC_CMD_GPIO_VALUE:
        ret = gpio_get_value(arg);
        //printk(KERN_INFO DRIVER_NAME ": CMD_GPIO_VALUE: pin=%ld val=%d\n", arg, ret);
        break;
    default:
        printk(KERN_ERR DRIVER_NAME ": ioctl error\n");
        ret = -EINVAL;
        break;
    }

    return ret;
}

static struct file_operations appsdev_gpio_fops = {
    .owner      = THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
    .ioctl      = appsdev_gpio_ioctl,
#else
    .unlocked_ioctl = appsdev_gpio_ioctl,
#endif
    .open       = appsdev_gpio_open,
    .release    = appsdev_gpio_close,
};

static int __init appsdev_init(void)
{
    unsigned int i;

    /**********************************************************/
    /* GPIO                                                   */
    /**********************************************************/
    if (register_chrdev(DEFAULT_MAJOR_NUM, DRIVER_NAME, &appsdev_gpio_fops)) {
        printk(KERN_ERR DRIVER_NAME ": failed to init\n");
        return -EBUSY;
    }

#if 0
    gpio_request(APPS_GPIO_PIN_CS, "apps_cs");
    gpio_direction_output(APPS_GPIO_PIN_CS, 0);
    gpio_request(APPS_GPIO_PIN_WU, "apps_wu");
    gpio_direction_input(APPS_GPIO_PIN_WU);
    gpio_request(APPS_GPIO_PIN_SW_CENTER, "apps_sw_center");
    gpio_direction_input(APPS_GPIO_PIN_SW_CENTER);
#endif

    /**********************************************************/
    /* Input Device                                           */
    /**********************************************************/
    apps_input_dev = input_allocate_device();
    if (!apps_input_dev) {
        printk(KERN_ERR DRIVER_NAME ": not enough memory\n");
        return -ENOMEM;
    }

    apps_input_dev->name = DRIVER_NAME;
#if 1
    apps_input_dev->id.vendor = 0x3204;
    apps_input_dev->id.product = 0x3204;
#endif
    apps_input_dev->open = appsdev_input_open;
    apps_input_dev->close = appsdev_input_close;
    input_set_capability(apps_input_dev, EV_REL, REL_X);
    input_set_capability(apps_input_dev, EV_REL, REL_Y);
    input_set_capability(apps_input_dev, EV_REL, REL_WHEEL);
    input_set_capability(apps_input_dev, EV_ABS, ABS_X);
    input_set_capability(apps_input_dev, EV_ABS, ABS_Y);
    input_set_capability(apps_input_dev, EV_ABS, ABS_Z);
#if 0
    input_set_capability(apps_input_dev, EV_KEY, BTN_LEFT);
    input_set_capability(apps_input_dev, EV_KEY, BTN_RIGHT);
    input_set_capability(apps_input_dev, EV_KEY, BTN_MIDDLE);
    input_set_capability(apps_input_dev, EV_KEY, KEY_ENTER);
    input_set_capability(apps_input_dev, EV_KEY, KEY_UP);
    input_set_capability(apps_input_dev, EV_KEY, KEY_LEFT);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DOWN);
    input_set_capability(apps_input_dev, EV_KEY, KEY_RIGHT);
    input_set_capability(apps_input_dev, EV_KEY, KEY_MENU);
    input_set_capability(apps_input_dev, EV_KEY, KEY_BACK);
    input_set_capability(apps_input_dev, EV_KEY, KEY_HOME);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DPAD_CENTER);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DPAD_UP);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DPAD_DOWN);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DPAD_LEFT);
    input_set_capability(apps_input_dev, EV_KEY, KEY_DPAD_RIGHT);
#else
    for (i = 1; i <= KEY_MAX; i++) {
        input_set_capability(apps_input_dev, EV_KEY, i);
    }
#endif

    if (input_register_device(apps_input_dev)) {
        printk(KERN_ERR DRIVER_NAME ": failed to register\n");
        input_free_device(apps_input_dev);
        return -EBUSY;
    }
    else {
        //printk(KERN_INFO DRIVER_NAME ": registered as input device\n");
    }

    return 0;
}

static void __exit appsdev_cleanup(void)
{
    input_free_device(apps_input_dev);
    unregister_chrdev(DEFAULT_MAJOR_NUM, DRIVER_NAME);
}

module_init(appsdev_init);
module_exit(appsdev_cleanup);

MODULE_DESCRIPTION(DESCRIPTION);
MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

