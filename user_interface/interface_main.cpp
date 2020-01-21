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
#include <map>


#define MAX_STREAM_SIZE (510)
#define MAX_TEXT_SIZE (84)

#define LCD_WIDTH 84
#define LCD_HEIGHT 48


void clear_buffer (char * buffer) {
    memset(buffer, 0, 32);
}


int main (int argc, char** argv) {
    char buffer[MAX_STREAM_SIZE];
    char buffer_2[MAX_STREAM_SIZE];
    clear_buffer(buffer);
    clear_buffer(buffer_2);

    int buffers_number = 0;


    bool delay = false;
    std::map<std::string, char*> variables;

    std::vector<std::string> argv_vec(argv, argv + argc);
    if (argv_vec.size() == 3) {
        std::string &readable_type = argv_vec[1];

        // Read input to buffer either as a file or an input-string.
        if ((readable_type == "-f") || (readable_type == "-i")) {
            std::ifstream f_stream(argv_vec[2]);
            if (f_stream.is_open() && !f_stream.rdstate()) {
                std::string content((std::istreambuf_iterator<char>(f_stream)),
                                    (std::istreambuf_iterator<char>()));
                if (readable_type == "-f") {
                    if (content.size() > MAX_TEXT_SIZE) {
                        delay = true;
                        strcpy(buffer, content.substr(0, MAX_TEXT_SIZE).c_str());
                        buffers_number = (int) content.size() / MAX_TEXT_SIZE;
                        std::string buff_name = "buffer";

                        for (int i = 2; i <= buffers_number; i++) {
                            std::string curr_buff_name = buff_name + "_" + std::to_string(i);
                            variables[curr_buff_name] = (char *) malloc (MAX_STREAM_SIZE * sizeof(char));
                            int index_to = MAX_TEXT_SIZE * (i - 1) + MAX_TEXT_SIZE;
                            if (i == buffers_number)
                                index_to = (int) content.size()  - (i - 1) * MAX_TEXT_SIZE;
                            strcpy(variables[curr_buff_name], content.substr(MAX_TEXT_SIZE * (i - 1), index_to).c_str());
                        }

                    } else
                        strcpy(buffer, content.substr(0, MAX_TEXT_SIZE).c_str());
                } else {
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

                    if (delay) {
                        for (auto const& x : variables){
                            x.second[0] = 0x43;
                            x.second[1] = 0x55;
                            std::istringstream iss_2(content);
                            int j = 0;
                            for (std::string line_2; std::getline(iss_2, line_2); ){
                                if (j >= image_length)
                                    break;

                                x.second[j + 2] = char(std::stoi(line_2));
                                j++;
                            }
                            x.second[image_length + 2] = 0x00;
                            x.second[image_length + 3] = '\0';
                        }
//                        buffer_2[0] = 0x43;
//                        buffer_2[1] = 0x55;
//                        std::istringstream iss_2(content);
//                        int j = 0;
//                        for (std::string line_2; std::getline(iss_2, line_2); ){
//                            if (j >= image_length)
//                                break;
//
//                            buffer_2[j + 2] = char(std::stoi(line_2));
//                            j++;
//                        }
//                        buffer_2[image_length + 2] = 0x00;
//                        buffer_2[image_length + 3] = '\0';
                    }
                }
            } else {
                std::cout << "Couldn't open file: " << argv_vec[2] << std::endl;
                exit(EXIT_FAILURE);
            }

        } else if (readable_type == "-m") {
            if (argv_vec[2].size() > MAX_TEXT_SIZE){
                delay = true;
                strcpy(buffer, argv_vec[2].substr(0, MAX_TEXT_SIZE).c_str());
                strcpy(buffer_2, argv_vec[2].substr(MAX_TEXT_SIZE, MAX_STREAM_SIZE).c_str());
            } else
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

        if (delay) {
            write(fd, buffer, sizeof(buffer));
            usleep(3000000);
            for (auto const& x : variables){
                write(fd, x.first.c_str(), sizeof(x.first.c_str()));
                usleep(3000000);
            }
//            while(true) {
//                write(fd, buffer, sizeof(buffer));
//                usleep(3000000);
//                write(fd, buffer_2, sizeof(buffer_2));
//                usleep(3000000);
//            }
        } else
            write(fd, buffer , sizeof(buffer));

        close(fd);
        clear_buffer(buffer);

        for (auto const& x : variables){
            free(x.second);
        }

        return 0;
    }

    return -1;
}