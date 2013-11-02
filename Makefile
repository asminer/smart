
# $Id$

all: 
	cd RngLib/trunk && make all check install
	cd SimLib/trunk && make all check install
	cd IntSets/trunk && make all check install
	cd GraphLib/trunk && make all check install
	cd LSLib/trunk && make all check install
	cd MCLib/trunk && make all check install
	cd StateLib/trunk && make all check install
	

check:
	cd RngLib/trunk && make check
	cd SimLib/trunk && make check
	cd IntSets/trunk && make check
	cd GraphLib/trunk && make check
	cd LSLib/trunk && make check
	cd MCLib/trunk && make check
	cd StateLib/trunk && make check


clean:
	cd RngLib/trunk && make clean
	cd SimLib/trunk && make clean
	cd IntSets/trunk && make clean
	cd GraphLib/trunk && make clean
	cd LSLib/trunk && make clean
	cd MCLib/trunk && make clean
	cd StateLib/trunk && make clean
	rm include/*.h lib/*


install:
	cd RngLib/trunk && make install
	cd SimLib/trunk && make install
	cd IntSets/trunk && make install
	cd GraphLib/trunk && make install
	cd LSLib/trunk && make install
	cd MCLib/trunk && make install
	cd StateLib/trunk && make install


