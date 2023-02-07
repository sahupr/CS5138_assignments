#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>

extern int errno;

#define SIZE_OF_SECTION_HEADER 40

int main()
{
    uint32_t number = 0;
    uint16_t totalSectionsSize = 0;

    int fd = open("./sample.exe", O_RDONLY);

    //open the file and check to see if it causes an error while opening
    if (fd < 0) {
        printf("Error opening file: %s", strerror(errno));
        return 1;
    }
    
    //seek to the 60th byte from the beginning of the file [inside _IMAGE_DOS_HEADER]
    lseek(fd, 60, SEEK_SET);
    int result = read(fd, &number, sizeof(uint32_t));
    printf("%d\n",number);
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

    // Read the next 4 bytes - these are the 4 bytes for reading the e_lfanew which takes us
    // to _IMAGE_OPTIONAL_HEADER
    
    result = read(fd, &number, sizeof(uint32_t));
    if (result != sizeof(uint32_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    printf("result is = %d\n", number);

    //calculating the size of all sections combined
    int sizeOfSections = 0;

    lseek(fd, 66, SEEK_SET);
    sizeOfSections = read(fd, &totalSectionsSize, sizeof(uint16_t));
    printf("size of sections =  %d\n", totalSectionsSize);
     if (sizeOfSections != sizeof(uint16_t)) {
        printf("Did not read the target value: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    

    //finding out the number of headers
    int numberOfSections = totalSectionsSize/SIZE_OF_SECTION_HEADER;

    printf("number of sections = %d\n", numberOfSections);
    
    //seeking to the beginning of the section header
    lseek(fd, 228+number, SEEK_SET);    

    close(fd);
    return 0;
}
