
#include "memory.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

unsigned long size = 0;
unsigned char buffer[2048000] = {0};
void *ptr_list[10240];
unsigned int size_list[10240];
unsigned short *mat_ptr[16];
#define PAGE_SIZE 0x100
void *start_ptr;
unsigned long count = 0;
const char file_name[] = "/dev/shm/memoryDisplay.bin";
unsigned char file[512] = {0};

void memory_display() {
    int fd = open(FILE_PATH, O_CREAT | O_RDWR, 0666);
    write(fd, start_ptr, 0x10000);
    close(fd);
}

extern struct memory_manage memory_manage_struct;

void memory_free(void *page_start_address, void *ptr);

void *memory_alloc(void *page_start_address, unsigned int size);

unsigned long real_size;


int main() {
    printf("Hello, World!\n");
    start_ptr = (void *) (((unsigned long) buffer & ~(AREA_ALIGN - 1)) + AREA_ALIGN);
    memset(start_ptr, 0, 0x2000);
    memory_manage_area_add(start_ptr, start_ptr + 0x1000);
    memory_manage_area_add(start_ptr + 0x6000, start_ptr + 0x8000);
    memory_manage_area_add(start_ptr + 0x9000, start_ptr + 0x10000);
//    memory_manage_area_add(start_ptr + 2048,start_ptr + 4096);
//    memory_manage_area_add(start_ptr,start_ptr + 4096);
    memory_manage_init();
    unsigned int times = 0;
    unsigned int free = 0;
    unsigned int flag = 1;
    for (times = 0,free = 0; times  < 1024; times++) {
        unsigned long num = rand();

        size_list[times] = ((num >> 3 << 3) & ((PAGE_SIZE  - 1))) ;
        if(size_list[times] == 0)
            size_list[times] = 8;
        void *ptr = memory_manage_allocate(size_list[times]);
        ptr_list[times] = ptr;
        if (ptr_list[times] == NULL) {
            size_list[times ] = 0;
            if (flag)
                break;
            else {
                for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++)
                    memory_manage_sort_out(&memory_manage_struct.area[index]);
                flag = 1;
                times--;
                continue;
            }
        }
        for (unsigned int index = 0; index < size_list[times] >> 2; index++) {
            ((unsigned int *) ptr_list[times])[index] = index;
        }
        printf("alloc the point: 0x%lx,length: 0x%x\n", ptr_list[times] - start_ptr, size_list[times]);
        if (free & 0x01) {
            memory_manage_free(ptr_list[times]);
            printf("free the point:0x%lx ,length:0x%x\n", ptr_list[times] - start_ptr, size_list[times]);
            times--;
            free++;
        }
    }

    for (unsigned int index = 0; index < times - free; index++) {
        size += size_list[index];
    }
    real_size = memory_manage_struct.area[2].alloc_size + memory_manage_struct.area[1].alloc_size +
                memory_manage_struct.area[0].alloc_size;
    size = 0;
    for (count = 0; count < times; count++) {
        memory_manage_free(ptr_list[count]);
    }
    for (times = 0,free = 0; times< 10240; times++) {
        unsigned long num = rand();

        size_list[times] = ((num >> 3 << 3) & ((PAGE_SIZE - 1))) + 8;
        void *ptr = memory_manage_allocate(size_list[times]);
        ptr_list[times ] = ptr;
        if (ptr_list[times ] == NULL) {
            size_list[times] = 0;
            if (flag)
                break;
            else {
                for (unsigned int index = 0; index < memory_manage_struct.memory_area_num; index++)
                    memory_manage_sort_out(&memory_manage_struct.area[index]);
                flag = 1;
                times--;
                continue;
            }
        }
        for (unsigned int index = 0; index < size_list[times] >> 2; index++) {
            ((unsigned int *) ptr_list[times])[index] = index;
        }
        printf("alloc the point: 0x%lx,length: 0x%x\n", ptr_list[times - free] - start_ptr, size_list[times - free]);
//        if (times & 0x01) {
//            memory_manage_free(ptr_list[times - free]);
//            size_list[times - free] = 0;
//            printf("free the point:0x%lx ,length:0x%x\n", ptr_list[times - free] - start_ptr, size_list[times - free]);
//            free++;
//        }
    }
    for (unsigned int index = 0; index < times - free; index++) {
        for (unsigned int indexs = 0; indexs < size_list[index] >> 2; indexs++) {
            if (((unsigned int *) ptr_list[index])[indexs] != indexs) {
                printf("error happen in point[%d]: 0x%lx,length: 0x%x\n", index, ptr_list[index] - start_ptr,
                       size_list[index]);
                break;
            }
        }
    }
    size = 0;
    for (unsigned int index = 0; index < times; index++) {
        size += size_list[index];
    }
    real_size = memory_manage_struct.area[2].alloc_size + memory_manage_struct.area[1].alloc_size +
                memory_manage_struct.area[0].alloc_size;
    printf("current allocate block :%d\n", times - free);
    printf("total size:0x%x", memory_manage_struct.total_size);
    memory_display();
    return 0;
}
