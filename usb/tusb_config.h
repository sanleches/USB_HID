#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

/*
 * =============================================================================
 * TinyUSB Configuration
 * =============================================================================
 * This file configures the TinyUSB stack for this project.
 * CFG_TUSB_MCU is NOT defined here — the Pico SDK build system sets it.
 */

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------

// Use Pico SDK's time support for board_millis() etc.
#define CFG_TUSB_OS              OPT_OS_PICO

// Enable Device stack
#define CFG_TUD_ENABLED          1

// Debug level (0 = disabled)
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG           0
#endif

// Memory section/alignment for USB DMA (Pico SDK defaults)
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN       __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// Device Configuration
//--------------------------------------------------------------------

// Endpoint 0 max packet size
#define CFG_TUD_ENDPOINT0_SIZE   64

// Maximum device speed
#define CFG_TUD_MAX_SPEED        OPT_MODE_DEFAULT_SPEED

// Roothub port 0 mode (device only)
#define CFG_TUSB_RHPORT0_MODE    OPT_MODE_DEVICE

//--------------------------------------------------------------------
// Class Support
//--------------------------------------------------------------------
#define CFG_TUD_CDC              0
#define CFG_TUD_MSC              0
#define CFG_TUD_HID              1
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0

// HID endpoint buffer size (must fit largest report including report ID)
#define CFG_TUD_HID_EP_BUFSIZE   64

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */
