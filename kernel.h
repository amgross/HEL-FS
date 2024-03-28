
#define MEM_SIZE 0x100

typedef enum
{
	helfs_success,
	helfs_mem_err, // Out of memory
	helfs_boundaries_err, // Out of boundaries
	halfs_param_err, // General parameter error
}helfs_ret;

typedef uint32_t file_id;

helfs_ret init_fs();

helfs_ret create_and_write(char *in, int size, file_id *out_id);

helfs_ret read_file(file_id id, char *out, int size);