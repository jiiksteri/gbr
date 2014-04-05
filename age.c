
#include "gbr.h"
#include "age.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <git2.h>

static const char *gbr_ctime(char *buf, size_t sz, git_time_t *ts)
{
	struct tm tm;
	size_t len;
	/*
	 * git_time_t is 64bits but we'll have bigger
	 * issues if it overflows regular time_t
	 */
	buf[len = strftime(buf, sz, "%c", localtime_r(ts, &tm))] = '\0';
	if (len == 0) {
		snprintf(buf, sz, "%s", strerror(errno));
	}
	return buf;
}

static void dump_commit_date(git_repository *repo, const char *name, const git_oid *oid)
{
	char tbuf[32];
	git_commit *commit;
	git_time_t ts;
	int err;

	err = git_commit_lookup(&commit, repo, oid);
	if (err == 0) {
		ts = git_commit_time(commit);
		printf("%s %s", gbr_ctime(tbuf, sizeof(tbuf), &ts), name);
		git_commit_free(commit);
	} else {
		printf("%s: %s", name, giterr_last()->message);
	}
}

static void dump_date(git_repository *repo, const char *name, git_object *obj)
{
	switch (git_object_type(obj)) {
	case GIT_OBJ_COMMIT:
		dump_commit_date(repo, name, git_object_id(obj));
		break;
	default:
		printf(" unsupported object: %s", git_object_type2string(git_object_type(obj)));
		break;
	}

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
