#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int generate_guess(int id, int round){
    srand((id+round)*323);
    return rand()%1001;
}

int main(int argc, char *argv[]){
    int player_id = atoi(argv[2]);
    for (int i = 1;i<=10;i++){
        printf("%d %d\n", player_id, generate_guess(player_id, i));
        fflush(stdout);
    }
    exit(0);
}