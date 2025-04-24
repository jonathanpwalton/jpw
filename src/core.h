#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern char const * program;
extern int indent;

/*
	logging, errors, early exit
*/

#define log_info(...) do { fprintf(stdout, "%.*s\033[1;97minfo: \033[0m", indent, ""); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while (0)
#define log_warn(...) do { fprintf(stderr, "%.*s\033[1;93mwarning: \033[0m", indent, ""); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while (0)
#define log_error(...) do { fprintf(stderr, "%.*s\033[1;91merror: \033[0m", indent, ""); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while (0)
#define failure(...) do { log_error(__VA_ARGS__); exit(1); } while (0)

#define todo() failure("%s:%d: %s: todo", __FILE__, __LINE__, __func__)
#define require(Condition) if (!(Condition)) failure("%s:%d: %s: assertion failure: %s", __FILE__, __LINE__, __func__, #Condition)
#define unreachable() failure("%s:%d: %s: unreachable", __FILE__, __LINE__, __func__)

/*
	temporaries
*/

static long TEMP_LONG;

/*
	pointers
*/

static inline void * ptr_swapend(void * data, size_t length, size_t offset, size_t bytes) {
	void * tmp = malloc(bytes);
	memcpy(tmp, (char*) data + offset, bytes);
	memmove((char*) data + offset, (char*) data + offset + bytes, length - offset - bytes);
	memcpy((char*) data + length - bytes, tmp, bytes);
	return data;
}

/*
	containers
*/

#define len(Container) (Container).length

/*
	container: list[Type]
*/

static inline size_t list_index_abs(size_t length, long index) {
	return index >= 0 ? (size_t) index : length + index;
}

#define list_typedef(Type) typedef struct list_type_##Type { Type * data; size_t length; size_t capacity; } list_type_##Type
#define list(Type) list_type_##Type
#define list_free(List) do { free((List).data); (List).data = NULL; (List).length = 0; (List).capacity = 0; } while (0)
#define list_at(List, Index) (List).data[list_index_abs((List).length * sizeof(*(List).data), Index)]
#define list_push(List, Value) do {\
	if ((List).length >= (List).capacity) {\
		(List).capacity = (List).capacity == 0 ? 3 : (size_t) ((List).capacity * 1.5);\
		(List).data = realloc((List).data, sizeof(*(List).data) * (List).capacity);\
	}\
	(List).data[(List).length++] = (Value);\
} while (0)
#define list_pop(List, Index) (\
	TEMP_LONG = (Index),\
	TEMP_LONG == -1\
		? (List).data[--(List).length]\
		: (\
			(List).data = ptr_swapend((List).data, (List).length * sizeof(*(List).data), list_index_abs((List).length, TEMP_LONG) * sizeof(*(List).data), sizeof(*(List).data)),\
			(List).data[--(List).length]\
		)\
)

/*
	main dispatch
*/

int main_help(char ** argv);
int main_pull(char ** argv);
#include "core.h"

/*
	curl
*/

bool urlopen(char const * url, FILE * file, bool display);
