
#define TEST_NO_MAIN
#include "acutest_hel_port.h"

#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <string.h>

#include "../kernel/hel_kernel.h"
#include "test_utils.h"


#define MY_STR1 "hello world!\n"
#define MY_STR2 "world hello\n"
#define MY_STR3 "foo bar foo bar\n"
#define BIG_STR1 "LSKDMFOIWE43 43 434 3 RE WRF34563453!@#$&^&**&&^DSFKGMSOFDKMGSLKDFMERREWKRkmokmokKNOMOK$#$#@@@@!##$#DSFGDF"

#define MIN_FILE_SIZE ATOMIC_WRITE_SIZE

#define DEFAULT_MEM_SIZE 0x400
#define DEFAULT_SECTOR_SIZE 0x20

 // This needed to the power down tests , where all locals erases.
static 	hel_file_id g_id1;
static bool g_file_created = false;

static hel_ret test_create_and_write_one_helper(void *buff, HEL_BASE_TYPE size, hel_file_id *id)
{
	return hel_create_and_write(&buff, &size, 1, id);
}

void fill_rand_buff(uint8_t *buff, size_t len)
{
	for(size_t i = 0;i < len; i++)
	{
		buff[i] = (uint8_t)(rand() % 0xff);
	}
}

void basic_test()
{	
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	
	ret = hel_read(id, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_delete(id);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_read(id, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void write_too_big_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[DEFAULT_MEM_SIZE * 2];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff), &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = test_create_and_write_one_helper(buff, DEFAULT_MEM_SIZE, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = test_create_and_write_one_helper(buff, DEFAULT_MEM_SIZE - MIN_FILE_SIZE + 1, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = test_create_and_write_one_helper(buff, DEFAULT_MEM_SIZE - MIN_FILE_SIZE, &id);
	TEST_ASSERT_(ret == hel_success, "got error - %d", ret);
}

void create_too_big_when_file_exist()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[DEFAULT_MEM_SIZE * 2];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, DEFAULT_MEM_SIZE, &id);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);
}

void write_exact_size_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[DEFAULT_MEM_SIZE];
	fill_rand_buff((uint8_t *)buff, sizeof(buff));
	uint8_t out_buff[sizeof(buff)];
	HEL_BASE_TYPE size_to_write = DEFAULT_MEM_SIZE - MIN_FILE_SIZE;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(buff, size_to_write, &id);
	TEST_ASSERT_(ret == hel_success, "got error - %d", ret);

	ret = hel_read(id, out_buff, 0, size_to_write);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, out_buff, size_to_write) == 0);
}

void read_out_of_boundaries_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id, buff, 0, sizeof(MY_STR1) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);
}

void read_part_of_file_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;
	HEL_BASE_TYPE size_to_read = sizeof(MY_STR1) - 1;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id, buff, 0, size_to_read);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, size_to_read) == 0);
}

void write_read_multiple_files()
{
	hel_file_id id1, id2;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2) + 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);
}

void null_params_test()
{
	hel_ret ret;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), NULL);
	TEST_ASSERT_(ret == hel_param_err, "expected error hel_param_err-%d but got %d", hel_param_err, ret);
}

void delete_in_middle_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

}

void write_big_when_there_hole_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	uint8_t buff[1000];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = test_create_and_write_one_helper(BIG_STR1, sizeof(BIG_STR1), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(BIG_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, BIG_STR1, sizeof(BIG_STR1)) == 0);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);
}

void basic_mem_leak_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;
	
	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	for(HEL_BASE_TYPE i = 0; i < DEFAULT_MEM_SIZE; i++)
	{
		ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
		TEST_ASSERT_(ret == hel_success, "got error %d, cycle %d", ret, i);
		
		ret = hel_read(id, buff, 0, sizeof(MY_STR1));
		TEST_ASSERT_(ret == hel_success, "got error %d, cycle %d", ret, i);

		TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

		ret = hel_delete(id);
		TEST_ASSERT_(ret == hel_success, "Got error %d, cycle %d", ret, i);
	}

	ret = hel_read(id, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void concatinate_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	uint8_t buff[(DEFAULT_MEM_SIZE / 3) * 2];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff) / 2, &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff) / 2, &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
}

