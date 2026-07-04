/*
shell 명령 > 은 fd를 바꾸는 기능
int dup2(int oldfd, int newfd) ;; oldfd 파이프라인을 newfd로 대체

./program > out.txt ;; stdout을 대신하여 out.txt에 저장 ;; fd = open(out.txt) 후 dup2(fd, 1) 이랑 비슷함.
./program 2> err.txt ;; stderr을 대신하여 err.txt에 저장 ;; fd = open(err.txt) 후 dup2 (fd, 2) 이랑 비슷
./program 2> /dev/null ;; stderr가 출력 되지 않음 ;; fd = open(/dev/null) 후 dup2 (fd, /dev/null)
./program > output.txt 2>&1 
;; 순서대로 풀면, fd = open(output.txt) ; dup2(fd,1) ; dup2(1,2)
;; output.txt가 먼저 stdout (1번 fd)를 대신해서 1번이 된다. 그 후 그 1번이 stderr (2번 fd)를 대체한다. 결과적으로 fd 1, 2가 output.txt으로 redirection
;; 결국 원래라면 stdout 또는 stderr을 타고 가야할 출력이 전부 파일로 저장된다.

++
dprintf(fd, buf, len)을 사용하면 fd를 마음대로 바꿀 수 있다.
printf()는 fd가 항상 stdout으로 고정인 상태

전통적 방식: snprintf로 버퍼(출력하고자 하는 내용)을 만들고, write 한다
snprintf(buf, sizeof(buf), "value = %d\n", 123);
write(fd, buf, strlen(buf));


*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(){
int fd_output = open("./output.txt", O_WRONLY|O_APPEND|O_CREAT);

// fd 생성 후 확인, 보통 0,1,2 다음 숫자인 3이 나옴.
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


dup2(fd_output, 2);
perror("this will be logged in output.txt file, not printed on terminal\n");
dup2(1, 2);
write(fd_output, "this will be logged in output.txt file, right now fd 1 and 2 is logging at output.txt\n", sizeof( "this will be logged in output.txt file, right now fd 1 and 2 is logging at output.txt")-1);
close(fd_null);
close(fd_output);
write(1, "stdout visible?\n", sizeof("stdout visible?\n")- 1);

    return 0;
}