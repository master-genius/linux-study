#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//反转字符串
char* revstr(char* s) {
    int len = strlen(s);
    int i=0;
    int j=len-1;
    char tmp;
    while(i<j) {
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
    return s;
}

int main(int argc, char *argv[]) {

    int rev_flag = 0;
    int rev_ind = 0;

    for(int i=1; i<argc; i++) {
        if (strcmp("-r", argv[i]) == 0) {
            if (rev_flag > 0 && rev_ind > 0) {
                //如果已经设置过，则表示-r重复，报错并退出。
                //使用dprintf输出到stderr，这可以使用 2> 进行错误输出重定向。
                dprintf(2, "Error: too many -r\n");
                return 1;
            }
            rev_flag = 1;
            rev_ind = i;
        }
    }

    for (int i=1; i<argc; i++) {
        if (rev_flag > 0 && rev_ind > 0) {
            if (rev_ind == i) {
                continue;
            }
            printf("%s\n", revstr(argv[i]));
        } else {
            printf("%s\n", argv[i]);
        }
    }

	return 0;
}

