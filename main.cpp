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

// Copyright (c) 2014-2016 John Seamons, ZL/KF6VO

#include "types.h"
#include "config.h"
#include "kiwi.h"
#include "clk.h"
#include "misc.h"
#include "str.h"
#include "web.h"
#include "peri.h"
#include "spi_dev.h"
#include "gps.h"
#include "coroutines.h"
#include "pru_realtime.h"
#include "debug.h"
#include "cfg.h"
#include "net.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include <sched.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

int version_maj, version_min;

int p0=0, p1=0, p2=0, wf_sim, wf_real, wf_time, ev_dump=0, wf_flip, wf_start=1, tone, down,
	rx_cordic, rx_cic, rx_cic2, rx_dump, wf_cordic, wf_cic, wf_mult, wf_mult_gen, do_slice=-1,
	rx_yield=1000, gps_chans=GPS_CHANS, spi_clkg, spi_speed=SPI_48M, wf_max, rx_num=RX_CHANS, wf_num=RX_CHANS,
	do_gps, do_sdr=1, navg=1, wf_olap, meas, spi_delay=100, do_fft, do_dyn_dns=1, debian_ver,
	noisePwr=-160, unwrap=0, rev_iq, ineg, qneg, fft_file, fftsize=1024, fftuse=1024, bg, alt_port,
	color_map, print_stats, ecpu_cmds, ecpu_tcmds, use_spidev, debian_maj, debian_min,
	gps_debug, gps_var, gps_lo_gain, gps_cg_gain;

bool create_eeprom, need_hardware, no_net, test_flag, sdr_hu_debug, have_ant_switch_ext, gps_e1b_only,
    disable_led_task;

char **main_argv;

