# Linux `write()`와 Stack 메모리 실험

이 실험은 Linux에서 `write()`가 문자열을 어떻게 출력하는지, 지역 배열이 메모리에 어디 저장되는지, 그리고 `/proc/<pid>/maps`를 통해 프로세스 메모리 권한을 어떻게 확인하는지 알아보기 위한 실습이다.

일반적인 `printf()`보다 더 낮은 수준에서 출력이 어떻게 동작하는지 보는 것이 목적이다.

---

## 1. 기본 개념

Linux 프로세스는 기본적으로 다음 세 개의 파일 디스크립터를 가진다.

```txt
fd 0 = stdin   = 표준 입력
fd 1 = stdout  = 표준 출력
fd 2 = stderr  = 표준 에러
```

`write()` 시스템콜의 원형은 다음과 같다.

```c
ssize_t write(int fd, const void *buf, size_t count);
```

의미는 다음과 같다.

```txt
fd에 buf 주소부터 count 바이트만큼 쓴다.
```

여기서 중요한 점은 `write()`가 문자열 함수가 아니라는 것이다.

`printf()`는 문자열을 출력할 때 `\0`을 기준으로 문자열의 끝을 판단하지만, `write()`는 그렇지 않다.  
`write()`는 세 번째 인자인 `count`만 보고 그 바이트 수만큼 출력하려고 한다.

즉, 다음과 같이 생각하면 된다.

```txt
write()는 “문자열을 출력한다”가 아니라
“이 주소부터 N바이트를 출력한다”에 가깝다.
```

---

## 2. 실험 코드

파일명: `linux_fd_print.c`

```c
/*
fd 0 = stdin   = 표준 입력
fd 1 = stdout  = 표준 출력
fd 2 = stderr  = 표준 에러

write() 원형:
    ssize_t write(int fd, const void *buf, size_t count);

write()는 문자열의 끝을 자동으로 판단하지 않는다.
세 번째 인자인 count만큼 buf 주소에서 읽어서 fd에 쓴다.
*/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(void)
{
    /*
    msg1, msg2는 main() 안에서 선언된 지역 배열이다.

    const char msg2[] = "...";

    이 형태는 문자열을 가리키는 포인터가 아니라,
    실제 배열을 현재 함수의 stack 영역에 만드는 방식이다.
    */
    const char msg1[] = "this is stdout, fd=1\n";
    const char msg2[] = "this is stderr, fd=2\n";

    /*
    정상적인 출력.

    sizeof(msg1)는 배열 전체 크기이다.
    문자열 끝의 '\0'까지 포함하므로 실제 출력 길이는 sizeof(msg1) - 1이다.
    */
    write(1, msg1, sizeof(msg1) - 1);
    write(2, msg2, sizeof(msg2) - 1);

    /*
    현재 프로세스의 PID와 msg2의 주소를 출력한다.

    나중에 /proc/<pid>/maps에서 이 주소가 어느 메모리 영역에
    포함되는지 확인할 수 있다.
    */
    printf("pid = %d\n", getpid());
    printf("msg2 address = %p\n", (void *)msg2);

    /*
    의도적인 over-read 실험.

    msg2는 실제로는 약 22바이트 정도의 작은 배열이다.
    하지만 write()에게 28,000,000바이트를 출력하라고 요청한다.

    그러면 write()는 msg2 뒤쪽의 stack 메모리까지 계속 읽으려고 한다.
    이 과정에서 문자열 뒤에 있던 다른 메모리 내용이 함께 출력될 수 있다.
    */
    errno = 0;
    ssize_t n = write(2, msg2, 28000000);

    /*
    write()는 실제로 쓴 바이트 수를 반환한다.
    실패하면 -1을 반환하고 errno에 오류 이유가 저장된다.
    */
    printf("\nwrite returned = %zd\n", n);
    printf("errno = %d (%s)\n", errno, strerror(errno));

    /*
    /proc/<pid>/maps를 확인할 시간을 주기 위해 잠시 대기한다.
    sleep의 단위는 초이다.
    */
    sleep(20);

    return 0;
}
```

