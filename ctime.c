
#include "ctime.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

static char *ctime_ts(char *buf, int sz, git_time_t ts)
{
	struct tm tm;
	size_t len;

	/*
	 * git_time_t is 64bits but we'll have bigger
	 * issues if it overflows regular time_t
	 */
	buf[len = strftime(buf, sz, "%F %H:%M:%S %z", localtime_r(&ts, &tm))] = '\0';
	if (len == 0) {
		snprintf(buf, sz, "%s", strerror(errno));
	}
	return buf;

}

char *gbr_ctime_commit(char *buf, int sz, git_commit *commit)
{
	return ctime_ts(buf, sz, git_commit_time(commit));
}

char *gbr_ctime_object(char *buf, int sz, git_object *object)
{
	git_time_t ts;

	switch (git_object_type(object)) {
	case GIT_OBJ_COMMIT:
		ts = git_commit_time((git_commit *)object);
		break;
	default:
		ts = 0;
		break;
	}

	return ctime_ts(buf, sz, ts);
}
