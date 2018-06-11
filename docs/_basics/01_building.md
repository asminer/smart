---
title: Building
---

## Included packages

* [MEDDLY](https://asminer.github.io/meddly/), for decision diagram support.
  This has been included with Smart since version 3.4.0.


## Packages required 

* gcc and g++, or other C/C++ compiler with support for C++11
* automake
* autoconfig
* libtool
* flex
* bison


## Packages strongly recommended 

* [GMP](https://gmplib.org), for very large integer support.


## Build Steps

### 1.  Configure

```bash
./Config-all
```

This step is necessary for the first build, and anytime
configure.ac or any Makefile.am changes.  The script performs
system-specific configuration,
and will generate a ```makefile``` in directories:

Build directory     |  Purpose
--- | ---
```bin-devel/```    |  for development and debugging executables
```bin-release/ ``` |  for production executables


The configuration script will look in a few places for libraries it needs.
If it fails to find a library, you can set certain environment variables
to help the script:

Environment variable  |  Purpose
--- | ---
```GMP_INCLUDE``` | Path containing GMP header files
```GMP_LIBRARY``` | Path containing GMP library files

For example, if you built GMP in your home directory, 
you might use
```bash
env GMP_INCLUDE=/home/username/gmp/include GMP_LIBRARY=/home/username/gmp/lib ./Config-all
```


The following arguments may be passed to the configuration script:

Argument | Description
-------- | -----------
```--without-gmp``` | Disables GMP support 


      
### 2.  Switch to one of the build directories

```bash
cd bin-devel
```
OR
```bash
cd bin-relese
```


### 3.  Run make

```bash
make
```

This builds everything.  For recent versions of automake,
you can use
```make V=0``` for silent builds (just gives a summary), and
```make V=1``` for noisy builds (gives build details).

### 4.  Run tests

```bash
make check
```
This requires automake version 1.12 or higher.
Alternatively, you can use
```bash
make check-old
```
which should always work.
    

### 5.  Run benchmarks (optional)

```bash
make bench
```
This runs timing tests.

### 6.  Install (optional)

```bash
make install
```
This copies binaries into ```bin/``` within the build directory;
otherwise they are in ```src/```.

