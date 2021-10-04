#include "memory_tool.h"


struct memory_manage memory_manage_struct = {0};



unsigned int memory_manage_area_add(unsigned long start_address, unsigned long end_address) {
    if (memory_manage_struct.memory_area_num >= MAX_MEMORY_AREA_NUM)
        return OS_FAILED;

    if (start_address & (PAGE_SIZE - 1))
        start_address = (start_address & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
    end_address = end_address & ~(PAGE_SIZE - 1);
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
    memory_manage_struct.area[index].start_page_id = 0;
    memory_manage_struct.area[index].locked = 0;
    memory_manage_struct.memory_area_num++;
    return OS_SUCCESS;
}

unsigned int memory_manage_init() {
    memory_manage_struct.total_size = 0;
//    memory_manage_struct.allocate_space = 0;
    for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++) {
        memory_manage_struct.total_size += memory_manage_struct.area[index].size;
//        memset(memory_manage_struct.area[index].start_address,0,memory_manage_struct.area[index].size);
    }
    unsigned int keep_per_sectors = PAGE_SIZE / sizeof(unsigned short);
    unsigned int MAT_page = 1;
    while (MAT_page * keep_per_sectors < memory_manage_struct.total_size / PAGE_SIZE - MAT_page)
        MAT_page++;
    if (MAT_page * PAGE_SIZE > memory_manage_struct.area[0].size)
        return OS_FAILED;
    memory_manage_struct.MAT_start = (unsigned short *) memory_manage_struct.area[0].start_address;
    memory_manage_struct.MAT_end = (unsigned short *) (memory_manage_struct.area[0].start_address +
                                                       MAT_page * PAGE_SIZE);
    memory_manage_struct.total_size -= MAT_page * PAGE_SIZE;

    if (memory_manage_struct.MAT_end == memory_manage_struct.area[0].end_address) {
        memory_manage_struct.memory_area_num--;
        if (memory_manage_struct.memory_area_num == 0)
            return OS_FAILED;
        for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++)
            memory_manage_struct.area[index] = memory_manage_struct.area[index + 1];
        memory_manage_struct.area[memory_manage_struct.memory_area_num].start_address = 0;
        memory_manage_struct.area[memory_manage_struct.memory_area_num].end_address = 0;
        memory_manage_struct.area[memory_manage_struct.memory_area_num].start_page_id = 0;
        memory_manage_struct.area[memory_manage_struct.memory_area_num].size = 0;
    } else {
        memory_manage_struct.area[0].start_address = memory_manage_struct.MAT_end;
        memory_manage_struct.area[0].size -= MAT_page * PAGE_SIZE;
    }

    for (unsigned int index = 1; index < memory_manage_struct.memory_area_num; index++) {
        memory_manage_struct.area[index].start_page_id = memory_manage_struct.area[index - 1].start_page_id +
                                                         memory_manage_struct.area[index - 1].size / PAGE_SIZE;
    }

    unsigned int MAT_sum = memory_manage_struct.total_size / PAGE_SIZE;
    memory_manage_struct.MAT_start = memory_manage_struct.MAT_end - MAT_sum;
    for (unsigned int times = 0; times < MAT_sum; times++)
        memory_manage_struct.MAT_start[times] = MAT_FREE;
    return OS_SUCCESS;
}


