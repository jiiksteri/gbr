#ifndef GBR_H__INCLUDED
#define GBR_H__INCLUDED

#include <git2.h>
#include "re.h"
#include "branch_tree.h"

struct gbr_sha {
	char sha[GIT_OID_HEXSZ+1];
};

struct gbr_dump_context {
	git_repository *repo;
	struct gbr_re *branch_re;

	/* SHA hash length for gbr_sha() */
	int abbrev;

	/* Command-specific cleanup after all iterations */
	void (*cleanup)(struct gbr_dump_context *ctx);

	/* branch tree, if used. */
	struct gbr_branch_tree *branch_tree;


	/* for normal operation (ie dump remote status */
	struct git_object *local_obj;
	const char *local_name;
	int uptodate_remotes;

	/* Should we prune local branches that are up-to-date
	 * wrt remotes.
	 */
	int prune;
};

#endif
