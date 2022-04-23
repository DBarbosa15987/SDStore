#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#define mainFIFO "../FIFOs/mainFIFO"

int main(){

    char usrInput[100];
    mkfifo(mainFIFO,0666);
    read(0,usrInput,100);


    //deteção de erros
    char pedido[110];
    memset(pedido,0,sizeof(pedido));
    sprintf(pedido,"%d;%s",getpid(), usrInput);
    printf("%d %s\n",getpid(),pedido);
    int mainFIFOfd = open(mainFIFO,O_WRONLY);
    if(write(mainFIFOfd,pedido,sizeof(pedido))<0){
        perror("bruh");
        exit(-1);
    }
    close(mainFIFOfd);
    

}



/*

0000 is ---------
0666 is rw-rw-rw-
0777 is rwxrwxrwx
0700 is rwx------
0100 is --x------
0001 is --------x
0002 is -------w-
0003 is -------wx
0004 is ------r--
0005 is ------r-x
0006 is ------rw-
0007 is ------rwx



*/