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

// Copyright (c) 2019 John Seamons, ZL/KF6VO

#pragma once

#include "types.h"
#include "config.h"
#include "kiwi.h"
#include "clk.h"
#include "misc.h"
#include "nbuf.h"
#include "web.h"
#include "spi.h"
#include "gps.h"
#include "coroutines.h"
#include "debug.h"
#include "data_pump.h"
#include "cfg.h"
#include "datatypes.h"
#include "ext_int.h"
#include "rx.h"
#include "noiseproc.h"
#include "dx.h"
#include "non_block.h"
#include "rx_sound.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <math.h>
#include <fftw3.h>

#ifdef USE_WF_NEW
#define	WF_USING_HALF_FFT	1	// the result is contained in the first half of the complex FFT
#define	WF_USING_HALF_CIC	1	// only use half of the remaining FFT after a CIC
#define	WF_BETTER_LOOKING	1	// increase in FFT size for better looking display
#else
#define	WF_USING_HALF_FFT	2	// the result is contained in the first half of the complex FFT
#define	WF_USING_HALF_CIC	2	// only use half of the remaining FFT after a CIC
#define	WF_BETTER_LOOKING	2	// increase in FFT size for better looking display
#endif

#define WF_OUTPUT	1024	// conceptually same as WF_WIDTH although not required
#define WF_C_NFFT	(WF_OUTPUT * WF_USING_HALF_FFT * WF_USING_HALF_CIC * WF_BETTER_LOOKING)	// worst case FFT size needed
#define WF_C_NSAMPS	WF_C_NFFT

#define	WF_WIDTH		1024	// width of waterfall display

struct fft_t {
	fftwf_plan hw_dft_plan;
	fftwf_complex hw_c_samps[sizeof(fftwf_complex) * (WF_C_NSAMPS)];
	fftwf_complex hw_fft[sizeof(fftwf_complex) * (WF_C_NFFT)];
};

struct wf_pkt_t {
	char id4[4];
	u4_t x_bin_server;
	#define WF_FLAGS_COMPRESSION 0x00010000
	u4_t flags_x_zoom_server;
	u4_t seq;
	union {
		u1_t buf[WF_WIDTH];
		struct {
			#define ADPCM_PAD 10
			u1_t adpcm_pad[ADPCM_PAD];
			u1_t buf2[WF_WIDTH];
		};
	} un;
} __attribute__((packed));

struct wf_inst_t {
	conn_t *conn;
	int rx_chan;
	int fft_used, plot_width, plot_width_clamped;
	int maxdb, mindb, send_dB;
	float fft_scale[WF_WIDTH], fft_offset;
	u2_t fft2wf_map[WF_C_NFFT / WF_USING_HALF_FFT];		// map is 1:1 with fft
	u2_t wf2fft_map[WF_WIDTH];							// map is 1:1 with plot
	int start, prev_start, zoom, prev_zoom;
	int mark, speed, fft_used_limit;
	bool new_map, new_map2, compression, isWF, isFFT;
	int flush_wf_pipe;
	int noise_blanker, noise_threshold, nb_click;
	u4_t last_noise_pulse;
	snd_t *snd;
	u4_t snd_seq;
	wf_pkt_t out;
	int out_bytes;
	bool check_overlapped_sampling, overlapped_sampling;
	int samp_wait_ms, chunk_wait_us;
};

struct wf_shmem_t {
    wf_inst_t wf_inst[MAX_RX_CHANS];        // NB: MAX_RX_CHANS even though there may be fewer MAX_WF_CHANS
    fft_t fft_inst[MAX_WF_CHANS];           // NB: MAX_WF_CHANS not MAX_RX_CHANS
    float window_function[WF_C_NSAMPS];
    int n_chunks;
};     

#ifdef MULTI_CORE
    //#define WF_SHMEM_DISABLE
#else
    #define WF_SHMEM_DISABLE
#endif

#ifdef WF_SHMEM_DISABLE
    extern wf_shmem_t *wf_shmem_p;
    #define WF_SHMEM wf_shmem_p
#else
    #define WF_SHMEM (&shmem->wf_shmem)
#endif
