# 260708_chardev_helloworld

리눅스 커널 모듈 Hello World 실습.

일반 C 프로그램처럼 `gcc`로 컴파일하고 `./main`으로 실행하는 것이 아니라, 커널 빌드 시스템을 이용해 `.ko` 모듈 파일을 만든 뒤 커널에 삽입한다.

---

## 파일 구조

```text
260708_chardev_helloworld/
├── hello.c
└── Makefile
```

---

## 빌드

```bash
make
```

빌드 성공 시 최종 결과물로 `hello.ko`가 생성된다.

```text
hello.c -> hello.o -> hello.ko
```

실제로 커널에 넣는 파일은 `hello.ko`이다.

---

## 실행

커널 모듈은 일반 프로그램처럼 실행하지 않는다.

```bash
sudo insmod hello.ko
```

`insmod`는 insert module의 의미이며, 모듈을 현재 실행 중인 커널에 삽입한다.

이때 `init_module()`이 실행된다.

---

## 로그 확인

```bash
sudo dmesg | tail
```

`printk()`는 터미널 출력이 아니라 커널 로그 출력이다.

따라서 `printf()`처럼 바로 화면에 보이지 않고 `dmesg`로 확인한다.

---

## 모듈 확인

```bash
lsmod | grep hello
```

현재 커널에 `hello` 모듈이 로드되어 있는지 확인한다.

---

## 모듈 제거

```bash
sudo rmmod hello
```

`rmmod`는 remove module의 의미이다.

주의할 점은 `hello.ko`가 아니라 모듈 이름인 `hello`를 사용한다.

모듈 제거 시 `cleanup_module()`이 실행된다.

---

## 전체 명령어 흐름

```bash
make
sudo insmod hello.ko
sudo dmesg | tail
lsmod | grep hello
sudo rmmod hello
sudo dmesg | tail
make clean
```

---

## 발생했던 에러

### gcc-12 없음

```text
/bin/sh: 1: gcc-12: not found
```

현재 커널은 gcc-12로 빌드되었는데 시스템에 gcc-12가 없어서 발생했다.

해결:

```bash
sudo apt install gcc-12 build-essential linux-headers-$(uname -r)
```

---

### MODULE_LICENSE 없음

```text
ERROR: modpost: missing MODULE_LICENSE() in hello.o
```

커널 모듈 코드에 라이선스 정보가 없어서 발생했다.

해결:

```c
MODULE_LICENSE("GPL");
```

---

## make 후 생성되는 파일

`make`를 실행하면 커널 빌드 시스템이 여러 중간 파일을 생성한다.

```text
hello.o
hello.mod.c
hello.mod.o
hello.ko
Module.symvers
modules.order
```

핵심 결과물은 `hello.ko`이다.

중간 파일을 지우려면:

```bash
make clean
```

---

## 실행 결과 해석

```text
hello: loading out-of-tree module taints kernel.
```

직접 빌드한 외부 모듈이 커널에 로드되었다는 의미이다.

```text
module verification failed: signature and/or required key missing
```

공식 서명된 모듈이 아니라는 의미이다.

실습용 모듈에서는 일반적으로 볼 수 있다.

```text
hello world
```

모듈 삽입 시 초기화 함수가 정상 실행되었다는 의미이다.

---

## 핵심 정리

```text
make                 -> hello.ko 생성
sudo insmod hello.ko -> 커널에 모듈 삽입
printk()             -> 커널 로그에 출력
sudo dmesg | tail    -> 커널 로그 확인
sudo rmmod hello     -> 커널에서 모듈 제거
make clean           -> 빌드 결과물 정리
```