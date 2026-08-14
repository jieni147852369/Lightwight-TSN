#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/ptp_clock.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#ifndef FD_TO_CLOCKID
#define FD_TO_CLOCKID(fd) ((((clockid_t) ~((fd))) << 3) | 3)
#endif

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s [--phc /dev/ptpX]               # 读取 PHC 时间\n"
            "  %s [--phc /dev/ptpX] --set-now     # 将 PHC 对齐到当前系统时间\n"
            "  %s [--phc /dev/ptpX] --set-abs-ns <ns>  # 绝对设置 PHC 时间 (ns)\n"
            "  %s [--phc /dev/ptpX] --add-ns <ns>      # 在 PHC 时间上加/减偏移 (ns)\n",
            prog, prog, prog, prog);
}

static uint64_t timespec_to_ns(const struct timespec *ts)
{
    return (uint64_t)ts->tv_sec * 1000000000ULL + (uint64_t)ts->tv_nsec;
}

static struct timespec ns_to_timespec(int64_t ns)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ns / 1000000000LL);
    ts.tv_nsec = (long)(ns % 1000000000LL);
    if (ts.tv_nsec < 0) {
        ts.tv_nsec += 1000000000L;
        ts.tv_sec -= 1;
    }
    return ts;
}

int main(int argc, char **argv)
{
    const char *phc_path = "/dev/ptp0";
    enum { MODE_READ = 0, MODE_SET_NOW, MODE_SET_ABS, MODE_ADD } mode = MODE_READ;
    int64_t value_ns = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--phc") == 0 && i + 1 < argc) {
            phc_path = argv[++i];
        } else if (strcmp(argv[i], "--set-now") == 0) {
            mode = MODE_SET_NOW;
        } else if (strcmp(argv[i], "--set-abs-ns") == 0 && i + 1 < argc) {
            mode = MODE_SET_ABS;
            value_ns = strtoll(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--add-ns") == 0 && i + 1 < argc) {
            mode = MODE_ADD;
            value_ns = strtoll(argv[++i], NULL, 10);
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    int fd = open(phc_path, O_RDWR);
    if (fd < 0) {
        perror("open phc");
        return 1;
    }

    struct ptp_clock_caps caps;
    if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps) != 0) {
        perror("PTP_CLOCK_GETCAPS (不是有效的 PHC?)");
        close(fd);
        return 1;
    }

    const clockid_t clkid = FD_TO_CLOCKID(fd);
    struct timespec ts;

    if (mode == MODE_SET_NOW) {
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("clock_gettime(CLOCK_REALTIME)");
            close(fd);
            return 1;
        }
        if (clock_settime(clkid, &ts) != 0) {
            perror("clock_settime(PHC, set-now)");
            close(fd);
            return 1;
        }
        printf("已将 %s 对齐到系统时间。\n", phc_path);
    } else if (mode == MODE_SET_ABS) {
        ts = ns_to_timespec(value_ns);
        if (clock_settime(clkid, &ts) != 0) {
            perror("clock_settime(PHC, abs)");
            close(fd);
            return 1;
        }
        printf("已将 %s 设置为绝对时间 %" PRId64 " ns。\n", phc_path, value_ns);
    } else if (mode == MODE_ADD) {
        if (clock_gettime(clkid, &ts) != 0) {
            perror("clock_gettime(PHC)");
            close(fd);
            return 1;
        }
        int64_t ns = (int64_t)timespec_to_ns(&ts) + value_ns;
        ts = ns_to_timespec(ns);
        if (clock_settime(clkid, &ts) != 0) {
            perror("clock_settime(PHC, add)");
            close(fd);
            return 1;
        }
        printf("已在 %s 上调整偏移 %" PRId64 " ns。\n", phc_path, value_ns);
    }

    if (clock_gettime(clkid, &ts) != 0) {
        perror("clock_gettime(PHC)");
        close(fd);
        return 1;
    }

    printf("当前 PHC(%s) 时间: %jd.%09ld (ns=%" PRIu64 ")\n",
           phc_path,
           (intmax_t)ts.tv_sec,
           ts.tv_nsec,
           timespec_to_ns(&ts));

    struct timespec sys_now;
    if (clock_gettime(CLOCK_REALTIME, &sys_now) == 0) {
        printf("当前系统时间:       %jd.%09ld (ns=%" PRIu64 ")\n",
               (intmax_t)sys_now.tv_sec,
               sys_now.tv_nsec,
               timespec_to_ns(&sys_now));
    } else {
        perror("clock_gettime(CLOCK_REALTIME)");
    }

    close(fd);
    return 0;
}
