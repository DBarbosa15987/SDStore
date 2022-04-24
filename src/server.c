#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#define mainFIFO "../FIFOs/mainFIFO"
#define server_client_fifo "../FIFOs/server_client_"
#define client_server_fifo "../FIFOs/client_server_"
#define LineLength 1024
#define PedidoMAX 16

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
            n = atoi(token);
            c->bcompress = n;

        }

        else if(strcmp("bdecompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->bdecompress = n;

        }

        else if(strcmp("gcompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->gcompress = n;

        }

        else if(strcmp("gdecompress",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->gdecompress = n;

        }

        else if(strcmp("encrypt",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->encrypt = n;

        }

        else if(strcmp("decrypt",token)==0){

            token = strtok(NULL," ");
            n = atoi(token);
            c->decrypt = n;

        }

    }

}


int strToStrArr(char *string, char **str_array, char *delimit) {
    
    int n_words = 0;
    char *p = strtok(string, delimit);
    while (p != NULL) {
        if(n_words==PedidoMAX) return -1;//erro de pedido muito grande

        str_array[n_words++] = p;
        p = strtok(NULL, delimit);
    }
    return n_words;
}

int main(int argc,char *argv[]){

    Conf c = malloc(sizeof(struct Config));
    initConf(c);
    readConf(c,argv[1]);

    //Esta é a mainFIFO, que apenas recebe os pedidos de conexão
    mkfifo(mainFIFO,0666);

    int mainFIFOfd;
    char pedido[100];
    char fifoRead[64];
    char fifoWrite[64];
    ssize_t tamanhoPedido;

    while(1){

        //Abrir main pipe para receber pedidos de conexão
        mainFIFOfd = open(mainFIFO,O_RDONLY);
        
        /*Quando uma conexão chega, o servidor lê o pedido e processa-o;
        O pedido será do tipo "pid pedido arg arg...";*/
        tamanhoPedido = read(mainFIFOfd,pedido,sizeof(pedido));
        //Aqui assume-se que o cliente introduzio um comando válido ou que o client side o tenha autenticado
        printf("Pedido: %s|\n",pedido);
        fflush(NULL);

        if(tamanhoPedido>0){
            
            if(fork()==0){

                char *pedidoArr[PedidoMAX];
                strToStrArr(pedido,pedidoArr," ");
                int pid = atoi(pedidoArr[0]);
                
                memset(fifoRead,0,sizeof(fifoRead));
                memset(fifoWrite,0,sizeof(fifoWrite));
                sprintf(fifoRead,"%s%d",client_server_fifo, pid);
                sprintf(fifoWrite,"%s%d",server_client_fifo, pid);
                
                //abrir os FIFOs privados do client
                mkfifo(fifoRead,0666);
                mkfifo(fifoWrite,0666);
                
                //int fdR = open(fifoRead,O_RDONLY);
                int fdW = open(fifoWrite,O_WRONLY);

                
                //falta defenir uma string, e não esquecer do memset e tal
                if(strcmp(pedidoArr[1],"status")==0){
                    
                    char a[] = "bruhhhhhhh";
                    write(fdW,a,sizeof(a));
                    close(fdW);

                }


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