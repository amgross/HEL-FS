
#define MEM_SIZE 0x100

typedef enum
{
	hel_success,
	hel_mem_err, // Out of memory
	hel_boundaries_err, // Out of boundaries
	hel_param_err, // General parameter error
}hel_ret;

typedef struct my_code
{
	uint32_t size;
	char data[];
}hel_file;

typedef uint32_t hel_file_id;

hel_ret init_fs();

hel_ret create_and_write(char *in, int size, hel_file_id *out_id);

hel_ret read_file(hel_file_id id, char *out, int size);