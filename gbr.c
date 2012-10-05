#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <getopt.h>

#include <git2.h>

#include "color.h"

#define HOPELESSLY_DIVERGED 100

static int abbrev_commit = 8;

struct gbr_dump_context {
	git_repository *repo;
};

struct gbr_sha {
	char sha[GIT_OID_HEXSZ+1];
};

static int gbr_repo_open(git_repository **repo)
{
	char path[GIT_PATH_MAX];
	int err;

	err = git_repository_discover(path, GIT_PATH_MAX, ".", 0, NULL);
	if (err != 0) {
		return err;
	}

	return git_repository_open(repo, path);
}

static void gbr_perror(const char *prefix)
{
	fprintf(stderr, "%s: %s\n", prefix, giterr_last()->message);
}

static const char *gbr_sha(struct gbr_sha *sha, const git_oid *oid)
{
	return git_oid_tostr(sha->sha, abbrev_commit+1, oid);
}

static git_revwalk *gbr_revwalk_setup_fromto(git_repository *repo, git_oid *base, const git_oid *head)
{
	git_revwalk *walker;

	if (git_revwalk_new(&walker, repo) != 0) {
		gbr_perror("gbr_revwalk_setup_fromto()");
		return NULL;
	}

	git_revwalk_sorting(walker, GIT_SORT_REVERSE);
	git_revwalk_push(walker, head);
	git_revwalk_hide(walker, base);

	return walker;
}

struct gbr_walk_context {
	git_oid base;
	const git_oid *o[2];
	git_oid iter[2];
	git_revwalk *w[2];
	int count[2];
	int tot[2];
	int err[2];
};

static void gbr_walk_free(struct gbr_walk_context *ctx)
{
	git_revwalk_free(ctx->w[0]);
	git_revwalk_free(ctx->w[1]);
	free(ctx);
}

struct gbr_walk_context *gbr_walk_init(git_repository *repo,
				       const git_oid *o1, const git_oid *o2)
{
	struct gbr_walk_context *ctx;
	int err;
	int i;

	ctx = malloc(sizeof(*ctx));
	if (ctx == NULL) {
		perror("gbr_walk_init()");
		return NULL;
	}

	memset(ctx, 0, sizeof(*ctx));

	err = git_merge_base(&ctx->base, repo, o1, o2);
	if (err != 0) {
		gbr_walk_free(ctx);
		gbr_perror("git_merge_base()");
		return NULL;
	}

	ctx->o[0] = o1;
	ctx->o[1] = o2;


	for (i = 0; i < 2; i++) {
		ctx->w[i] = gbr_revwalk_setup_fromto(repo, &ctx->base, ctx->o[i]);
		if (ctx->w[i] == NULL) {
			gbr_walk_free(ctx);
			return NULL;
		}
	}

	return ctx;
}


static int gbr_walk_next(struct gbr_walk_context *ctx)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (ctx->err[i] == 0) {
			ctx->err[i] = git_revwalk_next(&ctx->iter[i], ctx->w[i]);
			if (ctx->err[i] == 0) {
				ctx->tot[i]++;
			}
		}
	}

	if (ctx->err[0] == 0) {
		if (ctx->err[1] == 0) {
			/* Both had commits... */
			if (git_oid_cmp(&ctx->iter[0], &ctx->iter[1])) {
				/* ..different commits */
				ctx->count[0]++;
				ctx->count[1]++;
			}
		} else {
			/* Only first had commits */
			ctx->count[0]++;
		}
	} else if (ctx->err[1] == 0) {
		/* Only second had commits */
		ctx->count[1]++;
	}

	return
		ctx->count[0] <= HOPELESSLY_DIVERGED && ctx->count[1] <= HOPELESSLY_DIVERGED
		&& (ctx->err[0] == 0 || ctx->err[1] == 0);
}

static void dump_info(const char *branch, const char *sha, int c0, int c1)
{
	int div0, div1;
	enum color c = NONE;

	div0 = c0 > HOPELESSLY_DIVERGED;
	div1 = c1 > HOPELESSLY_DIVERGED;

        if (c0 == 0) {
                if (c1 == 0) {
                        /* Nobody's ahead */
                        c = GREEN;
                } else {
                        /* Remote is ahead */
                        c = YELLOW;
                }
        } else if (c1 > 0) {
                /* Diverged */
                c = RED;
        }

	c_fprintf(c, stdout, " %s:%s:%s%d/%s%d",
		  branch, sha,
		  div0 ? ">" : "",
		  div0 ? HOPELESSLY_DIVERGED : c0,
		  div1 ? ">" : "",
		  div1 ? HOPELESSLY_DIVERGED : c1);
}

