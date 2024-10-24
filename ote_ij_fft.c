#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "fft.h"
#include "tusb.h"
#include "tusb_config.h"

#define KEY_1 0x0E
#define KEY_2 0x02
#define KEY_3 0x03
#define KEY_4 0x04
#define KEY_5 0x05
#define TUD_USB_SPEED TUD_USB_SPEED_FULL



frequency_bin_t bins[] = {
  {"Sub-bass", 0, 63, 0},
  {"Bass", 64, 250, 0},
  {"Low Midrange", 251, 500, 0},
  {"Midrange", 501, 2000, 0},
  {"Upper Midrange", 2001, 4000, 0},
  {"Presence", 4001, 6000, 0},
  {"Brilliance", 6001, 20000, 0}
};

void print_frequency_histogram(frequency_bin_t *bins, int bin_count) {
  for (int i = 0; i < bin_count; i++) {
    printf("%s: ", bins[i].name);
    int bar_length = (int)(bins[i].amplitude / 10);
    for (int j = 0; j < bar_length; j++) {
      printf("#");
    }
    printf("\n");
    printf("%d", (int)(bins[i].amplitude));
    printf("\n");
  }

  /* Example output:
  * Sub-bass: ###
  * Bass: #####
  * Low Midrange: ####
  * Midrange: ########
  * Upper Midrange: ######
  * Presence: ######
  * Brilliance: ####
  */
}

// HID report descriptor for a keyboard
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x05, 0x07, // Usage Page (Keyboard/Keypad)
    0x19, 0x00, // Usage Minimum (Keyboard Left Control)
    0x29, 0x91, // Usage Maximum (Keyboard Right GUI)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x01, // Logical Maximum (1)
    0x75, 0x01, // Report Size (1)
    0x95, 0x08, // Report Count (8)
    0x81, 0x02, // Input (Data, Variable, Absolute)
    0x95, 0x01, // Report Count (1)
    0x75, 0x08, // Report Size (8)
    0x81, 0x01, // Input (Constant)
    0x95, 0x05, // Report Count (5)
    0x75, 0x01, // Report Size (1)
    0x05, 0x07, // Usage Page (Keyboard/Keypad)
    0x19, 0xE0, // Usage Minimum (Keyboard Left Control)
    0x29, 0xE7, // Usage Maximum (Keyboard Right GUI)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x01, // Logical Maximum (1)
    0x75, 0x01, // Report Size (1)
    0x95, 0x08, // Report Count (8)
    0x81, 0x02, // Input (Data, Variable, Absolute)
    0x05, 0x08, // Usage Page (LEDs)
    0x19, 0x01, // Usage Minimum (Num Lock)
    0x29, 0x05, // Usage Maximum (Kana)
    0x95, 0x05, // Report Count (5)
    0x75, 0x01, // Report Size (1)
    0x91, 0x02, // Output (Data, Variable, Absolute)
    0x95, 0x01, // Report Count (1)
    0x75, 0x03, // Report Size (3)
    0x91, 0x01, // Output (Constant)
    0xC0        // End Collection
};


// Configuration descriptor (simplified example)
static const uint8_t configuration_descriptor[] = {
    // Configuration Descriptor
    0x09, // bLength
    0x02, // bDescriptorType (Configuration)
    0x22, 0x00, // wTotalLength (Configuration descriptor + Interface descriptor + HID descriptor + Endpoint descriptor)
    0x01, // bNumInterfaces
    0x01, // bConfigurationValue
    0x00, // iConfiguration
    0xA0, // bmAttributes (Bus-powered, Remote wakeup)
    0x32, // bMaxPower (100 mA)

    // Interface Descriptor
    0x09, // bLength
    0x04, // bDescriptorType (Interface)
    0x00, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x01, // bNumEndpoints
    0x03, // bInterfaceClass (HID)
    0x01, // bInterfaceSubClass (Boot)
    0x01, // bInterfaceProtocol (Keyboard)
    0x00, // iInterface

    // HID Descriptor
    0x09, // bLength
    0x21, // bDescriptorType (HID)
    0x11, 0x01, // bcdHID (HID version 1.11)
    0x00, // bCountryCode
    0x01, // bNumDescriptors
    0x22, // bDescriptorType[0] (Report)
    sizeof(hid_report_descriptor), 0x00, // wDescriptorLength[0]

    // Endpoint Descriptor
    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x81, // bEndpointAddress (IN endpoint 1)
    0x03, // bmAttributes (Interrupt)
    0x08, 0x00, // wMaxPacketSize (8 bytes)
    0x0A  // bInterval (10 ms)
};

