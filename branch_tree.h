#ifndef BRANCH_TREE_H__INCLUDED
#define BRANCH_TREE_H__INCLUDED

#include <git2.h>

struct gbr_branch_tree;

typedef void (*gbr_branch_visit_fn)(git_repository *repo, const char *name, git_object *obj);

int gbr_branch_tree_add(struct gbr_branch_tree **treep, git_repository *repo, const char *branch, git_object *object);

void gbr_branch_tree_destroy(struct gbr_branch_tree *tree);

void gbr_branch_tree_walk(struct gbr_branch_tree *tree, gbr_branch_visit_fn walk);

#endif
