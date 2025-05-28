#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(int ppFd, int len, char *name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    char path[len + DIRSIZ];
    
    read(ppFd, path, len);
    fd = open(path, 0);
    fstat(fd, &st);

 



}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: fd dir name");
        exit(1);
    }

    int pp[2];
    pipe(pp);

    int fd;
    struct stat st;

    if (fork() == 0) {
        close(pp[1]);
        find(pp[0], strlen(argv[1]), argv[2]);
        close(pp[0]);
        exit(1);
    } else {
        close(pp[0]);
        
        if((fd = open(argv[1], 0)) < 0) {
            fprintf(2, "find: cannot open %s\n", argv[1]);
            exit(1);
        }

        if(fstat(fd, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", argv[1]);
            close(fd);
            exit(1);
        }

        if (st.type != T_DIR) {
            fprintf(2, "find: %s should be a dir", argv[1]);
            close(fd);
            exit(1);
        }

        write(pp[1], argv[1], strlen(argv[1]));
        close(pp[1]);
        wait(0);
        exit(0);
    }

  
}