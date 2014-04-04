#ifndef GBR_H__INCLUDED
#define GBR_H__INCLUDED

#include <git2.h>
#include "re.h"

struct gbr_sha {
	char sha[GIT_OID_HEXSZ+1];
};

struct gbr_dump_context {
	git_repository *repo;
	struct gbr_re *branch_re;

	/* Command-specific cleanup after all iterations */
	void (*cleanup)(struct gbr_dump_context *ctx);


	/* for normal operation (ie dump remote status */
	struct git_object *local_obj;
	const char *local_name;
	int uptodate_remotes;

};

#endif
