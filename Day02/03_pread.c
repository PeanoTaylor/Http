#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

int main()
{
    int fd = open("a.out", O_RDONLY);
    assert(fd != -1 && "Failed to open file");

    // 输出当前文件偏移量
    printf("%zd\n", lseek(fd, 0, SEEK_CUR));

    char buf[4096];
    int n = pread(fd, buf, 4096, 10);
    printf("%.*s\n", n, buf); // 只输出前n个字节
    
    printf("%zd\n", lseek(fd, 0, SEEK_CUR));
    return 0;
}
