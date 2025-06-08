#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char* dir, char *name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    fd = open(dir, 0);

    strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0 || !strcmp(de.name, ".") || !strcmp(de.name, "..")) {
            continue;
        }

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if(stat(buf, &st) < 0) {
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        switch (st.type) {
            case T_FILE:
                if (!strcmp(de.name, name)) {
                    printf("%s\n", buf);
                }
                break;
            case T_DIR:
                if (fork() == 0) {
                    close(fd);
                    find(buf, name);
                    exit(0);
                }
                break;
        }
    }

    close(fd);
    while (wait(0) != -1);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: fd dir name");
        exit(1);
    }

    int fd;
    struct stat st;

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

    if (fork() == 0) {
        find(argv[1], argv[2]);
        close(fd);
        exit(0);
    }

    close(fd);
    wait(0);
    exit(0);

  
}