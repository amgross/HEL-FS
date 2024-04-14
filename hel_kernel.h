
#pragma once

typedef enum
{
	hel_success,
	hel_mem_err, // Out of memory
	hel_boundaries_err, // Out of boundaries
	hel_param_err, // General parameter error
	hel_not_file_err,
	hel_out_of_heap_err,
}hel_ret;

#pragma pack(push, 1)
typedef struct
{
	uint16_t size : 14; // in case is_file_end - it size in bytes, else it size in sectors
	uint16_t is_file_begin : 1;
	uint16_t is_file_end : 1;
	uint16_t next_file_id;
	char data[];
}hel_chunk;
#pragma pack(pop)

typedef uint32_t hel_file_id;

hel_ret hel_format();

hel_ret hel_init();

hel_ret hel_close();

hel_ret hel_create_and_write(char *in, int size, hel_file_id *out_id);

hel_ret hel_read(hel_file_id id, char *out, int size);

hel_ret hel_delete(hel_file_id id);

hel_ret hel_iterate_files(hel_file_id *id, hel_chunk  *file);
