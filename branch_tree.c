
#include "branch_tree.h"

#include <errno.h>
#include <string.h>
#include <search.h>

struct gbr_branch_tree {
	void *root;
	gbr_branch_visit_fn visit;
};

struct node {
	struct gbr_branch_tree *tree;
	git_repository *repo;

	git_object *object;
	git_time_t commit_date;

	char *branch;
};

static int ensure_tree(struct gbr_branch_tree **rootp)
{
	if (*rootp == NULL) {
		*rootp = malloc(sizeof(**rootp));
		if (*rootp == NULL) {
			return errno;
		}
		memset(*rootp, 0, sizeof(**rootp));
	}
	return 0;
}

static git_time_t commit_date(git_repository *repo, git_object *object)
{
	git_time_t ts = 0;

	if (git_object_type(object) == GIT_OBJ_COMMIT) {
		ts = git_commit_time((git_commit *)object);
	}

	return ts;
}

static int compare_node(const void *_n1, const void *_n2)
{
	const struct node *n1 = _n1;
	const struct node *n2 = _n2;
	return n2->commit_date - n1->commit_date;
}

int gbr_branch_tree_add(struct gbr_branch_tree **rootp, git_repository *repo, const char *branch, git_object *object)
{
	struct node *node;
	struct node **stored;
	int err;

	if ((err = ensure_tree(rootp)) != 0) {
		return err;
	}

	node = malloc(sizeof(*node));
	if (node == NULL) {
		return ENOMEM;
	}
	memset(node, 0, sizeof(*node));

	node->object = object;
	node->commit_date = commit_date(repo, object);

	stored = tsearch(node, &(*rootp)->root, compare_node);
	if (stored != NULL && *stored == node) {
		node->repo = repo;
		node->branch = strdup(branch);
		node->tree = *rootp;
	} else {
		/* This object was not added as it was already there
		 * or an error happened. The caller expects us us to
		 * own the object they passed in and free it at
		 * dump_context->cleanup(). But as we didn't add it,
		 * free it here to avoid a leak.
		 */
		git_object_free(object);

		free(node);
	}

	return 0;
}

static void node_destroy(void *_node)
{
	struct node *node = _node;
	git_object_free(node->object);
	free(node->branch);
	free(node);
}

void gbr_branch_tree_destroy(struct gbr_branch_tree *tree)
{
	tdestroy(tree->root, node_destroy);
	free(tree);
}

static void walk_cb(const void *nodep, const VISIT visit, const int depth)
{
	const struct node *node = *(struct node **)nodep;

	switch (visit) {
	case leaf:
	case postorder:
		node->tree->visit(node->repo, node->branch, node->object);
		break;
	default:
		break;
	}
}

void gbr_branch_tree_walk(struct gbr_branch_tree *tree, gbr_branch_visit_fn walk)
{
	tree->visit = walk;
	twalk(tree->root, walk_cb);
}
