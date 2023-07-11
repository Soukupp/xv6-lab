#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, int **argv){
    if(argc<2){
        printf("Usage: sleep <number>\n");
        exit(1);
    }
    sleep(atoi(argv[1]));
    exit(0);
}