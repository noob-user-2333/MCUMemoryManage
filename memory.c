#include "memory_tool.h"


struct memory_manage memory_manage_struct = {0};


unsigned int memory_manage_area_add(unsigned long start_address, unsigned long end_address) {
    if (memory_manage_struct.memory_area_num >= MAX_MEMORY_AREA_NUM)
        return OS_FAILED;

    if (start_address & (AREA_ALIGN - 1))
        start_address = (start_address & ~(AREA_ALIGN - 1)) + AREA_ALIGN;
    end_address = end_address & ~(AREA_ALIGN - 1);
    if (end_address <= start_address)
        return OS_FAILED;


    unsigned int index = 0;
    for (index = 0; index < memory_manage_struct.memory_area_num; index++) {
        if ((void *) start_address >= memory_manage_struct.area[index].start_address &&
            (void *) start_address <= memory_manage_struct.area[index].end_address)
            return OS_FAILED;
        if ((void *) end_address >= memory_manage_struct.area[index].start_address &&
            (void *) end_address <= memory_manage_struct.area[index].end_address)
            return OS_FAILED;
    }


    memory_manage_struct.area[index].start_address = (void *) start_address;
    memory_manage_struct.area[index].end_address = (void *) end_address;
    memory_manage_struct.area[index].size = end_address - start_address;
    memory_manage_struct.area[index].header_ptr = (void*)start_address;
//    memory_manage_struct.area[index].locked = 0;
    memory_manage_struct.memory_area_num++;
    return OS_SUCCESS;
}

unsigned int memory_manage_init() {
    memory_manage_struct.total_size = 0;
//    memory_manage_struct.allocate_space = 0;
    for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++) {
        struct memory_area *area_ptr = &memory_manage_struct.area[index];
        struct memory_area_header *area_start = area_ptr->start_address;
        memory_manage_struct.total_size += area_ptr->size;
        area_start->allocate_number = 0;
        area_start->content_area_start = 0;
        area_start->max_fragment_offset = 0;
    }
    return OS_SUCCESS;
}


void *memory_manage_allocate(unsigned long size) {
    if (size == 0)
        return NULL;

    //8字节对齐
    if (size & 0x07)
        size = ((size >> 3) << 3) + 8;


    struct memory_area_header *area_start = NULL;
    struct memory_area *area_ptr = NULL;
    for (unsigned int area_index = 0; area_index < memory_manage_struct.memory_area_num; area_index++) {
        area_ptr = &(memory_manage_struct.area[area_index]);
        void *ptr = memory_alloc(area_ptr, size);
        if (ptr) {
            area_ptr->alloc_size += size;
            return ptr;
        }
    }


    return NULL;
}


void memory_manage_free(void *ptr) {
    //8字节对齐
    ptr = (void *) (((unsigned long) ptr) & ~(0x07));
    struct memory_area *area_ptr = NULL;
    for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++) {
        if (ptr >= memory_manage_struct.area[index].start_address &&
            ptr < memory_manage_struct.area[index].end_address) {
            area_ptr = &(memory_manage_struct.area[index]);
            break;
        }
    }
    if (area_ptr == NULL)
        return;

    area_ptr->alloc_size -= memory_free(area_ptr,ptr);
}

void memory_manage_sort_out(struct memory_area *area_ptr) {
    if (area_ptr == NULL)
        return;
    memory_fragment_merge(area_ptr);
}
