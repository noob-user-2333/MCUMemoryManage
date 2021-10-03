#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "memory.h"


struct memory_manage memory_manage_struct = {0};

void memory_alloc_info_insert(void *page_start_address, struct memory_alloc_info ma_info);

void *memory_alloc_from_page_top(void *page_start_address, unsigned long size);

void memory_fragment_insert(void *page_start_address, unsigned long start_offset_in_page, unsigned long size);

struct memory_fragment_info *memory_fragment_get(void *page_start_address, unsigned long size);

struct memory_alloc_info
memory_fragment_divide(void *page_start_address, struct memory_fragment_info *fragment_ptr, unsigned long size);

struct memory_alloc_info memory_alloc_info_delete(void *page_start_address, void *ptr);

unsigned int get_free_space(struct memory_page_header *page) {
    unsigned int free_space = page->content_area_start - 8 - sizeof(struct memory_alloc_info) * page->allocate_number;
    return free_space;
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
    memory_manage_struct.memory_area_num++;
    return OS_SUCCESS;
}

unsigned int memory_manage_init() {
    memory_manage_struct.total_size = 0;
    memory_manage_struct.allocate_space = 0;
    for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++) {
        memory_manage_struct.total_size += memory_manage_struct.area[index].size;
        memset(memory_manage_struct.area[index].start_address,0,memory_manage_struct.area[index].size);
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
                memory_manage_struct.allocate_space += size;
                return start_page_address;
            }
        }
    } else {
        unsigned int MAT_start, MAT_end;
        void *page_start_address = NULL;
        struct memory_page_header *page_start = NULL;
        struct memory_area *area_ptr = NULL;
        for (unsigned int area_index = 0; area_index < memory_manage_struct.memory_area_num; area_index++) {
            area_ptr = &(memory_manage_struct.area[area_index]);
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
                        memory_manage_struct.allocate_space += size;
                        return ptr;
                    }
                }
                if(memory_manage_struct.MAT_start[current_page_id] == MAT_USED)
                {
                    void* dest = memory_alloc(page_start_address,size);
                    if(dest) {
                        if(get_free_space(page_start) < 16)
                            memory_manage_struct.MAT_start[current_page_id] = MAT_END;
                        memory_manage_struct.allocate_space += size;
                        return dest;
                    }
                }
                page_start_address += PAGE_SIZE;
            }
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
        memory_manage_struct.allocate_space -= (current_page_id - start_page_id + 1) * PAGE_SIZE;
        return;
    }
    struct memory_page_header *start_page = (struct memory_page_header *) ((unsigned long) ptr & ~(PAGE_SIZE - 1));
    struct memory_alloc_info temp_info = memory_alloc_info_delete(start_page,ptr);
    unsigned long index = 0;
    if(temp_info.start_offset_in_page == 0|| temp_info.size == 0)
        return;
    //如果是大块内存则先释放其后的内存页
    if (temp_info.size >= PAGE_SIZE) {
        unsigned int pages = temp_info.size / PAGE_SIZE;
        for (unsigned int times = 0; times < pages; times++)
            memory_manage_struct.MAT_start[start_page_id + 1 + times] = MAT_FREE;
        memory_manage_struct.allocate_space -= pages * PAGE_SIZE;
        temp_info.size &= PAGE_SIZE - 1;
    }

    if (start_page->allocate_number == 0)
        memory_manage_struct.MAT_start[start_page_id] = MAT_FREE;
    else {
        memory_free(start_page,ptr);
        if(get_free_space(start_page) > 16)
            memory_manage_struct.MAT_start[start_page_id] = MAT_USED;
    }

    memory_manage_struct.allocate_space -= temp_info.size;
}

