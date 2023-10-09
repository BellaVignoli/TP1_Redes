#include "common.h"

int clientBoard[4][4]; //tabuleiro que aparece para o cliente
int currentBoard[4][4]; //tabuleiro answer key
struct action request;
struct action feedback;

void reset(){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            clientBoard[i][j] = -2;
        }
    }
}

bool win(struct action request){
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            if((currentBoard[i][j] != clientBoard[i][j] && currentBoard[i][j] != -1)){ //não tem bomba
                return false;
            }
        }
    }
    return true;
}

int newStatus(struct action request){
    int cellUpdated = currentBoard[request.coordinates[0]][request.coordinates[1]];
    if(cellUpdated == -1){ //se for uma bomba
        return 8; //GAMEOVER
    }else if(win(request)){
        return 6; //WIN
    }
    return 3;
}

struct action reveal(struct action request, struct action feedback){
    struct action action;
    switch(newStatus(request)){
        case 6: //WIN
            reset();
            action = nextMove(6, request.coordinates, currentBoard);
            break;
        case 8: //GAMEOVER
            reset();
            action = nextMove(8, request.coordinates, currentBoard);
            break;
        case 3: //STATE
            action = nextMove(3, request.coordinates, clientBoard);
            break;
    }

    return action;
}

struct action type(struct action request){
    struct action response;
    switch (request.type)
    {
    //START
    case 0:
        reset();
        int coordinates[2] = {0,0};
        response = nextMove(0, coordinates, clientBoard);
        break;

    //REVEAL
    case 1:
        //reveal position
        clientBoard[request.coordinates[0]][request.coordinates[1]] = currentBoard[request.coordinates[0]][request.coordinates[1]];
        response = reveal(request, feedback);
        break;

    //FLAG
    case 2:
        clientBoard[request.coordinates[0]][request.coordinates[1]] = -3; //FLAG
        response = nextMove(2, request.coordinates, clientBoard);
        break;

    //REMOVE_FLAG
    case 4:
        clientBoard[request.coordinates[0]][request.coordinates[1]] = -2;
        response = nextMove(4, request.coordinates, clientBoard);
        break;

    //RESET
    case 5:
        reset();
        printf("starting new game\n");
        response = nextMove(5, request.coordinates, clientBoard);
        break;

    //EXIT
    case 7:
        reset();
        printf("client disconnected\n");
        break;

    default:
        break;
    }

    return response;
}

int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[3], "-i") != 0) {
        errorHandler("Usage: ./server <ipVersion> <port> -i <inputFilePath>");
        exit(1);
    }
    // Leitura do arquivo de entrada
    FILE *file;
    file = fopen(argv[4], "r");
    if (file == NULL) {
        exit(1);
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            fscanf(file, "%d,", &currentBoard[i][j]);
        }
    }
    fclose(file);

    viewBoard(currentBoard);

    struct sockaddr_storage storage;
    if(server_sockaddr_init(argv[1], argv[2], &storage)){
        logexit("server_sockaddr_init");
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    // Listen operation (allows incoming connections)
    if(listen(s, 10) != 0){
        logexit("listen");
    }


    while(true) {

        // Accepts an incoming client connection
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) &cstorage;
        socklen_t caddrlen = sizeof(cstorage);


        int csock = accept(s, caddr, &caddrlen);
        printf("client connected\n");

        if(csock == -1){
            logexit("accept");
        }


        // Computes client's request
        while(true){
            //struct action request;
            // Receives client's command
            int count = recv(csock, &request, sizeof(request), 0);

            if(count == 0){
                break;
            } else if(count == -1){
                logexit("recv");
            }

            struct action feedback;
            feedback = type(request);

            count = send(csock, &feedback, sizeof(feedback), 0);
            if(count != sizeof(feedback)){
                logexit("send");
                break;
            }
        }
        close(csock);
    }
    return 0;
}