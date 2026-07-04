 # Linux FD Redirection 정리

이 문서는 Linux에서 `stdout`, `stderr`, `dup2()`, shell redirection, `/dev/null`, `dprintf()`, `snprintf() + write()` 방식의 차이를 정리한 문서이다.
<img width="721" height="136" alt="image" src="https://github.com/user-attachments/assets/491a5eca-025a-4760-9d57-e3770a5c8b16" />
<img width="505" height="181" alt="image" src="https://github.com/user-attachments/assets/4acc93b0-228c-4b4f-bcc2-82ec7d001c58" />

---

## 1. 기본 개념

Linux에서 입출력은 보통 **파일 디스크립터(fd)** 를 통해 이루어진다.

| fd 번호 | 이름       | 의미       |
| ----: | -------- | -------- |
|   `0` | `stdin`  | 표준 입력    |
|   `1` | `stdout` | 표준 출력    |
|   `2` | `stderr` | 표준 에러 출력 |

예를 들어 `printf()`는 보통 `stdout`, 즉 **fd 1번**으로 출력된다.

```c
printf("hello\n");
```

이는 개념적으로 다음과 비슷하다.

```c
write(1, "hello\n", 6);
```

---

## 2. Shell의 `>` 는 fd 연결 대상을 바꾸는 기능

Shell에서 사용하는 `>` 는 프로그램 내부 코드를 바꾸는 것이 아니라, **프로그램이 실행되기 전에 fd 연결 대상을 바꾸는 기능**이다.

예를 들어 다음 명령은

```bash
./program > out.txt
```

프로그램의 `stdout`, 즉 fd `1`번을 터미널이 아니라 `out.txt` 파일로 연결한다.

개념적으로는 다음 코드와 비슷하다.

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, 1);
close(fd);
```

즉, 원래 `stdout`으로 나가야 할 출력이 파일로 저장된다.

---

## 3. `dup2()`의 의미

```c
int dup2(int oldfd, int newfd);
```

`dup2()`는 `oldfd`가 가리키는 대상을 `newfd`가 대신 가리키게 만든다.

쉽게 말하면 다음과 같다.

```c
dup2(fd, 1);
```

이 코드는

> fd가 가리키는 파일을 stdout(fd 1번)의 새 출력 대상으로 만든다

는 뜻이다.

즉, `dup2(oldfd, newfd)`는 다음처럼 이해하면 된다.

```text
newfd를 oldfd와 같은 곳을 가리키게 만든다
```

---

## 4. 자주 쓰는 redirection 예시

### 4.1 stdout을 파일로 저장

```bash
./program > out.txt
```

의미:

```text
stdout(fd 1)을 out.txt로 보낸다
```

개념적 동작:

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, 1);
close(fd);
```

결과적으로 `printf()` 출력이 터미널이 아니라 `out.txt`에 저장된다.

---

### 4.2 stderr를 파일로 저장

```bash
./program 2> err.txt
```

의미:

```text
stderr(fd 2)를 err.txt로 보낸다
```

개념적 동작:

```c
int fd = open("err.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, 2);
close(fd);
```

`stderr`로 출력되는 에러 메시지만 `err.txt`에 저장된다.

---

### 4.3 stderr를 버리기

```bash
./program 2> /dev/null
```

의미:

```text
stderr(fd 2)를 /dev/null로 보낸다
```

개념적 동작:

```c
int fd = open("/dev/null", O_WRONLY);
dup2(fd, 2);
close(fd);
```

`/dev/null`은 쓰레기통 같은 특수 파일이다.
여기로 보낸 출력은 화면에 보이지 않고 저장되지도 않는다.

주의할 점:

```c
dup2(fd, /dev/null);   // 틀린 표현
```

이 아니라

```c
dup2(fd, 2);           // 맞는 표현
```

이다.

`/dev/null`은 fd 번호가 아니라 파일 경로이고, `2`가 stderr의 fd 번호이다.

---

### 4.4 stdout과 stderr를 같은 파일로 저장

```bash
./program > output.txt 2>&1
```

의미:

```text
stdout과 stderr를 모두 output.txt로 보낸다
```

순서대로 풀면 다음과 같다.

```c
int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

dup2(fd, 1);   // stdout을 output.txt로 변경
dup2(1, 2);    // stderr도 현재 stdout이 가리키는 곳으로 변경

close(fd);
```

중요한 점은 **순서**이다.

```bash
./program > output.txt 2>&1
```

는 다음 순서로 동작한다.

```text
1. output.txt를 연다
2. stdout(fd 1)을 output.txt로 바꾼다
3. stderr(fd 2)를 현재 stdout(fd 1)이 가리키는 곳으로 바꾼다
```

결과적으로 `stdout`과 `stderr`가 모두 `output.txt`로 저장된다.

---

## 5. `2>&1`의 의미

```bash
2>&1
```

은

```text
fd 2를 fd 1이 가리키는 곳과 같게 만든다
```

는 뜻이다.

즉, 개념적으로는 다음과 같다.

```c
dup2(1, 2);
```

여기서 `&1`은 숫자 `1`을 파일 이름이 아니라 **fd 번호 1번**으로 해석하라는 의미이다.

만약 `&`가 없다면 shell은 `1`을 fd가 아니라 파일 이름으로 볼 수 있다.

---

## 6. 파일 생성 시 권한

`open()`으로 새 파일을 만들 때는 보통 `O_CREAT`와 함께 권한 값을 준다.

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

여기서 `0644`는 새로 생성되는 파일의 기본 권한이다.

```text
0644 = 소유자는 읽기/쓰기 가능, 그룹과 다른 사용자는 읽기만 가능
```

표로 보면 다음과 같다.

| 권한     | 의미                           |
| ------ | ---------------------------- |
| `0600` | 소유자만 읽기/쓰기 가능                |
| `0644` | 소유자는 읽기/쓰기, 나머지는 읽기 가능       |
| `0666` | 모두 읽기/쓰기 가능                  |
| `0755` | 소유자는 읽기/쓰기/실행, 나머지는 읽기/실행 가능 |

하지만 실제 생성 권한은 `open()`에 넣은 값 그대로가 아니라, 현재 프로세스의 `umask` 영향을 받는다.

예를 들어 `0644`로 생성하려고 해도 `umask` 설정에 따라 일부 권한이 제거될 수 있다.

```text
실제 권한 = 요청한 권한 & ~umask
```

일반적으로 텍스트 출력 파일은 `0644`를 많이 사용한다.

```c
open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
```
잘못된 권한 예시: 읽지 못해 생기는 현상, fd 생성이 제대로 진행되지않았기에 뒤에서도 bad file descriptor 오류남.
<img width="922" height="945" alt="image" src="https://github.com/user-attachments/assets/e0a1a303-4583-4120-a3a6-1a0846218bd7" />

---

## 7. `printf()`, `dprintf()`, `snprintf() + write()` 차이

### 7.1 `printf()`

```c
printf("value = %d\n", 123);
```

`printf()`는 기본적으로 `stdout`으로 출력한다.

즉, 출력 대상이 보통 fd `1`번으로 고정되어 있다고 볼 수 있다.

```text
printf() → stdout → fd 1
```

단, shell redirection으로 fd 1번이 파일로 바뀌면 `printf()` 출력도 파일로 간다.

---

### 7.2 `dprintf()`

```c
dprintf(fd, "value = %d\n", 123);
```

`dprintf()`는 `printf()`처럼 포맷 문자열을 사용하지만, 출력 대상을 직접 fd로 지정할 수 있다.

예를 들어 다음 코드는 fd `2`, 즉 `stderr`로 출력한다.

```c
dprintf(2, "error code = %d\n", -1);
```

또는 파일 fd로 직접 출력할 수도 있다.

```c
int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

dprintf(fd, "value = %d\n", 123);

close(fd);
```

정리하면 다음과 같다.

```text
printf()  : stdout으로 포맷 출력
dprintf() : 원하는 fd로 포맷 출력
```

주의:

```c
dprintf(fd, buf, len);   // write() 방식과 헷갈린 잘못된 사용
```

`dprintf()`는 이렇게 쓰는 함수가 아니다.

올바른 형태는 다음과 같다.

```c
dprintf(fd, "value = %d\n", 123);
```

---

### 7.3 전통적 방식: `snprintf() + write()`

`dprintf()`를 쓰지 않고 직접 버퍼를 만든 뒤 `write()`하는 방식도 있다.

```c
char buf[128];

snprintf(buf, sizeof(buf), "value = %d\n", 123);
write(fd, buf, strlen(buf));
```

이 방식은 두 단계로 나뉜다.

```text
1. snprintf()로 출력할 문자열을 버퍼에 만든다
2. write()로 그 버퍼를 fd에 쓴다
```

즉, `snprintf()`는 출력 함수라기보다는 **문자열 생성 함수**이다.

---

## 8. 한눈에 보는 비교

| 함수/기능        | 출력 대상             | 특징                          |
| ------------ | ----------------- | --------------------------- |
| `printf()`   | `stdout`          | 가장 기본적인 화면 출력               |
| `fprintf()`  | `FILE *` 스트림      | 파일 스트림에 포맷 출력               |
| `dprintf()`  | fd                | 원하는 fd에 포맷 출력               |
| `snprintf()` | char 배열           | 출력하지 않고 문자열 생성              |
| `write()`    | fd                | 포맷 없이 raw byte 출력           |
| `>`          | shell redirection | 실행 전 fd 연결 변경               |
| `dup2()`     | fd 복제/대체          | 특정 fd가 다른 fd와 같은 대상을 가리키게 함 |

---

## 9. 핵심 요약

```text
write()    : fd에 raw byte를 쓴다
dprintf()  : fd에 printf 형식으로 쓴다
printf()   : stdout에 printf 형식으로 쓴다
snprintf() : 출력하지 않고 문자열 버퍼를 만든다
dup2()     : fd 연결 대상을 바꾼다
>          : shell이 프로그램 실행 전에 fd를 바꿔준다
```

`./program > out.txt`는 프로그램 내부의 `printf()`를 바꾸는 것이 아니다.
프로그램이 실행되기 전에 shell이 fd `1`번을 `out.txt`로 바꿔놓는 것이다.

따라서 프로그램 입장에서는 여전히 `stdout`으로 출력한다고 생각하지만, 실제 출력은 파일로 저장된다.
