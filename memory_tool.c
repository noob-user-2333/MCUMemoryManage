//
// Created by user on 2021/10/3.
//


#include "memory_tool.h"


unsigned int get_free_space(struct memory_area *area_ptr) {
    struct memory_area_header *area_start_ptr = area_ptr->start_address;
    unsigned long content_area_start = area_ptr->header_ptr->content_area_start;
    if(content_area_start == 0)
        content_area_start = area_ptr->size;
    unsigned int free_space = content_area_start - area_start_ptr->allocate_number * sizeof(struct memory_alloc_info);
    if(free_space < 32)
        return 0;
    return free_space;
}


void memory_fragment_merge(struct memory_area* area_ptr)
{

    struct memory_area_header *area_start = area_ptr->start_address;
    if(area_start->max_fragment_offset == 0)
        return;
    struct memory_alloc_info  fragment_info = {0};
    area_start->max_fragment_offset = 0;
    struct memory_alloc_info *previous_ma = NULL;
    struct memory_alloc_info *next_ma = area_start->ma_info;
    if(next_ma->start_offset_in_page + next_ma->size < area_ptr->size)
    {
        fragment_info.start_offset_in_page = next_ma->start_offset_in_page + next_ma->size;
        fragment_info.size = area_ptr->size - fragment_info.start_offset_in_page;
        memory_fragment_insert(area_ptr,fragment_info.start_offset_in_page,fragment_info.size);
    }
    for(unsigned int index = 1;index < area_start->allocate_number;index++)
    {
        previous_ma = next_ma;
        next_ma = &area_start->ma_info[index];
        if(next_ma->start_offset_in_page + next_ma->size > previous_ma->start_offset_in_page)
        {
            fragment_info.start_offset_in_page = next_ma->start_offset_in_page + next_ma->size;
            fragment_info.size = previous_ma->start_offset_in_page - fragment_info.start_offset_in_page;
            memory_fragment_insert(area_ptr,fragment_info.start_offset_in_page,fragment_info.size);
        }
    }



}
void *memory_alloc(struct memory_area *area_ptr, unsigned int size) {
    void *result = NULL;
    struct memory_fragment_info *fragment_info = memory_fragment_get(area_ptr, size);
    if (fragment_info == NULL)
        return memory_alloc_from_page_top(area_ptr, size);
    unsigned long fragment_offset = (unsigned long) fragment_info - (unsigned long) area_ptr->start_address;
    struct memory_alloc_info temp_info = memory_fragment_divide(area_ptr, fragment_info, size);
    if (temp_info.size && temp_info.start_offset_in_page)
        result = temp_info.start_offset_in_page + area_ptr->start_address;
    return result;
}

unsigned long memory_free(struct memory_area *area_ptr, void *ptr) {
    struct memory_area_header* area_start = area_ptr->start_address;
    struct memory_alloc_info temp_info = memory_alloc_info_delete(area_ptr, ptr);
    if (temp_info.start_offset_in_page == 0 || temp_info.size == 0) {

        return 0;
    }
    if (area_start->allocate_number == 0) {
        area_start->max_fragment_offset = 0;
        area_start->content_area_start = area_ptr->size;
        return temp_info.size;
    }
    if (area_start->content_area_start == temp_info.start_offset_in_page)
        area_start->content_area_start = area_start->ma_info[area_start->allocate_number - 1].start_offset_in_page;
    else
        memory_fragment_insert(area_ptr, temp_info.start_offset_in_page, temp_info.size);
    return  temp_info.size;
}

struct memory_alloc_info *memory_alloc_info_find(struct memory_area *area_ptr, void *ptr) {
    unsigned long ptr_offset = ptr - area_ptr->start_address;
    struct memory_area_header *area_start = area_ptr->start_address;

    for (unsigned int index = 0; index < area_start->allocate_number; index++)
        if(area_start->ma_info[index].start_offset_in_page == ptr_offset)
            return &area_start->ma_info[index];

    return NULL;
}

struct memory_alloc_info memory_alloc_info_delete(struct memory_area* area_ptr, void *ptr) {
    struct memory_alloc_info temp_info = {0};
    struct memory_area_header *area_start = area_ptr->start_address;
    unsigned long ptr_offset = ptr - area_ptr->start_address;
    for (unsigned int index = 0; index < area_start->allocate_number; index++) {
        if (area_start->ma_info[index].start_offset_in_page == ptr_offset) {
            temp_info = area_start->ma_info[index];
            for (; index < area_start->allocate_number; index++)
                area_start->ma_info[index] = area_start->ma_info[index + 1];
            area_start->allocate_number--;
            break;
        }
    }
    return temp_info;
}