---

## 3. 컴파일

```bash
gcc -Wall -Wextra linux_fd_print.c -o linux_fd_print
```

`-Wall -Wextra` 옵션은 컴파일러 경고를 더 많이 보여준다.  
시스템 프로그래밍을 공부할 때는 경고를 최대한 켜두는 것이 좋다.

---

## 4. 실행 결과

```bash
./linux_fd_print
```

실행하면 대략 다음과 같은 출력이 나온다.

```txt
this is stdout, fd=1
this is stderr, fd=2
pid = 1839
msg2 address = 0x7fff4deabe20
this is stderr, fd=2
... 이상한 문자들 ...
write returned = 8672
errno = 0 (Success)
```

여기서 이상한 문자가 나오는 이유는 `write()`가 `msg2`의 실제 크기보다 훨씬 많은 바이트를 읽으려고 했기 때문이다.

```c
write(2, msg2, 28000000);
```

`msg2` 자체는 작은 문자열 배열이지만, `write()`는 문자열의 끝을 보고 멈추지 않는다.  
그래서 `msg2` 뒤쪽에 있는 메모리까지 계속 읽으려고 한다.

---

## 5. `/proc/<pid>/maps` 확인

프로그램이 실행 중일 때 다른 터미널에서 다음 명령어를 실행한다.

```bash
cat /proc/<pid>/maps | grep -E 'linux_fd_print|\[stack\]'
```

예를 들어 프로그램에서 다음처럼 출력되었다면:

```txt
pid = 1839
msg2 address = 0x7fff4deabe20
```

다른 터미널에서는 이렇게 확인한다.

```bash
cat /proc/1839/maps | grep -E 'linux_fd_print|\[stack\]'
```

또는 한 번에 실행하려면 다음처럼 할 수 있다.

```bash
./linux_fd_print &
pid=$!
sleep 0.2
cat /proc/$pid/maps | grep -E 'linux_fd_print|\[stack\]'
```
<img width="1060" height="254" alt="image" src="https://github.com/user-attachments/assets/5ce96368-4b55-46ac-9aac-571293fbaa9c" />


---

## 6. 실험 결과 해석

예를 들어 다음 결과가 나왔다고 하자.

```txt
msg2 address = 0x7fff4deabe20
```

그리고 `/proc/<pid>/maps`에 다음 줄이 있다고 하자.

```txt
7fff4de8d000-7fff4deae000 rw-p 00000000 00:00 0 [stack]
```

주소 범위를 비교하면 다음과 같다.

```txt
stack 시작 주소 : 0x7fff4de8d000
msg2 주소       : 0x7fff4deabe20
stack 끝 주소   : 0x7fff4deae000
```

`msg2 address`가 `[stack]` 범위 안에 들어간다.

```txt
0x7fff4de8d000 <= 0x7fff4deabe20 < 0x7fff4deae000
```

따라서 다음 코드로 만든 지역변수 배열은 실제로 stack에 저장되어 있음을 확인할 수 있다.

```c
const char msg2[] = "this is stderr, fd=2\n";
```

---

## 7. 메모리 권한 의미

`/proc/<pid>/maps`에는 각 메모리 영역의 주소 범위와 권한이 표시된다.

예를 들어 다음 줄을 보자.

```txt
7fff4de8d000-7fff4deae000 rw-p 00000000 00:00 0 [stack]
```

여기서 중요한 부분은 `rw-p`이다.

```txt
r = read 가능
w = write 가능
x = execute 가능
p = private mapping
```

권한 해석 예시:

```txt
r--p : 읽기 가능, 쓰기 불가, 실행 불가
r-xp : 읽기 가능, 실행 가능, 쓰기 불가
rw-p : 읽기 가능, 쓰기 가능, 실행 불가
```

일반적인 프로세스의 메모리 영역은 다음과 비슷하게 나뉜다.

```txt
코드 영역          r-xp
읽기 전용 데이터   r--p
전역 데이터        rw-p
heap              rw-p
stack             rw-p
```

이번 실험에서 `[stack]` 영역은 다음과 같이 표시된다.

```txt
[stack] rw-p
```

즉, stack은 읽기와 쓰기가 가능하지만 실행 권한은 없다.

---

## 8. 왜 28,000,000바이트가 전부 출력되지 않았지?

코드에서 다음 줄은 의도적으로 잘못된 요청을 한다.

```c
ssize_t n = write(2, msg2, 28000000);
```

`msg2`는 실제로 작은 배열이다.

```txt
"this is stderr, fd=2\n\0"
```

하지만 write() 시스템콜은 다음 뜻으로 해석된다.

```txt
msg2 주소부터 28,000,000바이트를 읽어서 fd 2에 써라.
```

그러면 커널은 가능한 만큼 유저 메모리에서 데이터를 읽는다.


```txt
msg2 시작 주소
↓
stack 안의 읽기 가능한 메모리를 계속 읽음
↓
stack 끝 또는 접근 불가능한 페이지에 도달
↓
더 이상 읽을 수 없음
↓
실제로 쓴 바이트 수를 반환하거나 오류 발생
```

그래서 요청은 28,000,000바이트였지만 실제 출력은 그보다 훨씬 작다.
out2.txt의 파일 크기를 보면 알 수 있다.

이것은 공백이나 문자열 끝 문자 때문이 아니다.  
공백, `\0`, 이상한 바이너리 값도 모두 바이트로 취급된다.

핵심은 운영체제가 프로세스가 접근할 수 있는 메모리 영역을 제한한다는 점이다.

---

## 9. PID와 `/proc`의 의미

`getpid()`는 현재 실행 중인 프로세스의 PID를 반환한다.

```c
printf("pid = %d\n", getpid());
```

Linux에서는 실행 중인 프로세스마다 `/proc/<pid>` 디렉터리가 생긴다.

그중 `/proc/<pid>/maps`는 해당 프로세스의 가상 메모리 배치를 보여준다.

```bash
cat /proc/<pid>/maps
```

단, 프로세스가 이미 종료되면 `/proc/<pid>`도 사라진다.

그래서 프로그램이 너무 빨리 끝나면 다음과 같은 오류가 날 수 있다.

```txt
cat: /proc/<pid>/maps: No such file or directory
```

이 실험 코드의 `sleep(20)` 존재 이유도 pid를 보기도 전에 종료됨을 방지하기 위함이다.  
프로그램이 바로 끝나지 않게 해서, 실행 중인 동안 `/proc/<pid>/maps`를 확인할 시간을 확보한다.

---

## 10. 전체 요약

```txt
- fd 1은 stdout, fd 2는 stderr이다.

- write(fd, buf, count)는 buf 주소부터 count 바이트를 fd에 쓴다.

- write()는 문자열 끝의 '\0'을 보고 자동으로 멈추지 않는다.

- const char msg[] = "..." 형태의 지역 배열은 stack에 저장된다.

- sizeof(msg) - 1은 배열 문자열의 실제 출력 길이를 구할 때 사용할 수 있다.

- 버퍼 크기보다 큰 count를 write()에 넘기면 over-read가 발생한다.

- over-read가 발생하면 문자열 뒤의 메모리까지 출력될 수 있다.

- 하지만 접근 권한이 없는 메모리 영역을 만나면 더 이상 읽을 수 없다.

- 프로세스의 메모리 영역과 권한은 /proc/<pid>/maps에서 확인할 수 있다.

- [stack] 범위 안에 msg 주소가 있으면 해당 지역 배열은 stack에 저장된 것이다.
```

이 실험의 핵심은 `write()`가 문자열 출력 함수가 아니라는 점이다.  
`write()`는 메모리 주소와 바이트 수를 직접 다루는 낮은 수준의 시스템콜이다.
