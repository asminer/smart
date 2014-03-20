
# $Id$

# export CXXFLAGS=-ggdb -Wall
export CXXFLAGS=-O3 -Wall

all: libs apps

install:
	cd src; make install

clean:
	cd src; make clean

libs: 
	cd src; make

apps:
	cd examples; make

check:
