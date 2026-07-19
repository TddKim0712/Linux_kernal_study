# Character Device Driver 기초

## 1. 커널 모듈과 디바이스 드라이버

### 커널 모듈

커널 모듈은 실행 중인 커널에 코드를 동적으로 넣고 빼는 방식이다.

```text
insmod
  ↓
커널에 코드 추가

rmmod
  ↓
커널에서 코드 제거
```

커널 전체를 다시 빌드하거나 재부팅하지 않고 기능을 추가할 수 있다.

### 디바이스 드라이버

디바이스 드라이버는 커널의 특정 장치나 서브시스템에 자신의 처리 함수를 등록하고, 사용자 프로그램의 요청을 처리하는 코드다.

```text
커널 모듈
= 커널에 코드를 넣고 빼는 형식

디바이스 드라이버
= 장치 요청을 처리하도록 커널에 등록한 코드
```

이번 코드는 문자 디바이스 드라이버를 커널 모듈 형태로 구현한다.

---

## 2. 전체 구조

사용자 프로그램은 드라이버 함수를 직접 호출하지 않는다.

`open()`, `read()`, `write()`와 같은 시스템 콜을 호출하면 커널의 VFS가 적절한 드라이버 함수를 찾아 실행한다.

```text
사용자 프로그램
    |
    | open("/dev/chardev")
    | read(fd, ...)
    | write(fd, ...)
    v
시스템 콜
    |
    v
커널 VFS
    |
    v
문자 디바이스 드라이버
    |
    v
file_operations
```

---

## 3. `file_operations`

`file_operations`는 사용자 프로그램의 파일 연산을 어떤 드라이버 함수가 처리할지 연결하는 함수 포인터 모음이다.

```text
file_operations
    |
    +---- .open    → chardev_open()
    |
    +---- .read    → chardev_read()
    |
    +---- .write   → chardev_write()
    |
    +---- .release → chardev_release()
```

예를 들어 `.open` 칸에는 `chardev_open()` 함수의 주소가 저장된다.

```c
static const struct file_operations chardev_fops = {
    .owner = THIS_MODULE,
    .open  = chardev_open,
    .read  = chardev_read,
};
```

따라서 사용자 프로그램이 `/dev/chardev`를 열면 커널은 다음 순서로 동작한다.

```text
open("/dev/chardev")
    ↓
VFS
    ↓
file_operations.open
    ↓
chardev_open()
```

`file_operations`는 함수들을 직접 실행하는 구조체가 아니라, 각 요청을 처리할 함수의 주소를 기억하는 구조체다.

---

## 4. 드라이버 등록 과정

모듈이 로드되면 초기화 함수에서 문자 디바이스 드라이버를 등록한다.

```text
드라이버 모듈 로드
    ↓
chardev_init()
    ↓
register_chrdev(100, "chardev", &chardev_fops)
    ↓
커널:
"major 100번 장치의 요청은 chardev_fops로 처리하라"
```

현재 코드에서는 major 번호를 `100`으로 고정해 사용한다.

```text
major 100
    ↓
chardev_fops
    |
    +-- open → chardev_open()
    +-- read → chardev_read()
```

고정 major 번호를 사용한 경우 `register_chrdev()`의 반환값은 다음과 같다.

```text
0       등록 성공
음수    등록 실패
```

major 번호에 `0`을 전달하면 커널이 빈 major 번호를 자동 할당하고, 반환값으로 할당된 major 번호를 돌려준다.

---

## 5. `/dev/chardev`의 역할

`register_chrdev()`는 커널에 드라이버를 등록하지만 `/dev/chardev` 파일을 자동으로 만들지는 않는다.

따라서 현재 코드에서는 직접 장치 노드를 만들어야 한다.

```bash
sudo mknod /dev/chardev c 100 0
```

각 인자의 의미는 다음과 같다.

```text
/dev/chardev   장치 파일 이름
c              character device
100            major 번호
0              minor 번호
```

확인:

```bash
ls -l /dev/chardev
```

예상 형태:

```text
crw-r--r-- ... 100, 0 ... /dev/chardev
```

첫 글자 `c`는 문자 디바이스라는 뜻이다.

---

## 6. major와 minor

장치 파일에는 major 번호와 minor 번호가 있다.

### major 번호

어느 드라이버가 요청을 처리할지 구분하는 번호다.

```text
major
= 어느 드라이버로 보낼 것인가?
```

### minor 번호

같은 드라이버가 관리하는 여러 장치를 구분하는 번호다.

```text
minor
= 그 드라이버가 관리하는 장치 중 몇 번째인가?
```

예:

```text
major 100
    |
    +-- minor 0 → LED 0
    +-- minor 1 → LED 1
    +-- minor 2 → LED 2
```

장치 노드:

```bash
sudo mknod /dev/led0 c 100 0
sudo mknod /dev/led1 c 100 1
sudo mknod /dev/led2 c 100 2
```

세 장치 모두 major 번호가 같으므로 같은 드라이버로 전달된다.

드라이버는 inode에 들어 있는 minor 번호를 확인해 어느 장치에 대한 요청인지 구분할 수 있다.

---

## 7. `open()` 요청 흐름

사용자 프로그램:

```c
int fd = open("/dev/chardev", O_RDWR);
```

커널 내부 흐름:

```text
[User Space]

open("/dev/chardev", O_RDWR)
             |
             | 시스템 콜
             v
--------------------------------
          Kernel Space
--------------------------------
             |
            VFS
             |
      /dev/chardev inode
             |
      major 100 확인
             |
  등록된 문자 디바이스 검색
             |
       chardev_fops
             |
   .open = chardev_open
             |
             v
      chardev_open()
             |
          printk()
             |
             v
  kernel log ring buffer
             |
       dmesg로 확인
```

핵심 흐름:

```text
/dev/chardev
[major 100, minor 0]
    ↓
open("/dev/chardev")
    ↓
inode에서 major 100 확인
    ↓
major 100에 등록된 드라이버 검색
    ↓
chardev_fops
    ↓
.open
    ↓
chardev_open(inode, file)
```

---

## 8. 드라이버의 `open()` 함수

형태:

```c
int chardev_open(
    struct inode *inode,
    struct file *file
);
```

### `struct inode *inode`

열린 장치 파일 자체의 정보를 나타낸다.

주로 다음 정보를 확인할 수 있다.

```text
장치 파일의 major 번호
장치 파일의 minor 번호
장치의 종류와 메타데이터
```

감각적으로는 다음과 같다.

```text
inode
= 어떤 장치 파일을 열었는가?
```

예:

```text
/dev/chardev
major 100
minor 0
```

### `struct file *file`

이번에 열린 파일 하나를 나타내는 커널 객체다.

같은 `/dev/chardev`를 두 번 열면 inode는 같은 장치를 나타내지만, 열린 파일 객체는 각각 만들어질 수 있다.

```text
/dev/chardev
    |
    +-- struct file A
    |
    +-- struct file B
```

파일 디스크립터와 연결하면 다음과 같다.

```text
fd 3
  ↓
struct file A
  ↓
/dev/chardev

fd 4
  ↓
struct file B
  ↓
/dev/chardev
```

드라이버는 `file->private_data`에 이번 open과 관련된 장치 상태를 저장할 수 있다.

### `open()` 반환값

```text
0       성공
음수    실패
```

예:

```c
return 0;        // 성공
return -EBUSY;   // 장치가 사용 중
return -ENODEV;  // 장치가 없음
```

`open()`은 몇 바이트를 처리했는지 반환할 필요가 없기 때문에 반환형으로 `int`를 사용한다.

---

## 9. 사용자 `read()`와 드라이버 `.read`

사용자 프로그램의 `read()`는 읽기를 요청하는 함수다.

```c
char buf[100];

ssize_t ret = read(fd, buf, 100);
```

사용자 입장:

```text
"fd가 가리키는 장치에서
최대 100바이트를 읽어서
내 buf에 넣어줘."
```

드라이버의 `chardev_read()`는 그 요청을 실제로 처리하는 함수다.

```text
사용자 read()
= 읽어 달라고 요청

드라이버 chardev_read()
= 읽기 요청을 실제로 처리
```

전체 흐름:

```text
read(fd, buf, 100)
        ↓
시스템 콜
        ↓
커널이 fd로 struct file 검색
        ↓
VFS
        ↓
file_operations.read
        ↓
chardev_read(file, buf, count, offset)
        ↓
사용자 버퍼에 데이터 전달
        ↓
return N
        ↓
사용자 read()도 N 반환
```

---

## 10. 사용자 `read()`와 드라이버 인자 비교

사용자 함수:

```c
read(fd, buf, 100);
```

드라이버 함수:

```c
chardev_read(file, buf, count, offset);
```

대응 관계:

| 사용자 `read()` | 드라이버 `chardev_read()` | 의미 |
|---|---|---|
| `fd` | `struct file *file` | 어떤 열린 파일인가 |
| `buf` | `char __user *buf` | 데이터를 받을 사용자 버퍼 |
| `100` | `size_t count` | 최대 몇 바이트를 원하는가 |
| 내부적으로 관리 | `loff_t *offset` | 현재 읽기 위치 |

---

## 11. `chardev_read()`의 인자

형태:

```c
ssize_t chardev_read(
    struct file *file,
    char __user *buf,
    size_t count,
    loff_t *offset
);
```

### `struct file *file`

어느 open 요청에서 시작된 read인지 나타낸다.

```text
open()
  ↓
struct file 생성
  ↓
read()
  ↓
같은 struct file 전달
```

사용자 프로그램은 단순히 `fd` 번호를 전달하지만 커널은 그 번호를 실제 `struct file`로 변환해서 드라이버에 전달한다.

```text
사용자 fd 3
    ↓
커널의 파일 디스크립터 테이블
    ↓
struct file
```

