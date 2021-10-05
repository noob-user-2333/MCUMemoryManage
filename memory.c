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
//        memory_manage_struct.total_size += memory_manage_struct.area[index].size;
//        memset(memory_manage_struct.area[index].start_address,0,memory_manage_struct.area[index].size);
    }
//    unsigned int keep_per_sectors = PAGE_SIZE / sizeof(unsigned short);
//    unsigned int MAT_page = 1;
//    while (MAT_page * keep_per_sectors < memory_manage_struct.total_size / PAGE_SIZE - MAT_page)
//        MAT_page++;
//    if (MAT_page * PAGE_SIZE > memory_manage_struct.area[0].size)
//        return OS_FAILED;
//    memory_manage_struct.MAT_start = (unsigned short *) memory_manage_struct.area[0].start_address;
//    memory_manage_struct.MAT_end = (unsigned short *) (memory_manage_struct.area[0].start_address +
//                                                       MAT_page * PAGE_SIZE);
//    memory_manage_struct.total_size -= MAT_page * PAGE_SIZE;
//
//    if (memory_manage_struct.MAT_end == memory_manage_struct.area[0].end_address) {
//        memory_manage_struct.memory_area_num--;
//        if (memory_manage_struct.memory_area_num == 0)
//            return OS_FAILED;
//        for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++)
//            memory_manage_struct.area[index] = memory_manage_struct.area[index + 1];
//        memory_manage_struct.area[memory_manage_struct.memory_area_num].start_address = 0;
//        memory_manage_struct.area[memory_manage_struct.memory_area_num].end_address = 0;
//        memory_manage_struct.area[memory_manage_struct.memory_area_num].start_page_id = 0;
//        memory_manage_struct.area[memory_manage_struct.memory_area_num].size = 0;
//    } else {
//        memory_manage_struct.area[0].start_address = memory_manage_struct.MAT_end;
//        memory_manage_struct.area[0].size -= MAT_page * PAGE_SIZE;
//    }
//
//    for (unsigned int index = 1; index < memory_manage_struct.memory_area_num; index++) {
//        memory_manage_struct.area[index].start_page_id = memory_manage_struct.area[index - 1].start_page_id +
//                                                         memory_manage_struct.area[index - 1].size / PAGE_SIZE;
//    }
//
//    unsigned int MAT_sum = memory_manage_struct.total_size / PAGE_SIZE;
//    memory_manage_struct.MAT_start = memory_manage_struct.MAT_end - MAT_sum;
//    for (unsigned int times = 0; times < MAT_sum; times++)
//        memory_manage_struct.MAT_start[times] = MAT_FREE;
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
