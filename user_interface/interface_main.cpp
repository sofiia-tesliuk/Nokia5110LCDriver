#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <iosfwd>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>


#define MAX_STREAM_SIZE (510)
#define MAX_TEXT_SIZE (84)

#define LCD_WIDTH 84
#define LCD_HEIGHT 48


void clear_buffer (char * buffer) {
    memset(buffer, 0, 32);
}


int main (int argc, char** argv) {
    char buffer[MAX_STREAM_SIZE];
    clear_buffer(buffer);

    std::vector<std::string> argv_vec(argv, argv + argc);
    if (argv_vec.size() == 3) {
        std::string &readable_type = argv_vec[1];

        // Read input to buffer either as a file or an input-string.
        if ((readable_type == "-f") || (readable_type == "-i")) {
            std::ifstream f_stream(argv_vec[2]);
            if (f_stream.is_open() && !f_stream.rdstate()) {
                std::string content((std::istreambuf_iterator<char>(f_stream)),
                                        (std::istreambuf_iterator<char>()));
                if (readable_type == "-f"){
                    strcpy(buffer, content.substr(0, MAX_TEXT_SIZE).c_str());
                } else{
                    buffer[0] = 0x43;
                    buffer[1] = 0x55;
                    std::istringstream iss(content);
                    int image_length = LCD_WIDTH * LCD_HEIGHT / 8;
                    int i = 0;
                    for (std::string line; std::getline(iss, line); ){
                        if (i >= image_length)
                            break;

                        buffer[i + 2] = char(std::stoi(line));
                        i++;
                    }
                    buffer[image_length + 2] = 0x00;
                    buffer[image_length + 3] = '\0';
                }
            } else {
                std::cout << "Couldn't open file: " << argv_vec[2] << std::endl;
                exit(EXIT_FAILURE);
            }

        } else if (readable_type == "-m") {
            strcpy(buffer, argv_vec[2].substr(0, MAX_STREAM_SIZE).c_str());

        } else {
            std::cout << "Invalid option " << readable_type << "(only -f / -m supported)" << std::endl;
            exit(EXIT_FAILURE);
        }

        int fd;

        fd = open("/dev/gpio_lcd5110" , O_RDWR);
        if (fd == -1){
            fprintf(stderr, "Error opening file\n");
            exit(-1);
        }

        write(fd, buffer , sizeof(buffer));

        close(fd);
        clear_buffer(buffer);

        return 0;
    }

    return -1;
}