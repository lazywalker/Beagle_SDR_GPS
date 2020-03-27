/*
--------------------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA  02110-1301, USA.
--------------------------------------------------------------------------------
*/

// Copyright (c) 2016 John Seamons, ZL/KF6VO

#pragma once

#include "types.h"
#include "coroutines.h"
#include "datatypes.h"

// extensions to compile
#if 1
 #define EXT_WSPR
 #define EXT_EXAMPLE
 #define EXT_LORAN_C
 #define EXT_IQ_DISPLAY
 //#define EXT_S4285
 #define EXT_INTEGRATE
 #define EXT_S_METER
 #define EXT_TEST
#endif

typedef void (*ext_main_t)();
typedef void (*ext_close_conn_t)(int rx_chan);
typedef bool (*ext_receive_msgs_t)(char *msg, int rx_chan);
typedef void (*ext_receive_iq_samps_t)(int rx_chan, int ch, int ns_out, TYPECPX *samps);
typedef void (*ext_receive_real_samps_t)(int rx_chan, int ch, int ns_out, TYPEMONO16 *samps);
typedef void (*ext_receive_FFT_samps_t)(int rx_chan, int ch, int ratio, int ns_out, TYPECPX *samps);
typedef void (*ext_receive_S_meter_t)(int rx_chan, float S_meter_dBm);
typedef void (*ext_poll_t)(int rx_chan);

#define EXT_NEW_VERSION     0xcafebeef
#define EXT_NO_FLAGS        0x00
#define EXT_FLAGS_HEAVY     0x01

// used by extension server-part to describe itself
typedef struct {
	const char *name;					// name of extension, short, no whitespace
	ext_main_t main_unused;             // unused, ext_main_t routines are called via ext_init.c:extint_init()
	ext_close_conn_t close_conn;		// routine to cleanup when connection closed
	ext_receive_msgs_t receive_msgs;	// routine to receive messages from client-part

    u4_t version;                       // for backward compatibility with external extensions (e.g. antenna switch)
	u4_t flags;
	ext_poll_t poll_cb;                 // periodic callback that cal be used for polling (e.g. shared mem comm)
} ext_t;

void ext_register(ext_t *ext);

// call to start/stop receiving audio channel IQ samples, post-FIR filter, but pre- detector & AGC
void ext_register_receive_iq_samps(ext_receive_iq_samps_t func, int rx_chan);
void ext_register_receive_iq_samps_task(tid_t tid, int rx_chan);
void ext_unregister_receive_iq_samps(int rx_chan);
void ext_unregister_receive_iq_samps_task(int rx_chan);

// call to start/stop receiving audio channel real samples, post- FIR filter, detection & AGC
void ext_register_receive_real_samps(ext_receive_real_samps_t func, int rx_chan);
void ext_register_receive_real_samps_task(tid_t tid, int rx_chan);
void ext_unregister_receive_real_samps(int rx_chan);
void ext_unregister_receive_real_samps_task(int rx_chan);

// call to start/stop receiving audio channel FFT samples, pre- or post-FIR filter, detection & AGC
typedef enum { PRE_FILTERED, POST_FILTERED } ext_FFT_filtering_e;
void ext_register_receive_FFT_samps(ext_receive_FFT_samps_t func, int rx_chan, ext_FFT_filtering_e filtering);
void ext_unregister_receive_FFT_samps(int rx_chan);

// call to start/stop receiving S-meter data
void ext_register_receive_S_meter(ext_receive_S_meter_t func, int rx_chan);
void ext_unregister_receive_S_meter(int rx_chan);

// general routines
double ext_update_get_sample_rateHz(int rx_chan);		// return sample rate of audio channel
void ext_adjust_clock_offset(int rx_chan, double offset);

// routines to send messages to extension client-part
int ext_send_msg(int rx_chan, bool debug, const char *msg, ...);
int ext_send_msg_data(int rx_chan, bool debug, u1_t cmd, u1_t *bytes, int nbytes);
int ext_send_msg_data2(int rx_chan, bool debug, u1_t cmd, u1_t data2, u1_t *bytes, int nbytes);
int ext_send_msg_encoded(int rx_chan, bool debug, const char *dst, const char *cmd, const char *fmt, ...);
