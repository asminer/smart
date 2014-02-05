## $Id$

## Makefile.am -- Process this file with automake to produce Makefile.in

_revision:
	$(srcdir)/BuildRevNo $(srcdir) 

revision.h:
	$(srcdir)/BuildRevNo $(srcdir)

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

EXTRA_DIST = BuildRevNo revision.h
