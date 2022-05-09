#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#define mainFIFO "../FIFOs/mainFIFO"
#define exe "../exe/"
#define server_client_fifo "../FIFOs/server_client_"
#define client_server_fifo "../FIFOs/client_server_"
#define LineLength 512
#define PedidoMAX 32
#define CommandSize 256
#define statusSize 2048


typedef struct Nodo{

    
    char pedido[LineLength];
    int pid;
    struct Nodo *prox;

}*Lista;


Lista Espera = NULL;
Lista Execucao = NULL;


void removeEspera(){

    if(!Espera) return;
    Espera = Espera->prox;

}


Lista append(int pid,char *pedido,Lista l){

    Lista novo = malloc(sizeof(struct Nodo));
    strcpy(novo->pedido,pedido);
    novo->pid=pid;
    novo->prox=NULL;

    if(!l) l=novo;
    
    else{
        Lista aux=l;
        while(aux->prox){
            aux=aux->prox;
        }

        aux->prox=novo;
    }

    return l;

}


Lista removePid(int pid,Lista l){

    if(!l) {return NULL;}

    //Remover no primeiro elemento
    if(l->pid==pid){
        l = l->prox;
        return l;
    }

    //Remover no meio ou fim da lista
    Lista ant=l;
    Lista curr=l->prox;
    while(ant && curr){

        if(pid==curr->pid){

            ant->prox=curr->prox;
            break;

        }

        ant=ant->prox;
        curr=curr->prox;

    }
    return l;

}


