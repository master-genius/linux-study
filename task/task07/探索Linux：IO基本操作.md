## 探索Linux：IO基本操作

IO就是输入和输出，操作的是文件，文件的类型可以是普通文件，也可以是其他类型的文件，如设备文件，sock文件等。在计算机领域的一些基础理论课程上，往往重视调度而轻视IO。然而计算机的IO操作是最复杂的操作之一，而调度大部分情况是可以不必关心的，操作系统会处理好。通常的大部分操作其核心都是针对IO的操作。

### 文件的基本操作

对文件的基本操作是很常见的，这一节主要涉及到open，read，write，close四个函数。这四个函数是系统层面提供的文件操作接口。基本流程是：open --> [read/write] --> close。

先来看open函数：

| 头文件 | \#include <sys/types.h><br>\#include <sys/stat.h><br>\#include <fcntl.h> |
| ------ | ------------------------------------------------------------ |
| 原型   | int  open(const char * pathname, int flags);<br>int  open(const char * pathname, int flags, mode_t mode); |
| 参数   | pathname：要打开的文件；flags：打开选项；mode：权限。        |
| 返回值 | 正常打开返回文件描述符，出错返回-1。                         |

open函数的返回值是一个整数，被称为文件描述符，每个进程都保存一组打开的文件，并且每个打开的文件用一个数字来唯一标识。早期的系统设计就是一个结构体数组，文件描述符是数字索引。通过这个数字就可以找到打开的文件。open函数并不是有两个原型，这在C中是不允许的，实际上目前使用的系统调用都是glibc封装好的，glibc封装了内核十分原始的调用，对外提供了易用的接口。glibc把open实现成了变参函数，第三个参数是可选的。

flags常用选项：

| 选项名称   | 说明                                                         |
| ---------- | ------------------------------------------------------------ |
| O_APPEND   | 打开文件时，游标移动到文件末尾，以这种这种方式写入就是在文件末尾追加。 |
| O_CREAT    | 如果文件不存在则创建                                         |
| O_TRUNC    | 截断文件内容，如果文件已存在数据，则再次写入数据的长度如果小于之前的数据，则会截断，后面的内容会清空。 |
| O_ASYNC    | 异步模式，需要使用信号驱动的形式来配合，默认会触发SIGIO信号。 |
| O_DIRECT   | 直接存取模式，不使用缓存，这在一些特殊场景下有用，比如数据库软件需要使用自己的缓存机制。 |
| O_NONBLOCK | 非阻塞模式，在一些异步IO或是IO多路复用的场景要使用，比如使用epoll接口，套接字文件没有数据不会阻塞，程序可以处理活动的连接。 |

mode的常用权限值如下：

| 名称    | 说明                                 |
| ------- | ------------------------------------ |
| S_IWUSR | 文件所属用户具备写权限               |
| S_IRUSR | 文件所属用户具备读权限               |
| S_IXUSR | 文件所属用户具备可执行权限           |
| S_IRWXU | 文件所属用户具备可读可写可执行的权限 |
| S_IRGRP | 所属用户组具备可读权限               |
| S_IWGRP | 所属用户组具备可写权限               |
| S_IXGRP | 所属用户组具备可执行权限             |

实际上mode就是一个8进制的权限码，直接使用数字也可以，0644表示用户可读可写，用户组和其他用户仅可读。

