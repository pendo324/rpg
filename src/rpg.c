#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <curses.h>
#include "rpg.h"
#include "wad.h"

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   BigShort (short l)
{
	return l;
}


int    LittleLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l)
{
	return l;
}


#else


short   BigShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   LittleShort (short l)
{
	return l;
}


int    BigLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l)
{
	return l;
}

#endif

#ifndef _WIN32
void strupr (char *s)
{
	while (*s) {
		*s = toupper(*s);
		s++;
	}
}
#endif

void Error (char *error, ...)
{
	va_list argptr;

	endwin(); // terminate the curses window before printing message
	
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	
	exit (1);
}

/*
============================================================================

					GAME CODE

============================================================================
*/

tile		*tiletypes;
tile		tiles[MAP_HEIGHT][MAP_WIDTH];

int		currentMap = 1;

int		numActors;
entity		*actors;
player_t	player;

/*
============
=
= initializeMap
=
============
*/

void initializeMap()
{
	int x, y, lump;
	char mapName[9];
	byte *map;

	sprintf(mapName, "MAP%02d", currentMap);
	lump = GetLumpNumForName(mapName);
	map = GetLumpData(lump);
	actors = (entity *) GetLumpData(lump+1);
	numActors = GetLumpLength(lump+1)/sizeof(entity);

	for(y = 0; y < MAP_HEIGHT; y++) {
		for(x = 0; x < MAP_WIDTH; x++) {
			if (tiletypes[*map].type == 1) {	// found a player start
				player.actor.x = x;
				player.actor.y = y;
			}
			tiles[y][x] = tiletypes[*map++];
		}
	}
}

/*
============
=
= mapSpecials
=
============
*/

void mapSpecials(void)
{
	switch (tiles[player.actor.y][player.actor.x].type) {
	case PLAYER_EXIT:
		currentMap++;
		return;
	default:
		break;
	}

	return;
}

/*
============
=
= printMap
=
============
*/

int constrain (int a, int b) {
	b -= 1;
	return a < 0 ? 0 : a > b ? b : a;
}

void printMap() {
	int x, y;
	int viewTopLeft[] = {constrain(player.actor.x - VIEW_WIDTH / 2, MAP_WIDTH - VIEW_WIDTH), constrain(player.actor.y - VIEW_HEIGHT / 2, MAP_HEIGHT - VIEW_HEIGHT)};
	int viewBottomRight[] = {viewTopLeft[0] + VIEW_WIDTH, viewTopLeft[1] + VIEW_HEIGHT};
	
	clear();
	for (y = viewTopLeft[1]; y <= viewBottomRight[1]; y++) {
		for (x = viewTopLeft[0]; x <= viewBottomRight[0]; x++) {
			mvaddch(y - viewTopLeft[1], (x - viewTopLeft[0]) * 2, tiles[y][x].glyph);
		}
	}
	mvaddch(player.actor.y - viewTopLeft[1], (player.actor.x - viewTopLeft[0]) * 2, player.actor.glyph);
}

/*
============
=
= renderActors
=
============
*/

void renderActors()
{
	int i;
	
	for (i = 0; i < numActors; i++) {
		if (actors[i].health > 0)
			mvaddch(actors[i].y, actors[i].x * 2 + 1, actors[i].glyph);
	}
}

/*
============
=
= canWalk
=
============
*/

int canWalk(int x, int y)
{
	if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_WIDTH && tiles[y][x].isWalkable) {
		return 1;
	} else {
		return 0;
	}
}

/*
============
=
= getInput
=
============
*/

void getInput(void)
{
	move(MAP_HEIGHT,0);
	switch (getch()) {
		case 'w':
			player.actor.deltaX = 0;
			player.actor.deltaY = -1;
			break;
		case 's':
			player.actor.deltaX = 0;
			player.actor.deltaY = 1;
			break;
		case 'a':
			player.actor.deltaX = -1;
			player.actor.deltaY = 0;
			break;
		case 'd':
			player.actor.deltaX = 1;
			player.actor.deltaY = 0;
			break;
		case 'q':
			endwin();
			exit(1);
	}

	if (canWalk(player.actor.x + player.actor.deltaX, player.actor.y + player.actor.deltaY)) {
		player.actor.x += player.actor.deltaX;
		player.actor.y += player.actor.deltaY;
	}
}

/*
============
=
= updateStatusBar
=
============
*/

void updateStatusBar(void)
{
	char statbar[VIEW_WIDTH];

	sprintf(statbar, "Health: %d, Level %d",player.actor.health, currentMap);
	mvprintw(VIEW_HEIGHT+1,0,statbar);
}

/*
============
=
= main
=
============
*/

int main(void)
{
	int mapOn;
	
	WadInit("game.wad");
	tiletypes = (tile *)GetLumpData(GetLumpNumForName("TILES"));
	initscr();
	curs_set(0);
	while (1) {
		initializeMap();

		// assign the current map to mapOn, so if it currentMap changes we can restart the play loop
		mapOn = currentMap;
		player.actor.health = 100;
		player.actor.glyph = '@';
		while (1) {
			mapSpecials();
			
			if (mapOn != currentMap)
				break;
			
			printMap();
			renderActors();
			updateStatusBar();
			refresh();
			getInput();
		}
	}
}