int main(int argc, char *argv[])
{
	u2_t *up;
	int i;
	int p_gps=0;
	bool ext_clk = false;
	
	version_maj = VERSION_MAJ;
	version_min = VERSION_MIN;
	
	main_argv = argv;
	
	#ifdef DEVSYS
		do_sdr = 0;
		p_gps = -1;
	#else
		// enable generation of core file in /tmp
		scall("core_pattern", system("echo /tmp/core-%e-%s-%u-%g-%p-%t > /proc/sys/kernel/core_pattern"));
		const struct rlimit unlim = { RLIM_INFINITY, RLIM_INFINITY };
		scall("setrlimit", setrlimit(RLIMIT_CORE, &unlim));
	#endif
	
	kstr_init();

	for (i=1; i<argc; ) {
		if (strcmp(argv[i], "-test")==0) test_flag = TRUE;
		if (strcmp(argv[i], "-sdr_hu")==0) sdr_hu_debug = TRUE;
		if (strcmp(argv[i], "-bg")==0) { background_mode = TRUE; bg=1; }
		if (strcmp(argv[i], "-down")==0) down = 1;
		if (strcmp(argv[i], "+gps")==0) p_gps = 1;
		if (strcmp(argv[i], "-gps")==0) p_gps = -1;
		if (strcmp(argv[i], "+sdr")==0) do_sdr = 1;
		if (strcmp(argv[i], "-sdr")==0) do_sdr = 0;
		if (strcmp(argv[i], "+fft")==0) do_fft = 1;

		if (strcmp(argv[i], "-gps_debug")==0) {
		    errno = 0;
			if (i+1 < argc && (gps_debug = strtol(argv[i+1], 0, 0), errno == 0)) {
				i++;
			} else {
				gps_debug = -1;
			}
		}
		
		if (strcmp(argv[i], "-stats")==0 || strcmp(argv[i], "+stats")==0) {
		    errno = 0;
			if (i+1 < argc && (print_stats = strtol(argv[i+1], 0, 0), errno == 0)) {
				i++;
			} else {
				print_stats = STATS_TASK;
			}
		}
		
		if (strcmp(argv[i], "-led")==0 || strcmp(argv[i], "-leds")==0) disable_led_task = true;
		if (strcmp(argv[i], "-gps_e1b")==0) gps_e1b_only = true;
		if (strcmp(argv[i], "-gps_var")==0) { i++; gps_var = strtol(argv[i], 0, 0); printf("gps_var %d\n", gps_var); }
		if (strcmp(argv[i], "-e1b_lo_gain")==0) { i++; gps_lo_gain = strtol(argv[i], 0, 0); printf("e1b_lo_gain %d\n", gps_lo_gain); }
		if (strcmp(argv[i], "-e1b_cg_gain")==0) { i++; gps_cg_gain = strtol(argv[i], 0, 0); printf("e1b_cg_gain %d\n", gps_cg_gain); }

		if (strcmp(argv[i], "-debian")==0) { i++; debian_ver = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-ctrace")==0) web_caching_debug = true;
		if (strcmp(argv[i], "-ext")==0) ext_clk = true;
		if (strcmp(argv[i], "-use_spidev")==0) { i++; use_spidev = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-eeprom")==0) create_eeprom = true;
		if (strcmp(argv[i], "-cmap")==0) color_map = 1;
		if (strcmp(argv[i], "-sim")==0) wf_sim = 1;
		if (strcmp(argv[i], "-real")==0) wf_real = 1;
		if (strcmp(argv[i], "-time")==0) wf_time = 1;
		if (strcmp(argv[i], "-port")==0) { i++; alt_port = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-p")==0) { alt_port = 8074; }
		if (strcmp(argv[i], "-dump")==0 || strcmp(argv[i], "+dump")==0) { i++; ev_dump = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-flip")==0) wf_flip = 1;
		if (strcmp(argv[i], "-start")==0) wf_start = 1;
		if (strcmp(argv[i], "-mult")==0) wf_mult = 1;
		if (strcmp(argv[i], "-multgen")==0) wf_mult_gen = 1;
		if (strcmp(argv[i], "-wmax")==0) wf_max = 1;
		if (strcmp(argv[i], "-olap")==0) wf_olap = 1;
		if (strcmp(argv[i], "-meas")==0) meas = 1;
		
		// do_fft
		if (strcmp(argv[i], "-none")==0) unwrap = 0;
		if (strcmp(argv[i], "-norm")==0) unwrap = 1;
		if (strcmp(argv[i], "-rev")==0) unwrap = 2;
		if (strcmp(argv[i], "-qi")==0) rev_iq = 1;
		if (strcmp(argv[i], "-ineg")==0) ineg = 1;
		if (strcmp(argv[i], "-qneg")==0) qneg = 1;
		if (strcmp(argv[i], "-file")==0) fft_file = 1;
		if (strcmp(argv[i], "-fftsize")==0) { i++; fftsize = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-fftuse")==0) { i++; fftuse = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-np")==0) { i++; noisePwr = strtol(argv[i], 0, 0); }

		if (strcmp(argv[i], "-rcordic")==0) rx_cordic = 1;
		if (strcmp(argv[i], "-rcic")==0) rx_cic = 1;
		if (strcmp(argv[i], "-rcic2")==0) rx_cic2 = 1;
		if (strcmp(argv[i], "-rdump")==0) rx_dump = 1;
		if (strcmp(argv[i], "-wcordic")==0) wf_cordic = 1;
		if (strcmp(argv[i], "-wcic")==0) wf_cic = 1;
		if (strcmp(argv[i], "-clkg")==0) spi_clkg = 1;
		
		if (strcmp(argv[i], "-avg")==0) { i++; navg = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-tone")==0) { i++; tone = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-slc")==0) { i++; do_slice = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-rx")==0) { i++; rx_num = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-wf")==0) { i++; wf_num = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-spispeed")==0) { i++; spi_speed = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-spi")==0) { i++; spi_delay = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-ch")==0) { i++; gps_chans = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-y")==0) { i++; rx_yield = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-p0")==0) { i++; p0 = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-p1")==0) { i++; p1 = strtol(argv[i], 0, 0); }
		if (strcmp(argv[i], "-p2")==0) { i++; p2 = strtol(argv[i], 0, 0); }

		i++;
		while (i<argc && ((argv[i][0] != '+') && (argv[i][0] != '-'))) {
			i++;
		}
	}
	
	lprintf("KiwiSDR v%d.%d --------------------------------------------------------------------\n",
		version_maj, version_min);
    lprintf("compiled: %s %s\n", __DATE__, __TIME__);
    if (debian_ver) lprintf("-debian %d\n", debian_ver);
    char *reply = read_file_string_reply("/etc/debian_version");
    if (reply != NULL) {
        sscanf(kstr_sp(reply), "%d.%d", &debian_maj, &debian_min);
        kstr_free(reply);
        lprintf("/etc/debian_version %d.%d\n", debian_maj, debian_min);
    }
    
    #if defined(HOST) && defined(USE_VALGRIND)
    	lprintf("### compiled with USE_VALGRIND\n");
    #endif
    
    assert (TRUE != FALSE);
    assert (true != false);
    assert (TRUE == true);
    assert (FALSE == false);
    assert (NOT_FOUND != TRUE);
    assert (NOT_FOUND != FALSE);
    
    if (background_mode) {
    	lprintf("background mode: delaying start 30 secs...\n");
    	sleep(30);
    }
    
    clock_init();

	TaskInit();

    cfg_reload(CALLED_FROM_MAIN);
    
    do_gps = admcfg_bool("enable_gps", NULL, CFG_REQUIRED);
    if (p_gps != 0) do_gps = (p_gps == 1)? 1:0;
    
	if (down) do_sdr = do_gps = 0;
	need_hardware = (do_gps || do_sdr);

	// called early, in case another server already running so we can detect the busy socket and bail
	web_server_init(WS_INIT_CREATE);

	if (need_hardware) {
		peri_init();
		fpga_init();
		//pru_start();
		eeprom_update();
		
		u2_t ctrl = CTRL_EEPROM_WP;
		ctrl_clr_set(0xffff, ctrl);
		if (adc_clock_enable & !ext_clk) ctrl |= CTRL_OSC_EN;
		ctrl_clr_set(0, ctrl);

		if (ctrl & CTRL_OSC_EN)
			printf("ADC_CLOCK: %.6f MHz\n", ADC_CLOCK_NOM/MHz);
		else
			printf("ADC_CLOCK: EXTERNAL, J5 connector\n");
		
		// read device DNA
		ctrl_clr_set(CTRL_DNA_CLK | CTRL_DNA_SHIFT, CTRL_DNA_READ);
		ctrl_positive_pulse(CTRL_DNA_CLK);
		ctrl_clr_set(CTRL_DNA_CLK | CTRL_DNA_READ, CTRL_DNA_SHIFT);
		ddns.dna = 0;
		for (int i=0; i < 64; i++) {
		    stat_reg_t stat = stat_get();
		    ddns.dna = (ddns.dna << 1) | ((stat.word & STAT_DNA_DATA)? 1ULL : 0ULL);
		    ctrl_positive_pulse(CTRL_DNA_CLK);
		}
		ctrl_clr_set(CTRL_DNA_CLK | CTRL_DNA_READ | CTRL_DNA_SHIFT, 0);
		printf("device DNA %08x|%08x\n", PRINTF_U64_ARG(ddns.dna));
	}
	
	if (do_fft) {
		printf("==== IQ %s\n", rev_iq? "reverse":"normal");
		if (ineg) printf("==== I neg\n");
		if (qneg) printf("==== Q neg\n");
		printf("==== unwrap %s\n", (unwrap==0)? "none" : ((unwrap==1)? "normal":"reverse"));
	}
	
	rx_server_init();

#if RX_CHANS
	extint_setup();
#endif

	web_server_init(WS_INIT_START);

	if (do_gps) {
		if (!GPS_CHANS) panic("no GPS_CHANS configured");
		gps_main(argc, argv);
	}
	
	CreateTask(stat_task, NULL, MAIN_PRIORITY);

	// run periodic housekeeping functions
	while (TRUE) {
	
		TaskCollect();
		TaskCheckStacks();

		TaskSleepReasonSec("main loop", 10);
	}
}
