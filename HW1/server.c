#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
    bool status;
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

bool lck[21];
//FILE* rr;// = fopen("registerRecord", "r+");
//int file_fd;// = fileno(rr);

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

bool read_pref(request* req, int file_fd){
    char buf[256];
    int place = 10*(req->buf[4]-'0')+req->buf[5]-'0';
    if (req->buf[0]!='9' || req->buf[1]!='0' || req->buf[2]!='2' || req->buf[3]!='0' || place > 20 || place <= 0||req->buf_len!=6){
        sprintf(buf, "[Error] Operation failed. Please try again.\n");
        write(req->conn_fd, buf, strlen(buf));
        return false;
    }
    struct flock lock;
    lock.l_len = sizeof(registerRecord);
    lock.l_start = (place-1)*sizeof(registerRecord);
    lock.l_whence = SEEK_SET;
    lock.l_type = F_WRLCK;
    if (fcntl(file_fd, F_SETLK, &lock)<0 || lck[place]){
        sprintf(buf, "Locked.\n");
        write(req->conn_fd, buf, strlen(buf));
        return false;
    }
    lck[place]=true;
    //FILE *rr = fopen("registerRecord", "r");
    //int file_fd = fileno(rr);
    registerRecord readfile;
    lseek(file_fd, (place-1)*sizeof(registerRecord), SEEK_SET);
    //fread(&readfile, sizeof(registerRecord), 1, rr);
    read(file_fd, &readfile, sizeof(registerRecord));
    lseek(file_fd, 0, SEEK_SET);
    if (readfile.AZ==1){
        if (readfile.BNT==2){
            sprintf(buf, "Your preference order is AZ > BNT > Moderna.\n");
        }
        else{
            sprintf(buf, "Your preference order is AZ > Moderna > BNT.\n");
        }
    }
    else{
        if (readfile.BNT == 1){
            if (readfile.AZ==2){
                sprintf(buf, "Your preference order is BNT > AZ > Moderna.\n");
            }
            else{
                sprintf(buf, "Your preference order is BNT > Moderna > AZ.\n");
            }
        }
        else{
            if (readfile.AZ==2){
                sprintf(buf, "Your preference order is Moderna > AZ > BNT.\n");
            }
            else{
                sprintf(buf, "Your preference order is Moderna > BNT > AZ.\n");

            }

        }
    }
    //req.id = place+902000;//readfile.id;
    write(req->conn_fd, buf, strlen(buf));
    //sprintf(buf, "info %d, %d, %d, %d\n", readfile.id, readfile.AZ, readfile.BNT, readfile.Moderna);
    //write(req->conn_fd, buf, strlen(buf));
    req->id = readfile.id;
    //fclose(rr);
    //fprintf(stderr, "%d", req.id);
    //sprintf(buf, "%s : %s", accept_read_header, req.buf);
    //write(req.conn_fd, buf, strlen(buf));
    return true;
}

