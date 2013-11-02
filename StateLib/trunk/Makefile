
# $Id$

# export CXXFLAGS=-Wall -ggdb -DRANGE_CHECK
export CXXFLAGS=-Wall -O3

all: libs apps

install:
	cd src; make install

clean:
	cd src; make clean
	cd examples; make clean
	cd doxy; make clean

libs: 
	cd src; make

apps:
	cd examples; make

docs:
	cd doxy; make

check:
	cd examples; make check
