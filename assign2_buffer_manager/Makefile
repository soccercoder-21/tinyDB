CC=gcc
CFLAGS=-I.
DEPS = dberror.h storage_mgr.h test_helper.h buffer_mgr_stat.h buffer_mgr.h dt.h
OBJ1 = test_assign2_1.o dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o

%.o: %.c $(DEPS)
	$(CC) -g -O0 -c -o $@ $< $(CFLAGS)

test21: $(OBJ1)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean :
	del -rf test_assign1_1.o
	del -rf test_assign1_2.o
	del -rf test_assign2_1.o
	del -rf test_assign2_2.o
	del -rf dberror.o
	del -rf storage_mgr.o
	del -rf buffer_mgr.o
	del -rf buffer_mgr_stat.o
	del -rf test22.exe
	del -rf test21.exe
	del -rf test_pagefile.bin
	del -rf testbuffer.bin
	del -rf test_assign2_1.exe

run_test21:
	./test21


