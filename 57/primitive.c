#include <unistd.h>

#define BUFFER_SIZE 512

int main(void) {
    char buffer[BUFFER_SIZE];
    int n;
    while ((n = read(STDIN_FILENO, &buffer, BUFFER_SIZE)) == BUFFER_SIZE) {
        write(STDOUT_FILENO, buffer, BUFFER_SIZE);
    }
    read(STDIN_FILENO, &buffer, n);
    write(STDOUT_FILENO, buffer, n);
    return 0;
}