#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fft.h"
#include "tusb.h"
#include "tusb_config.h"
#include "usb_descriptors.h"
#include "bsp/board_api.h"
#include "common/tusb_types.h"
#include "hid.h"


#define CFG_TUSB_MCU OPT_MCU_RP2040
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
  BLINK_DETECTA = 10,
  BLINK_DETECTB = 20,
  BLINK_DETECTC = 30,
};

uint32_t current_time;
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
uint8_t cap_buf[NSAMP];
void led_blinking_task(void);
void hid_task(void);

//FFT PART_________________

// frequency_bin_t bins[] = {
//   {"Sub-bass", 0, 63, 0},
//   {"Bass", 64, 250, 0},
//   {"Low Midrange", 251, 500, 0},
//   {"Midrange", 501, 2000, 0},
//   {"Upper Midrange", 2001, 4000, 0},
//   {"Presence", 4001, 6000, 0},
//   {"Brilliance", 6001, 20000, 0}
// };

frequency_bin_t note_bins[] = 
{ {"C3", 120, 140, 0}, // C3 (130.81 Hz) 
{"E3", 150, 180, 0}, // E3 (164.81 Hz) 
{"A3", 200, 240, 0}, // A3 (220.00 Hz) 
{"C4", 250, 280, 0}, // C4 (261.63 Hz) 
{"E4", 310, 340, 0}, // E4 (329.63 Hz) 
{"A4", 420, 460, 0} // A4 (440.00 Hz) 
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






//FFT PART _________________

/*------------- MAIN -------------*/
int main(void)
{
  uint32_t current_time = board_millis();
  cap_buf[NSAMP];
  fft_setup();
  board_init();

  // init device stack on configured roothub port
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  while (1)
  {
    tud_task(); // tinyusb device task
    process_audio_and_send_keys();
    led_blinking_task();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint8_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;
  
  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      if ( btn )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = btn;
        
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
      int8_t const delta = 5;

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      static bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_KEYBOARD, btn);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

void send_key(uint8_t  key_pressed) 
{ 
    send_hid_report(REPORT_ID_KEYBOARD, key_pressed); 
    
}

// Define a global flag and timer to control signal sending
bool signal_sent = false;
uint32_t global_timer = 0;

void reset_global_flag() {
    uint32_t current_time = board_millis();
    if (signal_sent && (current_time - global_timer > 10)) {
        signal_sent = false;
    }
}

void process_audio_and_send_keys() {
    fft_sample(cap_buf);
    fft_process(cap_buf, note_bins, sizeof(note_bins) / sizeof(frequency_bin_t));

    
    reset_global_flag();

    

    if (!signal_sent && note_bins[0].amplitude > 100) { // C3
        printf("Sending key for C3\n");
        send_key(HID_KEY_1);
        signal_sent = true;
        global_timer = current_time;
    } else if (!signal_sent && note_bins[1].amplitude > 100) { // E3
        printf("Sending key for E3\n");
        send_key(HID_KEY_2);
        signal_sent = true;
        global_timer = current_time;
    } else if (!signal_sent && note_bins[2].amplitude > 100) { // A3
        printf("Sending key for A3\n");
        send_key(HID_KEY_4);
        signal_sent = true;
        global_timer = current_time;
    } else if (!signal_sent && note_bins[3].amplitude > 100) { // C4
        printf("Sending key for C4\n");
        send_key(HID_KEY_1);
        signal_sent = true;
        global_timer = current_time;
    } else if (!signal_sent && note_bins[4].amplitude > 100) { // E4
        printf("Sending key for E4\n");
        send_key(HID_KEY_2);
        signal_sent = true;
        global_timer = current_time;
    } else if (!signal_sent && note_bins[5].amplitude > 100) { // A4
        printf("Sending key for A4\n");
        send_key(HID_KEY_4);
        signal_sent = true;
        global_timer = current_time;
    } else {
        // Add a debug statement if no key is sent
        printf("No key sent based on frequency analysis\n");
    }
}

