#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    for (int i = 2; i <= 35; i++) {
        write(p[1], &i, 4);
    }

    int num;
    for (int i = 2; i <= 35; i++) {
        read(p[0], &num, 4);
        printf("%d\n", num);
    }
    exit(0);

}