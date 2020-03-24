#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FIB_DEV_File "/dev/fibonacci"

int main() {
    int fibonacciDevFile = open(FIB_DEV_File, O_RDWR);

    if (fibonacciDevFile < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    char buf[1];

    int number;

    do {
        printf("Please enter a number between 0-92: ");
        scanf("%d", &number);
    } while(number > 92 || number < 0);

    lseek(fibonacciDevFile, number, SEEK_SET);

    long long result = read(fibonacciDevFile, buf, 1);

    printf("Fib(%d) = %lld\n", number, result);

    close(fibonacciDevFile);
    return 0;
}

