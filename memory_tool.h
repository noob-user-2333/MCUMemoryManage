//
// Created by user on 2021/10/3.
//

#ifndef MCUMEMORYMANAGE_MEMORY_TOOL_H
#define MCUMEMORYMANAGE_MEMORY_TOOL_H

#include "memory.h"

unsigned int get_free_space(struct memory_page_header *page);

void memory_alloc_info_insert(void *page_start_address, struct memory_alloc_info ma_info);

void *memory_alloc_from_page_top(void *page_start_address, unsigned long size);

void memory_fragment_insert(void *page_start_address, unsigned long start_offset_in_page, unsigned long size);

struct memory_fragment_info *memory_fragment_get(void *page_start_address, unsigned long size);

struct memory_alloc_info *memory_alloc_info_find(void *page_start_address, void *ptr) ;
struct memory_alloc_info
memory_fragment_divide(void *page_start_address, struct memory_fragment_info *fragment_ptr, unsigned long size);

struct memory_alloc_info memory_alloc_info_delete(void *page_start_address, void *ptr);
void memory_fragment_merge(void* page_start_address);

void memory_fragment_merge(void* page_start_address);
void *memory_alloc(void *page_start_address, unsigned int size);

void memory_free(void *page_start_address, void *ptr);

#endif //MCUMEMORYMANAGE_MEMORY_TOOL_H
