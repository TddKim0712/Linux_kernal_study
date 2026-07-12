커널 모듈은 커널에 코드를 넣고 빼는 방식이고, 드라이버는 커널의 특정 장치/서브시스템에 자기 역할을 등록해서 요청을 처리하는 코드다.

write(fd, ...)
    ↓
시스템 콜
    ↓
커널

커널의 실행:

사용자 프로그램
    |
    | open("/dev/chardev")
    v
  커널 VFS
    |
    v
file_operations
    |
    +---- .open    → chardev_open()
    |
    +---- .read    → chardev_read()
    |
    +---- .write   → chardev_write()
    |
    +---- .release → chardev_release()


.open 칸에
    ↓
chardev_open 함수 주소 저장

마치 함수포인터처럼 역할 지정
    static const struct file_operations chardev_fops = {
    .owner = THIS_MODULE,
    .open  = chardev_open,
};

cdev_init(&my_cdev, &chardev_fops);
cdev_init()은 file_operations를 struct cdev에 기억시킨다. 이후 cdev_add()가 이 문자 디바이스를 시스템에서 활성화한다


my_cdev
   |
   +-- ops
        |
        v
   chardev_fops
        |
        +-- open  → chardev_open
        +-- write → chardev_write
        +-- read  → chardev_read


-----
[User Space]

int fd = open("/dev/chardev", O_RDWR);
                 |
                 | 시스템 콜
                 v
-----------------------------------
             Kernel Space
-----------------------------------
                 |
                VFS
                 |
           character device
                 |
            struct cdev
                 |
              cdev->ops
                 |
                 v
       struct file_operations
                 |
       .open  = chardev_open
                 |
                 v
          chardev_open()
                 |
              printk()
                 |
                 v
       kernel log ring buffer
                 |
              /dev/kmsg
                 |
               dmesg

---
드라이버 로드
   ↓
register_chrdev(100, "chardev", &chardev_fops)
   ↓
커널:
"major 100번 요청은 chardev_fops를 따라가라"
   ↓

/dev/chardev  [major 100, minor 0]
   ↓
사용자 open("/dev/chardev")
   ↓
inode를 보고 major 100 확인
   ↓
등록된 char device 찾음
   ↓
chardev_fops
   ↓
.open
   ↓
chardev_open(inode, file)

---

- major란
디바이스 번호 요청을 어느 드라이버로 보낼지구분하는 큰 번호
major = 어느 종류/드라이버 쪽?
minor = 그 드라이버가 관리하는 장치 중 몇 번?

major 100
   │
   ├── minor 0   LED 0
   ├── minor 1   LED 1
   └── minor 2   LED 2

mknod /dev/led0 c 100 0
mknod /dev/led1 c 100 1
mknod /dev/led2 c 100 2