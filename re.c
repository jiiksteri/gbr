#include "re.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <sys/types.h>

struct gbr_re {
	struct gbr_re *next;
	regex_t re;
};

int gbr_re_add(struct gbr_re **rep, const char *raw)
{
	int err;

	struct gbr_re *re;

	if ((re = malloc(sizeof(*re))) == NULL) {
		fprintf(stderr, "%s: Memory allocation failure\n", __func__);
		return 1;
	}
	memset(re, 0, sizeof(*re));

	err = regcomp(&re->re, raw, REG_EXTENDED|REG_NEWLINE|REG_NOSUB);
	if (err != 0) {
		fprintf(stderr, "Invalid regular expression: %s\n", raw);
		free(re);
		return err;
	}

	if (*rep) {
		re->next = *rep;
	}
	*rep = re;

	return 0;
}

int gbr_re_match(struct gbr_re *re, const char *needle)
{
	int res = REG_NOMATCH;

	while (re != NULL && res == REG_NOMATCH) {
		res = regexec(&re->re, needle, 0, NULL, 0);
		re = re->next;
	}

	return res;
}

void gbr_re_free(struct gbr_re *rep)
{
	struct gbr_re *next;
	while (rep) {
		regfree(&rep->re);
		next = rep->next;
		free(rep);
		rep = next;
	}
}


