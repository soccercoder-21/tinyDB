CC=gcc
CFLAGS=-I.
DEPS = dberror.h storage_mgr.h test_helper.h
OBJ = test_assign1_2.o dberror.o storage_mgr.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test11: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean :
	del -rf test_assign1_1.o
	del -rf test_assign1_2.o
	del -rf dberror.o
	del -rf storage_mgr.o
	del -rf test11.exe
	del -rf test_pagefile.bin

run_test11:
	./test11

	
