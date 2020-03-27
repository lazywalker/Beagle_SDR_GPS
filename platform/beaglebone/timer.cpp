#include "types.h"
#include "config.h"
#include "kiwi.h"
#include "timer.h"
#include "misc.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static bool init = false;
static u4_t epoch_sec;
static time_t server_build_unix_time, server_start_unix_time;

static void set_epoch()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	epoch_sec = ts.tv_sec;
	
	time(&server_start_unix_time);
	
	const char *server = background_mode? "/usr/local/bin/kiwid" : (BUILD_DIR "/kiwi.bin");
	struct stat st;
	scall("stat kiwi server", stat(server, &st));
	server_build_unix_time = st.st_mtime;
	
	init = true;
}

u4_t timer_epoch_sec()
{
	if (!init) set_epoch();
	return epoch_sec;
}

time_t timer_server_build_unix_time()
{
	if (!init) set_epoch();
	return server_build_unix_time;
}

time_t timer_server_start_unix_time()
{
	if (!init) set_epoch();
	return server_start_unix_time;
}

// overflows 136 years after timer epoch
u4_t timer_sec()
{
	struct timespec ts;

	if (!init) set_epoch();
	clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec - epoch_sec;
}

// overflows 49.7 days after timer epoch
u4_t timer_ms()
{
	struct timespec ts;

	if (!init) set_epoch();
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int msec = ts.tv_nsec/1000000;
	assert(msec >= 0 && msec < 1000);
    return (ts.tv_sec - epoch_sec)*1000 + msec;
}

// overflows 1.2 hours after timer epoch
u4_t timer_us()
{
	struct timespec ts;

	if (!init) set_epoch();
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int usec = ts.tv_nsec / 1000;
	assert(usec >= 0 && usec < 1000000);
    return (ts.tv_sec - epoch_sec)*1000000 + usec;	// ignore overflow
}

// never overflows (effectively)
u64_t timer_us64()
{
	struct timespec ts;
	u64_t t;

	if (!init) set_epoch();
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int usec = ts.tv_nsec / 1000;
	assert(usec >= 0 && usec < 1000000);
	t = ts.tv_sec - epoch_sec;
	t *= 1000000;
	t += usec;
    return t;
}

time_t utc_time()
{
	time_t t; time(&t);
	return t;
}

void utc_hour_min_sec(int *hour, int *min, int *sec)
{
	time_t t; time(&t);
	time_hour_min_sec(t, hour, min, sec);
}

void time_hour_min_sec(time_t t, int *hour, int *min, int *sec)
{
	struct tm tm; gmtime_r(&t, &tm);
	if (hour) *hour = tm.tm_hour;
	if (min) *min = tm.tm_min;
	if (sec) *sec = tm.tm_sec;
}

void utc_year_month_day(int *year, int *month, int *day)
{
	time_t t; time(&t);
	struct tm tm; gmtime_r(&t, &tm);
	if (year) *year = tm.tm_year;
	if (month) *month = tm.tm_mon + 1;
	if (day) *day = tm.tm_mday;
}
    
char *var_ctime_static(time_t *t)
{
    char *tb = asctime(gmtime(t));
    tb[CTIME_R_NL] = '\0';      // replace ending \n with \0
    return tb;
}

char *utc_ctime_static()
{
    time_t t; time(&t);
    char *tb = asctime(gmtime(&t));
    tb[CTIME_R_NL] = '\0';      // replace ending \n with \0
    return tb;
}

void utc_ctime_r(char *tb)
{
    time_t t; time(&t);
    asctime_r(gmtime(&t), tb);
    tb[CTIME_R_NL] = '\0';      // replace ending \n with \0
}

int utc_time_since_2018() {
    static time_t utc_time_2018;
    
    if (!utc_time_2018) {
        struct tm tm;
        memset(&tm, 0, sizeof (tm));
        tm.tm_isdst = 0;
        tm.tm_yday = 0;     // Jan 1
        tm.tm_wday = 1;     // Monday
        tm.tm_year = 118;   // 2018
        tm.tm_mon = 0;      // Jan
        tm.tm_mday = 1;     // Jan 1
        tm.tm_hour = 0;     // midnight
        tm.tm_min = 0;
        tm.tm_sec = 0;
        utc_time_2018 = timegm(&tm);
    }
    
    return (utc_time() - utc_time_2018);
}
