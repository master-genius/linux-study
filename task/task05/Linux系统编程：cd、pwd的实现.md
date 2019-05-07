## 探索Linux：cd、pwd的实现

----

这几个命令在Linux中经常会用到，但是，如何实现命令是本节课的主题。通过研究命令如何工作加深对Linux系统的理解。

### cd命令如何切换工作目录

cd切换工作目录使用的是chdir系统调用，通过查阅系统调用手册，发现chdir的信息如下：

```c
SYNOPSIS
       #include <unistd.h>

       int chdir(const char *path);
       int fchdir(int fd);

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):
       fchdir():
           _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && 	
               _XOPEN_SOURCE_EXTENDED
           || /* Since glibc 2.12: */ _POSIX_C_SOURCE >= 200809L
DESCRIPTION
    chdir()  changes  the current working directory of the calling process to the directory specified in path. fchdir() is identical to chdir(); the only difference is that the directory is given as an open file descriptor.

RETURN VALUE
    On  success, zero is returned.  On error, -1 is returned,and errno is set appropriately.
.......
.......
```

此文档说明调用此函数需要引入的头文件是unistd.h，chdir传递一个char*类型的参数指向要切换到目录字符串，返回值是int，0表示没有错误，-1表示出错。

似乎这一个函数不能解决所有的问题，想想cd命令的功能，当直接运行cd，会切换到当前用户主目录。所以要支持这个功能，需要获取当前用户的信息或者是直接获取主目录信息。解决这个问题的方法不止一种，但有一个比较快的方式是使用getenv函数，不过这不是一个Linux系统调用，而是C语言标准库函数。需要引入<stdlib.h>然后调用此函数：

char* getenv (const char *name);

getenv会从系统环境变量里去搜索name指定的变量，并返回指向此变量的指针，没有找到则返回NULL。这个函数的实现是依赖于平台的，并且环境变量的name在每个平台上都会有不同。在Linux/Unix上，使用getenv("HOME")获取用户主目录。

目前来说进展不错，现在的工作就是整合这些信息进行编码：

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    if (argc<2) {
        if(chdir( getenv("HOME") ) < 0){
            perror("chdir");
            return -1;
        }
    } else if (chdir(argv[1]) < 0) {
        perror("chdir");
        return -1;
    }

    return 0;
}
```

代码可以正常编译执行，可是结果却不是预期的那样切换工作目录。为什么会这样？



想想shell运行命令的流程：

**解析命令参数  -> 查找命令所在目录 -> fork子进程调用exec*相关函数运行命令**



答案很明显：cd操作发生在fork出的子进程中，对shell无效。所以shell把cd、pwd等一些操作实现为内建命令，由shell调用这些系统接口去实现。



### pwd如何工作

尽管pwd也是shell实现的内建命令，但是独立编写的命令也是可以显示当前工作目录的，再次考虑shell运行命令的过程，fork出的子进程会继承父进程的全部信息，工作目录和父进程是相同的，此时就可以获取工作目录并输出。

获取工作目录的系统调用：

```c
NAME
       getcwd, getwd, get_current_dir_name - get current working directory

SYNOPSIS
       #include <unistd.h>

       char *getcwd(char *buf, size_t size);

       char *getwd(char *buf);

       char *get_current_dir_name(void);

DESCRIPTION
       These functions return a null-terminated string containing 
       an absolute pathname that is  the  current working directory 
       of the calling process.  The pathname is returned as the 
       function result and via the argument buf, if present. 
	  	......
       getwd()  does  not malloc(3) any memory.  The buf argument 
       should be a pointer to an array at least PATH_MAX bytes 
       long.  If the length of the absolute pathname  of  the 
       current  working directory,  including  the terminating null 
       byte, exceeds PATH_MAX bytes, NULL is returned, and errno is 
       set to ENAMETOOLONG.

RETURN VALUE
       On  success, these functions return a pointer to a string 
       containing the pathname of the current working directory.  
       In the case getcwd() and getwd() this  is  the  same value 
       as buf.

       On  failure,  these  functions  return NULL, and errno is 
       set to indicate the error. The contents of the array pointed 
       to by buf are undefined on error.

......
```

通过文档描述发现，可使用的函数不止一个，这里使用getcwd获取当前工作目录，getcwd接受两个参数，第一个参数表明获取的目录信息要存储的位置，第二个是最大存储长度（字节数），这里引入了<linux/limits.h>，其中的宏定义PATH_MAX作为最大长度。成功获取当前工作目录则返回指向目录字符串的指针，否则返回NULL。

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>

int main (int argc, char *argv[])
{
    char buf[PATH_MAX] = {'\0'};
    if (getcwd(buf,PATH_MAX)==NULL) {
        perror("getcwd");
        return -1;
    }
    printf("%s\n",buf);
    return 0;
}
```

这个程序是可以正常执行的，会输出当前工作目录。
