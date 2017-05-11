

all: assign3_1 test_expr_1

test_expr.o: test_expr.c
	gcc -o test_expr.o -c test_expr.c
	
rm_serializer.o: rm_serializer.c
	gcc -o rm_serializer.o -c rm_serializer.c
	
expr.o: expr.c
	gcc -o expr.o -c expr.c
	
record_mgr.o: record_mgr.c
	gcc -o record_mgr.o -c record_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c
	gcc -o buffer_mgr_stat.o -c buffer_mgr_stat.c

buffer_mgr.o: buffer_mgr.c
	gcc -o buffer_mgr.o -c buffer_mgr.c
	
dberror.o: dberror.c
	gcc -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	gcc -o storage_mgr.o -c storage_mgr.c

test_assign3_1.o: test_assign3_1.c
	gcc -o test_assign3_1.o -c test_assign3_1.c

assign3_1: test_assign3_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o 
	gcc -o assign3_1 test_assign3_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o  -I.

test_expr_1: test_expr.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o 
	gcc -o test_expr_1 test_expr.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o  -I.


clean:
	rm assign3_1  test_assign3_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o test_expr.o
	rm test_expr_1  test_expr.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o rm_serializer.o expr.o record_mgr.o test_expr.o

