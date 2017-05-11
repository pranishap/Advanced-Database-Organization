

all: assign2_1

buffer_mgr_stat.o: buffer_mgr_stat.c
	gcc -o buffer_mgr_stat.o -c buffer_mgr_stat.c

buffer_mgr.o: buffer_mgr.c
	gcc -o buffer_mgr.o -c buffer_mgr.c
	
dberror.o: dberror.c
	gcc -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	gcc -o storage_mgr.o -c storage_mgr.c

test_assign2_1.o: test_assign2_1.c
	gcc -o test_assign2_1.o -c test_assign2_1.c

assign2_1: test_assign2_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o
	gcc -o assign2_1 test_assign2_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o -I.

clean:
	rm assign2_1  test_assign2_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o