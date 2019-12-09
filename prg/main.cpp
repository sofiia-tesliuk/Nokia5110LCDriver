#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>

void clear_buffer (char * buffer) {
    memset(buffer, 0, 32);
}

int main () {
    char buffer [32];
    char message[] = "Hello world";
    int fd;

    clear_buffer(buffer);
    fd = open("/native/lcd_test/led" , O_RDWR);
    if (fd == -1){
        fprintf(stderr, "Error opening file\n");
        exit(-1);
    }

    strcpy(buffer, message);
    write(fd, buffer , sizeof(message));

    clear_buffer(buffer);
    read(fd, buffer , 15);
    fprintf(stdout, "Readed: \n");
    fprintf(stdout, buffer);
    close(fd);
    return 0;
}