CC= g++
all: test_fanom test_lucky777
test_fanom: test_fanom.cpp check.inc.cpp fanom_hash.h fanom_hash32.h Makefile
	$(CC) -std=gnu++11 -O3 -ggdb3 test_fanom.cpp -o test_fanom
test_lucky777: test_lucky777.cpp check.inc.cpp lucky777.h Makefile
	$(CC) -std=gnu++11 -O3 -ggdb3 test_lucky777.cpp -o test_lucky777
