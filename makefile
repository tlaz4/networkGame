all: gameserver379 gameclient379

gameserver379: gameserver379.c
	gcc -std=c99  -o gameserver379 gameserver379.c -lncurses -pthread
gameclient379: gameclient379.c
	gcc -std=c99 -D_GNU_SOURCE -o gameclient379 gameclient379.c -lncurses -pthread
clean: 
	rm gameserver379
	rm gameclient379
