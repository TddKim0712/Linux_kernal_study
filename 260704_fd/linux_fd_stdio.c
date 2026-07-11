
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(){
 int fd_output = open("./output.txt",
                         O_WRONLY | O_CREAT | O_TRUNC,
                         0644);

// fd 생성 후 확인, 보통 0,1,2 다음 숫자인 3이 나옴.
    if (fd_output == -1) {
        perror("open output.txt failed");
        return 1;
    }
printf("file descriptor: %d\n", fd_output);

int fd_null = open("/dev/null", O_WRONLY);
printf("file descriptor: %d\n", fd_null);

if (fd_null == -1) {
    write(1, "/dev/null open failed\n", sizeof("/dev/null open failed\n") - 1);
    return -1;
}

int wn = write(1, "/dev/null opening \n", sizeof("/dev/null opening \n") - 1);

dprintf(1, "write returned: %d\n", wn);

write(1, "stdout still visible\n", sizeof("stdout still visible\n")- 1);

dup2(fd_null, 1);

write(fd_null, "writing on fd null.. can't u see??\n", sizeof("writing on fd null. can't u see??\n")-1);
perror("fd_null printed on stderr\n");


dup2(fd_output, 1);
dup2(fd_output, 2);
perror("this will be logged in output.txt file, not printed on terminal\n");

write(fd_output, "this will be logged in output.txt file,\n\n right now fd 1 and 2 is logging at output.txt\n", sizeof( "this will be logged in output.txt file,\n\n right now fd 1 and 2 is logging at output.txt\n")-1);
close(fd_null);
close(fd_output);
write(1, "stdout visible?\n", sizeof("stdout visible?\n")- 1);

    return 0;
}