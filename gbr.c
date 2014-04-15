#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <getopt.h>

#include <git2.h>

#include "gbr.h"
#include "age.h"
#include "remote_delta.h"
#include "perror.h"

static int gbr_repo_open(git_repository **repo, git_buf *path)
{
	int err;

	if (path->ptr == NULL) {
		err = git_repository_discover(path, ".", 0, NULL);
		if (err != 0) {
			return err;
		}
	}

	return git_repository_open(repo, path->ptr);
}

static void dump_version(void)
{
	int major, minor, rev;
	git_libgit2_version(&major, &minor, &rev);
	printf("gbr using libgit2 %d.%d.%d\n",
	       major, minor, rev);
}

typedef int (*gbr_command_fn)(const char *name, git_branch_t type, struct gbr_dump_context *ctx);

static int gbr_branch_foreach(git_repository *repo, git_branch_t type, gbr_command_fn cb, struct gbr_dump_context *cb_data)
{
	git_branch_iterator *iter;
	git_reference *ref;
	const char *name;
	int err;

	iter = NULL;
	ref = NULL;
	err = git_branch_iterator_new(&iter, repo, type);
	while (err == 0) {
		if ((err = git_branch_next(&ref, &type, iter)) != 0) {
			break;
		}
		if ((err = git_branch_name(&name, ref)) != 0) {
			break;
		}

		if (cb_data->branch_re == NULL || gbr_re_match(cb_data->branch_re, name) == 0) {
			err = cb(name, type, cb_data);
			git_reference_free(ref);
			ref = NULL;
		}
	}

	if (cb_data->cleanup) {
		cb_data->cleanup(cb_data);
	}

	if (ref != NULL) {
		git_reference_free(ref);
	}

	if (iter != NULL) {
		git_branch_iterator_free(iter);
	}

	if (err == GIT_ITEROVER) {
		err = 0;
	}

	return err;
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
		.name = "age",
		.has_arg = no_argument,
		.flag = NULL,
		.val = 'g',
	},
	{
		.name = "remote-age",
		.has_arg = no_argument,
		.flag = NULL,
		.val = 'm',
	},
	{
		.name = "prune",
		.has_arg = no_argument,
		.flag = NULL,
		.val = 'p',
	},
	{
		.name = "repository",
		.has_arg = required_argument,
		.flag = NULL,
		.val = 'r',
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
	git_buf path = { .ptr = NULL };
	git_branch_t branch_type;
	git_repository *repo;
	gbr_command_fn command;
	struct gbr_dump_context dump_context;
	int err;
	int ch, n;
	int branches_limited;

	command = gbr_remote_delta;
	branch_type = GIT_BRANCH_LOCAL;
	memset(&dump_context, 0, sizeof(dump_context));
	dump_context.abbrev = 8;

	err = 0;
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
			dump_context.abbrev = n;
			break;
		case 'v':
			dump_version();
			return 0;
			break;
		case 'p':
			dump_context.prune++;
			break;
		case 'r':
			git_buf_set(&path, optarg, strlen(optarg) + 1);
			break;
		case 'm':
			branch_type = GIT_BRANCH_REMOTE;
		case 'g':
			command = gbr_age;
			break;
		default:
			return EXIT_FAILURE;
		}
	}

	branches_limited = 0;
	while (optind < argc) {
		err = gbr_re_add(&dump_context.branch_re, argv[optind++]);
		if (err != 0) {
			gbr_re_free(dump_context.branch_re);
			git_buf_free(&path);
			return err;
		}
		branches_limited++;
	}

	/*
	 * Don't allow pruning unless there's a branch-limiting regexp.
	 * This is just a safety measure.
	 */
	if (dump_context.prune > 0 && branches_limited == 0) {
		fprintf(stderr, "Ignoring --prune as branches are not limited\n");
		dump_context.prune = 0;
	}

	err = gbr_repo_open(&repo, &path);
	if (err != 0) {
		gbr_perror("gbr_repo_open()");
		git_buf_free(&path);
		return err;
	}

	dump_context.repo = repo;
	err = gbr_branch_foreach(repo, branch_type, command, &dump_context);
	if (err != 0) {
		gbr_perror("git_branch_foreach()");
	}

	git_repository_free(repo);
	gbr_re_free(dump_context.branch_re);
	if (dump_context.branch_tree) {
		gbr_branch_tree_destroy(dump_context.branch_tree);
	}
	git_buf_free(&path);
	return err;
}
