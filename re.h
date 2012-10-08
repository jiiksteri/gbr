#ifndef GBR_RE_H__INCLUDED
#define GBR_RE_H__INCLUDED

struct gbr_re;

int gbr_re_add(struct gbr_re **re, const char *raw);
void gbr_re_free(struct gbr_re *re);

int gbr_re_match(struct gbr_re *re, const char *needle);

#endif
