
#include <string.h>

#include "naming.h"

hel_ret hel_naming_format()
{
	return hel_format();
}

hel_ret hel_naming_init()
{
	return hel_init();
}

hel_ret hel_naming_close()
{
	return hel_close();
}

hel_ret hel_naming_create_and_write(char name[FILE_NAME_SIZE], void **in, int *size, int num, hel_file_id *out_id)
{
	hel_ret ret;
	hel_file_id id;
	void *all_buffers[num + 1];
	int all_sizes[num + 1];

	ret = hel_naming_get_id(name, &id);
	if(hel_success == ret)
	{
		return hel_file_already_exist_err;
	}
	if(hel_file_not_exist_err != ret)
	{
		return ret;
	}

	all_buffers[0] = name;
	all_sizes[0] = FILE_NAME_SIZE;

	for(int i = 0; i < num; i++)
	{
		all_buffers[i + 1] = in[i];
		all_sizes[i + 1] = size[i];
	}

	return hel_create_and_write(all_buffers, all_sizes, num + 1, out_id);
}

hel_ret hel_naming_get_id(char name[FILE_NAME_SIZE], hel_file_id *id)
{
	hel_ret ret;
	char read_name[FILE_NAME_SIZE];

	ret = hel_get_first_file(id);
	
	while(ret == hel_success)
	{
		ret = hel_read(*id, read_name, 0, FILE_NAME_SIZE);
		if(ret != hel_success)
		{
			return ret;
		}

		if(memcmp(name, read_name, FILE_NAME_SIZE) == 0)
		{
			return hel_success;
		}

		ret = hel_iterate_files(id);
	}
	
	return ret;
}

hel_ret hel_naming_read(hel_file_id id, void *out, int begin, int size)
{
	return hel_read(id, out, begin + FILE_NAME_SIZE, size);
}

hel_ret hel_naming_delete(hel_file_id id)
{
	return hel_delete(id);
}