void fragmented_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	uint8_t buff[(DEFAULT_MEM_SIZE / 3) * 2];
	uint8_t buff_2[sizeof(buff)];
	fill_rand_buff((uint8_t *)buff, sizeof(buff));

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff) / 2, &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff) / 4, &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(buff, sizeof(buff), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id3, buff_2, 0, sizeof(buff_2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff_2, buff, sizeof(buff_2)) == 0);

}

void big_id_read_test()
{
	hel_ret ret;
	uint8_t out;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_read(DEFAULT_MEM_SIZE, &out, 0, 1);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);
}

void big_id_delete_test()
{
	hel_ret ret;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_delete(DEFAULT_MEM_SIZE);
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);
}

void ensure_fragmented_file_fully_deleted()
{
	hel_ret ret;
	hel_file_id id1, id2, id3;
	uint8_t buff[1000];
	uint8_t write_buff[DEFAULT_SECTOR_SIZE + 1]; // Writing this takes more than 1 sector

	fill_rand_buff((uint8_t *)write_buff, sizeof(write_buff));

	mem_driver_init_test(DEFAULT_SECTOR_SIZE * 3, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// fill the three sectors, each one with different file

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR1), &id2);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR1), &id3);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// Delete first and third files, to force fragmantation
	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// Sanity checks that things are as expected
	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	// Create a fragmanted file
	ret = test_create_and_write_one_helper(write_buff, sizeof(write_buff), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// Sanity check that writing the fragmented file succeeded
	ret = hel_read(id1, buff, 0, sizeof(write_buff));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, write_buff, sizeof(write_buff)) == 0);

	// Delete the fragmented file
	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// re-create the fragmented file, ensure that the space was really free
	fill_rand_buff((uint8_t *)write_buff, sizeof(write_buff));

	ret = test_create_and_write_one_helper(write_buff, sizeof(write_buff), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// Sanity check that writing the fragmented file succeeded
	ret = hel_read(id1, buff, 0, sizeof(write_buff));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, write_buff, sizeof(write_buff)) == 0);
}

void basic_close_hel_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	
	ret = hel_read(id, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);
}

void basic_init_sign_full_chunks_test()
{
	hel_file_id id1, id2;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	// Create system with just single sector
	mem_driver_init_test(DEFAULT_SECTOR_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	// Fill the only sector
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	
	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	// Ensure there is really no place now
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id2);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// Ensure still there is really no place now
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id2);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);


	ret = hel_read(id1, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);
}
void init_with_fragmented_file()
{
	hel_ret ret;
	hel_file_id id1, id2, id3;
	uint8_t buff[1000];
	uint8_t write_buff[DEFAULT_SECTOR_SIZE + 1]; // Writing this takes more than 1 sector

	fill_rand_buff((uint8_t *)write_buff, sizeof(write_buff));

	mem_driver_init_test(DEFAULT_SECTOR_SIZE * 3, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// fill the three sectors, each one with different file

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR1), &id2);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR1), &id3);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// Delete first and third files, to force fragmantation
	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// Sanity checks that things are as expected
	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id1, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);

	// Create a fragmanted file
	ret = test_create_and_write_one_helper(write_buff, sizeof(write_buff), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	// Sanity check that writing the fragmented file succeeded
	ret = hel_read(id1, buff, 0, sizeof(write_buff));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, write_buff, sizeof(write_buff)) == 0);

	// Ensure all space is allocated
	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR1), &id3);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);

	// re-open file system

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// Sanity check that the fragmented file still exist
	ret = hel_read(id1, buff, 0, sizeof(write_buff));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, write_buff, sizeof(write_buff)) == 0);

	// Ensure all space is allocated
	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR1), &id3);
	TEST_ASSERT_(ret == hel_mem_err, "expected error hel_mem_err-%d but got %d", hel_mem_err, ret);
}

void init_with_multiple_free_chunks_test()
{
	hel_file_id id;
	hel_ret ret;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	// re-open file system
	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
}

