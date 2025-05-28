#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc , char *argv[]) {
    if (argc != 1) {
        fprintf(2, "Usage: pingpong");
        exit(0);
    }

    int p[2];
    pipe(p);
    char byte[1];

    if (fork() == 0) {
        read(p[0], byte, 1);
        printf("%d: received ping\n", getpid());
        write(p[1], byte, 1);
        close(p[0]);
        close(p[1]);
        exit(0);
    } else {
        write(p[1], "a", 1);
        wait(0);
        read(p[0], byte, 1);
        printf("%d: received pong\n", getpid());
        write(p[1], byte, 1);
        close(p[0]);
        close(p[1]);
        exit(0);
    }
}