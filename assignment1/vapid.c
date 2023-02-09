#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>

extern int errno;

int main(int argc, char* argv[])
{
    uint32_t number = 0;
    uint32_t temp = 0;
    uint32_t totalSectionsSize = 0;
    uint32_t imageBaseSize = 0;
    uint32_t imageOptionalHeaderStart = 0;
    uint32_t targetSectionAddress = 0;
    uint32_t targetRawAddress = 0;
    uint32_t TARGET_NUMBER = 0;

    int numberOfSections = 0;
    int sizeOfSectionHeader = 40;

     if(argc != 3){
        printf("Incorrect number of arguments provided (must be 2)!\n");
        return 1;
    }

    char* hex_value = argv[2];
    char* temp_string = &hex_value[2];

    TARGET_NUMBER= (int)strtol(temp_string, NULL, 16);
    // printf("%x\n", TARGET_NUMBER);

    int fd = open("./sample.exe", O_RDONLY);

    //open the file and check to see if it causes an error while opening
    if (fd < 0) {
        printf("Error opening file: %s", strerror(errno));
        return 1;
    }
    
    //seek to the 60th byte from the beginning of the file [inside _IMAGE_DOS_HEADER]
    lseek(fd, 60, SEEK_SET);
    int result = read(fd, &number, sizeof(uint32_t));
    //printf("seeking to 60: %d\n", number);
    if (result != sizeof(uint32_t)) {
        printf("Did not read the entire index: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    // The four bytes that we read from the beginning of the file tell
    // us the location (in the file!) of the integer that we want to read.
    // So, we will *seek* to that location in the file.
    if (number != lseek(fd, number, SEEK_SET)) {

        printf("Did not seek to the proper place in the file!\n");
        close(fd);
        return 1;
    }

     //finding the imageBase
    int imageBase = 0;
    lseek(fd, number+52, SEEK_SET);
    //printf("seeking to 60+52: %d\n", number);
    imageBase = read(fd, &imageBaseSize, sizeof(uint32_t));
    // printf("size of image base =  %x bytes\n", imageBaseSize);
     if (imageBase != sizeof(uint32_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    //calculating the size of all sections combined
    int sizeOfSections = 0;
    lseek(fd, number+28, SEEK_SET);
    //printf("seeking to 60+28: %d\n", number);
    sizeOfSections = read(fd, &totalSectionsSize, sizeof(uint32_t));
    // printf("size of sections =  %d bytes\n", totalSectionsSize);
     if (sizeOfSections != sizeof(uint32_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    //finding number of sections in the file
    int numSections = 0;
    lseek(fd, number+6, SEEK_SET);
    //printf("seeking to 60+6: %d\n", number);
    numSections = read(fd, &numberOfSections, sizeof(uint16_t));
    //  printf("number of sections =  %d\n", numberOfSections);
     if (numSections != sizeof(uint16_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    int sizeOfEachSection = totalSectionsSize / numberOfSections;

    // printf("size of each section = %d bytes\n", sizeOfEachSection);

    // Read the next 4 bytes - these are the 4 bytes for reading the e_lfanew which takes us
    // to _IMAGE_OPTIONAL_HEADER
  
    int imageOptionalHeader = 0;
    lseek(fd, number+24, SEEK_SET);
    //printf("seeking to 60+24: %d\n", number);
    result = read(fd, &temp, sizeof(uint32_t));
    if (result != sizeof(uint32_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }    

    //INSIDE _IMAGE_OPTIONAL_HEADER
    // printf("image optional header starting address = 0x%x\n", temp);

    // TARGET_NUMBER += imageBase;

    //seeking into the beginning of the section header

    lseek(fd, number+24+224, SEEK_SET);
    //printf("seeking to 60+228: %d\n", number);
    read(fd, &temp, sizeof(uint32_t));
    // printf("\n0x%x\n", temp);

    // adding the image base to the target number
    TARGET_NUMBER += imageBaseSize;

    uint32_t virtualAddressOfSection = 0;
    int count = 12;
    int loop_counter = 0;

    //finding the section where the data belongs to
    while(TARGET_NUMBER > virtualAddressOfSection) {
        lseek(fd, number+248+count, SEEK_SET);
        read(fd, &virtualAddressOfSection, sizeof(uint32_t));
        virtualAddressOfSection += imageBaseSize;
        count+=sizeOfSectionHeader;

        // printf("%d\t%d\t0x%x\t0x%x\t0x%x\n", loop_counter, numberOfSections, TARGET_NUMBER, virtualAddressOfSection, virtualAddressOfSection+sizeOfEachSection);

        if ((loop_counter >= numberOfSections) && (TARGET_NUMBER > (virtualAddressOfSection+sizeOfEachSection))) {
            // printf("(in the if conditional) %d\t%d\t0x%x\t0x%x\n", loop_counter, numberOfSections, TARGET_NUMBER, virtualAddressOfSection);
            // printf("%d\n", (numberOfSections*sizeOfSectionHeader)+12);
            printf("0x%x -> ??\n", TARGET_NUMBER-imageBaseSize);
            return 1;
        }
        
        loop_counter+=1;
    }

    // printf("count= %d\n", count);

    // FINDING THE OFFSET

    // find the virtual address of the section
    lseek(fd, number+248+count-80, SEEK_SET);
    read(fd, &targetSectionAddress, sizeof(uint32_t));

    int offset = TARGET_NUMBER - (targetSectionAddress+imageBaseSize);

    // printf("offset: 0x%x - 0x%x = 0x%x\n", TARGET_NUMBER, targetSectionAddress+imageBaseSize, offset);

    //finding the physical address on disk for the requested byte address
    lseek(fd, number+248+count-80+8, SEEK_SET);
    read(fd, &targetRawAddress, sizeof(uint32_t));

    int targetNumberOnPhysicalDisk = offset + targetRawAddress;

    // printf("FINAL ANSWER: 0x%x + 0x%x = 0x%x\n", offset, targetRawAddress, targetNumberOnPhysicalDisk);

    printf("0x%x -> 0x%x\n", TARGET_NUMBER-imageBaseSize, targetNumberOnPhysicalDisk);

    close(fd);
    return 0;
}