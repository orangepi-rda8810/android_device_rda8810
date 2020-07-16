/*
 * Copyright (C) 2015 RDA Microsystems
 *
 * comreg0 debugger.
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


#define TEST_SYSFS_TRIGGER      "/sys/devices/platform/rda-comreg0.0/magic"
#define SYSFS_MODEM_CRASH_EVENT "/sys/devices/platform/rda-comreg0.0/modem_crash"

int main(int argc, const char *argv[])
{
    int cnt, f, rv;
    char attributes[100];
    struct pollfd ufds[1];

    if ((f = open(SYSFS_MODEM_CRASH_EVENT, O_RDONLY)) < 0)
    {
        perror("Unable to open notify");
        exit(1);
    }
    cnt = read(f, attributes, 100);

    ufds[0].fd = f;
    ufds[0].events = POLLPRI|POLLERR;
    ufds[0].revents = 0;
    if (( rv = poll( ufds, 1, 10000)) < 0) {
        perror("poll error");
    }
    else if (rv == 0) {
        printf("Timeout occurred!\n");
    }
    else if (ufds[0].revents & (POLLIN|POLLPRI|POLLERR)) {
        printf("triggered\n");
        cnt = read(f, attributes, 3);
        printf("Attribute file value: %s [%d]\n", attributes, cnt);
    }
    printf("revents[0]: %08X\n", ufds[0].revents);

    close(f);

    return 0;
}

