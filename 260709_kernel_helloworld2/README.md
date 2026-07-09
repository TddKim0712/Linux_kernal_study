# kernel module init / exit 

커널 모듈 Hello World를 `module_init()` / `module_exit()` 방식으로 작성한 실습.

---

## 이전 코드 방식

이전 코드는 커널이 미리 약속된 함수 이름을 직접 찾는 방식이다. 

이것은 마치 main.c를 기본으로 실행하는 유저프로그램과 비슷하다.

```c
int init_module(void)
{
    ...
}

void cleanup_module(void)
{
    ...
}
```

의미:

```text
insmod 실행 -> init_module() 자동 호출
rmmod 실행  -> cleanup_module() 자동 호출
```

단점은 함수 이름이 고정되어 있다는 점이다.

---

## 현재 코드 방식

현재 코드는 내가 원하는 이름의 함수를 만들고,  
그 함수를 모듈의 시작/종료 함수로 등록하는 방식이다.

```c
static int __init init_hello(void)
{
    printk(KERN_INFO "hello world\n");
    return 0;
}

static void __exit exit_hello(void)
{
    printk(KERN_INFO "GOODBYE\n");
}

module_init(init_hello);
module_exit(exit_hello);
```

의미:

```text
module_init(init_hello)
-> 이 모듈이 삽입될 때 init_hello()를 실행하라

module_exit(exit_hello)
-> 이 모듈이 제거될 때 exit_hello()를 실행하라
```

---
