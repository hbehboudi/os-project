#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FIB_DEV_File "/dev/fibonacci"

int main() {
    int fibonacci_dev_file = open(FIB_DEV_File, O_RDWR);

    if (fibonacci_dev_file < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    char buf[1];

    int number;

    do {
        printf("Please enter a number between 0-92: ");
        scanf("%d", &number);
    } while(number > 92 || number < 0);

    lseek(fibonacci_dev_file, number, SEEK_SET);

    long long result = read(fibonacci_dev_file, buf, 1);

    printf("Fib(%d) = %lld\n", number, result);

    close(fibonacci_dev_file);
    return 0;
}

