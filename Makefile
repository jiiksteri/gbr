PROG = gbr
OBJS = perror.o color.o re.o ctime.o age.o branch_tree.o gbr.o

# If you have a sane libgit2 install you should drop the
# hand-coded prefix + -rpath trickery below.
#
# I have a semi-installed local copy, so questionable
# shenanigans are in order.

LIBGIT2_PREFIX=$(HOME)/libgit2
libgit2_CFLAGS = -I$(LIBGIT2_PREFIX)/include
libgit2_LDFLAGS = -L$(LIBGIT2_PREFIX)/lib -Wl,--rpath=$(LIBGIT2_PREFIX)/lib -lgit2


CFLAGS += -g -D_GNU_SOURCE -Wall -Werror $(libgit2_CFLAGS)
LDFLAGS += $(libgit2_LDFLAGS)

.PHONY: all clean test

all: $(PROG)
clean:
	$(RM) $(PROG) $(OBJS)


$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

test:
	cd test && $(MAKE)
