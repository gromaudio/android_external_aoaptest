/*
 * usbhost_iso.c
 *
 *  Created on: Jun 22, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#define LOG_TAG "AOAP"

#include "log/log.h"
#include "common.h"
#include "libusb/libusb.h"
#include "intercom/usbhost_iso.h"

//-----------------------------------------------------------------------------
#define AOAP_VID                        0x18d1
#define AOAP_INTERFACE                  1
#define AOAP_IN_EP                      0x81

/* We queue many transfers to ensure no packets are missed. */
#define NUM_TRANSFERS 10
/* Each transfer will have a max of NUM_PACKETS packets. */
#define NUM_PACKETS 10
#define PACKET_SIZE 256//192

//-----------------------------------------------------------------------------
const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };
struct libusb_device_handle *dev_handle;
struct libusb_transfer *transfer;

iso_ondata_cb cb_fn = NULL;

const char* libusb_error_name(int errcode);
int iso_start_transfers(void);

//-----------------------------------------------------------------------------
static void iso_callback_fn(struct libusb_transfer *transfer)
{
	int rc = 0;
	int len = 0;
	unsigned int i;
	static uint8_t recv[PACKET_SIZE * NUM_PACKETS];

	/* All packets are 192 bytes. */
	//uint8_t* recv = malloc(PACKET_SIZE * transfer->num_iso_packets);
	uint8_t* recv_next = recv;

	//ALOGD("iso_callback_fn();\n");

	for (i = 0; i < transfer->num_iso_packets; i++) {
		struct libusb_iso_packet_descriptor *pack = &transfer->iso_packet_desc[i];
		if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
			ALOGE("Error (status %d: %s)\n", pack->status,
					libusb_error_name(pack->status));
			continue;
		}
		const uint8_t *data = libusb_get_iso_packet_buffer_simple(transfer, i);
		/* PACKET_SIZE == 192 == pack->length */
		memcpy(recv_next, data, pack->actual_length);
		recv_next += pack->actual_length;
		len += pack->actual_length;
	}

	/* At this point, recv points to a buffer containing len bytes of audio. */
	if (cb_fn != NULL && len > 0) {
		cb_fn(recv, len);
	}

	//free(recv);

	/* Sanity check. If this is true, we've overflowed the recv buffer. */
	if (len > PACKET_SIZE * transfer->num_iso_packets) {
		ALOGE("Error: incoming transfer had more data than we thought.\n");
		return;
	}
	if ((rc = libusb_submit_transfer(transfer)) < 0) {
		ALOGE("libusb_submit_transfer: %s.\n", libusb_error_name(rc));
	}
}

//-----------------------------------------------------------------------------
//           Isochronous transfer initialization
//-----------------------------------------------------------------------------
signed char iso_init(iso_ondata_cb cb)
{
  unsigned int  i;
  int           r,
                active_congif;
  cb_fn = cb;

  r = libusb_init(NULL);
  if(r < 0)
    return ERR_INIT_FAILED;

  dev_handle = NULL;
  for(i = 0; i < NELEMS(aSupportedPIDs); i++)
  {
    dev_handle = libusb_open_device_with_vid_pid(NULL, AOAP_VID, aSupportedPIDs[ i ]);
    if(dev_handle != NULL)
    {
      ALOGE("Accessory device found(%04x:%04x).\n", AOAP_VID, aSupportedPIDs[ i ]);
      break;
    }
  }

  if(dev_handle == NULL)
  {
	  ALOGE("No accessory device found.\n");
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  r = libusb_get_configuration(dev_handle, &active_congif);
  if(r < 0)
  {
	  ALOGE("Get active configuration error: %d %s\n", r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  ALOGE("Active configuration: %d\n", active_congif);


  if(1 == libusb_kernel_driver_active(dev_handle, AOAP_INTERFACE))
  {
	  ALOGE("Kernel driver active on interface %d.\n", AOAP_INTERFACE);
    r = libusb_detach_kernel_driver(dev_handle, AOAP_INTERFACE);
    if(r < 0)
    	ALOGE("Detach kernel driver error: %d %s\n", r, libusb_error_name(r));
  }

  r = libusb_claim_interface(dev_handle, AOAP_INTERFACE);
  if (r < 0) {
	ALOGE("Claim interface error: %d %s\n", r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  ALOGE("Set alt settings\n");
  r = libusb_set_interface_alt_setting(dev_handle, AOAP_INTERFACE, 1);
  if (r < 0) {
	ALOGE("Set alt settings error: %d %s\n", r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  return OK;
}

//-----------------------------------------------------------------------------
/* Once setup has succeded, this is called once to start transfers. */
int iso_start_transfers(void) {

	static uint8_t buf[PACKET_SIZE * NUM_PACKETS];
	static struct libusb_transfer *xfr[NUM_TRANSFERS];
	int num_iso_pack = NUM_PACKETS;
    int i, r;

    for (i=0; i<NUM_TRANSFERS; i++) {
        xfr[i] = libusb_alloc_transfer(num_iso_pack);
        if (!xfr[i]) {
            ALOGD("libusb_alloc_transfer failed [%d].\n", i);
            return -ENOMEM;
        }

        libusb_fill_iso_transfer(xfr[i], dev_handle, AOAP_IN_EP, buf,
                sizeof(buf), num_iso_pack, iso_callback_fn, NULL, 1000);
        libusb_set_iso_packet_lengths(xfr[i], sizeof(buf)/num_iso_pack);

        r = libusb_submit_transfer(xfr[i]);
        if (r < 0) {
        	ALOGE("Submit ISO transfer error [%d]: %d %s\n", i, r, libusb_error_name(r));
        }
    }
    ALOGD("usb_start_transfers OK;\n");
    return 0;
}

//-----------------------------------------------------------------------------
int iso_process(void) {
    int rc = libusb_handle_events(NULL);
    if (rc != LIBUSB_SUCCESS) {
        ALOGE("libusb_handle_events: %s.\n", libusb_error_name(rc));
        return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
//                    Isochronous IN transfer
//-----------------------------------------------------------------------------
signed char ISO_IN( volatile unsigned char *aBuff, unsigned short iSize,
					unsigned int *piBytesRead )
{

  return OK;
}

