#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define HOST_ID 0
#define DEPTH 1
#define LUCKY_NUMBER 2
#define P1_READ 0
#define P1_WRITE 1
#define P2_READ 2
#define P2_WRITE 3

void parse(char *arg[], int data[3]){
    for (int i = 1;i<=6;i+=2){
        if (arg[i][1]=='m'){ //host id
            data[HOST_ID] = atoi(arg[i+1]);
        }
        else if (arg[i][1]=='d'){
            data[DEPTH] = atoi(arg[i+1]);
        }
        else if (arg[i][1]=='l'){
            data[LUCKY_NUMBER] = atoi(arg[i+1]);
        }
    }
}

void read_player(int fd, int *id, int *sum){
    char c=0;
    int flag = 0;
    *id=0;
    *sum=0;
    while(1){
        read(fd,&c,1);
        if (c=='\n') break;
        if (!flag){
            if (c==' '){
                flag=1;
                continue;
            }
            (*id)=(*id)*10+c-'0';
        }
        else{
            *sum = (*sum)*10+c-'0';
        }
    }
}

void read_node(int fd, int *res){
    *res=0;
    char c =0;
    while(1){
        read(fd,&c,1);
        if (c=='\n') break;
        (*res) = (*res)*10+c-'0';
    }
}

void split_id(int *arr, char *b1, char *b2, int len){
    sprintf(b1, "%d", arr[0]);
    for (int i = 1;i<len/2;i++){
        sprintf(b1, "%s %d", b1, arr[i]);
    }
    //sprintf(b1, "%s\n", b1);
    //printf("%s", b1);
    //fflush(stdout);
    sprintf(b2, "%d", arr[len/2]);
    for (int i = len/2+1;i<len;i++){
        sprintf(b2, "%s %d", b2, arr[i]);
    }
    //sprintf(b2, "%s\n", b2);
}

void read_id(int fd, char *buf){
    int id[8]={0}, count =0, flag =0;
    char b=0;
    //buf[0]='\0';
    while (1){
        read(fd, &b, 1);
        if (b=='\n') break;
        if (b=='-'){
            flag = 1;
            //len++;
            continue;
        }
        if (b==' '){
            if (flag==1) id[count] = -id[count];
            flag =0;
            count++;
            continue;
        }
        
        id[count] = id[count]*10+b-'0';
    }
    count ++;
    sprintf(buf, "%d", id[0]);
    for (int i = 1;i<count;i++){
        sprintf(buf, "%s %d", buf, id[i]);
    }
    sprintf(buf, "%s\n", buf);
    /*for (int i = 0;i<count/2;i++){
        sprintf(buf1, "%s %d", buf1, id[i]);
    }
    sprintf(buf1, "%s\n", buf1);
    for (int i = count/2;i<count;i++){
        sprintf(buf2, "%s %d", buf2, id[i]);
    }
    sprintf(buf2, "%s\n", buf2);*/

}

void buf_to_int(char *buf, int *arr, int *arrlen){
    int len = 0, count=0, flag = 0;
    for (int i =0 ;i<8;i++){
        arr[i]=0;
    }
    while(len<strlen(buf)){
        if (buf[len]==' '){
            if (flag == 1){
                arr[count] = -arr[count];
            }
            len++;
            count++;
            flag = 0;
            continue;
        }
        if (buf[len]=='-'){
            flag = 1;
            len++;
            continue;
        }
        if (buf[len]=='\n'){
            if (flag){
                arr[count]=-arr[count];
            }
            break;
        }
        arr[count] = arr[count]*10+buf[len]-'0';
        len++;
    }
    count++;
    *arrlen=count;
}

