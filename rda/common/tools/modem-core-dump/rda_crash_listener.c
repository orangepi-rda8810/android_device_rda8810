/*
 * Copyright (C) 2015 RDA Microsystems
 *
 * Modem crash listner.
 * Wait for a crash event and return the name of the cpu that crashed.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <poll.h>

#define LOG_NDEBUG 0
#define LOG_TAG "RDAModemCoreDumper"
#undef LOG
#include <utils/Log.h>

#define DEBUG(...) do { if (debug) fprintf(stderr,__VA_ARGS__); } while(0)

#define EVENT_BUF_SIZE  60
#define EVENT_LEN      (4+4) // xcpuwcpu

#define WAIT_EVENTS \
	"xcpu"          \
	"wcpu"          \

static const char doc[] = 

" Usage rda_crash_listner [OPTIONS] [Crash sysfs node]\n"
"\n"
"  OPTIONS\n"
"	-t <timeout> : polling intervals\n"
"	-d           : enable debug\n";


#define SYSFS_MODEM_CRASH_EVENT "/sys/devices/platform/rda-comreg0.0/modem_crash"

int main(int argc, char *argv[])
{
	int opt, cnt, f, rv;
	char event[EVENT_BUF_SIZE+4];
	struct pollfd ufds[1];
	/* Options: */
	const char *event_file = SYSFS_MODEM_CRASH_EVENT;
	int msecs = -1; // infinite timeout
	int debug = 0;

	while ((opt = getopt(argc, argv, "dht:")) != -1) {
		switch (opt) {
		case 't':
			msecs = atoi(optarg);
			DEBUG("timeout %d\n",msecs);
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
		default:
			printf(doc);
			return 0;
		}
	}
	if (optind < argc) {
		event_file = argv[optind];
	}
	DEBUG("Using %s as event source\n", event_file);
	
	if ((f = open(event_file, O_RDONLY)) < 0)
	{
		perror("modem_crash event");
		return 1;
	}

	/* flush */
	cnt = read(f, event, EVENT_BUF_SIZE);

	/* setup poll */
	ufds[0].fd = f;
	ufds[0].events = POLLPRI;

	/* poll until we get a "xcpuwcpu", "wcpu" or "xcpu" event */
	do {
		cnt = 0;
		ufds[0].revents = 0;

		if ((rv = poll(ufds, 1, msecs)) < 0) {
			perror("poll error");
			return 1;
		}

		lseek(f, 0, SEEK_SET);
		if (rv == 0) {
			DEBUG("Timeout occurred!\n");
			cnt = read(f, event, EVENT_BUF_SIZE);
		}
		else if (ufds[0].revents & (POLLPRI)) {
			DEBUG("triggered\n");
			cnt = read(f, event, EVENT_BUF_SIZE);
		}
		else if (ufds[0].revents & (POLLERR)) {
			DEBUG("Error\n");
			return 1;
		}
		else if (ufds[0].revents & (POLLIN)) {
			cnt = read(f, event, EVENT_BUF_SIZE);
		}
		// Terminate the event string:
		if (cnt != 0 && event[cnt-1] == '\n')
			--cnt;
		event[cnt] = 0;
		event[EVENT_LEN] = 0;

		DEBUG("Attribute file value: %s [%d]\n", event, cnt);
		DEBUG("revents[0]: %08X\n", ufds[0].revents);

	} while (*event==0 || strstr(WAIT_EVENTS, event) == NULL);

	/* return the event */
	printf("%s\n", event);
	
	close(f);
	return 0;
}