void *memory_alloc_from_page_top(struct memory_area *area_ptr, unsigned long size) {
    if (size == 0 && size >= area_ptr->size)
        return NULL;
    struct memory_area_header *area_start = area_ptr->start_address;
    if (area_start->allocate_number == 0) {
        area_start->max_fragment_offset = 0;
        area_start->content_area_start = (unsigned long)area_ptr->size - size;
        area_start->allocate_number = 1;
        area_start->ma_info[0].start_offset_in_page = (unsigned long)area_ptr->size - size;
        area_start->ma_info[0].size = size;
        return area_ptr->end_address - size;
    }
    if (get_free_space(area_ptr) <= size + sizeof(struct memory_alloc_info))
        return NULL;
    area_start->content_area_start -= size;
    area_start->ma_info[area_start->allocate_number].start_offset_in_page = area_start->content_area_start;
    area_start->ma_info[area_start->allocate_number].size = size;
    area_start->allocate_number++;
    return area_ptr->start_address + area_start->content_area_start;
}


void memory_alloc_info_insert(struct memory_area *area_ptr, struct memory_alloc_info ma_info) {
    struct memory_area_header *area_start = area_ptr->start_address;
    unsigned int index = 0;
    for (index = 0; index < area_start->allocate_number; index++) {
        if (ma_info.start_offset_in_page > area_start->ma_info[index].start_offset_in_page)
            break;
    }
    for (unsigned int ma_index = area_start->allocate_number; ma_index > index; ma_index--) {
        area_start->ma_info[ma_index] = area_start->ma_info[ma_index - 1];
    }
    area_start->ma_info[index] = ma_info;
}


struct memory_alloc_info
memory_fragment_divide(struct memory_area *area_ptr, struct memory_fragment_info *fragment_ptr, unsigned long size) {
    struct memory_alloc_info result = {0};
    unsigned long fragment_offset = (unsigned long) fragment_ptr - (unsigned long)area_ptr->start_address;
    if (fragment_ptr->size >= size) {
        if (fragment_ptr->size - size < 8)
            size = fragment_ptr->size;
        unsigned long mini_fragment_start_offset = fragment_offset + size;
        unsigned long mini_fragment_size = fragment_ptr->size - size;
        memory_fragment_insert(area_ptr, mini_fragment_start_offset, mini_fragment_size);
        result.size = size;
        result.start_offset_in_page = fragment_offset;
    }
    return result;
}

struct memory_fragment_info *memory_fragment_get(struct memory_area*area_ptr, unsigned long size) {
    struct memory_area_header *area_start = area_ptr->start_address;
    if (area_start->max_fragment_offset == 0 || size == 0)
        return NULL;
    struct memory_fragment_info *next_fragment = area_ptr->start_address + area_start->max_fragment_offset;
    struct memory_fragment_info *previous_fragment = NULL;
    struct memory_fragment_info *target_fragment = NULL;
    if (next_fragment->size < size)
        return NULL;
    if (next_fragment->next_fragment_page_offset == 0) {
        target_fragment = next_fragment;
        area_start->max_fragment_offset = 0;
    } else {
        target_fragment = next_fragment;
        next_fragment = area_ptr->start_address + next_fragment->next_fragment_page_offset;
        while (next_fragment->next_fragment_page_offset) {
            if (next_fragment->size < size)
                break;
            previous_fragment = target_fragment;
            target_fragment = next_fragment;
            next_fragment = area_ptr->start_address + next_fragment->next_fragment_page_offset;
        }
        if (next_fragment->size >= size) {
            previous_fragment = target_fragment;
            target_fragment = next_fragment;
        }
        if (previous_fragment == NULL)
            area_start->max_fragment_offset = target_fragment->next_fragment_page_offset;
        else
            previous_fragment->next_fragment_page_offset = target_fragment->next_fragment_page_offset;
    }
    return target_fragment;
}

void memory_fragment_insert(struct memory_area *area_ptr, unsigned long start_offset_in_page, unsigned long size) {
    if (start_offset_in_page == 0 || size == 0)
        return;
    struct memory_area_header *area_start = area_ptr->start_address;
    struct memory_fragment_info *current_fragment = area_ptr->start_address + start_offset_in_page;
    current_fragment->next_fragment_page_offset = 0;
    current_fragment->size = size;
    if (area_start->max_fragment_offset == 0) {
        area_start->max_fragment_offset = start_offset_in_page;
        return;
    }
    struct memory_fragment_info *previous_fragment = NULL;
    struct memory_fragment_info *next_fragment = area_ptr->start_address + area_start->max_fragment_offset;
    while (next_fragment->next_fragment_page_offset) {
        if (next_fragment->size <= current_fragment->size)
            break;
        previous_fragment = next_fragment;
        next_fragment = area_ptr->start_address + next_fragment->next_fragment_page_offset;
    }
    if (next_fragment->size > size)
        previous_fragment = next_fragment;

    if (previous_fragment == NULL) {
        current_fragment->next_fragment_page_offset = area_start->max_fragment_offset;
        area_start->max_fragment_offset = start_offset_in_page;
    } else {
        current_fragment->next_fragment_page_offset = previous_fragment->next_fragment_page_offset;
        previous_fragment->next_fragment_page_offset = start_offset_in_page;
    }
}