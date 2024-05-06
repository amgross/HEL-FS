
#include "acutest_hel_port.h"

#define ADD_TEST(func) \
		extern void func();

#define MULTIPLE_TESTS_ADDER \
	ADD_TEST(basic_test)\
	ADD_TEST(write_too_big_test)\
	ADD_TEST(create_too_big_when_file_exist)\
	ADD_TEST(write_exact_size_test)\
	ADD_TEST(read_out_of_boundaries_test)\
	ADD_TEST(read_part_of_file_test)\
	ADD_TEST(write_read_multiple_files)\
	ADD_TEST(null_params_test)\
	ADD_TEST(delete_in_middle_test)\
	ADD_TEST(write_big_when_there_hole_test)\
	ADD_TEST(basic_mem_leak_test)\
	ADD_TEST(concatinate_test)\
	ADD_TEST(fragmented_test)\
	ADD_TEST(big_id_read_test)\
	ADD_TEST(big_id_delete_test)\
	ADD_TEST(ensure_fragmented_file_fully_deleted)\
	ADD_TEST(basic_close_hel_test)\
	ADD_TEST(basic_init_sign_full_chunks_test)\
	ADD_TEST(init_with_fragmented_file)\
	ADD_TEST(init_with_multiple_free_chunks_test)\
	ADD_TEST(power_down_in_basic_creation_loop_test)\
	ADD_TEST(power_down_in_fragmented_files_creation_loop_test)\
	ADD_TEST(power_down_in_middle_writing_test)\
	ADD_TEST(power_down_in_defragment_test)\
	ADD_TEST(basic_iterator_test)\
	ADD_TEST(read_in_middle_test)\
	ADD_TEST(basic_defragment_test)\
	ADD_TEST(big_mem_test)\
	ADD_TEST(get_first_file_when_empty_test)\
	ADD_TEST(basic_creation_2_buffs_test)\
	ADD_TEST(random_multi_buffer_creation_test)\
	\
	ADD_TEST(naming_basic_test)\
	ADD_TEST(naming_file_recreation_test)\


// For running with debugger, run just single test due to acutest needs
// #undef MULTIPLE_TESTS_ADDER
// #define MULTIPLE_TESTS_ADDER ADD_TEST(naming_basic_test)

// This externs all the tests
MULTIPLE_TESTS_ADDER

#undef ADD_TEST
#define ADD_TEST(func) \
		{#func, func},

TEST_LIST = {
	// thhis adds all testst to the test list
	MULTIPLE_TESTS_ADDER

	{ NULL, NULL }
};
