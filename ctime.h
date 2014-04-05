#ifndef CTIME_H__INCLUDED

#include <git2.h>

char *gbr_ctime_commit(char *buf, int sz, git_commit *commit);

#endif
