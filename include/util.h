#ifndef _UTIL_H_
#define _UTIL_H_

#include "structs.h"
#include "types.h"

#define ALIGN(_value, _alignment) (((_value) + (_alignment) - 1) & ~((_alignment) - 1))

void dump_data(const u8 *data, u64 size);

int get_file_size(const char *file_path, u64 *size);
int read_file(const char *file_path, u8 *data, u64 size);
int write_file(const char *file_path, u8 *data, u64 size);
int mmap_file(const char *file_path, u8 **data, u64 *size);
int unmmap_file(u8 *data, u64 size);

u64 align_to_pow2(u64 offset, u64 alignment);
u64 djb2_hash(const char *str, u64 hash);

#endif /* !_UTIL_H_ */
