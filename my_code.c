#include "acutest.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MY_STR "hello world\n"
#define MY_STR2 "world hello\n"


#define MEM_SIZE 0x100

static uint8_t mem_buff[MEM_SIZE];

typedef struct my_code
{
	int size;
	char data[];
}file;

typedef enum
{
	helfs_success,
	helfs_mem_err, // Out of memory
	helfs_boundaries_err, // Out of boundaries
	halfs_param_err, // General parameter error
}helfs_ret;

typedef uint32_t file_id;

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

helfs_ret create_and_write(char *in, int size, file_id *out_id)
{
	file *new_file;

	if(NULL == out_id)
	{
		return halfs_param_err;
	}
	
	new_file = find_empty_place(size);
	if(new_file == NULL)
	{
		return helfs_mem_err;
	}

	new_file->size = size;

	memcpy(new_file->data, in, size);

	*out_id = (uint8_t *)new_file - mem_buff;

	return 0;
}

helfs_ret read_file(file_id id, char *out, int size)
{
	file *read_file = (file *)(mem_buff + id);

	// TODO need much more checks to ensure we are not out of boundaries
	if(read_file->size < size)
	{
		return helfs_boundaries_err;
	}

	memcpy(out, read_file->data, size);
	return 0;
}

/////////////////////

void basic_test()
{
	file_id id;
	helfs_ret ret;
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
	helfs_ret ret;
	char buff[MEM_SIZE * 2];

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(buff, sizeof(buff), &id);
	TEST_ASSERT_(ret == helfs_mem_err, "expected error helfs_mem_err-%d but got %d", helfs_mem_err, ret);

	ret = create_and_write(buff, MEM_SIZE, &id);
	TEST_ASSERT_(ret == helfs_mem_err, "expected error helfs_mem_err-%d but got %d", helfs_mem_err, ret);

	ret = create_and_write(buff, MEM_SIZE - sizeof(file) + 1, &id);
	TEST_ASSERT_(ret == helfs_mem_err, "expected error helfs_mem_err-%d but got %d", helfs_mem_err, ret);
}

void write_exact_size_test()
{
	file_id id;
	helfs_ret ret;
	char buff[MEM_SIZE];
	char out_buff[sizeof(buff)];
	int size_to_write = MEM_SIZE - sizeof(file);

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
	helfs_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = init_fs();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = create_and_write(MY_STR, sizeof(MY_STR), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = read_file(id, buff, sizeof(MY_STR) + 1);
	TEST_ASSERT_(ret == helfs_boundaries_err, "expected error helfs_boundaries_err-%d but got %d", helfs_boundaries_err, ret);
}

void read_part_of_file_test()
{
	file_id id;
	helfs_ret ret;
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
	helfs_ret ret;
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
	TEST_ASSERT_(ret == helfs_boundaries_err, "expected error helfs_boundaries_err-%d but got %d", helfs_boundaries_err, ret);

	ret = read_file(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);
}

TEST_LIST = {
    { "basic-test", basic_test },
	{ "write_too_big_test", write_too_big_test},
	{ "write_exact_size_test", write_exact_size_test},
	{ "read_out_of_boundaries_test", read_out_of_boundaries_test},
	{ "read_part_of_file_test", read_part_of_file_test},
	{ "write_read_multiple_files", write_read_multiple_files},
    { NULL, NULL }
};