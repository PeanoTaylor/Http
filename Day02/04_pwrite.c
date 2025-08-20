#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

int main(){
    int fd = open("a.txt", O_WRONLY | O_CREAT, 0666);
    assert(fd != -1 && "Failed to open file");

    printf("%zd\n", lseek(fd, 0, SEEK_CUR));   // 输出当前文件偏移量，应该是0

    // int n = write(fd, "kitty", 5);
    int n = pwrite(fd, "kitty", 5, 6);
    printf("%zd\n", lseek(fd, 0, SEEK_CUR));   // 输出当前文件偏移量，应该是0
    
    return 0;
}

