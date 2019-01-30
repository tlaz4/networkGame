#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>


/* ---------------------------------------------------------------------
   Client program that works with server program
   run as:
   ./gameclient379 <ip address> <port>
   --------------------------------------------------------------------- */

bool moveUp(int *y);
bool moveDown(int *y, int *n);
bool moveLeft(int *x);
bool moveRight(int *x, int *n);
void * serverListener(void * arg);
int getPlayerPID(int playerMask);
char getPlayerDirection(int playerMask);
bool playerFiring(int playerMask);
void drawBoardEdge(void);
void signalHandler(int sig);
void gracefulExit();

int size;
WINDOW *win;
struct sigaction act;
int s;

typedef struct {
	int sock;	
	int *board;
	int myPID;
	
} board_t;


int main(int argc, char * argv[])
{
	int number;
	int MY_PORT;
	char message[1];    
	struct	sockaddr_in	server;
 	board_t boardS;
	struct	hostent		*host;


	MY_PORT = atoi(argv[2]);

	act.sa_handler = signalHandler;
	sigaction(SIGINT, &act, NULL);

	host = gethostbyname (argv[1]);

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}


	s = socket (AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror ("Client: cannot open socket");
		exit (1);
	}

	bzero (&server, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(host->h_name);
	server.sin_port = htons (MY_PORT);

	if (connect(s, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server");
		exit (1);
	}
	
	int init[4];
	int ca[1] = {1}; 
  	int *board;
	FILE * test_file; 
	recv(s,init,sizeof(init),0);
	size = init[1];
	boardS.myPID = init[0];
	board = (int*)malloc(size * size * sizeof(int));
	boardS.board = board;
	boardS.sock = s;
	recv(s, board, sizeof(int) * size * size, 0);
        

        int ch;
	int bwidth = init[2];
	int bheight = init[3];
        int x = 0; 
        int y = 0;

        win = newwin(bwidth, bheight, 0, 0);
        int updateNeeded = false;	// flag indicating if update is needed    -----------------------------------------
        int playerMovement = 0;
        int TESTING_playerMovedFlag = 0;
        char playerMovementChar;    


        initscr();			// must be done first
        curs_set(0);		// turns off visible cursor on window
        noecho();
        keypad(stdscr, TRUE); 	// get special keys (arrow keys)
	int nd = nodelay(stdscr, TRUE); 
	drawBoardEdge();

	// create server listening thread
	pthread_t threadID;
	pthread_create(&threadID, NULL, serverListener, (void *) &boardS);

	while (1) {
	
	    if ((ch = getch()) == ERR) {
	        ch = 'z';
	    }

  	    switch (ch) {
	        case('i'):
		    playerMovementChar = 'i';
		    TESTING_playerMovedFlag = 1;
		    break;
	
	        case('k'):
		    playerMovementChar = 'k';
		    TESTING_playerMovedFlag = 1;
		    break;

	        case('j'):
		    playerMovementChar = 'j';
		    TESTING_playerMovedFlag = 1;
		    break;

	        case('l'):
		    playerMovementChar = 'l';
		    TESTING_playerMovedFlag = 1;
		    break;
		case(' '):
		    playerMovementChar = ' ';
		    TESTING_playerMovedFlag = 1;
		    break;
		case('x'):
		    playerMovementChar = 'x';
		    TESTING_playerMovedFlag = 1;
		    break;
	    }
	    
	    if (TESTING_playerMovedFlag == 1) {
		send(s, &playerMovementChar, sizeof(playerMovementChar), 0); // send to server
		TESTING_playerMovedFlag = 0;
	    }
	}
}



bool moveUp(int *y) {
    bool value = false;
    if (*y  > 0) {
	*y -= 1;
	value = true;
    }  
    return value;
}


// returns true if movement was valid, false otherwise
// indicates positional update needed)
bool moveDown(int *y, int *n) {
    bool value = false;
    if (*y < (*n-1)) {
	*y += 1;
	value = true;
    }  
    return value;
}


// returns true if movement was valid, false otherwise
// indicates positional update needed)
bool moveLeft(int *x) {
    bool value = false;
    if (*x > 0) {
	*x -= 1;
	value = true;
    }  
    return value;
}

// returns true if movement was valid, false otherwise
// indicates positional update needed)
bool moveRight(int *x, int *n) {
    bool value = false;
    if (*x < (*n-1)) {
	*x += 1;
	value = true;
    }  
    return value;
}


// draws a board
void drawBoard(board_t *boardS) {
    int valueAtBoardSpot; 

    // 0000 0000  0000 0000  0000 0000 0000 0000
    // fddd dddd  dppp pppp  pppp pppp pppp pppp
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {			
	    valueAtBoardSpot = *(boardS->board + row * size + col);
	    move(row + 1, col + 1);

 	    // if there is a player in this spot and that player is me
	    if ((valueAtBoardSpot != 0) && ((boardS->myPID) == getPlayerPID(valueAtBoardSpot))) {
		addch(getPlayerDirection(valueAtBoardSpot) | A_BOLD);

	    // if there is something  in this spot BUT its not me is not me
	    } else if ((valueAtBoardSpot != 0) && ((boardS->myPID) != getPlayerPID(valueAtBoardSpot))) {
		if (valueAtBoardSpot == -1) {
		    addch('o');
	        } else {
		    addch(getPlayerDirection(valueAtBoardSpot));
		}
	    } 
	    // this spot is empty
	    else {
		addch(' ');
	    }



	}
    }        
    wrefresh(win);

}


