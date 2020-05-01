CC=gcc
CFLAGS=-I.

all : test3 testexpr

DEPS = dberror.h storage_mgr.h test_helper.h buffer_mgr_stat.h buffer_mgr.h dt.h expr.h record_mgr.h tables.h
OBJ1 = dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o record_mgr.o rm_serializer.o test_assign3_1.o
OBJ2 = dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o record_mgr.o rm_serializer.o test_expr.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test3: $(OBJ1)
	$(CC) -o $@ $^ $(CFLAGS)

testexpr: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean :
	del -rf test3.exe 
	del -rf testexpr.exe 
	del -rf test_table_t
	del -rf test_table_r
	del -rf *.o 