void *memory_manage_allocate(unsigned long size) {
    if (size == 0)
        return NULL;
    //如果size大于unsigned short最大值，则其只能进行整块分配
    if (size >= 0x10000 && size & (PAGE_SIZE - 1))
        size = (size + PAGE_SIZE) & ~(PAGE_SIZE - 1);
    //8字节对齐
    if (size & 0x07)
        size = ((size >> 3) << 3) + 8;
    if ((size & (PAGE_SIZE - 1)) > PAGE_SIZE - 32)
        size = (size + PAGE_SIZE) & ~(PAGE_SIZE - 1);

    if (size >= PAGE_SIZE) {
        unsigned int alloc_page = size / PAGE_SIZE;
        if (size & (PAGE_SIZE - 1))
            alloc_page++;
        unsigned int start_page_id = 0xFFFF;
        unsigned int MAT_start_page;
        unsigned int MAT_end_page;
        unsigned int current_page_id;

        for (unsigned int area_index = 0; area_index < memory_manage_struct.memory_area_num; area_index++) {
            MAT_start_page = memory_manage_struct.area[area_index].start_page_id;
            MAT_end_page = MAT_start_page + memory_manage_struct.area[area_index].size / PAGE_SIZE;
            if(memory_manage_struct.area[area_index].locked)
                continue;
            memory_manage_struct.area[area_index].locked  = 1;
            for (current_page_id = MAT_start_page; current_page_id < MAT_end_page; current_page_id++) {
                //如果当前MAT为FREE,若未设置start_page_id 则设置 start_page_id

                if (memory_manage_struct.MAT_start[current_page_id] == MAT_FREE) {
                    if (start_page_id == 0xFFFF)
                        start_page_id = current_page_id;
                    if (current_page_id - start_page_id >= alloc_page - 1)
                        break;
                }
                //如果非MAT_FREE则重置start_page_id
                if (memory_manage_struct.MAT_start[current_page_id] != MAT_FREE && start_page_id != 0xFFFF)
                    start_page_id = 0xFFFF;
            }
            if (start_page_id != 0xFFFF && current_page_id - start_page_id == alloc_page - 1) {
                struct memory_page_header *start_page_address = memory_manage_struct.area[area_index].start_address +
                                                                (start_page_id -
                                                                 memory_manage_struct.area[area_index].start_page_id) *
                                                                PAGE_SIZE;
                for (unsigned int MAT_index = start_page_id; MAT_index < current_page_id; MAT_index++)
                    memory_manage_struct.MAT_start[MAT_index] = MAT_index + 1;
                memory_manage_struct.MAT_start[current_page_id] = MAT_END;
                if (size & (PAGE_SIZE - 1)) {
                    memory_manage_struct.MAT_start[start_page_id] = MAT_USED;
                    start_page_address->content_area_start = PAGE_SIZE - (size & (PAGE_SIZE - 1));
                    start_page_address->max_fragment_offset = 0;
                    start_page_address->ma_info->size = size;
                    start_page_address->allocate_number = 1;
                    start_page_address->ma_info->start_offset_in_page = PAGE_SIZE - (size & (PAGE_SIZE - 1));
                    start_page_address = ((void *) start_page_address) + start_page_address->content_area_start;
                }
//                memory_manage_struct.allocate_space += size;

                memory_manage_struct.area[area_index].locked  = 0;
                return start_page_address;
            }
            memory_manage_struct.area[area_index].locked  = 0;
        }
    } else {
        unsigned int MAT_start, MAT_end;
        void *page_start_address = NULL;
        struct memory_page_header *page_start = NULL;
        struct memory_area *area_ptr = NULL;
        for (unsigned int area_index = 0; area_index < memory_manage_struct.memory_area_num; area_index++) {
            area_ptr = &(memory_manage_struct.area[area_index]);
            if(area_ptr->locked)
                continue;
            area_ptr->locked = 1;
            page_start_address = area_ptr->start_address;
            MAT_start = memory_manage_struct.area[area_index].start_page_id;
            MAT_end = MAT_start + memory_manage_struct.area[area_index].size / PAGE_SIZE;
            for (unsigned int current_page_id = MAT_start; current_page_id < MAT_end; current_page_id++) {
                page_start = page_start_address;
                if(memory_manage_struct.MAT_start[current_page_id] == MAT_FREE)
                {
                    page_start->allocate_number = 0;
                    page_start->max_fragment_offset = 0;
                    page_start->content_area_start = 0;
                    memory_manage_struct.MAT_start[current_page_id] = MAT_USED;
                    void* ptr = memory_alloc(page_start_address,size);
                    if(ptr) {
//                        memory_manage_struct.allocate_space += size;
                        area_ptr->locked = 0;
                        return ptr;
                    }
                }
                if(memory_manage_struct.MAT_start[current_page_id] == MAT_USED)
                {
                    void* dest = memory_alloc(page_start_address,size);
                    if(dest) {
                        if(get_free_space(page_start) < 16)
                            memory_manage_struct.MAT_start[current_page_id] = MAT_END;
//                        memory_manage_struct.allocate_space += size;
                        area_ptr->locked = 0;
                        return dest;
                    }
                }
                page_start_address += PAGE_SIZE;
            }
            area_ptr->locked  = 0;
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

    unsigned long start_page_id =
            ((unsigned long) ptr - (unsigned long) area_ptr->start_address) / PAGE_SIZE + area_ptr->start_page_id;
    unsigned long end_MAT = area_ptr->start_page_id + area_ptr->size / PAGE_SIZE;
    if (((unsigned long) ptr & (PAGE_SIZE - 1)) == 0) {
        //对整块进行释放
        unsigned int current_page_id;
        if (memory_manage_struct.MAT_start[start_page_id] == MAT_FREE)
            return;
        for (current_page_id = start_page_id; current_page_id < end_MAT; current_page_id++) {
            if (memory_manage_struct.MAT_start[current_page_id] == MAT_END)
                break;
            memory_manage_struct.MAT_start[current_page_id] = MAT_FREE;
        }
        memory_manage_struct.MAT_start[current_page_id] = MAT_FREE;

//        memory_manage_struct.allocate_space -= (current_page_id - start_page_id + 1) * PAGE_SIZE;
        return;
    }
    struct memory_page_header *start_page = (struct memory_page_header *) ((unsigned long) ptr & ~(PAGE_SIZE - 1));
    struct memory_alloc_info* temp_info = memory_alloc_info_find(start_page,ptr);
    unsigned long index = 0;
    if(temp_info == NULL)
        return;
    //如果是大块内存则先释放其后的内存页
    if (temp_info->size >= PAGE_SIZE) {
        unsigned int pages = temp_info->size / PAGE_SIZE;
        for (unsigned int times = 0; times < pages; times++)
            memory_manage_struct.MAT_start[start_page_id + 1 + times] = MAT_FREE;
//        memory_manage_struct.allocate_space -= pages * PAGE_SIZE;
        temp_info->size &= PAGE_SIZE - 1;
    }

    if (start_page->allocate_number == 0)
        memory_manage_struct.MAT_start[start_page_id] = MAT_FREE;
    else {
        memory_free(start_page,ptr);
        if(get_free_space(start_page) > 16)
            memory_manage_struct.MAT_start[start_page_id] = MAT_USED;
    }

//    memory_manage_struct.allocate_space -= temp_info->size;
}

void memory_manage_sort_out(unsigned int pageID)
{
    struct memory_area *area_ptr = NULL;
    for(unsigned int index = 1;index < memory_manage_struct.memory_area_num;index++)
    {
        if(pageID < memory_manage_struct.area[index].start_page_id)
        {
            area_ptr = &memory_manage_struct.area[index - 1];
            break;
        }
    }
    if(area_ptr == NULL)
        area_ptr = &memory_manage_struct.area[memory_manage_struct.memory_area_num - 1];
    unsigned int page_offset = pageID - area_ptr->start_page_id;
    void * page_start_address = area_ptr->start_address + page_offset * PAGE_SIZE;
    memory_fragment_merge(page_start_address);
}