void power_down_in_basic_creation_loop_test()
{
	hel_ret ret;
	int round = 0;
	uint8_t buff[100];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	power_down = PD_BEFORE_OERATION_RANDOMLY;
	power_down_prob = 20;
	g_id1 = -1;

	setjmp(env);

	round++;

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	if(round == 1000)
	{
		return;
	}

	while(true)
	{
		if(!g_file_created)
		{
			ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &g_id1);
			TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

			g_file_created = true;
		}

		// Sanity check that the file exist
		ret = hel_read(g_id1, buff, 0, sizeof(MY_STR1));
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);


		ret = hel_delete(g_id1);
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		g_file_created = false;
	}
}

void power_down_in_fragmented_files_creation_loop_test()
{
	hel_ret ret;
	hel_file_id id1, id2;
	int round = 0;
	uint8_t buff[DEFAULT_SECTOR_SIZE + 1];
	uint8_t buff_out[sizeof(buff)];

	mem_driver_init_test(DEFAULT_SECTOR_SIZE * 3, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR1), &id2);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);



	power_down = PD_BEFORE_OERATION_RANDOMLY;
	power_down_prob = 20;
	g_id1 = -1;

	setjmp(env);

	round++;

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	if(round == 1000)
	{
		return;
	}

	while(true)
	{
		if(!g_file_created)
		{
			fill_rand_buff((uint8_t *)buff, sizeof(buff));
			ret = test_create_and_write_one_helper(buff, sizeof(buff), &g_id1);
			TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

			g_file_created = true;
		}

		// Sanity check that the file exist
		ret = hel_read(g_id1, buff_out, 0, sizeof(buff));
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		TEST_ASSERT(memcmp(buff, buff_out, sizeof(buff)) == 0);

		ret = hel_delete(g_id1);
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		g_file_created = false;
	}
}

void power_down_in_middle_writing_test()
{
	hel_ret ret;
	int round = 0;
	uint8_t buff[100];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	power_down = PD_IN_MIDDLE_RANDOMLY;
	power_down_prob = 20;
	g_id1 = -1;

	setjmp(env);

	round++;

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	if(round == 1000)
	{
		return;
	}

	while(true)
	{
		if(!g_file_created)
		{
			ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &g_id1);
			TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

			g_file_created = true;
		}

		// Sanity check that the file exist
		ret = hel_read(g_id1, buff, 0, sizeof(MY_STR1));
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

		ret = hel_delete(g_id1);
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		g_file_created = false;
	}
}

void power_down_in_defragment_test()
{
	hel_ret ret;
	hel_file_id id1, id2;
	int round = 0;
	uint8_t buff[DEFAULT_SECTOR_SIZE + 4];
	uint8_t buff_out[sizeof(buff)];

	mem_driver_init_test(DEFAULT_SECTOR_SIZE * 3, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	power_down_prob = 20;
	g_id1 = -1;

	setjmp(env);

	round++;

	ret = hel_close();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_init();
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	if(round == 1000)
	{
		return;
	}

	while(true)
	{
		power_down = PD_IN_MIDDLE_RANDOMLY;

		if(!g_file_created)
		{
			fill_rand_buff((uint8_t *)buff, sizeof(buff));
			ret = test_create_and_write_one_helper(buff, sizeof(buff), &g_id1);
			TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

			g_file_created = true;
		}

		// Sanity check that the file exist
		ret = hel_read(g_id1, buff_out, 0, sizeof(buff));
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		TEST_ASSERT(memcmp(buff, buff_out, sizeof(buff)) == 0);

		ret = hel_delete(g_id1);
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d", ret, round);

		g_file_created = false;

		power_down = PD_NONE;
		ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
		TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

		ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR1), &id2);
		TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

		ret = hel_delete(id1);
		TEST_ASSERT_(ret == hel_success, "got error %d", ret);

		ret = hel_delete(id2);
		TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	}
}

void basic_iterator_test()
{
	hel_file_id id1, id2, id3, id4;
	hel_ret ret;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	
	ret = hel_get_first_file(&id4);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	TEST_ASSERT_(id1 == id4, "original - %d, got - %d", id1, id4);

	ret = hel_iterate_files(&id4);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	TEST_ASSERT_(id3 == id4, "original - %d, got - %d", id3, id4);

	ret = hel_iterate_files(&id4);
	TEST_ASSERT_(ret == hel_file_not_exist_err, "expected error hel_file_not_exist_err-%d but got %d", hel_file_not_exist_err, ret);
}

