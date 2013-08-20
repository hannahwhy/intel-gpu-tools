#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "rc6.h"

int rc6_init(struct rc6 *rc6)
{
	struct stat st;
	int fd;

	memset(rc6, 0, sizeof(*rc6));

	fd = stat("/sys/class/drm/card0/power", &st);
	if (fd == -1)
		return rc6->error = errno;

	return 0;
}

static uint64_t file_to_u64(const char *path)
{
	char buf[4096];
	int fd, len;

	fd = open(path, 0);
	if (fd < 0)
		return 0;

	len = read(fd, buf, sizeof(buf)-1);
	close(fd);

	if (len < 0)
		return 0;

	buf[len] = '\0';

	return strtoull(buf, 0, 0);
}

static uint64_t clock_ms_to_u64(void)
{
	struct timespec tv;

	if (clock_gettime(CLOCK_MONOTONIC, &tv) < 0)
		return 0;

	return (uint64_t)tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}

int rc6_update(struct rc6 *rc6)
{
	struct rc6_stat *s = &rc6->stat[rc6->count++&1];
	struct rc6_stat *d = &rc6->stat[rc6->count&1];
	uint64_t d_time, d_rc6, d_rc6p, d_rc6pp;
	struct stat st;

	if (rc6->error)
		return rc6->error;

	if (stat("/sys/class/drm/card0/power", &st) < 0)
		return rc6->error = errno;

	if (stat("/sys/class/drm/card0/power/rc6_residency_ms", &st) < 0)
		return ENOENT;

	s->rc6_residency = file_to_u64("/sys/class/drm/card0/power/rc6_residency_ms");
	s->rc6p_residency = file_to_u64("/sys/class/drm/card0/power/rc6p_residency_ms");
	s->rc6pp_residency = file_to_u64("/sys/class/drm/card0/power/rc6pp_residency_ms");
	s->timestamp = clock_ms_to_u64();

	if (rc6->count == 1)
		return EAGAIN;

	d_time = s->timestamp - d->timestamp;

	d_rc6 = s->rc6_residency - d->rc6_residency;
	rc6->rc6 = 100 * d_rc6 / d_time;

	d_rc6p = s->rc6p_residency - d->rc6p_residency;
	rc6->rc6p = 100 * d_rc6p / d_time;

	d_rc6pp = s->rc6pp_residency - d->rc6pp_residency;
	rc6->rc6pp = 100 * d_rc6pp / d_time;

	rc6->rc6_combined = 100 * (d_rc6 + d_rc6p + d_rc6pp) / d_time;
	return 0;
}