### `char __user *buf`

사용자 프로그램이 데이터를 받아갈 메모리 주소다.

```c
char buf[100];
read(fd, buf, 100);
```

드라이버가 보는 `char __user *buf`는 사용자 프로그램의 `buf`를 가리킨다.

```text
커널 드라이버
    |
    | copy_to_user()
    v
사용자 프로그램의 buf
```

`__user`는 사용자 공간의 주소를 가리킨다는 표시다.

커널은 사용자 공간 주소에 일반 포인터처럼 바로 접근하지 않고 보통 다음 함수를 사용한다.

```c
copy_to_user();
```

데이터 전달 방향:

```text
커널 데이터
    ↓
copy_to_user()
    ↓
사용자 buf
```

### `size_t count`

사용자가 최대 몇 바이트를 요청했는지 나타낸다.

```c
read(fd, buf, 100);
```

이면:

```text
count = 100
```

이는 반드시 100바이트를 전달해야 한다는 뜻은 아니다.

드라이버가 실제로 5바이트만 전달했다면 다음과 같이 반환한다.

```c
return 5;
```

### `loff_t *offset`

현재까지 어디까지 읽었는지를 나타내는 위치 값이다.

```text
HELLO WORLD
^
offset = 0
```

5바이트를 읽은 후:

```text
HELLO WORLD
     ^
offset = 5
```

일반 파일에서는 다음 read가 이전 위치에 이어서 실행된다.

문자 디바이스는 장치의 특성에 따라 offset을 사용할 수도 있고 사용하지 않을 수도 있다.

---

## 12. `read()`의 반환형이 `ssize_t`인 이유

`read()`는 성공 여부뿐 아니라 실제로 몇 바이트를 읽었는지도 반환해야 한다.

```text
양수    실제 읽은 바이트 수
0       EOF, 읽을 데이터가 끝남
음수    오류
```

예:

```c
return 10;       // 10바이트 전달
return 0;        // EOF
return -EFAULT;  // 사용자 메모리 접근 오류
```

따라서 양수, 0, 음수를 모두 표현할 수 있는 signed 자료형인 `ssize_t`를 사용한다.

### `open()`과 `read()` 반환형 비교

```text
open()
"장치를 열 수 있는가?"
    ↓
int
성공 또는 오류
```

```text
read()
"데이터를 몇 바이트 전달했는가?"
    ↓
ssize_t
바이트 수, EOF 또는 오류
```

| 함수 | 반환형 | 반환값 의미 |
|---|---|---|
| `open` | `int` | 성공 `0`, 실패 `음수` |
| `read` | `ssize_t` | 읽은 바이트 수, EOF 또는 오류 |

---

## 13. 현재 `chardev_read()`의 동작

현재 read 함수는 로그를 출력한 뒤 `0`을 반환한다.

```text
cat /dev/chardev
    ↓
open("/dev/chardev")
    ↓
chardev_open()
    ↓
read()
    ↓
chardev_read()
    ↓
return 0
    ↓
EOF
    ↓
cat 종료
```

따라서 터미널에는 장치 데이터가 출력되지 않고, 커널 로그에는 다음 내용이 나타난다.

```text
chardev_open
chardev_read
```

확인:

```bash
sudo dmesg | tail
```

실시간 확인:

```bash
sudo dmesg -w
```

`cat`에 아무 내용도 나타나지 않는 것은 오류가 아니라, 드라이버가 `return 0`으로 EOF를 전달했기 때문이다.

---

## 14. `printk()`와 커널 로그

드라이버에서 호출한 `printk()`는 일반 터미널의 표준 출력으로 바로 나가지 않는다.

```text
chardev_open()
    ↓
printk()
    ↓
커널 로그 링 버퍼
    |
    +-- dmesg로 확인
    |
    +-- /dev/kmsg를 통해 접근 가능
```

따라서 드라이버 로그는 다음 명령으로 확인한다.

```bash
sudo dmesg | tail
```

또는:

```bash
sudo dmesg -w
```

---

## 15. 모듈 로드부터 테스트까지

### 빌드

```bash
make
```

빌드가 성공하면 다음 파일이 생성된다.

```text
chardev.ko
```

### 모듈 삽입

```bash
sudo insmod chardev.ko
```

### 등록 확인

```bash
lsmod | grep chardev
sudo dmesg | tail
```

### 장치 노드 생성

```bash
sudo mknod /dev/chardev c 100 0
```

### 장치 노드 확인

```bash
ls -l /dev/chardev
```

### open 및 read 테스트

```bash
sudo cat /dev/chardev
```
<img width="845" height="845" alt="Screenshot from 2026-07-12 21-47-33" src="https://github.com/user-attachments/assets/28e82ca6-f3b0-457e-b7d1-1367b996d542" />

### 커널 로그 확인

```bash
sudo dmesg | tail
```
<img width="1130" height="359" alt="image" src="https://github.com/user-attachments/assets/784afbb5-0606-46e2-817e-4762e4cbddaa" />
