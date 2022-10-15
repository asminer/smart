
## Makefile.am -- Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src Tests

check-old:
	cd Tests; make check-old

bench:
	cd Tests; make bench

release: bin/smart
	cp bin/smart smart.`bin/smart | awk '/SMART/{print $$3}'`.`gcc -dumpmachine`
