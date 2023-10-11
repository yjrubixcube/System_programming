#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
bool PROCESS=false;
bool **board;
int proceed=0, column;
int QUANTITY, LMAX, which=0;
char *infile, *outfile;
pthread_mutex_t mutt=PTHREAD_MUTEX_INITIALIZER, mutt2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condi=PTHREAD_COND_INITIALIZER, cond2=PTHREAD_COND_INITIALIZER;

typedef struct Data{
    int start, end, where, rs, threadno, times;
}data;

typedef struct {
    pthread_mutex_t mlock;
    pthread_cond_t pcond;
    int done,bnum, continent[2];
    bool bd1[13769229], bd2[13769229];
}bftype;

void to_int(char *ch, int *row, int *col, int *epoch){
    int len, tmp[3]={0,0,0},where=0;
    for(len=0;ch[len]!='\n';len++){
        if (ch[len]!=' '){
            tmp[where]*=10;
            tmp[where]+=ch[len]-'0';
        }else{
            where++;
        }
    }
    *row=tmp[0];
    *col=tmp[1];
    *epoch=tmp[2];
}

void copy_board(FILE *fds, bool **board, int row, int col){
    char buf[col+3];
    for (int i = 0;i<row+2;i++){
        if (i>0&&i!=row+1){
            fgets(buf, sizeof(buf), fds);
            //printf("red:%s", buf);
        }
        
        for (int j = 0;j<col+2;j++){
            if (i==0||j==0||i==row+1||j==col+1){
                board[0][i*(col+2)+j]=false;
            }else{
                //printf("%d,%d %c\n", i,j,buf[j-1]);
                if (buf[j-1]=='.'){
                    board[0][i*(col+2)+j]=false;
                }else if (buf[j-1]=='O'){
                    board[0][i*(col+2)+j]=true;
                }
            }
        }
    }
}

void process_copy_board(FILE *fds, bool *board, bool *board2, int row, int col){
    char buf[col+3];
    for (int i = 0;i<row+2;i++){
        if (i>0&&i!=row+1){
            fgets(buf, sizeof(buf), fds);
            //printf("red:%s", buf);
        }
        
        for (int j = 0;j<col+2;j++){
            if (i==0||j==0||i==row+1||j==col+1){
                board[i*(col+2)+j]=false;
                //board2[i*(col+2)+j]=false;
            }else{
                //printf("%d,%d %c\n", i,j,buf[j-1]);
                if (buf[j-1]=='.'){
                    board[i*(col+2)+j]=false;
                    //board2[i*(col+2)+j]=false;
                }else if (buf[j-1]=='O'){
                    board[i*(col+2)+j]=true;
                    //board2[i*(col+2)+j]=false;
                }
            }
        }
    }
    //printf("donecopy");
}

