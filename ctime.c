
#include "ctime.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

char *gbr_ctime_commit(char *buf, int sz, git_commit *commit)
{
	struct tm tm;
	git_time_t ts;
	size_t len;

	ts = git_commit_time(commit);
	/*
	 * git_time_t is 64bits but we'll have bigger
	 * issues if it overflows regular time_t
	 */
	buf[len = strftime(buf, sz, "%c", localtime_r(&ts, &tm))] = '\0';
	if (len == 0) {
		snprintf(buf, sz, "%s", strerror(errno));
	}
	return buf;
}
