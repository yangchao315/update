#include <stdio.h>

int main(void)
{
    FILE* f_bt;
        f_bt=fopen("/tmp/update/update.zip","rb");
    if(f_bt==NULL)
    printf("no update.zip\n");
    else
    printf("get update.zip\n");
    return 0;
}
