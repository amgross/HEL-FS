#include "acutest.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MY_STR "hello world\n"

#define MEM_SIZE 0x100

static uint8_t mem_buff[MEM_SIZE];

typedef struct my_code
{
	int size;
	char data[];
}file;

int init_fs()
{
	memset(mem_buff, 0, MEM_SIZE);
	
	return 0;
}

file *find_empty_place(int size)
{
	for(file *curr_file = (file *)mem_buff;; curr_file = (file *)((uint8_t *)curr_file + sizeof(file) + curr_file->size))
	{
		// TODO, ensure no wrap around
		if((uint8_t *)curr_file + sizeof(file) + size > mem_buff + MEM_SIZE)
		{
			return NULL;
		}

		if(curr_file->size == 0)
		{
			return curr_file;
		}
	}
}

int create_and_write(char *in, int size)
{
	file *new_file = find_empty_place(size);
	if(new_file == NULL)
	{
		return -1;
	}

	new_file->size = size;

	memcpy(new_file->data, in, size);

	return 0;
}

int read_file(char *out, int size)
{
	file *read_file = (file *)mem_buff;
	if(read_file->size < size)
	{
		return -1;
	}

	memcpy(out, read_file->data, size);
	return 0;
}

/////////////////////

void basic_test()
{
	int ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(buff, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR, sizeof(MY_STR)) == 0);
}

void write_to_big_test()
{
	int ret;
	char buff[MEM_SIZE * 2];

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(buff, sizeof(buff));
	TEST_ASSERT_(ret == -1, "expected error -1 but got %d", ret);

	ret = create_and_write(buff, MEM_SIZE);
	TEST_ASSERT_(ret == -1, "expected error -1 but got %d", ret);

	ret = create_and_write(buff, MEM_SIZE - sizeof(file) + 1);
	TEST_ASSERT_(ret == -1, "expected error -1 but got %d", ret);
}

void write_exact_size_test()
{
	int ret;
	char buff[MEM_SIZE];
	char out_buff[sizeof(buff)];
	int size_to_write = MEM_SIZE - sizeof(file);

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(buff, size_to_write);
	TEST_ASSERT_(ret == 0, "got error - %d", ret);

	ret = read_file(out_buff, size_to_write);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, out_buff, size_to_write) == 0);
}

void read_out_of_boundaries()
{
	int ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(buff, sizeof(MY_STR) + 1);
	TEST_ASSERT_(ret == -1, "expected error -1 but got %d", ret);
}

void read_part_of_file()
{
	int ret;
	char buff[100];
	buff[0] = 0;
	int size_to_read = sizeof(MY_STR) - 1;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(buff, size_to_read);
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR, size_to_read) == 0);
}

TEST_LIST = {
    { "basic-test", basic_test },
	{ "write_to_big_test", write_to_big_test},
	{ "write_exact_size_test", write_exact_size_test},
	{ "read_out_of_boundaries", read_out_of_boundaries},
	{ "read_part_of_file", read_part_of_file},
    { NULL, NULL }
};