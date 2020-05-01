CC=gcc
CFLAGS=-I.

all : test4 testexpr

DEPS = dberror.h storage_mgr.h test_helper.h buffer_mgr_stat.h buffer_mgr.h dt.h expr.h record_mgr.h btree_mgr.h tables.h
OBJ1 = dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o record_mgr.o rm_serializer.o btree_mgr.o test_assign4_1.o
OBJ2 = dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o record_mgr.o rm_serializer.o btree_mgr.o test_expr.o

%.o: %.c $(DEPS)
	$(CC) -g -O0 -c -o $@ $< $(CFLAGS)

test4: $(OBJ1)
	$(CC) -o $@ $^ $(CFLAGS)

testexpr: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean :
	del -rf test4.exe 
	del -rf testexpr.exe 
	del -rf *.o 

