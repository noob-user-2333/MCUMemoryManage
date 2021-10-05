#ifndef _MEMORY_H_
#define _MEMORY_H_


#include "defs.h"




#define MAX_MEMORY_AREA_NUM   4
#define AREA_ALIGN 0x1000



struct memory_area {
    //unsigned int locked;

    unsigned long alloc_size;
    unsigned long size;
    void* start_address;
    void* end_address;
    struct memory_area_header *header_ptr;
};


struct memory_manage{
    unsigned long total_size;
//    unsigned long allocate_space;
    unsigned long memory_area_num;
    struct memory_area area[MAX_MEMORY_AREA_NUM];
//    unsigned short *MAT_start;
//    unsigned short *MAT_end;

};


//内存分配实现参考FAT16
//但由于大块内存必须连续分配
//故对其进行大量简化，主要保留FAT
struct memory_alloc_info {
    unsigned short size;
    unsigned short start_offset_in_page;//分配内存起始地址在该页的偏移量
};
struct memory_fragment_info {
    unsigned short size;
    unsigned short next_fragment_page_offset;
};
//仅适用于被部分分配的内存页
struct memory_area_header {
    unsigned short content_area_start; //内容区域起始点在页内偏移量
    unsigned short max_fragment_offset; //最大的碎片的页内偏移量
    unsigned int allocate_number; //被分配的内存块的数量
    struct memory_alloc_info ma_info[256];
};
//page_header后紧跟memory_alloc_information的数组




unsigned int memory_manage_area_add(unsigned long start_address, unsigned long end_address);
unsigned int memory_manage_init();

void *memory_manage_allocate(unsigned long size);
void memory_manage_free(void *ptr);
void memory_manage_sort_out(struct memory_area *area_ptr);

#endif






