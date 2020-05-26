#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define DEV_File "/dev/process"

int main(int argc, char const *argv[]) {
    char *pid_tag = "--pid", *period_tag = "--period";

    int pid, period = 1, i;

    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], pid_tag) == 0) {
            pid = atoi(argv[++i]);
        } else if (strcmp(argv[i], period_tag) == 0) {
            period = atoi(argv[++i]);
        }
    }

    int dev_file = open(DEV_File, O_RDWR);

    if (dev_file < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    char buf[4096];

    while (true)
    {
        lseek(dev_file, pid, SEEK_SET);
        ssize_t result = read(dev_file, buf, 4096);

        printf("\nPID: %d\n%s\n\n----------------------\n", pid, buf);

        sleep(period);
    }
}
