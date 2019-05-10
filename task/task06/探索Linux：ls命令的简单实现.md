## 探索Linux：ls命令的简单实现

ls是Linux上最常用的命令之一。可以显示目录/文件的名称，路径，大小，权限，所属用户等基本信息。看起来ls的工作似乎只是显示文件、目录的基本信息这么简单。仔细想想，这些事情做起来却不是很容易：如何判断参数是不是一个存在的文件/目录、如何读取目录内容、如何判断是目录还是普通文件还是符号链接或是其他类型的文件、递归读取目录信息、输出信息排列、不同类型输出不同颜色、分类统计...；

所以ls命令的实现，至少需要以下要实现的操作：

1. 判断文件/目录是否存在
2. 打开，读取，关闭目录
3. 获取文件/目录的详细信息
4. 获取文件/目录的所属用户和组
5. 对输出进行排序
6. 递归读取目录
7. 对不同类型用不同颜色标记
8. 支持使用尾部字符标记文件类型
9. 支持统计功能，统计不同类型的文件数量
10. 如果是软链接，获取目标文件的路径

那么如何实现一个简单的ls命令，这里通过实现简单的ls命令探索Linux系统。我们不会一次实现所有的功能，而是实现一个最简单的，然后逐步添加，甚至会重构代码。

一个简单的ls命令处理过程：

**获取参数 -> 判断文件是否存在 -> 获取文件状态判断类型 -> 如果是目录则显示目录内容，否则直接输出**



### 如何判断文件或目录存不存在

access接口可以完成此功能，通过man 2 acccess查看文档部分内容如下：

```C
ACCESS(2)                           Linux Programmer's Manual                           ACCESS(2)

NAME
       access, faccessat - check user's permissions for a file

SYNOPSIS
       #include <unistd.h>

       int access(const char *pathname, int mode);

		...

DESCRIPTION
    access()  checks whether the calling process can access the file pathname.  If pathname is a symbolic link, it is dereferenced.
            
    The mode specifies the accessibility check(s) to be performed, and is  either  the  value F_OK, or a mask consisting of the bitwise OR of one or more of R_OK, W_OK, and X_OK.  F_OK tests for the existence of the file.  R_OK, W_OK, and X_OK test whether  the file exists and grants read, write, and execute permissions, respectively.

    The  check is done using the calling process's real UID and GID, rather than the effective IDs as is done when actually attempting 
		...
RETURN VALUE
    On success (all requested permissions granted, or mode is F_OK and the file exists),  zero is returned.  On error (at least one bit in mode asked for a permission that is denied, or mode is F_OK and the file does not exist, or some other error occurred), -1  is returned,and errno is set appropriately.
...
```

access接口接受两个参数，第一个是字符串指针，指向文件/目录的路径字符串，第二个是整数类型，整数的值使用宏定义表示，R_OK, W_OK, X_OK分别表示可读，可写，可执行。F_OK表示文件/目录存在。access成功返回0，错误返回-1。以下代码是access使用示例，用于检测文件是否存在。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{

    if (argc < 2) {
        dprintf(2, "Error: less arguments -> file/dir name\n");
        return 1;
    }

    for (int i=1; i < argc; i++) {
        if (access(argv[i], F_OK) < 0)
            dprintf(2, "Error: %s not exists\n", argv[i]);
        else
            printf("%s exists\n", argv[i]);
    }

    return 0;
}

```
<br>

### 如何获取文件详细信息

Linux提供了stat，lstat，fstat系统调用用于获取文件详细信息。几个系统调用存在些许区别，stat和lstat通过字符串路径传递参数，并把获取的信息放在一个struct stat结构中，lstat和stat的区别是如果路径是一个符号链接，lstat获取的是符号链接文件自己的信息，而stat获取的是符号链接指向的目标文件的信息。通过man 2 stat查看联机文档：

```c
STAT(2)                             Linux Programmer's Manual                             STAT(2)

NAME
       stat, fstat, lstat, fstatat - get file status

SYNOPSIS
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <unistd.h>

       int stat(const char *pathname, struct stat *statbuf);
       int fstat(int fd, struct stat *statbuf);
       int lstat(const char *pathname, struct stat *statbuf);
		......
		......
DESCRIPTION
    These  functions return information about a file, in the buffer 
pointed to by statbuf.  No permissions are required on the file 
itself, but—in the case  of  stat(),  fstatat(),  and 
lstat()—execute (search) permission is required on all of the
directories in pathname that lead to the file.

		......
   lstat()  is  identical  to  stat(),  except  that  if pathname is a symbolic link, then it returns information about the link itself,not the file that it refers to.

    fstat() is identical to stat(), except that the file about which information  is  to  be retrieved is specified by the file 
