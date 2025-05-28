#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void prime(int* pl) {
   int p;
   if (read(pl[0], &p, 4) != 4) {
      close(pl[0]);
      exit(0);
   }

   printf("prime %d\n", p);
   int pr[2];
   pipe(pr);

   if (fork() == 0) {
      close(pl[0]);
      close(pr[1]);
      prime(pr);
      close(pr[0]);
      exit(0);
   } else {
      close(pr[0]);
      int n;
      while (read(pl[0], &n, 4) == 4) {
         if (n % p != 0) {
            write(pr[1], &n, 4); 
         }
      }
      close(pl[0]);
      close(pr[1]);
      wait(0);
      exit(0);
   
   }

   

}

int main(int argc, char *argv[]) {
   int pp[2];
   pipe(pp);

   if (fork() == 0) {
      close(pp[1]);
      prime(pp);
      close(pp[0]);
      exit(0);
   } else {
      for (int i = 2; i <= 35; i++) {
         write(pp[1], &i, 4);  
      }
      close(pp[1]);
      wait(0);
      exit(0);
   }
   
   
}