void *calculate(void *info){
    data *D=(data*)info;
    for (int m=0;m<D->times;m++){ //for each epoch
        for (int x=D->start;x<=D->end;x++){
            int count=0;
            if (x%(column+2)==0||x%(column+2)==column+1){ //do nothing
                board[1-which][x]=false;
            }else{
                if (board[which][x-1-column-2]){
                    count++;
                }if (board[which][x-column-2]){
                    count++;
                }if (board[which][x+1-column-2]){
                    count++;
                }if (board[which][x-1]){
                    count++;
                }if (board[which][x+1]){
                    count++;
                }if (board[which][x-1+column+2]){
                    count++;
                }if (board[which][x+column+2]){
                    count++;
                }if (board[which][x+1+column+2]){
                    count++;
                }
                if (board[which][x]){ //live
                    if (count==2||count==3){
                        board[1-which][x]=true;
                    }else{
                        board[1-which][x]=false;
                    }
                }else{
                    if (count==3){
                        board[1-which][x]=true;
                    }
                    else{
                        board[1-which][x]=false;
                    }
                }
                
            }
        }
        pthread_mutex_lock(&mutt);
        LMAX+=D->threadno;
        pthread_cond_signal(&condi);
        pthread_mutex_unlock(&mutt);
        
        pthread_mutex_lock(&mutt);
        while (m==proceed){
            pthread_cond_wait(&cond2, &mutt);
        }
        pthread_mutex_unlock(&mutt);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    //inputs
    bftype *bufptr;
    pthread_mutexattr_t mattr;
    pthread_condattr_t condattr;
    if (argv[1][1]=='p'){
        PROCESS=true;
        //printf("process\n");
        bufptr=(bftype *)mmap(NULL, sizeof(bftype), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        pthread_mutexattr_init(&mattr);
        pthread_condattr_init(&condattr);
        pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&(bufptr->mlock), &mattr);
        pthread_cond_init(&(bufptr->pcond), &condattr);
    }
    QUANTITY=atoi(argv[2]);
    infile=argv[3];
    outfile=argv[4];
    pthread_t threads[QUANTITY];
    FILE *fds, *out;
    out=fopen(outfile, "w");
    fds=fopen(infile, "r");
    char buf[20];
    int row,col,eopch;
    fgets(buf, sizeof(buf), fds);
    to_int(buf,&row,&col,&eopch);
    column=col;
    
    //printf("row:%d\ncol:%d\nepo:%d\n", row,col,eopch);
    if (PROCESS){
        process_copy_board(fds, bufptr->bd1, bufptr->bd2, row, col);
        int blockleft=row*(col+2)-2;
        blockleft/=2;
        bufptr->done=0;
        bufptr->bnum=0;
        bufptr->continent[0]=0;
        bufptr->continent[1]=0;
        int pstart=col+3,pend=blockleft+pstart-1,cstart=pend+1,cend=(row+1)*(col+2)-2;
        
        if (fork()==0){
            for (int i = 0;i<eopch;i++){
                for (int x=cstart;x<=cend;x++){
                    int count=0;
                    if (bufptr->bnum==0){
                        if (x%(col+2)==0||x%(col+2)==col+1){
                            bufptr->bd2[x]=false;
                        }
                        else{
                            if (bufptr->bd1[x-1-col-2]){
                                count++;
                            }if (bufptr->bd1[x-col-2]){
                                count++;
                            }if (bufptr->bd1[x+1-col-2]){
                                count++;
                            }if (bufptr->bd1[x-1]){
                                count++;
                            }if (bufptr->bd1[x+1]){
                                count++;
                            }if (bufptr->bd1[x-1+col+2]){
                                count++;
                            }if (bufptr->bd1[x+col+2]){
                                count++;
                            }if (bufptr->bd1[x+1+col+2]){
                                count++;
                            }
                            if (bufptr->bd1[x]){
                                if (count==2||count==3){
                                    bufptr->bd2[x]=true;
                                }else{
                                    bufptr->bd2[x]=false;
                                }
                            }else{
                                if (count==3){
                                    bufptr->bd2[x]=true;
                                }else{
                                    bufptr->bd2[x]=false;
                                }
                            }
                        }
                    }
                    else{
                        if (x%(col+2)==0||x%(col+2)==col+1){
                            bufptr->bd1[x]=false;
                        }
                        else{
                            if (bufptr->bd2[x-1-col-2]){
                                count++;
                            }if (bufptr->bd2[x-col-2]){
                                count++;
                            }if (bufptr->bd2[x+1-col-2]){
                                count++;
                            }if (bufptr->bd2[x-1]){
                                count++;
                            }if (bufptr->bd2[x+1]){
                                count++;
                            }if (bufptr->bd2[x-1+col+2]){
                                count++;
                            }if (bufptr->bd2[x+col+2]){
                                count++;
                            }if (bufptr->bd2[x+1+col+2]){
                                count++;
                            }
                            if (bufptr->bd2[x]){
                                if (count==2||count==3){
                                    bufptr->bd1[x]=true;
                                }else{
                                    bufptr->bd1[x]=false;
                                }
                            }else{
                                if (count==3){
                                    bufptr->bd1[x]=true;
                                }else{
                                    bufptr->bd1[x]=false;
                                }
                            }
                        }
                    }
                    
                }

                //printf("childdone\n");
                //fflush(stdout);
                if (bufptr->bnum==1){
                    pthread_mutex_lock(&(bufptr->mlock));
                    bufptr->done++;
                    if (bufptr->done==1){ //only1done
                        while (bufptr->continent[0]==0){
                            pthread_cond_wait(&(bufptr->pcond), &(bufptr->mlock));
                        }
                    }else{
                        bufptr->done=0;
                        bufptr->bnum=0;
                        bufptr->continent[1]=0;
                        bufptr->continent[0]=1;
                        /*printf("side1\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd1[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }printf("side2\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd2[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }*/
                        pthread_cond_signal(&(bufptr->pcond));
                    }
                    pthread_mutex_unlock(&(bufptr->mlock));
                }else{
                    pthread_mutex_lock(&(bufptr->mlock));
                    bufptr->done++;
                    if (bufptr->done==1){ //only1done
                        while (bufptr->continent[1]==0){
                            pthread_cond_wait(&(bufptr->pcond), &(bufptr->mlock));
                        }
                    }else{
                        bufptr->done=0;
                        bufptr->bnum=1;
                        bufptr->continent[0]=0;
                        bufptr->continent[1]=1;
                        /*printf("side1\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd1[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }printf("side2\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd2[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }*/
                        pthread_cond_signal(&(bufptr->pcond));
                    }
                    pthread_mutex_unlock(&(bufptr->mlock));
                }
                //printf("childcont\n");
                //fflush(stdout);

            }
            exit(0);
        }else{
            for (int i = 0;i<eopch;i++){
                for (int x=pstart;x<=pend;x++){
                    int count=0;
                    if (bufptr->bnum==0){
                        if (x%(col+2)==0||x%(col+2)==col+1){
                            bufptr->bd2[x]=false;
                        }
                        else{
                            if (bufptr->bd1[x-1-col-2]){
                                count++;
                            }if (bufptr->bd1[x-col-2]){
                                count++;
                            }if (bufptr->bd1[x+1-col-2]){
                                count++;
                            }if (bufptr->bd1[x-1]){
                                count++;
                            }if (bufptr->bd1[x+1]){
                                count++;
                            }if (bufptr->bd1[x-1+col+2]){
                                count++;
                            }if (bufptr->bd1[x+col+2]){
                                count++;
                            }if (bufptr->bd1[x+1+col+2]){
                                count++;
                            }
                            if (bufptr->bd1[x]){
                                if (count==2||count==3){
                                    bufptr->bd2[x]=true;
                                }else{
                                    bufptr->bd2[x]=false;
                                }
                            }else{
                                if (count==3){
                                    bufptr->bd2[x]=true;
                                }else{
                                    bufptr->bd2[x]=false;
                                }
                            }
                        }
                    }
                    else{
                        if (x%(col+2)==0||x%(col+2)==col+1){
                            bufptr->bd1[x]=false;
                        }
                        else{
                            if (bufptr->bd2[x-1-col-2]){
                                count++;
                            }if (bufptr->bd2[x-col-2]){
                                count++;
                            }if (bufptr->bd2[x+1-col-2]){
                                count++;
                            }if (bufptr->bd2[x-1]){
                                count++;
                            }if (bufptr->bd2[x+1]){
                                count++;
                            }if (bufptr->bd2[x-1+col+2]){
                                count++;
                            }if (bufptr->bd2[x+col+2]){
                                count++;
                            }if (bufptr->bd2[x+1+col+2]){
                                count++;
                            }
                            if (bufptr->bd2[x]){
                                if (count==2||count==3){
                                    bufptr->bd1[x]=true;
                                }else{
                                    bufptr->bd1[x]=false;
                                }
                            }else{
                                if (count==3){
                                    bufptr->bd1[x]=true;
                                }else{
                                    bufptr->bd1[x]=false;
                                }
                            }
                        }
                    }
                }
                
                //printf("parentdone\n");
                //fflush(stdout);
                if (bufptr->bnum==1){
                    pthread_mutex_lock(&(bufptr->mlock));
                    bufptr->done++;
                    if (bufptr->done==1){ //only1done
                        while (bufptr->continent[0]==0){
                            pthread_cond_wait(&(bufptr->pcond), &(bufptr->mlock));
                        }
                    }else{
                        bufptr->done=0;
                        bufptr->bnum=0;
                        bufptr->continent[1]=0;
                        bufptr->continent[0]=1;
                        /*printf("side1\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd1[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }printf("side2\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd2[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }*/
                        pthread_cond_signal(&(bufptr->pcond));
                    }
                    pthread_mutex_unlock(&(bufptr->mlock));
                }else{
                    pthread_mutex_lock(&(bufptr->mlock));
                    bufptr->done++;
                    if (bufptr->done==1){ //only1done
                        while (bufptr->continent[1]==0){
                            pthread_cond_wait(&(bufptr->pcond), &(bufptr->mlock));
                        }
                    }else{
                        bufptr->done=0;
                        bufptr->bnum=1;
                        bufptr->continent[0]=0;
                        bufptr->continent[1]=1;
                        /*printf("side1\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd1[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }printf("side2\n");
                        for (int a=0;a<row+2;a++){
                            for (int b=0;b<col+2;b++){
                                if (bufptr->bd2[a*(col+2)+b]){
                                    printf("O");
                                }else{
                                    printf(".");
                                }
                            }
                            printf("\n");
                        }*/
                        pthread_cond_signal(&(bufptr->pcond));
                    }
                    pthread_mutex_unlock(&(bufptr->mlock));
                }
                /*printf("pardone\n");
                fflush(stdout);*/
            }
            wait(NULL);
        }
        //fflush(stdout);
        //wa
        //printf("%d", bufptr->bnum);
        if (bufptr->bnum==1){
            for (int j=1;j<row+1;j++){
                for (int k =1;k<col+1;k++){
                    if(bufptr->bd2[j*(col+2)+k]){
                        fwrite("O", sizeof(char), 1, out);
                    }
                    else{
                        //printf(".");
                        fwrite(".", sizeof(char), 1, out);
                    }
                }
                if (j==row) break;
                fwrite("\n", sizeof(char), 1, out);
            }
        }
        else{
            for (int j=1;j<row+1;j++){
                for (int k =1;k<col+1;k++){
                    if(bufptr->bd1[j*(col+2)+k]){
                        fwrite("O", sizeof(char), 1, out);
                    }
                    else{
                        //printf(".");
                        fwrite(".", sizeof(char), 1, out);
                    }
                }
                if (j==row) break;
                fwrite("\n", sizeof(char), 1, out);
            }
        }
        
    }else{
        board=(bool **)malloc(2*sizeof(bool *));
        board[0]=(bool *)malloc((row+2)*(col+2)*sizeof(bool));
        board[1]=(bool *)malloc((row+2)*(col+2)*sizeof(bool));
        copy_board(fds, board, row, col);
        
        int tds[QUANTITY],w=0;
        int blockleft=row*(col+2)-2, span;
        int checking=0;
        for (int i=QUANTITY;i>0;i--){
            checking+=i;
        }
        for (int j = QUANTITY;j>0;j--){
            data *D=(data*)malloc(sizeof(data));
            D->end=blockleft+col+2;
            D->start=D->end-blockleft/j+1;
            D->where=w;
            D->threadno=j;
            D->times=eopch;
            blockleft-=blockleft/j;
            tds[j-1]=pthread_create(&threads[j-1], NULL, calculate, (void *)D);
        }
        for (int j = 0;j<eopch;j++){
            pthread_mutex_lock(&mutt);
            while (LMAX!=checking){
                pthread_cond_wait(&condi,&mutt);
            }
            pthread_mutex_unlock(&mutt);
            pthread_mutex_lock(&mutt);
            which=1-which;
            proceed++;
            LMAX=0;
            pthread_cond_broadcast(&cond2);
            pthread_mutex_unlock(&mutt);
        }

        for (int j = QUANTITY;j>0;j--){
            pthread_join(threads[j-1], NULL);
        }
        for (int j=1;j<row+1;j++){
            for (int k =1;k<col+1;k++){
                if(board[which][j*(col+2)+k]){
                    fwrite("O", sizeof(char), 1, out);
                }
                else{
                    fwrite(".", sizeof(char), 1, out);
                }
            }
            if (j==row) break;
            fwrite("\n", sizeof(char), 1, out);
        }
    }    
    //printf("shaochun");
    fclose(fds);
}