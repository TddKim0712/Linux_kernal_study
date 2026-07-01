/*
fd 0 = stdin   = 표준 입력
fd 1 = stdout  = 표준 출력
fd 2 = stderr  = 표준 에러
*/
#include <unistd.h>
#include <stdio.h>
/*
ssize_t write(size_t count;
                     int fd, const void buf[count], size_t count);


*/
int main(){
/*
    msg1, msg2는 main() 안의 지역 배열이다.
    따라서 문자열 내용이 stack에 저장된다.
    */
    const char msg1[] = "this is stdout, fd=1\n"; 
    const char msg2[] = "this is stderr, fd=2\n";
    write(1, "this is stdout, fd=1\n", sizeof(msg1) - 1);
   // write(2, "this is stderr, fd=2\n", 28000000);

   // write(2, "this is stderr, fd=2\n", 12260);
    write(2, "this is stderr, fd=2\n", sizeof(msg2) - 1);
    printf("pid= %d\n", getpid());
    printf("msg address= %p\n", msg2);
    sleep(20);
    return 0;
}


/*
컴파일 후 linux_fd_print를 실행파일로 만들고 나면,
shell 코드:  ./linux_fd_print > out1.txt 와 ./linux_fd_print 2> out2.txt 를 해보자.

첫 shell 코드의 > 는 fd==1 (stdout) 스트림을 out1.txt로 저장, 남은 this is stderr...은 그대로 쭉 출력
둘째 shell 코드의 2> 는 fd==2 (stderr) 스트림을 out2.txt로 저장, 남은 this is stdout...은 그대로 출력

문제는 의도적으로 write(2 ...)에서 세번째 인자 count를 매우 큰 숫자로 설정해서 멈추지 않고 메모리를 읽는 상황이다.
그래서 터미널에서 깨지는 글씨가 나오며, 둘째 shell 코드를 실행하면 out2.txt의 크기가 매우크다. (out1.txt: 21 byte, out2.txt: 12,262 byte )

실제로 sizeof(msg1)은 22바이트이므로 out1.txt의 크기는 OKAY
하지만 out2.txt의 크기는 write가 진행된 만큼의 크기가 아님.
만약 count 값을 줄여서 12264로 할 경우, out2.txt 크기는 여전히 12,262 byte
만약 count 값을 줄여서 12260으로 할 경우, out2.txt 크기는 12,260 byte

이를 통해 "this is stderr, fd=2\n" 문구가 write되는 것은 메모리를 받아온다는 개념이고, 12262바이트까지가 읽기 가능한 메모리임 유추가능

메모리 맵 구조 : <.text> <.rodata> <.data/.bss> <heap> ... <stack>--> 기본(초기) 메모리 구조
OS 멀티태스킹 구조 --> 가상메모리 활용: 실제 물리주소 그대로 맵핑되지 않고 가상주소로 분리됨
복수의 프로세스들이 별도의 넓은 메모리를 각자 따로 사용하는 것처럼 가상화 

이번엔 더 자세히, pid를 확인한 후 
./linux_fd_print &
pid=$!
sleep 0.2
cat /proc/$pid/maps | grep -E 'linux_fd_print|\[stack\]'



*/