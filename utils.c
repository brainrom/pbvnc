#include "utils.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define EXECUTABLE_SYMLINK "/proc/self/exe"

bool get_executable_dirname(char *buffer, int buffer_size) {
	int path_len = readlink(EXECUTABLE_SYMLINK, buffer, buffer_size - 1);
	if (path_len == -1)
		return false;
	buffer[path_len] = '\0';
	
	const char *res = dirname(buffer);
	int dirname_len = strlen(res);
	if (dirname_len > buffer_size - 1)
		dirname_len = buffer_size - 1;
	memmove(buffer, res, dirname_len);
	buffer[dirname_len] = '\0';
	return true;
}

void get_default_config_path(const char *name, char *buffer, int buffer_size) {
	if (!get_executable_dirname(buffer, buffer_size)) {
		strncpy(buffer, name, buffer_size);
		buffer[buffer_size - 1] = '\0';
		return;
	}
	int directory_len = strlen(buffer);
	snprintf(buffer + directory_len, buffer_size - directory_len, "/%s", name);
}