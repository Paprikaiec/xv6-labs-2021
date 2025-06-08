#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    char *buf[MAXARG], c;
    int count = argc - 1, sLen;
    static char s[100];

    for (int i = 1; i < argc; i++) {
        buf[i-1] = argv[i];
    }


    while ((read(0, &c, 1) > 0) ) {
        if (count >= MAXARG) {
            fprintf(2, "xargs: too many args");
            exit(1);
        }
        sLen = strlen(s);

        switch (c) {
          
            case ' ':    
                if (sLen > 0) {
                    buf[count] = (char*) malloc((sLen + 1));
                    memset(buf[count], 0, sLen + 1);
                    strcpy(buf[count], s);
                    count ++;
                    memset(s, 0, 100);
                }
                break;
                
            case '\n':
                if (sLen > 0) {
                    buf[count] = (char*) malloc((sLen + 1));
                    memset(buf[count], 0, sLen + 1);
                    strcpy(buf[count], s);
                    count ++;
                    memset(s, 0, 100);
                }

                buf[count] = 0;
             
            
                if (fork() == 0) {
                    exec(buf[0], buf);
                    fprintf(2, "exec %s failed\n", buf[0]);
                }

                for (int j = argc - 1; j < count; j++) {
                    free(buf[j]);
                }
                count = argc - 1;
                
                break;

            default:
                *(s + sLen) = c;
                break;
        }   
    }

    while (wait(0) > 0);
    exit(0);
    

}