void read_in_middle_test()
{
	hel_file_id id1, id2;
	hel_ret ret;
	uint8_t buff[1000];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(BIG_STR1, sizeof(BIG_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, sizeof(BIG_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, BIG_STR1, sizeof(BIG_STR1)) == 0);

	ret = hel_read(id2, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR2, sizeof(MY_STR2)) == 0);

	ret = hel_read(id1, buff, 1, sizeof(BIG_STR1) - 1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, BIG_STR1 + 1, sizeof(BIG_STR1) - 1) == 0);

	for(int i = 0; i < 20; i ++)
	{
		HEL_BASE_TYPE offset = rand() % sizeof(BIG_STR1);
		HEL_BASE_TYPE size = rand() % (sizeof(BIG_STR1) - offset);
		ret = hel_read(id1, buff, offset, size);
		TEST_ASSERT_(ret == hel_success, "got error %d, round %d, begin %d, size %d, string size %ld", ret, i, offset, size, sizeof(BIG_STR1));

		TEST_ASSERT_(memcmp(buff, BIG_STR1 + offset, size) == 0, "compare failed round %d", i);
	}
}

void basic_defragment_test()
{
	hel_file_id id1, id2, id3;
	hel_ret ret;
	uint8_t buff[1000];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(BIG_STR1, DEFAULT_SECTOR_SIZE + 1, &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_read(id1, buff, 0, DEFAULT_SECTOR_SIZE + 1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, BIG_STR1, DEFAULT_SECTOR_SIZE + 1) == 0);

	ret = hel_read(id3, buff, 0, sizeof(MY_STR3));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR3, sizeof(MY_STR3)) == 0);
}

void big_mem_test()
{
#if HEL_BASE_TYPE_BITS == 32
	hel_ret ret;
	mem_driver_init_test(1 << 18, 8);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_boundaries_err, "expected error hel_boundaries_err-%d but got %d", hel_boundaries_err, ret);

	mem_driver_init_test((1 << 18) - 8, 8);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
#endif
}

void get_first_file_when_empty_test()
{
	hel_ret ret;
	hel_file_id id;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = hel_get_first_file(&id);
	TEST_ASSERT_(ret == hel_file_not_exist_err, "expected error hel_file_not_exist_err-%d but got %d", hel_file_not_exist_err, ret);
}

void basic_creation_2_buffs_test()
{
	hel_file_id id;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;
	HEL_BASE_TYPE sizes[2];
	void *buffs[2];

	sizes[0] = sizeof(MY_STR1) / 2;
	sizes[1] = sizeof(MY_STR1) - sizes[0];
	buffs[0] = MY_STR1;
	buffs[1] = MY_STR1 + sizes[0];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = hel_create_and_write(buffs, sizes, 2, &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	
	ret = hel_read(id, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_delete(id);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_read(id, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void random_multi_buffer_creation_test()
{
	hel_ret ret;
	hel_file_id id1, id2, id3;
	void *buffs[2];
	HEL_BASE_TYPE sizes[2];
	uint8_t buff[sizeof(BIG_STR1)];

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR1, sizeof(MY_STR1), &id1);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR2, sizeof(MY_STR2), &id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_create_and_write_one_helper(MY_STR3, sizeof(MY_STR3), &id3);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_delete(id2);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	for(int i = 0; i < 300; i++)
	{
		sizes[0] = sizeof(BIG_STR1) - (rand() % sizeof(BIG_STR1));
		sizes[1] = sizeof(BIG_STR1) - sizes[0];
		buffs[0] = MY_STR1;
		buffs[1] = MY_STR1 + sizes[0];

		
		ret = hel_create_and_write(buffs, sizes, 2, &id2);
		TEST_ASSERT_(ret == hel_success, "got error %d", ret);
		
		ret = hel_read(id2, buff, 0, sizeof(MY_STR1));
		TEST_ASSERT_(ret == hel_success, "got error %d", ret);

		TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

		ret = hel_delete(id2);
		TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	}

}