descriptor fd.

   The stat structure All of these system calls return a stat structure, which contains the following fields:

    struct stat {
        dev_t     st_dev;      /* ID of device containing file */
        ino_t     st_ino;      /* Inode number */
        mode_t    st_mode;     /* File type and mode */
        nlink_t   st_nlink;    /* Number of hard links */
        uid_t     st_uid;      /* User ID of owner */
        gid_t     st_gid;      /* Group ID of owner */
        dev_t     st_rdev;     /* Device ID (if special file) */
        off_t     st_size;     /* Total size, in bytes */
        blksize_t st_blksize;   /* Block size for filesystem I/O */
        blkcnt_t  st_blocks;  /* Number of 512B blocks allocated */

        /* Since Linux 2.6, the kernel supports nanosecond
           precision for the following timestamp fields.
           For the details before Linux 2.6, see NOTES. */

        struct timespec st_atim;  /* Time of last access */
        struct timespec st_mtim;  /* Time of last modification */
        struct timespec st_ctim;  /* Time of last status change */

        /* Backward compatibility */
        #define st_atime st_atim.tv_sec
        
        #define st_mtime st_mtim.tv_sec
        #define st_ctime st_ctim.tv_sec
    };

       ......
       ......
       ......
RETURN VALUE
    On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
......
```

此文档清晰地说明了函数调用方式，struct stat结构体中每个变量的含义。接下来我们写一个简单的程序测试lstat函数：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void out_st_info(char * path, struct stat * st) {
    printf("%s -> size: %lu bytes\n", path, st->st_size);
    printf("    i-node: %-9lu  hard link: %-2lu ", st->st_ino, st->st_nlink);

    char * ptype = "";
    switch (st->st_mode & S_IFMT) {
        case S_IFDIR:
            ptype = "dir";
            break;
        case S_IFIFO:
            ptype = "fifo";
            break;
        case S_IFBLK:
            ptype = "block device";
            break;
        case S_IFCHR:
            ptype = "char device";
            break;
        case S_IFSOCK:
            ptype = "sock";
            break;
        case S_IFREG:
            ptype = "regular";
            if (access(path, X_OK)==0) { //如果可执行，输出可执行标记
                ptype = "regular*";
            }
            break;
        case S_IFLNK:
            ptype = "link";
            break;
    };

    printf("%s\n",ptype);
}

int main(int argc, char *argv[])
{
    struct stat st;

    for (int i=1; i<argc; i++) {
        //检测文件/目录是否存在
        if (access(argv[i], F_OK)) {
            dprintf(2, "Error: %s is not exists\n", argv[i]);
            continue;
        }
		//获取状态信息，成功则输出，失败输出错误信息
        if (lstat(argv[i], &st)==0) {
            out_st_info(argv[i], &st);
        } else {
            perror("lstat");
        }
    }

    return 0;
}
```

这个程序对argv获取的每个值作为路径传递给lstat获取文件信息，并使用out_st_info函数输出名称、大小、I-node号、硬链接数、类型等信息。


### 如何操作目录

基于一切皆是文件的设计，目录也是文件，这意味着open函数可以打开目录，确实如此，但是打开后读取的数据却不正常，早期的类Unix系统会显示很多目录、文件名称并有很多其他字符，因为是结构化的目录数据。而在目前的Linux上测试发现并不会输出任何结果。目录存储的是文件名称，文件类型，文件大小等信息，需要一个存储结构。这需要一套对应的接口进行操作。

Linux提供了目录操作相关的系统调用：opendir，readdir，closedir。通过名称就可以知道相应的操作。通过man 3 [API NAME]可以查看对应的说明手册。

opendir接受一个字符串作为目录路径，成功打开目录返回一个DIR结构的指针，错误返回NULL。

readdir从opendir返回的DIR*读取目录内容，每次返回一个struct dirent结构体指针，我们最关心的是其中的d_name存储文件的名称。

closedir关闭opendir打开的目录，传递参数是DIR*，就是opendir的返回值。

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

int main(int argc, char *argv[])
{
    if (argc<2) {
        dprintf(2,"Error:less DIR_NAME\n");
        return -1;
    }

    DIR * dr = opendir(argv[1]);
    if (dr==NULL) {
        perror("opendir");
        return -1;
    }
    
    struct dirent * r = NULL;
    while((r=readdir(dr))!=NULL) {
        printf("%s\n", r->d_name);
    }
    
    return 0;
}
```
<br>

### ls命令的实现：最简版本

第一个版本是一个对目录相关系统调用的示例，仅仅打开目录并列出目录内容。实现起来比较容易。这个程序通过读取参数作为目录或文件的名称，并显示相关信息。如果没有参数则默认读取 . （当前目录）。

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

int list_dir(char * path);

int main(int argc, char *argv[])
{
    if (argc<2) {
        list_dir(".");
    }

    for (int i=1; i<argc; i++) {
        if (access(argv[i], F_OK)==0) {
            list_dir(argv[i]);
        } else {
            dprintf(2, "Error: %s is not exists\n", argv[i]);
        }
    }

    return 0;
}

int list_dir(char * path) {
    struct stat st;
    char flag = '\0';
    DIR * d = NULL;
    struct dirent * rd = NULL;
    if (lstat(path, &st)<0) {
        perror("lstat");
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        if ((d=opendir(path))==NULL) {
            perror("opendir");
            return -1;
        }
        printf("%s/:\n",path);
        while((rd=readdir(d))!=NULL) {
            printf("%s\n", rd->d_name);
        }
        closedir(d);
    } else {
        printf("%s\n", path);
    }
    return 0;
}

```

这个程序功能十分简单，而且这仅仅是一个测试程序。为了能够区分我们自己实现的命令和系统默认的ls，我们把自己实现的版本命名为li。并且在后续课程中也使用li表示我们自己实现的ls命令。
