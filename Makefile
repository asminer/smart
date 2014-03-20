
# $Id$

all: 
	cd RngLib && make all check install
	cd SimLib && make all check install
	cd IntSets && make all check install
	cd GraphLib && make all check install
	cd LSLib && make all check install
	cd MCParse && make all check install
	cd MCLib && make all check install
	cd StateLib && make all check install
	

check:
	cd RngLib && make check
	cd SimLib && make check
	cd IntSets && make check
	cd GraphLib && make check
	cd LSLib && make check
	cd MCParse && make check
	cd MCLib && make check
	cd StateLib && make check


clean:
	cd RngLib && make clean
	cd SimLib && make clean
	cd IntSets && make clean
	cd GraphLib && make clean
	cd LSLib && make clean
	cd MCParse && make clean
	cd MCLib && make clean
	cd StateLib && make clean
	rm include/*.h lib/*


install:
	cd RngLib && make install
	cd SimLib && make install
	cd IntSets && make install
	cd GraphLib && make install
	cd LSLib && make install
	cd MCParse && make install
	cd MCLib && make install
	cd StateLib && make install