void write_pref(request req, int file_fd){
    char buf[256];
    int place = req.id%100;
    registerRecord readfile;
    lseek(file_fd, 0, SEEK_SET);
    lseek(file_fd, (place-1)*sizeof(registerRecord), SEEK_SET);
    readfile.AZ = req.buf[0]-'0';
    readfile.BNT = req.buf[2]-'0';
    readfile.Moderna = req.buf[4]-'0';
    if (readfile.AZ<=0 || readfile.AZ>3  || readfile.BNT<=0 || readfile.BNT>3 || readfile.Moderna<=0 || readfile.Moderna>3||req.buf_len!=5){
	sprintf(buf, "[Error] Operation failed. Please try again.\n");
	write(req.conn_fd, buf, strlen(buf));
	return;
    }
    if (readfile.AZ+readfile.BNT+readfile.Moderna!=6 || (readfile.AZ==2&&readfile.BNT==2&&readfile.Moderna==2)){
	sprintf(buf, "[Error] Operation failed. Please try again.\n");
	write(req.conn_fd, buf, strlen(buf));
	return;
    }
    readfile.id = req.id;
    lseek(file_fd, (place-1)*sizeof(registerRecord), SEEK_SET);
    write(file_fd, &readfile, sizeof(registerRecord));
    lseek(file_fd, 0, SEEK_SET);
    lseek(file_fd, (place-1)*sizeof(registerRecord), SEEK_SET);
    lseek(file_fd, 0, SEEK_SET);
    //sprintf(buf, "%d %d %d %d\n", readfile.id, readfile.AZ, readfile.BNT, readfile.Moderna);
    //write(req.conn_fd, buf, strlen(buf));
    //fclose(rr);
    //rr = fopen("registerRecord", "r+");
    //sprintf(buf, "buffer is %s, id is %d", req.buf, readfile.id);
    //write(req.conn_fd, buf, strlen(buf));
    if (readfile.AZ==1){
        if (readfile.BNT==2){
            sprintf(buf, "Preference order for %d modified successed, new preference order is AZ > BNT > Moderna.\n", readfile.id);
        }
        else{
            sprintf(buf, "Preference order for %d modified successed, new preference order is AZ > Moderna > BNT.\n", readfile.id);
        }
    }
    else{
        if (readfile.BNT == 1){
            if (readfile.AZ==2){
                sprintf(buf, "Preference order for %d modified successed, new preference order is BNT > AZ > Moderna.\n", readfile.id);
            }
            else{
                sprintf(buf, "Preference order for %d modified successed, new preference order is BNT > Moderna > AZ.\n", readfile.id);
            }
        }
        else{
            if (readfile.AZ==2){
                sprintf(buf, "Preference order for %d modified successed, new preference order is Moderna > AZ > BNT.\n", readfile.id);
            }
            else{
                sprintf(buf, "Preference order for %d modified successed, new preference order is Moderna > BNT > AZ.\n", readfile.id);
            }

        }
    }
    lck[place]=false;
    /*struct flock lock;
    lock.l_len = sizeof(registerRecord);
    lock.l_start = (place-1)*sizeof(registerRecord);
    lock.l_whence = SEEK_SET;
    lock.l_type = F_UNLCK;
    fcntl(file_fd, F_SETLK, &lock);*/
    write(req.conn_fd, buf, strlen(buf));
    //fclose(rr);
    fprintf(stderr, "%s", req.buf);
    //sprintf(buf, "%s : %s", accept_read_header, req.buf);
    //write(req.conn_fd, buf, strlen(buf));
    //close(cl);
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_len = sizeof(registerRecord);
    //lock.l_start = 0;
    for (int i = 0;i<21;i++){
        lck[i]=false;
    }
    int conn_fd;  // fd for a new connection with client
    //int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;
    //FILE* rr = fopen("registerRecord", "r+");
    int file_fd = open("registerRecord", O_RDWR);//fileno(rr);
    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    
    fd_set masterset;
    int socket_size, client[maxfd], activity, sd;
    int max_client = 20;
    /*
    int max_cl = 21, client[max_cl], master;
    //set of socket d.
    fd_set fds;
    for (int i = 0;i<max_cl;i++){
        client[i]=0;
    }
    if ((master = socket(AF_INET, SOCK_STREAM, 0))==0){
        //exit failure
    }
    //*/
    FD_ZERO(&masterset);
    FD_SET(svr.listen_fd, &masterset);
    ///*
    for (int i = 0;i<maxfd;i++){
        client[i]=0;
    }
    //*/
    socket_size = 0;
    while (1) {
        // TODO: Add IO multiplexing
        //clear
        FD_ZERO(&masterset);
        //add
        FD_SET(svr.listen_fd, &masterset);
        ///*
        socket_size = svr.listen_fd;
        for (int i = 0;i<maxfd;i++){
            sd = client[i];

            if (sd>0){
                FD_SET(sd, &masterset);
            }
            if (sd > socket_size){
                socket_size = sd;
            }
        }
        //*/
        //memcpy(&workingset, &masterset, sizeof(masterset));
        activity = select(socket_size+1, &masterset, NULL, NULL, NULL);

        if (activity<0 && errno!=EINTR){
            //error
            //printf("erreo");
        }

        int ret;
        if (FD_ISSET(svr.listen_fd, &masterset)){ // is new
            // Check new connection
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            requestP[conn_fd].conn_fd = conn_fd;
            requestP[conn_fd].status=false;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
            //FD_SET(conn_fd, &masterset);
            //socket_size++;
            for (int i =0 ;i<maxfd;i++){
                if (client[i]==0){
                    client[i]=conn_fd;
                    break;
                }
            }
            sprintf(buf,"Please enter your id (to check your preference order):\n");
            write(requestP[conn_fd].conn_fd, buf, strlen(buf));
            //ret = handle_read(&requestP[conn_fd]);
            //fprintf(stderr, "ret = %d\n", ret);
        
        }

        //fprintf(stderr, "%d ", socket_size);
        for (int i = 0;i<maxfd;i++){    
            //sprintf(buf,"Please enter your id (to check your preference order):");
            //write(requestP[client[i]].conn_fd, buf, strlen(buf));
            //int ret = handle_read(&requestP[client[i]]);
            if (client[i]!=0){
            if(FD_ISSET(client[i], &masterset)){
                //fprintf(stderr, "lol");
                int ret = handle_read(&requestP[client[i]]); // parse data from client to requestP[conn_fd].buf
                fprintf(stderr, "ret = %d\n", ret);
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[client[i]].host);
                    continue;
                }
        //write()
    // TODO: handle requests from clients
        //FILE *rr;
#ifdef READ_SERVER  

       read_pref(&requestP[client[i]], file_fd); 
       close(client[i]);    
       free_request(&requestP[client[i]]);
       client[i]=0;
#elif defined WRITE_SERVER
        int ide;
        if (requestP[client[i]].status == false){
            //read_pref(requestP[client[i]]);
            //requestP[client[i]].status = true;
            requestP[client[i]].status = read_pref(&requestP[client[i]], file_fd);
            //ide = read_pref(requestP[client[i]]);
            if (requestP[client[i]].status){
                /*lock.l_type = F_WRLCK;
                int place = 10*(requestP[client[i]].buf[4]-'0')+requestP[client[i]].buf[5]-'0';
                lock.l_whence = SEEK_SET;
                lock.l_len = sizeof(registerRecord);
                lock.l_start = (place-1)*sizeof(registerRecord);
                fcntl(client[i], F_SETLK, &lock);
                lck[requestP[client[i]].id-902000]=true;
                */
                sprintf(buf,"Please input your preference order respectively(AZ,BNT,Moderna):\n");
                write(requestP[client[i]].conn_fd, buf, strlen(buf));
            
            }
            else{
                close(client[i]);
                free_request(&requestP[client[i]]);
                client[i]=0;
            }
            
        }
        else{
            write_pref(requestP[client[i]], file_fd); 
            lck[requestP[client[i]].id-902000]=false;
            struct flock lock;
            lock.l_len = sizeof(registerRecord);
            lock.l_start = (requestP[client[i]].id-902001)*sizeof(registerRecord);
            lock.l_whence = SEEK_SET;
            lock.l_type = F_UNLCK;
            fcntl(file_fd, F_SETLK, &lock);
            //write(requestP[client[i]].conn_fd, buf, strlen(buf));
            close(client[i]);
            free_request(&requestP[client[i]]);
            client[i]=0;
        }
        
        //close(client[i]); 
        //
        //client[i]=0;
#endif
        //close(requestP[conn_fd].conn_fd);
        //free_request(&requestP[conn_fd]);
        //close(client[i]);
        //free_request(&requestP[client[i]]);
        //fprintf(stderr, "free once\n");
        //client[i]=0;
        }
        }}
        //fprintf(stderr, "%d ", socket_size);
    }
    free(requestP);
    //fclose(rr);
    close(file_fd);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
