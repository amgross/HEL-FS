
#define TEST_NO_MAIN
#include "acutest_hel_port.h"

#include <assert.h>
#include <string.h>

#include "test_utils.h"
#include "../naming_wrapper/naming.h"

#define DEFAULT_MEM_SIZE 0x400
#define DEFAULT_SECTOR_SIZE 0x20

#define NAME1 "MY_FILE"
static_assert(sizeof(NAME1) == FILE_NAME_SIZE);

#define NAME2 "MY FILE"
static_assert(sizeof(NAME2) == FILE_NAME_SIZE);

#define NAME3 "MY-FILE"
static_assert(sizeof(NAME3) == FILE_NAME_SIZE);

#define MY_STR1 "hello world!\n"
#define MY_STR2 "world hello\n"
#define MY_STR3 "foo bar foo bar\n"
#define BIG_STR1 "LSKDMFOIWE43 43 434 3 RE WRF34563453!@#$&^&**&&^DSFKGMSOFDKMGSLKDFMERREWKRkmokmokKNOMOK$#$#@@@@!##$#DSFGDF"

static hel_ret test_naming_create_and_write_one_helper(char name[FILE_NAME_SIZE], void *buff, HEL_BASE_TYPE size, hel_file_id *id)
{
	return hel_naming_create_and_write(name, &buff, &size, 1, id);
}

void naming_basic_test()
{
	hel_file_id id, id_ret;
	hel_ret ret;
	uint8_t buff[100];
	buff[0] = 0;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_naming_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_naming_get_id(NAME1, &id_ret);
	TEST_ASSERT_(ret == hel_file_not_exist_err, "expected error hel_file_not_exist_err-%d but got %d", hel_file_not_exist_err, ret);
	
	ret = test_naming_create_and_write_one_helper(NAME1, MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = hel_naming_get_id(NAME1, &id_ret);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);
	TEST_ASSERT(id == id_ret);

	ret = hel_naming_read(id, buff, 0, sizeof(MY_STR1));
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	TEST_ASSERT(memcmp(buff, MY_STR1, sizeof(MY_STR1)) == 0);

	ret = hel_naming_delete(id);
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);

	ret = hel_read(id, buff, 0, sizeof(MY_STR2));
	TEST_ASSERT_(ret == hel_not_file_err, "expected error hel_not_file_err-%d but got %d", hel_not_file_err, ret);
}

void naming_file_recreation_test()
{
	hel_file_id id;
	hel_ret ret;

	mem_driver_init_test(DEFAULT_MEM_SIZE, DEFAULT_SECTOR_SIZE);
	
	ret = hel_naming_format();
	TEST_ASSERT_(ret == hel_success, "Got error %d", ret);
	
	ret = test_naming_create_and_write_one_helper(NAME1, MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_naming_create_and_write_one_helper(NAME2, MY_STR1, sizeof(MY_STR2), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_naming_create_and_write_one_helper(NAME3, MY_STR1, sizeof(MY_STR3), &id);
	TEST_ASSERT_(ret == hel_success, "got error %d", ret);

	ret = test_naming_create_and_write_one_helper(NAME1, MY_STR1, sizeof(MY_STR1), &id);
	TEST_ASSERT_(ret == hel_file_already_exist_err, "expected error hel_file_already_exist_err-%d but got %d", hel_file_already_exist_err, ret);

	ret = test_naming_create_and_write_one_helper(NAME1, MY_STR1, sizeof(MY_STR2), &id);
	TEST_ASSERT_(ret == hel_file_already_exist_err, "expected error hel_file_already_exist_err-%d but got %d", hel_file_already_exist_err, ret);

	ret = test_naming_create_and_write_one_helper(NAME1, MY_STR1, sizeof(MY_STR3), &id);
	TEST_ASSERT_(ret == hel_file_already_exist_err, "expected error hel_file_already_exist_err-%d but got %d", hel_file_already_exist_err, ret);
}
