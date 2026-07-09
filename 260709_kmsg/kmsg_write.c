#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main (void) {

    int fd = open("/dev/kmsg", O_WRONLY);

    if (fd < 0){
        perror("failed to open kmsg\n");
        return -1;
    }
    const char *msg = "C code to use write() to /dev/kmsg\n";

    write(fd, msg, strlen(msg));

    close (fd);

    return 0;   
}