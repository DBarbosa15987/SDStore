#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

#define mainFIFO "../FIFOs/mainFIFO"
#define exe "../exe/"
#define server_client_fifo "../FIFOs/server_client_"
#define client_server_fifo "../FIFOs/client_server_"
#define fifo_signals "../FIFOs/fifo_signals"
#define LineLength 512
#define PedidoMAX 32
#define CommandSize 256
#define statusSize 4096
#define nTransf 7
#define messageSize 256


typedef struct Nodo{
   
    char pedido[LineLength];
    int transfCliente[nTransf];
    int pid;
    struct Nodo *prox;

}*Lista;


/*
Config & Ativos & nTransf:

    0 nop;
    1 bcompress;
    2 bdecompress;
    3 gcompress;
    4 gdecompress;
    5 encrypt;
    6 decrypt;

*/


int Config[nTransf];
int Ativos[nTransf];
Lista Espera = NULL;
Lista Execucao = NULL;


// Append numa lista ligada, retornada por output
Lista append(int pid,int *transfCliente,char *pedido,Lista l){
    
    Lista novo = malloc(sizeof(struct Nodo));
    strcpy(novo->pedido,pedido);
    novo->pid = pid;
    for(int i=0;i<nTransf;i++){
        novo->transfCliente[i] = transfCliente[i];
    }
    novo->prox = NULL;

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


// Remove a cabeça da lista de Espera
Lista removeEspera(){

    Lista r = Espera;
    Espera = Espera->prox;
    r->prox = NULL;

    return r;

}


// Remove o pid da lista de execução
Lista removePid(int pid){

    if(!Execucao) return NULL;

    //Remover no primeiro elemento
    if(Execucao->pid==pid){
        Lista r = Execucao;
        Execucao = Execucao->prox;
        r->prox=NULL;
        return r;
    }

    //Remover no meio ou fim da lista
    Lista ant=Execucao;
    Lista curr=Execucao->prox;
    while(ant && curr){

        if(pid==curr->pid){

            ant->prox=curr->prox;
            curr->prox=NULL;
            return curr;

        }

        ant=ant->prox;
        curr=curr->prox;

    }

    // Não retorna nada
    return NULL;

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


void println(char *str){

    char nl = 10;
    write(1,str,strlen(str));
    write(1,&nl,1);

}

//Os valores das configs são inicializados a zero caso não constem no ficheiro de config
void initAtivos(){

    for(int i=0;i<nTransf;i++) Ativos[i]=0;

}


void readConf(const char *path,char file[]){

    int fd = open(path,O_RDONLY);
    read(fd,file,LineLength);
    close(fd);

}

// Formatação da string status que será enviada ao cliente 
// (aqui assume-se que a string tem espaço suficiente)
void getStatus(char *status){

    Lista auxEspera = Espera;
    Lista auxExecucao = Execucao;
    char aux[64];

    // Processos Ativos
    strcat(status,"Lista de Ativos: ");
    if(auxExecucao){
        strcat(status,"\n\n");
    }
    else{
        strcat(status,"Vazia\n\n");
    }

    while(auxExecucao){
        strcat(status,"\ttask : ");
        strcat(status,auxExecucao->pedido);
        strcat(status,"\n");
        auxExecucao=auxExecucao->prox;
    }

    strcat(status,"\n");

    // Lista de Espera
    strcat(status,"Lista de Espera: ");
    if(auxEspera){
        strcat(status,"\n\n");
    }
    else{
        strcat(status,"Vazia\n\n");
    }

    while(auxEspera){
        strcat(status,"\ttask : ");
        strcat(status,auxEspera->pedido);
        strcat(status,"\n");
        auxEspera=auxEspera->prox;
    }

    strcat(status,"\n");

    // Config (running/max)
    sprintf(aux,"transf nop: %d/%d (running/max)\n",Ativos[0],Config[0]);
    strcat(status,aux);
    sprintf(aux,"transf bcompress: %d/%d (running/max)\n",Ativos[1],Config[1]);
    strcat(status,aux);
    sprintf(aux,"transf bdecompress: %d/%d (running/max)\n",Ativos[2],Config[2]);
    strcat(status,aux);
    sprintf(aux,"transf gcompress: %d/%d (running/max)\n",Ativos[3],Config[3]);
    strcat(status,aux);
    sprintf(aux,"transf gdecompress: %d/%d (running/max)\n",Ativos[4],Config[4]);
    strcat(status,aux);
    sprintf(aux,"transf encrypt: %d/%d (running/max)\n",Ativos[5],Config[5]);
    strcat(status,aux);
    sprintf(aux,"transf decrypt: %d/%d (running/max)\n\n",Ativos[6],Config[6]);
    strcat(status,aux);

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


void signalRemoveExecucao(int signum){

    int fd = open(fifo_signals,O_RDONLY);
    int pid;
    read(fd,&pid,sizeof(int));

    Lista removido = removePid(pid);
    for(int i=0;i<nTransf;i++){
        Ativos[i] -= removido->transfCliente[i];
    }

    // Ver os que estão à espera
    Lista EsperaAux = Espera;

    //while(EsperaAux){
    if(Espera){
        int tchau=0;

        for(int i=0;i<nTransf;i++){
            if(Ativos[i] + EsperaAux->transfCliente[i] > Config[i]) tchau=1;       
        }

        if(!tchau){

            //Aqui as transformações estão disponíveis
            int pidNovo = EsperaAux->pid;
            Lista nodo = removeEspera();

            // Marcar os filtros como usados
            for(int i=0;i<nTransf;i++){
                Ativos[i] += nodo->transfCliente[i];
            }

            Execucao = append(nodo->pid,nodo->transfCliente,nodo->pedido,Execucao);
            kill(pidNovo,SIGUSR2);
            return;
        
        }
        //EsperaAux = EsperaAux->prox;

    }

}


void signada(){}



int main(int argc,char *argv[]){

    char configStr[LineLength] = {0};
    readConf(argv[1],configStr);
    char *configArr[nTransf*2];
    int sizeConfig = strToStrArr(configStr,configArr," \n");

    for(int i=1,j=0;i<sizeConfig;j++,i+=2){
        Config[j] = atoi(configArr[i]);
    }

    initAtivos();

    //Esta é a mainFIFO, que apenas recebe os pedidos de conexão
    mkfifo(mainFIFO,0666);
    mkfifo(fifo_signals,0666);

    char pedido[LineLength];
    char fifoRead[64];
    char fifoWrite[64];
    ssize_t tamanhoPedido;

    signal(SIGUSR1,signalRemoveExecucao);

    //abrir um fifo com escrita para o pipe nunca levar EOF, isto tudo em vez do while(1) e colocar while(read>0)
    //Abrir main pipe para receber pedidos de conexão
    int mainFIFOfd = open(mainFIFO,O_RDONLY);
    int mainFIFOfdW = open(mainFIFO,O_WRONLY);

    /*Quando uma conexão chega, o servidor lê o pedido e processa-o;
    O pedido será do tipo "pid pedido arg arg...";*/
    while((tamanhoPedido = read(mainFIFOfd,pedido,sizeof(pedido)))>0){

        

        //Aqui assume-se que o cliente introduziu um comando válido,
        //ou que o client side o tenha autenticado!
        if(tamanhoPedido>0){            

            int transfCliente[nTransf] = {0};
            char *pedidoArr[128];
            char *pedidoCopy = strdup(pedido);
            int pedidoSize = strToStrArr(pedidoCopy,pedidoArr," ");
            int pid = atoi(pedidoArr[0]);
            int temEspaco=1;
            
            memset(fifoRead,0,sizeof(fifoRead));
            memset(fifoWrite,0,sizeof(fifoWrite));
            sprintf(fifoRead,"%s%d",client_server_fifo, pid);
            sprintf(fifoWrite,"%s%d",server_client_fifo, pid);
            
            //Abrir os FIFOs privados do client
            mkfifo(fifoRead,0666);
            mkfifo(fifoWrite,0666);

            //Não esquecer do memset e tal
            if(strcmp(pedidoArr[1],"status")==0){
                
                //Aqui apenas é preciso um FIFO de write
                int fdW = open(fifoWrite,O_WRONLY);
                if(fdW==-1){
                    perror("Erro ao abrir o pipe");
                    exit(-1);
                }
                char statusString[statusSize] = "";
                getStatus(statusString);
                write(fdW,statusString,sizeof(statusString));
                close(fdW);
                unlink(fifoRead);
                unlink(fifoWrite);

            }

            else if(strcmp(pedidoArr[1],"proc-file")==0){//falta aqui validar o path

                for(int i=4;i<pedidoSize;i++){
                    for(int j=0,k=0;j<nTransf;j++,k+=2){
                        if(strcmp(configArr[k],pedidoArr[i])==0) transfCliente[j]++;
                    }
                }

                //Ver se existem transformações disponíveis
                for(int i=0;i<nTransf;i++){
                    if(Ativos[i]+transfCliente[i] > Config[i]) temEspaco=0;
                }
   
                //Se tem espaço, marcar a utilização destas transformações
                if(temEspaco){
                    for(int i=0;i<nTransf;i++){
                        Ativos[i] += transfCliente[i];
                    }
                }
                
                int pid_fileProc;

                // Exemplos de pedidos:
                // 12345 status
                // 12345 proc-file path_in path_out nop nop nop ...
                // { 0       1        2        3     4   5   6  ...}
            
                //Cada fork aqui representa um cliente
                if((pid_fileProc=fork())==0){                    

                    signal(SIGUSR2,signada);

                    int fdW = open(fifoWrite,O_WRONLY);
                    char resposta[messageSize];

                    if(!temEspaco){                       
                        sprintf(resposta,"O servidor não tem capacidade de o atender, o seu pedido foi colocado na lista de espera...\n");
                        write(fdW,resposta,strlen(resposta));
                        pause();
                        sprintf(resposta,"O seu pedido saiu da fila de espera e vai começar a ser executado\n");
                        write(fdW,resposta,strlen(resposta));
                    }
                    else{

                        sprintf(resposta,"O seu pedido vai começar a ser executado\n");
                        write(fdW,resposta,strlen(resposta));

                    }

                    int n_op = pedidoSize-4;

                    // Apenas uma operação, por isso não é preciso pipes,
                    // é feito diretamente do input-file para o output-file
                    if(n_op==1){

                        if(fork()==0){
                            
                            int fdInput = open(pedidoArr[2],O_RDONLY);
                            int fdOutput = open(pedidoArr[3],O_WRONLY | O_CREAT,0666);

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
                        for(int i=0;i<n_op;i++) wait(NULL);

                    }

                    // Acabou o proc-file
                    kill(getppid(),SIGUSR1);
                    sleep(1);
                    int fifoFD = open(fifo_signals,O_WRONLY);
                    int pidFilho = getpid();
                    write(fifoFD,&pidFilho,sizeof(int));
                    close(fifoFD);

                    unlink(fifoRead);
                    unlink(fifoWrite);

                    exit(0);

                }

                // "Pai do cliente"
                else{

                    if(temEspaco){
                        Execucao = append(pid_fileProc,transfCliente,pedido,Execucao);
                    }
                    else{
                        Espera = append(pid_fileProc,transfCliente,pedido,Espera);
                    }

                }

            }
            else{
                printf("Operação inválida\n");
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
    unlink(fifo_signals);

}