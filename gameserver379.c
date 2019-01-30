#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>


/* ---------------------------------------------------------------------
   This	is  a complex server
*/

// global size
int size;
int serverUpdate;
int id;

typedef struct List list_t;

// struct to store player info
typedef struct{
	int sockfd;
	int pid;
	int x;
	int y;
	char lastCmd;
	char facing;
	int *gameBoard;
	int score;
	list_t *list;
} player_t;

// struct to store playerlist + maybe do some dynamic work
struct List{
	int size;
	player_t *players[100];
	int *gameBoard;
	int left;
};


// a packet to contain data to send to client
// struct that will contain an array of packets


//player_t *players[10*10];

void * handleClient(void * arg);
void * harvestAndAct(void *arg);
void connectClient(player_t *player, int *gameBoard);
int randomInt(int n);
void handleDisconnect(player_t *thisPlayer, list_t *list, int pid);
void clearGameBoard(int *gameBoard);
void printGameBoard(int *gameBoard);
int checkOccupied(int *gameBoard, int x, int y);
void broadCastState(player_t *player, int *gameBoard);
void movePlayer(player_t *player, int *gameBoard);
int maskPacket(int pid, char facing);
void handleFire(player_t *thisPlayer, int *gameBoard, list_t *list);	

int main(int argc, char * argv[]){
	//skeleton_daemon ();
	int sock, snew;

	struct	sockaddr_in server, client;

	// init id and board size
	size = atoi(argv[1]);
	serverUpdate = atoi(argv[2]);
	id = 1;
	int port = atoi(argv[3]);
	int seed = atoi(argv[4]);
	// create gameboard for logic
	int *gameBoard = (int*)malloc(size * size * sizeof(int)); 

	// initialize list wrapper to store players
	list_t list;
	list.size = 0;
	list.left = 0; 
	list.gameBoard = gameBoard;

	// allocate space in lists players array
	for(int i = 0; i < 100; i++){
		list.players[i] = malloc(sizeof(player_t));
	}

	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(port);

	if (bind(sock, (struct sockaddr*) &server, sizeof(server))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}

	listen(sock, 5);

	//printf("Server started\n");

	int clientLength = sizeof(client);

	// random seed
	srand(seed);

	// create thread that will serve all clients
	pthread_t threadID2;
	pthread_create(&threadID2, NULL, harvestAndAct, (void *) &list);

	while(1){
		snew = accept (sock, (struct sockaddr*) &client, &clientLength);
		if (snew < 0) {
			perror ("Server: accept failed");
			exit (1);
		}

		player_t *player = (player_t*)malloc(sizeof(player_t));
		//list.players[id]->sockfd = snew;
		//list.players[id]->pid = id;
		//printf("List size: %d\n", list.size);

		player->sockfd = snew;
		player->pid = id;
		player->facing = 'i';
		player->gameBoard = gameBoard;
		player->score = 0;
		player->list = &list;
		//player->list = &list;

		//printf("List size: %d\n", list.size);

		connectClient(player, gameBoard);
		list.players[id - 1] = player;
//		printGameBoard(gameBoard);

		pthread_t threadID;
		pthread_create(&threadID,NULL, handleClient, (void *) player);

		if(list.left == 0){
			list.size++;
		}else{
			list.left = 0;
		}
		id = list.size + 1;

		sleep(1);
	}
	close(sock);
	
}

void broadcastState(player_t *player, int *gameBoard){
	send(player->sockfd, gameBoard, sizeof(int) * size * size, 0);
}

void connectClient(player_t *player, int *gameBoard){
	int init[4];	

	init[0] = player->pid;
	init[1] = size;

	// make sure spot isnt occupied
	// keep generating random numbers until unoccupied
	while(1){
		for(int i = 2; i < 4; i++){
			init[i] = randomInt(size);
		}

		if(checkOccupied(gameBoard, init[2], init[3]) != 0){
			player->x = init[2];
			player->y = init[3];
			break;
		}
	}

	// add player to gameboard
	*(gameBoard + player->y * size + player->x) = maskPacket(player->pid, player->facing);

	send(player->sockfd, init, sizeof(init), 0);
	broadcastState(player, gameBoard);
}

void clearGameBoard(int *gameBoard){
	for (int i = 0; i <  size; i++){
      		for (int j = 0; j < size; j++){
			if(*(gameBoard + i*size + j) == -1){
				//printf("detected a fire\n");
         			*(gameBoard + i*size + j) = 0;
			}
		}
	}
//	printf("\n\n");
}

