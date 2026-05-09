#include <signal.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) return -1;

    char *input = argv[1];
    size_t len = strlen(input);    
    if (len >= 1 && input[0] == 'a') {
        if (len >= 2 && input[1] == 'b') {
            if (len >= 3 && input[2] == 'c') {
                raise(SIGTERM);
            }
            return 2;
        }
        return 1;
    }
    return 0;
}