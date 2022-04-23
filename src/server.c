#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#define mainFIFO "../FIFOs/mainFIFO"
#define LineLength 1024

typedef struct Config{

    int nop;
    int bcompress;
    int bdecompress;
    int gcompress;
    int gdecompress;
    int encrypt;
    int decrypt;

}*Conf;


//Os valores das configs são inicializados a zero caso não constem no ficheiro de config
void initConf(Conf c){

    c->nop = 0;
    c->bcompress = 0;
    c->bdecompress = 0;
    c->gcompress = 0;
    c->gdecompress = 0;
    c->encrypt = 0;
    c->decrypt = 0;

}


void readConf(Conf c,const char *path){

    ssize_t size;
    int fd = open(path,O_RDONLY);
    char line[LineLength];
    
    //A linha é do tipo "comando limite"
    while((size = readln(fd,line,LineLength))>0){

        char *token = strtok(line," ");
        int n;

        if(strcmp("nop",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

        else if(strcmp("bcompress",token)==0){

            token = strtok(NULL," ");
            atoi(token);

        }

        else if(strcmp("bdecompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

        else if(strcmp("gcompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

        else if(strcmp("gdecompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

        else if(strcmp("encrypt",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

        else if(strcmp("decrypt",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->nop = n;

        }

    }

}


//Função para ler uma linha de um ficheiro
ssize_t readln(int fd, char* line, size_t size) {
    ssize_t bytes_read = read(fd, line, size);
    if(!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;

    if(bytes_read < line_length) line_length = bytes_read;

    //colocar o '\0' no final da string para ser "válida"
    line[line_length] = 0;
    
    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}


int main(int argc,char *argv[]){

    Conf c;

    initConf(c);
    readConf(c,argv[1]);


    int mainFIFOfd;

    //Esta é a mainFIFO, que apenas recebe os pedidos de conexão
    mkfifo(mainFIFO,0666);

    char pedido[100];
    ssize_t tamanhoPedido;
    char nl = 10;

    while(1){

        mainFIFOfd = open(mainFIFO,O_RDONLY);
        //ficar a ler a mainFIFO à espera de pedidos de conexão,
        //quando uma conexão chega, o servidor lê o pedido e processa-o;
        //O pedido será do tipo "pid;pedido"
        tamanhoPedido = read(mainFIFOfd,pedido,sizeof(pedido));

        if(tamanhoPedido>0){
            
            if(fork()==0){
                write(1,pedido,tamanhoPedido);
                write(1,&nl,1);
                exit(0);
            }

            sleep(1);
            wait(NULL);
        }
        close(mainFIFOfd);
        
    }




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