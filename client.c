#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FIB_DEV_File "/dev/fibonacci"

#define size 1024

int main() {
    int fibonacci_dev_file = open(FIB_DEV_File, O_RDWR);

    if (fibonacci_dev_file < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    lseek(fibonacci_dev_file, 4, SEEK_SET);

    char buf[size];

    ssize_t result = read(fibonacci_dev_file, buf, size);

    printf("%s\n", buf);

    close(fibonacci_dev_file);
    return 0;
}

