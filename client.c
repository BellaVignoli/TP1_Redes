#include "common.h"

int clientBoard[4][4];

void hiddenBoard(){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            clientBoard[i][j] = -2;
        }
    }
}

int inputCommand(char *cmd){
    char *command = strtok(cmd, "");
    if(strcmp(command, "start") == 0){
        return 0; //START
    }else if(strcmp(command, "reveal") == 0){
        return 1; //REVEAL
    }else if(strcmp(command, "flag") == 0){
        return 2;
    }else if(strcmp(command, "remove_flag") == 0){
        return 4; //REMOVE FLAG
    }else if(strcmp(command, "reset") == 0){
        return 5;
    }else if(strcmp(command, "exit") == 0){
        return 7;
    }else{
        return -1; //ERRO
    }
}

bool verified(struct action input){
    if((input.coordinates[0] >= 0 && input.coordinates[0] < 4) && (input.coordinates[1] >= 0 && input.coordinates[1] < 4)){
        return true;
    }
    //errorHandler("error: invalid cell");
    return false;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(1);
    }
    hiddenBoard();
    
    struct sockaddr_storage storage;
    if(addrparse(argv[1], argv[2], &storage) != 0){
        logexit("addrparse");
    }

    // Inicializar Socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Inicializar Conexao
    struct sockaddr *addr = (struct sockaddr *)(&storage); 
    if(connect(s, addr, sizeof(storage)) != 0){
        logexit("connect");
    }

    char cmd[1024];
    // bool status = false;

    while(1){
        scanf("%s", cmd);
        int command = inputCommand(cmd);
        int coordinates[2];
        bool free = true;
        struct action request;

        switch(command){
        case 0: //START
            // status = true;
            request = nextAction(0, coordinates, clientBoard);
            break;
        case 1: //REVEAL
            scanf("%d,%d", &coordinates[0], &coordinates[1]);
            request = nextAction(0, coordinates, clientBoard);
            if(!verified(request)){
                free = false;
                request = nextAction(-5, coordinates, clientBoard); 
            }else if(clientBoard[coordinates[0]][coordinates[1]] != -2){ //ja foi revelado
                errorHandler("error: cell already revealed");
                free = false;
                request = nextAction(-5, coordinates, clientBoard);
            }else{
                request = nextAction(1, coordinates, clientBoard); //REVEAL
            }
            break;
        case 2: //FLAG
            scanf("%d,%d", &coordinates[0], &coordinates[1]);
            if(clientBoard[coordinates[0]][coordinates[1]] == -3){ //FLAGGED
                errorHandler("error: cell already has a flag");
                free = false;
            }else if(clientBoard[coordinates[0]][coordinates[1]] != -2){

                errorHandler("error: cell already revealed");
                free = false;
            }else{
                request = nextAction(2, coordinates, clientBoard);
            }
            break;
        case 4: //REMOVE FLAG
            scanf("%d,%d", &coordinates[0], &coordinates[1]);
            if(!verified(request)){
                free = false;
            }else{
                request = nextAction(4, coordinates, clientBoard);
            }
            break;
        case 5: //RESET
            request = nextAction(5, coordinates, clientBoard);
            free = true;
            break;
        case 7: //EXIT
            request = nextAction(7, coordinates, clientBoard);
            free = true;
            break;
        case -1: //ERROR
            errorHandler("Usage: ./server <ipVersion> <port> -i <inputFilePath>");
            request = nextAction(-1, coordinates, clientBoard);
            break;
        default:
            break;
        }

        if(free){
            int count = send(s, &request, sizeof(request), 0);
            if(command == 7){ //EXIT
                close(s);
                break;
            }
            if(count != sizeof(request)){
                logexit("send");
            }
            struct action result;
            count = recv(s, &result, sizeof(result), 0);
            if(result.type == 8) { //GAMEOVER
                printf("GAME OVER\n");
                // status = false; //ENDGAME
            } else if (result.type == 6) { //WIN
                printf("YOU WIN!\n");
                // status = false;
            }
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    clientBoard[i][j] =  result.board[i][j];
                }
            }
            viewBoard(result.board);
        }
    }

}