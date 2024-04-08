#include "acutest.h"

#include <stdint.h>
#include <stdio.h>
#include "hel_kernel.h"


#define MY_STR1 "hello world!\n"
#define MY_STR2 "world hello\n"
#define MY_STR3 "foo bar foo bar\n"
#define BIG_STR1 "LSKDMFOIWE43 43 434 3 RE WRF34563453!@#$&^&**&&^DSFKGMSOFDKMGSLKDFMERREWKRkmokmokKNOMOK$#$#@@@@!##$#DSFGDF"

#define MIN_FILE_SIZE sizeof(hel_chunk)

void basic_test()
{	
	hel_file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;
	
	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);
	
	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);
	
	ret = hel_read(id, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_delete(id);
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_read(id, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void write_too_big_test()
{
	hel_file_id id;
	hel_ret ret;
	char buff[MEM_SIZE * 2];

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(buff, sizeof(buff), &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_create_and_write(buff, MEM_SIZE, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_create_and_write(buff, MEM_SIZE - MIN_FILE_SIZE + 1, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_create_and_write(buff, MEM_SIZE - MIN_FILE_SIZE, &id);
	TEST_ASSERT_(ret == 0, "got error - %d", ret);
}

void create_too_big_when_file_exist()
{
	hel_file_id id;
	hel_ret ret;
	char buff[MEM_SIZE * 2];
	buff[0] = 0;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(buff, MEM_SIZE, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);
}

void write_exact_size_test()
{
	hel_file_id id;
	hel_ret ret;
	char buff[MEM_SIZE];
	char out_buff[sizeof(buff)];
	int size_to_write = MEM_SIZE - MIN_FILE_SIZE;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(buff, size_to_write, &id);
	TEST_ASSERT_(ret == 0, "got error - %d", ret);

	ret = hel_read(id, out_buff, size_to_write);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, out_buff, size_to_write) == 0);
}

void read_out_of_boundaries_test()
{
	hel_file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id, buff, sizeof(MY_STR1) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);
}

void read_part_of_file_test()
{
	hel_file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;
	int size_to_read = sizeof(MY_STR1) - 1;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id, buff, size_to_read);
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, size_to_read) == 0);
}

void write_read_multiple_files()
{
	hel_file_id id1, id2;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id1, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, sizeof(MY_STR2) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);

	ret = hel_read(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);
}

void null_params_test()
{
	hel_ret ret;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), NULL);
	TEST_ASSERT_(ret == hel_param_err, "expected error hel_param_err-%d but got %d", hel_param_err, ret);
}

void delete_in_middle_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id1, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id3, buff, sizeof(MY_STR3));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = hel_create_and_write(MY_STR3, sizeof(MY_STR3), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id1, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, sizeof(MY_STR3));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_read(id3, buff, sizeof(MY_STR3));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

}

void write_big_when_there_hole_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	char buff[1000];
	buff[0] = 0;

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id1, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id3, buff, sizeof(MY_STR3));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id2, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = hel_create_and_write(BIG_STR1, sizeof(BIG_STR1), &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_read(id1, buff, sizeof(MY_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, sizeof(BIG_STR1));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, BIG_STR1, sizeof(BIG_STR1)) == 0);

	ret = hel_read(id3, buff, sizeof(MY_STR3));
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);
}

void basic_mem_leak_test()
{
	hel_file_id id;
	hel_ret ret;
	char buff[100];
	buff[0] = 0;
	
	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);
	
	for(int i = 0; i < MEM_SIZE; i++)
	{
		ret = hel_create_and_write(MY_STR1, sizeof(MY_STR1), &id);
		TEST_ASSERT_(ret == 0, "got error %d, cycle %d", ret, i);
		
		ret = hel_read(id, buff, sizeof(MY_STR1));
		TEST_ASSERT_(ret == 0, "got error %d, cycle %d", ret, i);

		TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

		ret = hel_delete(id);
		TEST_ASSERT_(ret == 0, "Got error %d, cycle %d", ret, i);
	}

	ret = hel_read(id, buff, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void concatinate_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	char buff[(MEM_SIZE / 3) * 2];

	ret = hel_format();
	TEST_ASSERT_(ret == 0, "Got error %d", ret);

	ret = hel_create_and_write(buff, sizeof(buff) / 2, &id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(buff, sizeof(buff) / 2, &id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == 0, "got error %d", ret);

	ret = hel_create_and_write(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == 0, "got error %d", ret);
}


TEST_LIST = {
    { "basic-test", basic_test },
	{ "write_too_big_test", write_too_big_test},
	{ "create_too_big_when_file_exist", create_too_big_when_file_exist},
	{ "write_exact_size_test", write_exact_size_test},
	{ "read_out_of_boundaries_test", read_out_of_boundaries_test},
	{ "read_part_of_file_test", read_part_of_file_test},
	{ "write_read_multiple_files", write_read_multiple_files},
	{ "null_params_test", null_params_test},
	{ "delete_in_middle_test", delete_in_middle_test},
	{ "write_big_when_there_hole_test", write_big_when_there_hole_test},
	{ "basic_mem_leak_test", basic_mem_leak_test},
	{ "concatinate_test", concatinate_test},

    { NULL, NULL }
};
