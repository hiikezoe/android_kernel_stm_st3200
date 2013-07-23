#ifndef _APPSDEV_H_
#define _APPSDEV_H_

#include <linux/ioctl.h>

// device file path & major number
#define APPS_DEV_NODE_NAME      "/dev/appsdev"
#define APPS_DEV_NODE_MAJOR     244

// GPIO PIN
#define APPS_GPIO_PIN_CS        89
#define APPS_GPIO_PIN_WU        120
#define APPS_GPIO_PIN_SW_CENTER 106

// ioctl command
#define APPS_IOC_TYPE           'V'
#define APPS_IOC_CMD_SET_CS     _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x01, sizeof(int))
#define APPS_IOC_CMD_GET_WU     _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x02, 0)
#define APPS_IOC_CMD_GET_SW     _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x03, sizeof(int))
#define APPS_IOC_CMD_GPIO_HIGH  _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x11, sizeof(int))
#define APPS_IOC_CMD_GPIO_LOW   _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x12, sizeof(int))
#define APPS_IOC_CMD_GPIO_VALUE _IOC(_IOC_NONE, APPS_IOC_TYPE, 0x13, sizeof(int))

// DPADs keycode
#define KEY_DPAD_CENTER         232
#define KEY_DPAD_UP             103
#define KEY_DPAD_DOWN           108
#define KEY_DPAD_LEFT           105
#define KEY_DPAD_RIGHT          106

#endif  //_APPSDEV_H_