void printList(Lista lista){
    Lista l=lista;
    while(l){
        printf("Lista de execução %s|\n",l->pedido);
        l=l->prox;
    }
    putchar('\n');
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


typedef struct Filtros{

    int nop;
    int bcompress;
    int bdecompress;
    int gcompress;
    int gdecompress;
    int encrypt;
    int decrypt;

}*Conf,*Ativos;

Ativos a;

void println(char *str){

    char nl = 10;
    write(1,str,strlen(str));
    write(1,&nl,1);

}

//Os valores das configs são inicializados a zero caso não constem no ficheiro de config
void initAtivos(Ativos a){

    a->nop = 0;
    a->bcompress = 0;
    a->bdecompress = 0;
    a->gcompress = 0;
    a->gdecompress = 0;
    a->encrypt = 0;
    a->decrypt = 0;

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

// Formatação da string status que será enviada ao cliente 
// (aqui assume-se que a string tem espaço suficiente)
void getStatus(char *status,Conf c){

    Lista auxEspera = Espera;
    Lista auxExecucao = Execucao;
    char aux[64];

    // Processos Ativos
    for(;auxExecucao;auxExecucao=auxExecucao->prox){
        strcat(status,"task : ");
        strcat(status,auxExecucao->pedido);
        strcat(status,"\n");
    }

    // Config (running/max)
    sprintf(aux,"transf nop: %d/%d (running/max)\n",a->nop,c->nop);
    strcat(status,aux);
    sprintf(aux,"transf bcompress: %d/%d (running/max)\n",a->bcompress,c->bcompress);
    strcat(status,aux);
    sprintf(aux,"transf bdecompress: %d/%d (running/max)\n",a->bdecompress,c->bdecompress);
    strcat(status,aux);
    sprintf(aux,"transf gcompress: %d/%d (running/max)\n",a->gcompress,c->gcompress);
    strcat(status,aux);
    sprintf(aux,"transf gdecompress: %d/%d (running/max)\n",a->gdecompress,c->gdecompress);
    strcat(status,aux);
    sprintf(aux,"transf encrypt: %d/%d (running/max)\n",a->encrypt,c->encrypt);
    strcat(status,aux);
    sprintf(aux,"transf decrypt: %d/%d (running/max)\n",a->decrypt,c->decrypt);
    strcat(status,aux);

}


void incrementaAtivos(char *transformacao){

    if(strcmp("nop",transformacao)==0) a->nop++;

    else if(strcmp("bcompress",transformacao)==0) a->bcompress++;

    else if(strcmp("bdecompress",transformacao)==0) a->bdecompress++;

    else if(strcmp("gcompress",transformacao)==0) a->gcompress++;

    else if(strcmp("gdecompress",transformacao)==0) a->gdecompress++;

    else if(strcmp("encrypt",transformacao)==0) a->encrypt++;

    else if(strcmp("decrypt",transformacao)==0) a->decrypt++;

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

    Conf c = malloc(sizeof(struct Filtros));
    readConf(c,argv[1]);

    a = malloc(sizeof(struct Filtros));
    initAtivos(a);

    //Esta é a mainFIFO, que apenas recebe os pedidos de conexão
    mkfifo(mainFIFO,0666);

    char pedido[LineLength];
    char fifoRead[64];
    char fifoWrite[64];
    ssize_t tamanhoPedido;

    //abrir um fifo com escruta para o pipe nunca levar EOF, isto tudo em vez do while(1) e colocar while(read>0)
    //Abrir main pipe para receber pedidos de conexão
    int mainFIFOfd = open(mainFIFO,O_RDONLY);
    int mainFIFOfdW = open(mainFIFO,O_WRONLY);

    /*Quando uma conexão chega, o servidor lê o pedido e processa-o;
    O pedido será do tipo "pid pedido arg arg...";*/
    while((tamanhoPedido = read(mainFIFOfd,pedido,sizeof(pedido)))>0){


        //Aqui assume-se que o cliente introduziu um comando válido,
        //ou que o client side o tenha autenticado!
        if(tamanhoPedido>0){
        
            //printf("Pedido: %s|\n",pedido);
            //fflush(NULL);
            
            //Cada fork aqui representa um cliente
            if(fork()==0){

                char *pedidoArr[128];
                char *pedidoCopy = strdup(pedido);
                int pedidoSize = strToStrArr(pedidoCopy,pedidoArr," ");
                int pid = atoi(pedidoArr[0]);
                
                memset(fifoRead,0,sizeof(fifoRead));
                memset(fifoWrite,0,sizeof(fifoWrite));
                sprintf(fifoRead,"%s%d",client_server_fifo, pid);
                sprintf(fifoWrite,"%s%d",server_client_fifo, pid);
                
                //Abrir os FIFOs privados do client
                mkfifo(fifoRead,0666);
                mkfifo(fifoWrite,0666);
 
                // Exemplos de pedidos:
                // 12345 status
                // 12345 proc-file path_in path_out nop nop nop ...
                // { 0       1        2        3     4   5   6  ...}


                //falta defenir uma string, e não esquecer do memset e tal
                if(strcmp(pedidoArr[1],"status")==0){
                    
                    //aqui falta calcular a string de status
                    //Aqui apenas é preciso um FIFO de write
                    int fdW = open(fifoWrite,O_WRONLY);
                    if(fdW==-1){
                        perror("Erro ao abrir o pipe");//error check
                        exit(-1);
                    }
                    char statusString[statusSize];
                    getStatus(statusString,c);
                    write(fdW,statusString,sizeof(statusString));
                    close(fdW);

                }

                else if(strcmp(pedidoArr[1],"proc-file")==0){//falta aqui validar o path
                    
                    int n_op = pedidoSize-4;
                    //printf("\nproc-file:%d\n",n_op);
                    Execucao = append(pid,pedido,Execucao);
                    printList(Execucao);
                    fflush(NULL);

                    // Apenas uma operação, por isso não é preciso pipes,
                    // é feito diretamente do input-file para o output-file
                    if(n_op==1){

                        if(fork()==0){

                            int fdInput = open(pedidoArr[2],O_RDONLY);
                            int fdOutput = open(pedidoArr[3],O_WRONLY | O_CREAT,0666);

                            printf("%s\n",pedidoArr[4]);
                            //a->nop=3;
                            incrementaAtivos(pedidoArr[4]);
                            printf("%d\n",a->nop);
                            //write(1,pedidoArr[0],strlen(pedidoArr[0]));
                            //write(1,pedidoArr[1],strlen(pedidoArr[1]));
                            //write(1,pedidoArr[2],strlen(pedidoArr[2]));
                            //write(1,pedidoArr[3],strlen(pedidoArr[3]));
                            //write(1,pedidoArr[4],strlen(pedidoArr[4]));
                            //write(1,pedidoArr[4],strlen(pedidoArr[4]));
                            dup2(fdInput,0);
                            close(fdInput);
                            dup2(fdOutput,1);
                            close(fdOutput);

                            char command[CommandSize];
                            sprintf(command,"%s%s",exe,pedidoArr[4]);
                            execl(command,command,NULL);

                            // só será usado se o exec se vergar, não esquecer de ver isto!!
                            exit(-1);

                        }
                        else{

                            int status;
                            wait(&status);

                        }

                    }
                    
                    //Mais do que uma transformação aplicada a um ficheiro
                    //Um ou mais pipes serão necessários
                    else{

                        //Criação dos pipes necessários
                        int n_pipes = n_op - 1;
                        int p[n_pipes][2];
                        int opIndex;

                        for(int i=0;i<n_op;i++){

                            opIndex = 4+i;
                            //No último comando, não é necessário criar pipe
                            if(i!=(n_op-1)){
                                if(pipe(p[i])==-1){
                                    perror("Erro ao abrir o pipe: ");
                                    exit(-1);
                                }
                            }
                            
                            //Primeiro argumento, em que não lêmos de um pipe,
                            //mas sim do pedido original e escrevemos num pipe
                            if(i==0){

                                if(fork()==0){
                                    
                                    close(p[i][0]);
                                    int fdInput = open(pedidoArr[2],O_RDONLY);

                                    dup2(fdInput,0);
                                    close(fdInput);
                                    dup2(p[i][1],1);
                                    close(p[i][1]);

                                    char command[CommandSize];
                                    sprintf(command,"%s%s",exe,pedidoArr[opIndex]);
                                    incrementaAtivos(pedidoArr[opIndex]);
                                    execl(command,command,NULL);

                                    // só será usado se o exec se vergar, não esquecer de ver isto!!
                                    //talvez executar o _exit???
                                    exit(-1);
                                }
                                else{
                                    
                                    close(p[i][1]);

                                }

                            }
                            //Último argumento, que o conteúdo é lido de um pipe
                            //e escrito para o ficheiro de output recebido no pedido
                            else if(i==(n_op-1)){

                                if(fork()==0){

                                    int fdOutput = open(pedidoArr[3],O_WRONLY | O_CREAT,0666);

                                    dup2(fdOutput,1);
                                    close(fdOutput);
                                    dup2(p[i-1][0],0);
                                    close(p[i-1][0]);

                                    char command[CommandSize];
                                    sprintf(command,"%s%s",exe,pedidoArr[opIndex]);
                                    incrementaAtivos(pedidoArr[opIndex]);
                                    execl(command,command,NULL);

                                    // só será usado se o exec se vergar, não esquecer de ver isto!!
                                    exit(-1);

                                }
                                else{

                                    close(p[i-1][0]);                                    

                                }

                            }

                            //Caso intermédio de escrita do conteúdo em pipes
                            else{

                                if(fork()==0){

                                    close(p[i][0]);
                                    dup2(p[i-1][0],0);
                                    close(p[i-1][0]);
                                    dup2(p[i][1],1);
                                    close(p[i][1]);

                                    char command[CommandSize];
                                    sprintf(command,"%s%s",exe,pedidoArr[opIndex]);
                                    incrementaAtivos(pedidoArr[opIndex]);
                                    execl(command,command,NULL);

                                    // só será usado se o exec se vergar, não esquecer de ver isto!!
                                    exit(-1);

                                }
                                else{

                                    close(p[i-1][0]);
                                    close(p[i][1]);

                                }

                            }

                        }

                        // Esperar para que todos os filhos
                        for(int i=0;i<n_op;i++){
                            
                            int status;
                            wait(&status);

                        }

                    }

                }

                else{

                    printf("Comando inválido\n");
                    fflush(NULL);

                }


                unlink(fifoRead);
                unlink(fifoWrite);

                exit(0);
            }            

        }

        else{

            perror("Erro na Leitura: ");
            exit(-1);

        }

    }

    close(mainFIFOfdW);
    close(mainFIFOfd);
    unlink(mainFIFO); 

}



/*

./client proc-file ../b ../bruh bcompress encrypt decrypt bdecompress


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