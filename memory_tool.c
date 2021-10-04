//
// Created by user on 2021/10/3.
//


#include "memory_tool.h"


unsigned int get_free_space(struct memory_page_header *page) {
    unsigned int free_space = page->content_area_start - 8 - sizeof(struct memory_alloc_info) * page->allocate_number;
    return free_space;
}


void memory_fragment_merge(void* page_start_address)
{
    struct memory_page_header *page_start = page_start_address;
    if(page_start->max_fragment_offset == 0)
        return;
    struct memory_alloc_info  fragment_info = {0};
    page_start->max_fragment_offset = 0;
    struct memory_alloc_info *previous_ma = NULL;
    struct memory_alloc_info *next_ma = page_start->ma_info;
    if(next_ma->start_offset_in_page + next_ma->size < PAGE_SIZE)
    {
        fragment_info.start_offset_in_page = next_ma->start_offset_in_page + next_ma->size;
        fragment_info.size = PAGE_SIZE - fragment_info.start_offset_in_page;
        memory_fragment_insert(page_start_address,fragment_info.start_offset_in_page,fragment_info.size);
    }
    for(unsigned int index = 1;index < page_start->allocate_number;index++)
    {
        previous_ma = next_ma;
        next_ma = &page_start->ma_info[index];
        if(next_ma->start_offset_in_page + next_ma->size > previous_ma->start_offset_in_page)
        {
            fragment_info.start_offset_in_page = next_ma->start_offset_in_page + next_ma->size;
            fragment_info.size = previous_ma->start_offset_in_page - fragment_info.start_offset_in_page;
            memory_fragment_insert(page_start_address,fragment_info.start_offset_in_page,fragment_info.size);
        }
    }



}
void *memory_alloc(void *page_start_address, unsigned int size) {
    void *result = NULL;
    struct memory_fragment_info *fragment_info = memory_fragment_get(page_start_address, size);
    if (fragment_info == NULL)
        return memory_alloc_from_page_top(page_start_address, size);
    unsigned long fragment_offset = (unsigned long) fragment_info - (unsigned long) page_start_address;
    struct memory_alloc_info temp_info = memory_fragment_divide(page_start_address, fragment_info, size);
    if (temp_info.size && temp_info.start_offset_in_page)
        result = temp_info.start_offset_in_page + page_start_address;
    return result;
}

void memory_free(void *page_start_address, void *ptr) {
    struct memory_page_header *page_start = page_start_address;
    struct memory_alloc_info temp_info = memory_alloc_info_delete(page_start_address, ptr);
    if (temp_info.start_offset_in_page == 0 || temp_info.size == 0)
        return;
    if (page_start->allocate_number == 0) {
        page_start->max_fragment_offset = 0;
        page_start->content_area_start = 0;
        return;
    }
    if (page_start->content_area_start == temp_info.start_offset_in_page)
        page_start->content_area_start = page_start->ma_info[page_start->allocate_number - 1].start_offset_in_page;
    else
        memory_fragment_insert(page_start_address, temp_info.start_offset_in_page, temp_info.size & (PAGE_SIZE - 1));

}

struct memory_alloc_info *memory_alloc_info_find(void *page_start_address, void *ptr) {
    unsigned long ptr_offset = ptr - page_start_address;
    struct memory_page_header *page_start = page_start_address;

    for (unsigned int index = 0; index < page_start->allocate_number; index++)
        if(page_start->ma_info[index].start_offset_in_page == ptr_offset)
            return &page_start->ma_info[index];

    return NULL;
}

struct memory_alloc_info memory_alloc_info_delete(void *page_start_address, void *ptr) {
    struct memory_alloc_info temp_info = {0};
    struct memory_page_header *page_start = page_start_address;
    unsigned long ptr_offset = ptr - page_start_address;
    for (unsigned int index = 0; index < page_start->allocate_number; index++) {
        if (page_start->ma_info[index].start_offset_in_page == ptr_offset) {
            temp_info = page_start->ma_info[index];
            for (; index < page_start->allocate_number; index++)
                page_start->ma_info[index] = page_start->ma_info[index + 1];
            page_start->allocate_number--;
            break;
        }
    }
    return temp_info;
}


