#ifndef AGE_H__INCLUDED
#define AGE_H__INCLUDED

#include <git2.h>

int gbr_age(const char *name, git_branch_t type, struct gbr_dump_context *ctx);

#endif