void printGameBoard(int *gameBoard){
        for (int i = 0; i <  size; i++){
		//printf("\n");
                for (int j = 0; j < size; j++){
                    //printf("%d", *(gameBoard + i*size + j));
                }
        }
}




int checkOccupied(int *gameBoard, int x, int y){
	if(*(gameBoard + y * size + x) == 0){
		return 1;
	}else{
		return 0;
	}
}

// needs seed
int randomInt(int n){
	return rand()%n;
}

void handleDisconnect(player_t *thisPlayer, list_t *list, int pid){
	*(thisPlayer->gameBoard + thisPlayer->y * size + thisPlayer->x) = 0;

	close(thisPlayer->sockfd);
	
	player_t *newPlayer = (player_t*)malloc(sizeof(player_t));
	list->players[pid - 1] = newPlayer;
	list->left = 1;
	id = pid;
}

void sendScore(player_t *thisPlayer){
	int exitCodeAndScore[2];
	//int test = -2;
	//char response[1]; 
	//exitCodeAndScore[-2, thisPlayer->score];
	exitCodeAndScore[0] = -2;
	exitCodeAndScore[1] = thisPlayer->score;
	//send(thisPlayer->sockfd, &thisPlayer->score, sizeof(int), 0);
	//printf("sent score = %d\n", exitCodeAndScore[1]);
	send(thisPlayer->sockfd, exitCodeAndScore, sizeof(int) * 2, 0);
	//recv(thisPlayer->sockfd, response, 1, 0);
	//if (response[0] == 'q') {
	//    printf("player quit (x)");
	//}
}

void * handleClient(void * arg){
	int ret;
	player_t *thisPlayer = arg;
	//printf("Player %d has joined\n", thisPlayer->pid);
 
	char data[1];
	while(1){
		ret = recv(thisPlayer->sockfd, data, 1, 0);
		if(ret == 0){
			handleDisconnect(thisPlayer, thisPlayer->list, thisPlayer->pid);
			pthread_exit(NULL);
			//printf("disconnect");
		}
		//printf("Data from client: %c\n", data[0]);
		thisPlayer->lastCmd = data[0]; 
		//players[thisPlayer->pid] = thisPlayer;
		//sleep(1);
		//pthread_exit(NULL);
	}
}



void * harvestAndAct(void * arg){
	while(1){
		sleep(serverUpdate);
		list_t *list = (list_t*)arg;
		//printGameBoard(list->gameBoard);
		//clearGameBoard(list->gameBoard);
		// init killList
		list_t killList;
		killList.size = 0;
		for(int i = 0; i < 100; i++){
			killList.players[i] = malloc(sizeof(player_t));
		}

		
		for(int i = 0; i < list->size; i++){
			player_t *curPlayer = list->players[i];			
			char lastCmd = curPlayer->lastCmd;
			//printf("Harvest is: %c\n", curPlayer->lastCmd);
			if(lastCmd == ' '){
				killList.players[killList.size] = curPlayer;
				killList.size = killList.size + 1;	
			}else if(lastCmd == 'x'){
				sendScore(curPlayer);
				handleDisconnect(curPlayer, list, curPlayer->pid);
			}else{
				movePlayer(curPlayer, curPlayer->gameBoard);
			}
		}

		for(int i = 0; i < killList.size + 1; i++){
			player_t *firingPlayer = killList.players[i];
			handleFire(firingPlayer, firingPlayer->gameBoard, list);			
		}

		for(int i = 0; i <list->size; i++){
			player_t *curPlayer = list->players[i];
			//printGameBoard(curPlayer->gameBoard);
			broadcastState(curPlayer, curPlayer->gameBoard);
		}
		clearGameBoard(list->gameBoard);	
	} 
}

// player packet will be represented as an int where last bit indicates player has fired, and next 8 bits represent the direction
// mask these 9 bits out and the rest is the player id
int maskPacket(int pid, char facing){

	// if firing we only add a 1 to the last bit 
	if(facing == ' '){
		return pid | (1 << 31);

	// we have a change in direction, mask the character into the pid
	}else{
		return ((facing | 0)<<24) | pid;
	}

}

int getPlayerID(int mask){
	int pid = mask << 9;
	return pid >> 9;
}

