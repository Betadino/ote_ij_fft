#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_MCU               OPT_MCU_RP2040
#define CFG_TUSB_RHPORT0_MODE      OPT_MODE_DEVICE
#define CFG_TUSB_OS                OPT_OS_PICO // Uncomment and choose appropriate OS
#define CFG_TUD_ENDPOINT0_SIZE     64

// Enable relevant classes
#define CFG_TUD_CDC                1
#define CFG_TUD_HID                1

// Buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE     128
#define CFG_TUD_CDC_TX_BUFSIZE     128
#define CFG_TUD_HID_BUFSIZE        64

// USB Speed
#define TUD_USB_SPEED              TUD_USB_SPEED_FULL
// Max power consumption
#define TUD_USB_MAX_POWER          100 // 100mA

#endif // _TUSB_CONFIG_H_
