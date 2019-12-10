#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <iosfwd>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <fstream>

#define MAX_STREAM_SIZE (116)


void clear_buffer (char * buffer) {
    memset(buffer, 0, 32);
}


int main (int argc, char** argv) {
    char buffer [MAX_STREAM_SIZE];

    std::vector<std::string> argv_vec(argv, argv + argc);

    if (argv_vec.size() == 3) {

        std::string& readable_type = argv_vec[1];

        // Read input to buffer either as a file or an input-string.
        if (readable_type == "-f ") {
            std::ifstream f(argv_vec[2].c_str());
            if (f.good()) {
                int fd = open(argv_vec[2].c_str(), O_RDONLY);
                read(fd, buffer , MAX_STREAM_SIZE);
            } else {
                std::cout << "Invalid file given: " << argv_vec[2] << std::endl;
                exit(EXIT_FAILURE);
            }

        } else if (readable_type == "-m") {
            strcpy(buffer, argv_vec[2].c_str());

        } else {
            std::cout << "Invalid options (only -f / -m supported)" << std::endl;
            exit(EXIT_FAILURE);
        }

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

    return -1;
}