void handleFire(player_t *thisPlayer, int *gameBoard, list_t *list){	
	//*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(*(gameBoard + thisPlayer->y * size + thisPlayer->x), ' ');
	switch(thisPlayer->facing){
		case('i'):
			for(int i = 1; i < 3; i++){
				if((thisPlayer->y - i) >= 0){
					int location = *(gameBoard + (thisPlayer->y - i) * size + thisPlayer->x);
					if(location != 0){
						thisPlayer->score = thisPlayer->score + 1;
						int pid = getPlayerID(location);
						sendScore(list->players[pid -1]);
						//printf("score %d\n", (list->players[pid -1])->score);
						handleDisconnect(list->players[pid -1], list, pid);
					}
					*(gameBoard + (thisPlayer->y - i) * size + thisPlayer->x) = -1;
				}
			}
	
			break;

		case('k'):	
			for(int i = 1; i < 3; i++){
				if((thisPlayer->y + i) <= size - 1){
					int location = *(gameBoard + (thisPlayer->y + i) * size + thisPlayer->x);
					if(location != 0){
						thisPlayer->score = thisPlayer->score + 1;
						int pid = getPlayerID(location);
						sendScore(list->players[pid -1]);
						handleDisconnect(list->players[pid - 1], list, pid);
					}
					*(gameBoard + (thisPlayer->y + i) * size + thisPlayer->x) = -1;
				}
			}
	
			break;

		case('l'):
			for(int i = 1; i < 3; i++){
				if((thisPlayer->x + i) <= size){
					int location = *(gameBoard + thisPlayer->y * size + (thisPlayer->x + i));
					if(location != 0){
						thisPlayer->score = thisPlayer->score + 1;
						int pid = getPlayerID(location);
						sendScore(list->players[pid -1]);
						handleDisconnect(list->players[pid - 1], list, pid);
					}
					*(gameBoard + (thisPlayer->y) * size + (thisPlayer->x + i)) = -1;
				}
			}
	
			break;

		case('j'):	
			for(int i = 1; i < 3; i++){
				if((thisPlayer->x - i) >= 0){
					int location = *(gameBoard + (thisPlayer->y) * size + (thisPlayer->x - i));
					if(location != 0){
						thisPlayer->score = thisPlayer->score + 1;
						int pid = getPlayerID(location);
						sendScore(list->players[pid -1]);
						handleDisconnect(list->players[pid - 1], list, pid);
					}
					*(gameBoard + (thisPlayer->y) * size + (thisPlayer->x - i)) = -1;
				}
			}
			break;
	}
	thisPlayer->lastCmd = 'z';
}

void movePlayer(player_t *thisPlayer, int *gameBoard){
	char thisCmd = thisPlayer->lastCmd;
	switch(thisCmd){
		case('i'):
			if((thisPlayer->y > 0) && (*(gameBoard + (thisPlayer->y - 1)*size + thisPlayer->x) == 0)){
				//printf("i triggered\n");
				thisPlayer->facing = thisCmd;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = 0;
				thisPlayer->y = thisPlayer->y - 1;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(thisPlayer->pid, thisCmd);
				
			}
			break;

		case('k'):	
			if((thisPlayer->y < size - 1) && (*(gameBoard + (thisPlayer->y + 1)*size + thisPlayer->x) == 0)){
				//printf("k triggered\n");
				thisPlayer->facing = thisCmd;			
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = 0;
				thisPlayer->y = thisPlayer->y + 1;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(thisPlayer->pid, thisCmd);
			}
			break;

		case('j'):			
			if((thisPlayer->x > 0) && (*(gameBoard + thisPlayer->y *size + (thisPlayer->x - 1)) == 0)){
				//printf("j triggered\n");
				thisPlayer->facing = thisCmd;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = 0;
				thisPlayer->x = thisPlayer->x - 1;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(thisPlayer->pid, thisCmd);
			}
			break;

		case('l'):
			if((thisPlayer->x < size - 1) && (*(gameBoard + thisPlayer->y*size + (thisPlayer->x + 1)) == 0)){
				//printf("l triggered\n");
				thisPlayer->facing = thisCmd;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = 0;
				thisPlayer->x = thisPlayer->x + 1;
				*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(thisPlayer->pid, thisCmd);
			}
			break;

		//case(' '):
			
			// update data by using players existing mask and telling client they fired
			//*(gameBoard + thisPlayer->y * size + thisPlayer->x) = maskPacket(*(gameBoard + thisPlayer->y * size + thisPlayer->x), ' ');
			//break;
	}	
	thisPlayer->lastCmd = 'z';	
}