int main(int argc, char *argv[]){
    //int host_id, depth, lucky_number;
    int data[3];
    int id_list[8] = {1,2,3,4,5,6,7,8};
    parse(argv, data);
    const char *file="host", *buf;
    int readfd[2][2], writefd[2][2]; //childread/write
    int child1,child2;
    for (int i = 0;i<2;i++){
        pipe(readfd[i]);
        pipe(writefd[i]);
    }
    int root_read, root_write;
    if (data[DEPTH]==0){ // root, prepare fifo
        char buf[512];
        sprintf(buf, "fifo_%d.tmp", data[HOST_ID]);
        root_read = open(buf, O_RDONLY);
        //printf("%s", buf);
        //fflush(stdout);
        root_write = open("fifo_0.tmp", O_WRONLY);
        dup2(root_read, STDIN_FILENO);
        //dup2(STDERR_FILENO, STDOUT_FILENO);
        dup2(root_write, STDOUT_FILENO);
    }
    if (data[DEPTH]!=2){ //nonleaf
        if ((child1=fork())==0){
            dup2(readfd[0][0], STDIN_FILENO);
            dup2(writefd[0][1], STDOUT_FILENO);
            char hid[512], ln[512], dep[512];
            sprintf(hid, "%d", data[HOST_ID]);
            sprintf(ln, "%d", data[LUCKY_NUMBER]);
            sprintf(dep, "%d", data[DEPTH]+1);
            execl("./host", "host", "-m", hid, "-d", dep, "-l", ln, (char*)0);
            
        }
        else{
            if ((child2 = fork())==0){
                dup2(readfd[1][0], STDIN_FILENO);
                dup2(writefd[1][1], STDOUT_FILENO);
                char hid[512], ln[512], dep[512];
                sprintf(hid, "%d", data[HOST_ID]);
                sprintf(ln, "%d", data[LUCKY_NUMBER]);
                sprintf(dep, "%d", data[DEPTH]+1);
                execl("./host", "host", "-m", hid, "-d", dep, "-l", ln, (char*)0);
                //execl("./host", "host", "-m", data[HOST_ID], "-d", data[DEPTH]+1, "-l", data[LUCKY_NUMBER], (char*)0);
            }
            else{
            }
        }
    }
    else{ //leaf
        
    }
    while (1){
        if (data[DEPTH]!=2){
            if (child1==0){
                //printf("ch1 %d\n", data[DEPTH]+1);
                //fflush(stdout);
                /*char hid[512], ln[512], dep[512];
                sprintf(hid, "%d", data[HOST_ID]);
                sprintf(ln, "%d", data[LUCKY_NUMBER]);
                sprintf(dep, "%d", data[DEPTH]+1);
                execl("./host", "host", "-m", hid, "-d", dep, "-l", ln, (char*)0);
                */
                //char idbuf[512];
                //read(readfd[0], 512)
                //read_id(readfd[0][0], idbuf);
                //split_id()
                //write(writefd[0][1], idbuf, strlen(idbuf));
                
            }
            else if (child2==0){
                //printf("ch2 %d\n", data[DEPTH]+1);
                //fflush(stdout);
                /*char hid[512], ln[512], dep[512];
                sprintf(hid, "%d", data[HOST_ID]);
                sprintf(ln, "%d", data[LUCKY_NUMBER]);
                sprintf(dep, "%d", data[DEPTH]+1);
                execl("./host", "host", "-m", hid, "-d", dep, "-l", ln, (char*)0);
                */
                //execl("./host", "host", "-m", data[HOST_ID], "-d", data[DEPTH]+1, "-l", data[LUCKY_NUMBER], (char*)0);
            }
            else{ //parent
                char b1[512], b2[512];
                char input[512];
                if (data[DEPTH]==0){
                    //fprintf(stderr, "reading\n");
                //fflush(stdout);
                }
                
                read_id(STDIN_FILENO, input);
                int arr[8], arrlen=0;
                buf_to_int(input, arr,&arrlen);
                if (data[DEPTH]==0){
                    //fprintf(stderr, "%d ", arrlen);
                //fflush(stdout);
                for (int i = 0;i<arrlen;i++){
                    //fprintf(stderr, "%d,", arr[i]);
                }
                //fflush(stdout);
                }
            
                split_id(arr, b1, b2, arrlen);
                //printf("%s, %s", b1, b2);
                //fflush(stdout);
                sprintf(b1, "%s\n", b1);
                sprintf(b2, "%s\n", b2);
                write(readfd[0][1], b1, strlen(b1));
                write(readfd[1][1], b2, strlen(b2));
                if (arr[0]==-1){
                    waitpid(child1, NULL, 0);
                    waitpid(child2, NULL, 0);
                    /*if (data[DEPTH]==0){
                        break;
                    }*/
                    exit(0);
                }
                int score[2][8];
                if (data[DEPTH]==0){
                    for (int i = 0;i<8;i++){
                        score[1][i]=0;
                        score[0][i]=arr[i];
                    }
                }
                
                for (int i = 0;i<10;i++){
                    int win[2], gus[2];
                    char result[512];
                    read_player(writefd[1][0], &win[1], &gus[1]);
                    read_player(writefd[0][0], &win[0], &gus[0]);
                    //printf("%d,%d,%d,%d\n", win[0], gus[0], win[1], gus[1]);
                    //fflush(stdout);
                    if (data[DEPTH]==0){
                        if (abs(data[LUCKY_NUMBER]-gus[0])<=abs(data[LUCKY_NUMBER]-gus[1])){
                            for (int j = 0;j<8;j++){
                                if (arr[j]==win[0]){
                                    score[1][j]+=10;
                                }
                            }
                            //sprintf(result, "%d %d\n", win[0], gus[0]);
                            //printf("%s", result);
                            //fflush(stdout);
                            //write(STDOUT_FILENO, result, strlen(result));
                        }
                        else{
                            for (int j = 0;j<8;j++){
                                if (arr[j]==win[1]){
                                    score[1][j]+=10;
                                }
                            }
                            //sprintf(result, "%d %d\n", win[1], gus[1]);
                            //printf("%s", result);
                            //fflush(stdout);
                            //write(STDOUT_FILENO, result, strlen(result));
                        }
                    }
                    else{
                        if (abs(data[LUCKY_NUMBER]-gus[0])<=abs(data[LUCKY_NUMBER]-gus[1])){
                            sprintf(result, "%d %d\n", win[0], gus[0]);
                            //printf("%s", result);
                            //fflush(stdout);
                            write(STDOUT_FILENO, result, strlen(result));
                        }
                        else{
                            sprintf(result, "%d %d\n", win[1], gus[1]);
                            //printf("%s", result);
                            //fflush(stdout);
                            write(STDOUT_FILENO, result, strlen(result));
                        }
                    }
                    
                }
                if (data[DEPTH]==0){
                    char atom[1024];
                    sprintf(atom, "%d\n", data[HOST_ID]);
                    //printf("%d\n", data[HOST_ID]);
                    for (int i = 0;i<8;i++){
                        sprintf(atom, "%s%d %d\n",atom, score[0][i], score[1][i]);
                    }
                    //fflush(stdout);
                    printf("%s", atom);
                    fflush(stdout);
                }
                //waitpid(child1, NULL, 0);
                //waitpid(child2, NULL, 0);
            }
        }
        else{ //leaf
            int pid1, pid2;
            int fds[2][2];
            pipe(fds[0]);
            pipe(fds[1]);
            char buf[512], b1[512], b2[512];
            read_id(STDIN_FILENO, buf);
            int len=0, ids[8]={0}, count =0;
            buf_to_int(buf, ids, &count);
            if (ids[0]==-1){
                exit(0);
            }
            split_id(ids,b1,b2,count);
            //printf("id1:%s, id2:%s\n", b1, b2);
            //fflush(stdout);
            if ((pid1=fork())==0){
                dup2(fds[0][1], STDOUT_FILENO);
                //execl("./player", "player", "-n", "3", (char*)0);
            }
            else{
                if ((pid2=fork())==0){
                    dup2(fds[1][1], STDOUT_FILENO);
                    //execl("./player", "player", "-n", "4", (char*)0);
                }
                else{
                    //dup2(fds[0][0], STDIN_FILENO);
                    /*
                    for (int i = 0;i<10;i++){
                        int id[2], sum[2];
                        read_player(fds[0][0], &id[0], &sum[0]);
                        read_player(fds[1][0], &id[1], &sum[1]);
                        printf("%d %d %d\n",i,id[0], sum[0]);
                        printf("%d %d %d\n",i,id[1], sum[1]);
                        char buf[64];
                        if (sum[0]>sum[1]){
                            sprintf(buf, "%d %d\n" ,id[0], sum[0]);
                            write(STDOUT_FILENO, buf, strlen(buf));
                        }
                        else{
                            sprintf(buf, "%d %d\n", id[1], sum[1]);
                            write(STDOUT_FILENO, buf, strlen(buf));
                        }
                    }*/
                }
                //waitpid(pid1, NULL, 0);
                //waitpid(pid2, NULL, 0);
                
            }
            if (pid1 == 0){
                execl("./player", "player", "-n", b1, (char*)0);
            }
            else if (pid2 == 0){
                execl("./player", "player", "-n", b2, (char*)0);
            }
            else{
                int win[2], gus[2];
                for (int i = 0;i<10;i++){
                    read_player(fds[0][0], &win[0], &gus[0]);
                    read_player(fds[1][0], &win[1], &gus[1]);
                    char result[512];
                    //if ()
                    if (abs(data[LUCKY_NUMBER]-gus[0])<=abs(data[LUCKY_NUMBER]-gus[1])){
                        sprintf(result, "%d %d\n", win[0], gus[0]);
                        //printf("%s", result);
                        //fflush(stdout);
                        write(STDOUT_FILENO, result, strlen(result));
                    }
                    else{
                        sprintf(result, "%d %d\n", win[1], gus[1]);
                        //printf("%s", result);
                        //fflush(stdout);
                        write(STDOUT_FILENO, result, strlen(result));
                    }
                }
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);

            }
        }
    }   
    //sleep(100);
    
}