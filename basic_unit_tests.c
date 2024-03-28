#include "acutest.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"


#define MY_STR "hello world\n"
#define MY_STR2 "world hello\n"

#define MIN_FILE_SIZE 4

void basic_test()
{
	file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(id, buff, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR, sizeof(MY_STR)) == 0);
}

void write_too_big_test()
{
	file_id id;
	hel_ret ret;
	char buff[MEM_SIZE * 2];

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(buff, sizeof(buff), &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = create_and_write(buff, MEM_SIZE, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = create_and_write(buff, MEM_SIZE - MIN_FILE_SIZE + 1, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = create_and_write(buff, MEM_SIZE - MIN_FILE_SIZE, &id);
	TEST_ASSERT_(ret == 0, "got error - %d", ret);
}

void write_exact_size_test()
{
	file_id id;
	hel_ret ret;
	char buff[MEM_SIZE];
	char out_buff[sizeof(buff)];
	int size_to_write = MEM_SIZE - MIN_FILE_SIZE;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(buff, size_to_write, &id);
	TEST_ASSERT_(ret == 0, "got error - %d", ret);

	ret = read_file(id, out_buff, size_to_write);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, out_buff, size_to_write) == 0);
}

void read_out_of_boundaries_test()
{
	file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(id, buff, sizeof(MY_STR) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);
}

void read_part_of_file_test()
{
	file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;
	int size_to_read = sizeof(MY_STR) - 1;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(id, buff, size_to_read);
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR, size_to_read) == 0);
}

void write_read_multiple_files()
{
	file_id id1, id2;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), &id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = create_and_write(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(id1, buff, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR, sizeof(MY_STR)) == 0);

	ret = read_file(id2, buff, sizeof(MY_STR2) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);

	ret = read_file(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);
}

void null_params_test()
{
	hel_ret ret;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), NULL);
	TEST_ASSERT_(ret == hel_param_err, "expected error hel_param_err-%d but got %d", hel_param_err, ret);
}

TEST_LIST = {
    { "basic-test", basic_test },
	{ "write_too_big_test", write_too_big_test},
	{ "write_exact_size_test", write_exact_size_test},
	{ "read_out_of_boundaries_test", read_out_of_boundaries_test},
	{ "read_part_of_file_test", read_part_of_file_test},
	{ "write_read_multiple_files", write_read_multiple_files},
	{ "null_params_test", null_params_test},
    { NULL, NULL }
};