static void do_walk(git_repository *repo, const char *branch, const git_oid *o1, const git_oid *o2)
{
	struct gbr_sha sha;
	struct gbr_walk_context *ctx;

	ctx = gbr_walk_init(repo, o1, o2);
	if (ctx == NULL) {
		return;
	}


	/* Walk from the base down towards the head of each commit and
	 * note when they start diverging.
	 */
	while (gbr_walk_next(ctx) != 0) {
		;
	}

	dump_info(branch, gbr_sha(&sha, o2),
		  ctx->count[0], ctx->count[1]);

	gbr_walk_free(ctx);
}


static void dump_matching_branch(git_repository *repo, const char *remote, const void *arg)
{
	char full_remote[512];
	const char *local = arg;
	git_object *obj;
	int err;


	snprintf(full_remote, sizeof(full_remote), "remotes/%s/%s", remote, local);

	obj = NULL;
	err = git_revparse_single(&obj, repo, full_remote);

	switch (err) {
	case GIT_ENOTFOUND:
		/* FIXME: Log this once we have --verbose */
		break;
	case 0:
		break;
	default:
		printf(" %s:%d", remote, err);
		break;
	}

	if (err == 0) {
		/* FIXME: Pass the object for the callback
		 * so we don't need the lookup.
		 */
		git_object *local_obj;
		err = git_revparse_single(&local_obj, repo, local);
		if (err == 0) {
			/* fprintf(stderr, "%s(): Looking at %s\n", __func__, local); */
			do_walk(repo, remote, git_object_id(local_obj), git_object_id(obj));
			git_object_free(local_obj);
		} else {
			/* How could the local object lookup fail _here_? */
			gbr_perror("dump_matching_branch()");
		}
	}

	git_object_free(obj);
}

static void gbr_for_each_remote(git_repository *repo,
				void (*cb)(git_repository *repo, const char *remote, const void *arg),
				const void *arg)
{
	git_strarray remotes;
	int i;

	git_remote_list(&remotes, repo);

	for (i = 0; i < remotes.count; i++) {
		cb(repo, remotes.strings[i], arg);
	}

	git_strarray_free(&remotes);
}


static int dump_branch(const char *name, git_branch_t type, void *_ctx)
{
	struct gbr_sha sha;
	struct gbr_dump_context *ctx = _ctx;
	git_object *local;
	int err;

	printf("%s", name);

	err = git_revparse_single(&local, ctx->repo, name);
	if (err == 0) {
		printf(" %s", gbr_sha(&sha, git_object_id(local)));
		gbr_for_each_remote(ctx->repo, dump_matching_branch, name);
		git_object_free(local);
	} else {
		printf(" ERROR [%d] %s", err, giterr_last()->message);
	}

	printf("\n");
	return err;
}

static void dump_version(void)
{
	int major, minor, rev;
	git_libgit2_version(&major, &minor, &rev);
	printf("gbr using libgit2 %d.%d.%d\n",
	       major, minor, rev);
}

static struct option lopts[] = {
	{
		.name = "version",
		.has_arg = no_argument,
		.flag = NULL,
		.val = 'v',
	},
	{
		.name = "abbrev",
		.has_arg = required_argument,
		.flag = NULL,
		.val = 'a'
	},
	{
		.name= NULL,
		.has_arg = 0,
		.flag = NULL,
		.val = 0,
	},
};

int main(int argc, char **argv)
{

	git_repository *repo;
	struct gbr_dump_context dump_context;
	int err;
	int ch, n;

	while ((ch = getopt_long_only(argc, argv, "", lopts, NULL)) != -1) {
		switch (ch) {
		case 'a':
			n = atoi(optarg);
			if (n < 8) {
				n = 8;
			}
			if (n > GIT_OID_HEXSZ) {
				n = GIT_OID_HEXSZ;
			}
			abbrev_commit = n;
			break;
		case 'v':
			dump_version();
			return 0;
			break;
		default:
			return EXIT_FAILURE;
		}
	}

	err = gbr_repo_open(&repo);
	if (err != 0) {
		gbr_perror("gbr_repo_open()");
		return err;
	}

	dump_context.repo = repo;
	err = git_branch_foreach(repo, GIT_BRANCH_LOCAL, dump_branch, &dump_context);
	if (err != 0) {
		gbr_perror("git_branch_foreach()");
	}

	git_repository_free(repo);
	return err;
}
