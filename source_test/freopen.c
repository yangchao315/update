#include <stdio.h>

int main ()
{
   FILE *fp;

   printf("该文本重定向到 stdout\n");

   fp = freopen("file.txt", "w+", stdout);

   printf("该文本重定向到 file.txt\n");

   fclose(fp);
   
   return(0);
}