void * serverListener(void * arg) {
    board_t * boardS = (board_t*) arg;
    char intervalChars[2] = "/\\";   
    int interval = 0;
    char buffer[20];
    while (1) {
        int ret = recv(boardS->sock,boardS->board,sizeof(int) * size * size, 0);
	if(ret == 0){
		return NULL;
	}

	if (*(boardS->board) == -2) {
	    int score = *(boardS->board + 1);
	    close(s);
	    sprintf(buffer, "%d", score);
	    mvprintw(size + 2, 0, "GAME OVER: you killed %s people", buffer);
	    
	    sleep(2);
	    gracefulExit();

 	}
	move(size + 2, 0);
	addch(intervalChars[interval % 2]);
	drawBoard(boardS);
	interval++;
   }
}


int getPlayerPID(int playerMask) {
    int pid = playerMask << 9;
    pid = pid >> 9;
    return pid;
}

char getPlayerDirection(int playerMask) {

    int charMask = 255 << 24; 
    char playerDirectionChar = (playerMask & charMask) >> 24;
    switch (playerDirectionChar) {
	case('i'):
	    playerDirectionChar = '^';
            break;
	case('k'):
	    playerDirectionChar = 'V';
	    break;
	case('j'):
	    playerDirectionChar = '<';
	    break;
	case('l'):
	    playerDirectionChar = '>';
	    break;
    }

    return playerDirectionChar;
}

bool playerFiring(int playerMask) {

    bool firing = false;
    int fireMask = 1 << 31;
    
    if ((playerMask & fireMask) != 0) {
        firing = true;
    }

    return firing;

}



void drawBoardEdge() {
   

    // draw top
    for (int xtop = 0; xtop < size + 2; xtop++) {
        move(0, xtop);
	addch('X');
    }

    // draw right side
    for (int yright = 0; yright < size + 2; yright++) {
        move(yright, size + 1);
	addch('X');
    }

    // draw bottom
    for (int xbot = 0; xbot < size + 2; xbot++) {
	move(size + 1, xbot);
	addch('X');
    }

    // draw left side
    for (int yleft = 0; yleft < size + 2; yleft++) {
	move(yleft, 0);
	addch('X');
    }
     
}

void gracefulExit() {

    // close ncurses window
    endwin();
    // close socket
    close(s);
    exit(EXIT_SUCCESS);
}

void signalHandler(int sig){

    switch(sig){
	
        case SIGINT:
	    gracefulExit();
            break;
            
    }
}
























// anchor