void *memory_alloc_from_page_top(void *page_start_address, unsigned long size) {
    if (size == 0)
        return NULL;
    struct memory_page_header *page_start = page_start_address;
    if (page_start->allocate_number == 0) {
        page_start->max_fragment_offset = 0;
        page_start->content_area_start = PAGE_SIZE - size;
        page_start->allocate_number = 1;
        page_start->ma_info[0].start_offset_in_page = PAGE_SIZE - size;
        page_start->ma_info[0].size = size;
        return page_start_address + PAGE_SIZE - size;
    }
    if (get_free_space(page_start_address) <= size + sizeof(struct memory_alloc_info))
        return NULL;
    page_start->content_area_start -= size;
    page_start->ma_info[page_start->allocate_number].start_offset_in_page = page_start->content_area_start;
    page_start->ma_info[page_start->allocate_number].size = size;
    page_start->allocate_number++;
    return page_start_address + page_start->content_area_start;
}


void memory_alloc_info_insert(void *page_start_address, struct memory_alloc_info ma_info) {
    struct memory_page_header *page_start = page_start_address;
    unsigned int index = 0;
    for (index = 0; index < page_start->allocate_number; index++) {
        if (ma_info.start_offset_in_page > page_start->ma_info[index].start_offset_in_page)
            break;
    }
    for (unsigned int ma_index = page_start->allocate_number; ma_index > index; ma_index--) {
        page_start->ma_info[ma_index] = page_start->ma_info[ma_index - 1];
    }
    page_start->ma_info[index] = ma_info;
}


struct memory_alloc_info
memory_fragment_divide(void *page_start_address, struct memory_fragment_info *fragment_ptr, unsigned long size) {
    struct memory_alloc_info result = {0};
    unsigned long fragment_offset = (unsigned long) fragment_ptr - (unsigned long) page_start_address;
    if (fragment_ptr->size >= size) {
        if (fragment_ptr->size - size < 8)
            size = fragment_ptr->size;
        unsigned long mini_fragment_start_offset = fragment_offset + size;
        unsigned long mini_fragment_size = fragment_ptr->size - size;
        memory_fragment_insert(page_start_address, mini_fragment_start_offset, mini_fragment_size);
        result.size = size;
        result.start_offset_in_page = fragment_offset;
    }
    return result;
}

struct memory_fragment_info *memory_fragment_get(void *page_start_address, unsigned long size) {
    struct memory_page_header *page_start = page_start_address;
    if (page_start->max_fragment_offset == 0 || size == 0)
        return NULL;
    struct memory_fragment_info *next_fragment = page_start_address + page_start->max_fragment_offset;
    struct memory_fragment_info *previous_fragment = NULL;
    struct memory_fragment_info *target_fragment = NULL;
    if (next_fragment->size < size)
        return NULL;
    if (next_fragment->next_fragment_page_offset == 0) {
        target_fragment = next_fragment;
        page_start->max_fragment_offset = 0;
    } else {
        target_fragment = next_fragment;
        next_fragment = page_start_address + next_fragment->next_fragment_page_offset;
        while (next_fragment->next_fragment_page_offset) {
            if (next_fragment->size < size)
                break;
            previous_fragment = target_fragment;
            target_fragment = next_fragment;
            next_fragment = page_start_address + next_fragment->next_fragment_page_offset;
        }
        if (next_fragment->size >= size) {
            previous_fragment = target_fragment;
            target_fragment = next_fragment;
        }
        if (previous_fragment == NULL)
            page_start->max_fragment_offset = target_fragment->next_fragment_page_offset;
        else
            previous_fragment->next_fragment_page_offset = target_fragment->next_fragment_page_offset;
    }
    return target_fragment;
}

void memory_fragment_insert(void *page_start_address, unsigned long start_offset_in_page, unsigned long size) {
    if (start_offset_in_page == 0 || size == 0)
        return;
    struct memory_page_header *page_start = page_start_address;
    struct memory_fragment_info *current_fragment = page_start_address + start_offset_in_page;
    current_fragment->next_fragment_page_offset = 0;
    current_fragment->size = size;
    if (page_start->max_fragment_offset == 0) {
        page_start->max_fragment_offset = start_offset_in_page;
        return;
    }
    struct memory_fragment_info *previous_fragment = NULL;
    struct memory_fragment_info *next_fragment = page_start_address + page_start->max_fragment_offset;
    while (next_fragment->next_fragment_page_offset) {
        if (next_fragment->size <= current_fragment->size)
            break;
        previous_fragment = next_fragment;
        next_fragment = page_start_address + next_fragment->next_fragment_page_offset;
    }
    if (next_fragment->size > size)
        previous_fragment = next_fragment;

    if (previous_fragment == NULL) {
        current_fragment->next_fragment_page_offset = page_start->max_fragment_offset;
        page_start->max_fragment_offset = start_offset_in_page;
    } else {
        current_fragment->next_fragment_page_offset = previous_fragment->next_fragment_page_offset;
        previous_fragment->next_fragment_page_offset = start_offset_in_page;
    }
}