static uint8_t const desc_device[] = {

    18,     // length
    1,      // descriptor type (Device)
    0x00, 0x02,  // USB version
    0x00,   // device class
    0x00,   // device subclass
    0x00,   // device protocol
    64,     // max packet size
    0x01, 0x02,  // vendor ID
    0x03, 0x04,  // product ID
    0x01, 0x01,  // device release number
    1,      // manufacturer string index
    2,      // product string index
    0,      // serial number string index
    1       // number of configurations
};


// Callbacks
uint8_t const* tud_descriptor_device_cb(void) { // Correct signature without parameter
    return desc_device;
}



uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index; // only one configuration
    return configuration_descriptor;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t desc_str[32]; // Support up to 32 characters

    // Language Code (0x0409 for English)
    if (index == 0) {
        desc_str[0] = (TUSB_DESC_STRING << 8 ) | 4;
        desc_str[1] = 0x0409;
        return desc_str;
    }

    // Array of strings
    const char *string_desc[] = { "Raspberry", "Pico", "123456" };
    
    if (index < sizeof(string_desc)/sizeof(string_desc[0])) {
        uint8_t chr_count = strlen(string_desc[index]);
        if (chr_count > 31) chr_count = 31; // truncate if too long

        // Convert ASCII string to UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[i+1] = string_desc[index][i];
        }
        desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
        return desc_str;
    }

    return NULL;
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance; // suppress unused parameter warning
    return hid_report_descriptor;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void)instance;  // suppress unused parameter warning
    (void)report_id; // suppress unused parameter warning
    (void)report_type; // suppress unused parameter warning
    (void)buffer; // suppress unused parameter warning
    (void)bufsize; // suppress unused parameter warning
    // Empty, no action needed for now
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    // Empty, no action needed for now
    return 0;
}



void send_key(uint8_t key_code) {
    uint8_t report[6] = {0};
    
    // Set the bit corresponding to the key code
    report[key_code & 7] |= (1 << (key_code >> 3));
    
     tud_hid_keyboard_report(0, 0, report);
    
    // Wait for a short time
    sleep_ms(50);
    
    // Release the key
     tud_hid_keyboard_report(0, 0, report);
}


void process_audio_and_send_keys() {
    uint8_t cap_buf[NSAMP];
    fft_sample(cap_buf);
    fft_process(cap_buf, bins, sizeof(bins) / sizeof(frequency_bin_t));

    // Send keys based on frequency analysis
    if (bins[0].amplitude > 100) { // Sub-bass
        send_key(KEY_1);
    } else if (bins[1].amplitude > 100) { // Bass
        send_key(KEY_2);
    } else if (bins[2].amplitude > 100) { // Low Midrange
        send_key(KEY_3);
    } else if (bins[3].amplitude > 100) { // Midrange
        send_key(KEY_4);
    } else if (bins[4].amplitude > 100) { // Upper Midrange
        send_key(KEY_5);
    } else if (bins[5].amplitude > 100) { // Presence
        send_key(KEY_1);
    } else if (bins[6].amplitude > 100) { // Brilliance
        send_key(KEY_2);
    }
}




int main() {
  uint8_t cap_buf[NSAMP];
  fft_setup();
  
  int ret = tusb_init();
    if (ret != 0) {
        printf("Failed to initialize TinyUSB: %d\n", ret);
        return 1;
    }
  uart_init(uart0, 115200);

  //tud_hid_descriptor_set(hid_report_descriptor, sizeof(hid_report_descriptor));
  while (!tud_mounted()) {
    printf("Waiting for USB...\n");
    sleep_ms(1000);
  }
  printf("USB mounted.\n");
  while (1) {
    //process_audio_and_send_keys()
    tud_task(); //refresh of tinyusb
    //print_frequency_histogram(bins, sizeof(bins) / sizeof(frequency_bin_t));
   
    send_key(KEY_1); // Or KEY_2, KEY_3, KEY_4, KEY_5 depending on your requirement
    //  if (1==1) { // Implement button_pressed() function
    //  }
    sleep_ms(10);
  }


        

  return 0;
}
