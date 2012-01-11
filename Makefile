# Makefile for idle3-tools

# DESTDIR is for non root installs (eg packages, NFS) only!
DESTDIR =

binprefix = 
manprefix = /usr
exec_prefix = $(binprefix)/
sbindir = $(exec_prefix)sbin
mandir = $(manprefix)/share/man
oldmandir = $(manprefix)/man

CC = gcc
STRIP = strip

CFLAGS := -g -O2 -W -Wall -Wbad-function-cast -Wcast-align -Wpointer-arith -Wcast-qual -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -fkeep-inline-functions -Wwrite-strings -Waggregate-return -Wnested-externs -Wtrigraphs $(CFLAGS)

LDFLAGS = -s
#LDFLAGS = -s -static
INSTALL = install
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_DIR = $(INSTALL) -m 755 -d
INSTALL_PROGRAM = $(INSTALL)

OBJS = idle3ctl.o sgio.o

all: idle3ctl

idle3ctl: $(OBJS)
	$(CC) $(LDFLAGS) -o idle3ctl $(OBJS)
	#$(STRIP) idle3ctl

idle3ctl.o: idle3ctl.c sgio.h

sgio.o:	sgio.c sgio.h

install: all idle3ctl.8
	if [ ! -z $(DESTDIR) ]; then $(INSTALL_DIR) $(DESTDIR) ; fi
	if [ ! -z $(DESTDIR)$(sbindir) ]; then $(INSTALL_DIR) $(DESTDIR)$(sbindir) ; fi
	if [ ! -z $(DESTDIR)$(mandir) ]; then $(INSTALL_DIR) $(DESTDIR)$(mandir) ; fi
	if [ ! -z $(DESTDIR)$(mandir)/man8/ ]; then $(INSTALL_DIR) $(DESTDIR)$(mandir)/man8/ ; fi
	if [ -f $(DESTDIR)$(sbindir)/idle3ctl ]; then rm -f $(DESTDIR)$(sbindir)/idle3ctl ; fi
	if [ -f $(DESTDIR)$(mandir)/man8/idle3ctl.8 ]; then rm -f $(DESTDIR)$(mandir)/man8/idle3ctl.8 ;\
	elif [ -f $(DESTDIR)$(oldmandir)/man8/idle3ctl.8 ]; then rm -f $(DESTDIR)$(oldmandir)/man8/idle3ctl.8 ; fi
	$(INSTALL_PROGRAM) -D idle3ctl $(DESTDIR)$(sbindir)/idle3ctl
	if [ -d $(DESTDIR)$(mandir) ]; then $(INSTALL_DATA) -D idle3ctl.8 $(DESTDIR)$(mandir)/man8/idle3ctl.8 ;\
	elif [ -d $(DESTDIR)$(oldmandir) ]; then $(INSTALL_DATA) -D idle3ctl.8 $(DESTDIR)$(oldmandir)/man8/idle3ctl.8 ; fi

clean:
	-rm -f idle3ctl $(OBJS) core 2>/dev/null

