/**
 * Simple stdio-based full file saving and loading
 * 
 * How to use:
 *   - In any one file:
 *     - Define FILE_UTILS_IMPLEMENTATION
 *     - Include "FileUtils.h"
 */

#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#include <windows.h>
#endif

bool FULoad(const char * const path, void ** const data, size_t * const length);
char *FULoadString(const char *path);

bool FUSave(const char * const restrict path, const void * const data, const size_t length);
bool FUSaveString(const char * const restrict path, const char *string);

bool FUIsFile(const char * const restrict path);

#ifdef FILE_UTILS_IMPLEMENTATION

#ifndef FU_MALLOC
#define FU_MALLOC(x) malloc(x)
#endif

#ifndef FU_FREE
#define FU_FREE(x) free(x)
#endif

#define RET_ERROR() if (file) { fclose(file); } if (buffer) { FU_FREE(buffer); } return false; 

bool FULoad(const char * const restrict path, void ** const data, size_t * const length) {
	/**
	 * Load data from a file into a dynamically allocated buffer. A pointer to
	 * the data and the length of the file will be written to data and length,
	 * respectively. If length is NULL, then the buffer is appended with an
	 * extra NUL byte and the length is not written. If there is an error (this
	 * function returns false) then nither the data pointer nor the length is
	 * written.
	 * 
	 * @param path Path to the file to open
	 * @param data Pointer to a (void *) which points to loaded data
	 * @param length Pointer to a (size_t) which will contain the size of loaded
	 * data. May be NULL.
	 * @return true if successful, and false if not
	 */
	
	if (!path || !data) {
		return false;
	}
	
	FILE *file = fopen(path, "rb");
	void *buffer = NULL;
	
	if (!file) {
		RET_ERROR();
	}
	
	// While the C standard does not require meaningfully supporting SEEK_END
	// for binary files, there are few common OSes which actually don't do that,
	// and even the ones that do are just likely to add more NUL bytes instead
	// of bailing completely.
	if (fseek(file, 0, SEEK_END)) {
		RET_ERROR();
	}
	
	long file_length = ftell(file);
	
	if (file_length < 0) {
		RET_ERROR();
	}
	
	rewind(file);
	
	buffer = FU_MALLOC(file_length + (length ? 0 : 1));
	
	if (!buffer) {
		RET_ERROR();
	}
	
	if (fread(buffer, 1, file_length, file) < file_length) {
		RET_ERROR();
	}
	
	fclose(file);
	
	// Yay, we did it!
	if (length) {
		*length = file_length;
	}
	else {
		((uint8_t *) buffer)[file_length] = '\0';
	}
	
	*data = buffer;
	
	return true;
}

#undef RET_ERROR

char *FULoadString(const char * const restrict path) {
	/**
	 * Load a file to a dynamically allocated string.
	 */
	
	char *string;
	
	if (FULoad(path, (void **) &string, NULL)) {
		return string;
	}
	
	return NULL;
}

#define RET_ERROR() if (file) { fclose(file); remove(temp_path); } return false;

bool FUSave(const char * const restrict path, const void * const data, const size_t length) {
	/**
	 * Saves the length bytes at data to a file. The data is first written to
	 * the given path with ".new" appended to it, then rename()'d over the old
	 * file (or on windows, ReplaceFile()'d). This will fail if the ".new" file
	 * already exists.
	 */
	
	FILE *file = NULL;
	
	char temp_path[strlen(path) + 5];
	strcpy(temp_path, path);
	strcat(temp_path, ".new");
	
	if (FUIsFile(temp_path)) {
		return false;
	}
	
	file = fopen(temp_path, "wb");
	
	if (!file) {
		RET_ERROR();
	}
	
	if (fwrite(data, 1, length, file) < length) {
		RET_ERROR();
	}
	
	fclose(file);
	
	// rename() on POSIX (and most OSes) and ReplaceFile() on windows, since
	// Windows rename() does not overwrite the files (acc. to wikipedia).
#ifndef _WIN32
	if (rename(temp_path, path)) {
#else
	if (!ReplaceFileA(path, temp_path, NULL, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {
#endif
		remove(temp_path);
		return false;
	}
	
	// Attempt to remove the temporary file, but don't care if it somehow fails.
	remove(temp_path);
	return true;
}

#undef RET_ERROR

bool FUSaveString(const char * const restrict path, const char *string) {
	return FUSave(path, string, strlen(string));
}

bool FUIsFile(const char * const restrict path) {
	/**
	 * Check if a file exists and can be read.
	 */
	
	FILE *file = fopen(path, "rb");
	
	if (file) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}

#endif

#endif // _FILE_UTILS_H_
