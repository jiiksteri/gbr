
#include "gbr.h"
#include "age.h"
#include "ctime.h"
#include <stdio.h>
#include <git2.h>

static void dump_date(git_repository *repo, const char *name, git_object *obj)
{
	char tbuf[32];

	printf("%s %s", gbr_ctime_object(tbuf, sizeof(tbuf), obj), name);

	printf("\n");
}


static void dump_sorted(struct gbr_dump_context *ctx)
{
	if (ctx->branch_tree) {
		gbr_branch_tree_walk(ctx->branch_tree, dump_date);
	}
}



int gbr_age(const char *name, git_branch_t type, struct gbr_dump_context *ctx)
{
	git_object *head;
	int err;

	if (ctx->cleanup == NULL) {
		ctx->cleanup = dump_sorted;
	}

	err = git_revparse_single(&head, ctx->repo, name);
	if (err == 0) {
		gbr_branch_tree_add(&ctx->branch_tree, ctx->repo, name, head);
	} else {
		printf("%s %s\n", name, giterr_last()->message);
	}

	return err;
}
