# printk, dmesg, /dev/kmsg 실험


/dev/kmsg는 커널 로그를 저장하는 파일 자체가 아니라,
커널 내부 printk 로그 버퍼에 read/write로 접근하게 해주는 character device 


실제 저장 공간 = 커널 메모리 안의 <b> printk ring buffer </b>

```
/dev/null
write() -> 그냥 버림
read()  -> EOF

/dev/kmsg
write() -> 커널 로그 버퍼에 기록
read()  -> 커널 로그 버퍼에서 로그 읽기
```

## 출발점

커널 모듈에서 다음 코드를 실행했다.

```c
printk(KERN_INFO "hello world\n");
```

그런데 `printk()`는 `printf()`처럼 현재 터미널에 바로 출력되지 않는다.

확인은 다음 명령어로 했다.

```bash
sudo dmesg | tail -n 3
```

---

## 질문 1: printk는 어디에 출력하는가?

`printk()`는 터미널에 직접 출력하는 함수가 아니다.

```text
printk()
-> 커널 로그 버퍼에 기록
```

즉 실제 흐름은 다음과 같다.

```text
커널 모듈
-> printk()
-> 커널 로그 버퍼
-> dmesg가 읽음
-> 터미널에 출력
```

---

## 질문 2: dmesg는 무엇인가?

`dmesg`는 커널 로그 버퍼를 읽어서 사용자에게 보여주는 프로그램이다.

```text
dmesg
-> 커널 로그 읽기
-> stdout(fd 1)에 출력
-> 터미널에 보임
```

---

## 질문 3: 리눅스는 파일처럼 다룬다

리눅스에서는 많은 커널 자원을 파일처럼 다룬다.

커널 로그도 `/dev/kmsg`라는 문자 디바이스 파일로 접근할 수 있다.

확인:

```bash
ls -l /dev/kmsg
```

출력 맨 앞이 `c`이면 character device.

```text
crw-r--r-- ...
```

---

## 질문 4: 그럼 write도 가능?

가능하다.

`/dev/kmsg`에 쓰면 커널 로그 버퍼에 기록된다.

```bash
echo "hello from user space" | sudo tee /dev/kmsg
```

확인:

```bash
sudo dmesg | tail
```

흐름:

```text
echo / tee
-> /dev/kmsg open
-> write()
-> 커널 로그 버퍼에 기록
-> dmesg로 확인
```

---

## C 코드로 write 실험


컴파일:

```bash
gcc kmsg_write.c -o kmsg_write
```

실행:

```bash
sudo ./kmsg_write
```

확인:

```bash
sudo dmesg | tail -n 3
```
커널 모듈 관찰:

```bash
sudo demsg -w
```
---

<img width="1066" height="352" alt="image" src="https://github.com/user-attachments/assets/533a1258-835f-416a-9562-fa5cb1fbdf36" />

## 핵심 정리

```text
printk()
-> 커널 내부에서 커널 로그 버퍼에 기록

dmesg
-> 커널 로그 버퍼를 읽어서 터미널에 출력

/dev/kmsg
-> 커널 로그 버퍼에 접근하기 위한 character device file

write(1, ...)
-> stdout, 즉 터미널에 출력

write(fd_of_/dev/kmsg, ...)
-> 커널 로그 버퍼에 기록
```
