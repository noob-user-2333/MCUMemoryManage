//
// Created by user on 2021/10/3.
//

#ifndef MCUMEMORYMANAGE_MEMORY_TOOL_H
#define MCUMEMORYMANAGE_MEMORY_TOOL_H

#include "memory.h"

unsigned int get_free_space(struct memory_area *area_ptr);

void memory_alloc_info_insert(struct memory_area *area_ptr, struct memory_alloc_info ma_info);

void *memory_alloc_from_page_top(struct memory_area *area_ptr, unsigned long size);

void memory_fragment_insert(struct memory_area *area_ptr, unsigned long start_offset_in_page, unsigned long size);

struct memory_fragment_info *memory_fragment_get(struct memory_area *area_ptr, unsigned long size);

struct memory_alloc_info *memory_alloc_info_find(struct memory_area *area_ptr, void *ptr) ;
struct memory_alloc_info
memory_fragment_divide(struct memory_area *area_ptr, struct memory_fragment_info *fragment_ptr, unsigned long size);

struct memory_alloc_info memory_alloc_info_delete(struct memory_area *area_ptr, void *ptr);
void memory_fragment_merge(struct memory_area* area_ptr);

void *memory_alloc(struct memory_area *area_ptr, unsigned int size);

unsigned long memory_free(struct memory_area *area_ptr, void *ptr);

#endif //MCUMEMORYMANAGE_MEMORY_TOOL_H