打开文件示例：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {

    /*
    	以可读可写的方式打开文件，如果没有则创建。另一种形式：
    	open("dataopen", O_CREAT|O_RDWR, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
    	如果文件不存在，则创建文件的权限就是：0644
    */
    int fd = open("dataopen", O_CREAT|O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    close(fd);

    return 0;
}
```

！如果仅仅是读取文件，mode参数可以没有，但是如果文件不存在，以O_CREAT模式打开文件，如果没有mode参数，文件创建后没有任何可操作权限，读写会失败。

**从文件中读取数据**

read函数从已经打开的文件中获取一条数据，read函数详细信息：

| 头文件 | \#include <unistd.h>                                         |
| ------ | ------------------------------------------------------------ |
| 原型   | ssize_t read(int fd, void* buf, size_t count);               |
| 参数   | fd：文件描述符；buf：读取到的数据存储在此；count：要读取的字节数 |
| 返回值 | 成功执行返回实际读取到的字节数，出错返回-1                   |

一定要通过read返回值确定读取到的字节数，因为如果count限制每次读取1000字节，但是数据只有500字节，则返回值是500。

以下程序是open - read - close的简单示例，打开一个文件，然后读取一条数据。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DATA_BUF_LEN    2048

int main(int argc, char *argv[]) {

    int ret = 0;
    char buffer[DATA_BUF_LEN+1] = {'\0', };

    int fd = open("datatest", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    int count = 0;

    count = read(fd, buffer, DATA_BUF_LEN);
    if (count < 0) { //出错设置错误返回值
        perror("read");
        ret = 2;
    } else if(count == 0) { //文件为空
        printf("[File empty]\n");
    } else { //输出读取到的数据
        printf("--read: \n%s\n--\n", buffer);
    }

    close(fd); //最后要关闭打开的文件

    return ret;
}
```

**写入文件**

写入文件要使用write函数，write函数的详细信息如下：

| 头文件 | \#include <unistd.h>                                   |
| ------ | ------------------------------------------------------ |
| 原型   | ssize_t write(int fd, const void *buf, size_t count);  |
| 参数   | fd：文件描述符；buf：要写入的数据；count：写入的字节数 |
| 返回值 | 成功返回实际写入的字节数，失败返回-1                   |

！count不要随便给一个值，最好是使用strlen(buf)，比如buf长度是100，但是count=150，则50个字节会被填充50个ascii码为0的字节。

write函数示例：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DATA_BUF_LEN    2048

int main(int argc, char *argv[]) {

    int ret = 0;
    char buffer[DATA_BUF_LEN+1] = "#!/bin/bash\nlscpu\nuname -a\nexit 0\n";

    int fd = open("datatest", O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    int count = 0;

    count = write(fd, buffer, strlen(buffer));
    if (count < 0) {
        perror("write");
        ret = 2;
    } else if(count == 0) { //没有写入任何数据
        printf("Write nothing\n");
    } else {
        printf("--write: \n%s\n--%d bytes\n", buffer, count);
    }

    close(fd);

    return ret;
}
```

关于write函数有一些需要特别注意的情况，在一些高级语言的编程中，进行文件操作，通常可以传递字符串参数控制文件打开的模式，比如"r","w","a","w+"等，"w"表示写入模式打开，"w+"表示写入模式打开，没有则创建，但是这种方式会在打开文件时把长度截断为0，之前的内容会被清空。而我们实现的这个程序不会这样做，如果你想设计成那样的形式，需要用到O_TRUNC选项，这个选项要配合可写选项O_RDWR或者是O_WRONLY使用，在打开文件时就会截断文件长度为0。

write(1, buf, count);就是在向标准输出写入数据，没有输出重定向的情况，就是在向屏幕输出。



### IO重定向的实现原理

在shell中，重定向是经常要使用的功能。这里讨论的是IO重定向的原理并给出程序的实现。IO重定向的实现基于Linux的一个设计原则：

**最低可用文件描述符原则。**

简单来说，文件描述符是一个数组索引号，每个进程都记录了一组打开的文件，这组文件用一个数组来保存。数组的索引就是文件描述符。当一个进程在打开文件的时候，由于最低可用文件描述符原则，系统内核总会为此文件安排的描述符总是数组中最低可用的位置索引。

并且除此以外，还有一个依赖：文件描述符0，1，2作为标准输入，标准输出，标准错误输出和设备文件或者是其他类型的文件相关联。默认情况，0关联键盘对应的设备文件；1，2都是关联屏幕的设备文件。无论使用什么编程语言，当使用输出函数进行一些数据的输出时，就是输出到标准输出。当程序运行时，文件描述符0，1，2都是已经关联好的。

由于多数情况下就是在向关联的屏幕设备文件输出信息，所以这容易导致一个错误理解：输出函数就是直接向屏幕输出。

那程序如何控制重定向，以输出重定向为例。基于上文讨论的原则，当程序开始就使用close关闭描述符1。这个时候，最低可用文件描述符是1，此时open打开的文件，返回的描述符肯定是1。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    close(1); //此时描述符1和屏幕设备文件的关联已经断开

    int fd = -1;
    //成功打开文件后，fd的值是1
    fd = open("ioout",O_CREAT|O_APPEND|O_RDWR, 0644);
    if (fd<0) {
        perror("open");
        return -1;
    }

    /*
    	由于描述符1是作为标准输出，而此时关联到了一个普通文件，所以
    	标准输出函数最终的结果是输出到了ioout文件
    */
    printf("Linux is great %d\n", fd);

    return 0;
}
```

这个程序演示了如何实现输出重定向，还有一种实现方式，可以更好的控制实现重定向。使用dup2系统调用。

| 头文件 | \#include <unistd.h>                                     |
| ------ | -------------------------------------------------------- |
| 原型   | int dup2(int oldfd, int newfd);                          |
| 参数   | oldfd：要被复制的文件描述符；newfd：要复制到的文件描述符 |
| 返回值 | 成功则返回newfd，否则返回-1。                            |

dup2接收两个参数都是表示文件描述符的整数，把第一个文件描述符的数据复制到第二个参数，而在此之前会检测后一个描述符是不是已经关联其他打开的文件，如果是则会先尝试关闭。这个函数可以精确控制要对哪个描述符进行重定向。

以下程序使使用dup2的重定向示例：

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    int fd = -1;
    fd = open("ioout",O_CREAT|O_APPEND|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd<0) {
        perror("open");
        return -1;
    }

    /*
    	fd指向新打开的文件，dup2会先执行close(1)，然后把fd的数据复制到文件描述符1
    	此时文件描述符1已经不再关联屏幕的设备文件，而是ioout文件，这时候文件描述符
    	fd和1都指向ioout文件，所以下一步可以执行close(fd)。
    */
    
    dup2(fd,1);
    close(fd);
    
    printf("IO redirect\n");

    return 0;
}
```
