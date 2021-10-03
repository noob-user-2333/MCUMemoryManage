
#include "memory.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char buffer[2048000] = {0};
void *ptr_list[1024];
unsigned int size_list[1024];
unsigned short* mat_ptr[16];

void *start_ptr;
const char file_name[] = "/dev/shm/memoryDisplay.bin";
unsigned char file[512] = {0};

void memory_display() {
    int fd = open(FILE_PATH, O_CREAT | O_RDWR, 0666);
    write(fd, start_ptr, 1024);
    close(fd);
}

extern struct memory_manage memory_manage_struct;

void memory_free(void *page_start_address,void *ptr);

void *memory_alloc(void *page_start_address,unsigned int size);








int main() {
    printf("Hello, World!\n");
    start_ptr = (void*)(((unsigned long)buffer &~(PAGE_SIZE - 1)) + PAGE_SIZE);
    memset(start_ptr,0,0x2000);
    memory_manage_area_add(start_ptr,start_ptr + 0x10000);
//    memory_manage_area_add(start_ptr + 0x3000,start_ptr + 0x4000);
//    memory_manage_area_add(start_ptr + 2048,start_ptr + 4096);
//    memory_manage_area_add(start_ptr,start_ptr + 4096);
    memory_manage_init();
    for(unsigned int times = 0;times <512;times++)
        mat_ptr[times] = &memory_manage_struct.MAT_start[times];
    unsigned int times = 0;
    unsigned int free = 0;
    unsigned int flag = 0;
    for(times = 0;times - free <1024;times++)
    {
        unsigned long num = rand();

        size_list[times - free] = ((num >> 3 << 3) & ((PAGE_SIZE - 1))) ;
        if(num & 0x01)
            size_list[times - free] += PAGE_SIZE;
        void *ptr= memory_manage_allocate(size_list[times - free]);
        ptr_list[times - free] = ptr;
        if(ptr_list[times - free] == NULL)
            break;
        for(unsigned int index = 0 ;index < size_list[times - free] >> 2;index++)
        {
            ((unsigned int*)ptr_list[times - free])[index] = index;
        }
        printf("alloc the point: 0x%lx,length: 0x%x\n",ptr_list[times - free] - start_ptr ,size_list[times - free]);
        if(times & 0x01)
        {
            memory_manage_free(ptr_list[times - free]);
            printf("free the point:0x%lx ,length:0x%x\n",ptr_list[times - free] - start_ptr,size_list[times - free]);
            free++;
        }
    }

    for(unsigned int index = 0;index < times - free;index++)
    {
        for(unsigned int indexs = 0;indexs < size_list[index] >> 2;indexs++)
        {
            if(((unsigned int*)ptr_list[index])[indexs] != indexs)
            {
                printf("error happen in point[%d]: 0x%lx,length: 0x%x\n",index,ptr_list[index] - start_ptr ,size_list[index]);
                break;
            }
        }

    }
    printf("current allocate block :%d\n",times - free);
    printf("total size:%d alloca size:%d",memory_manage_struct.total_size,memory_manage_struct.allocate_space);
    memory_display();
    return 0;
}
