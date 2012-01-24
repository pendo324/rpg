#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <curses.h>

#define VIEW_WIDTH 24
#define VIEW_HEIGHT 24

#define MAP_WIDTH 64
#define MAP_HEIGHT 64

// Tile specials
#define PLAYER_START 1
#define PLAYER_EXIT 2

typedef unsigned char byte;

#pragma pack(1)

typedef struct {
	char	glyph;		// the char used to display
	int	type;		// what type of tile (i.e. Player Start)
	int	damageTaken;	// how much damage is taken by stepping on tile, default 0
	int	isWalkable;	// tells if the tile can be moved onto or not (walls or floors)
} tile;

typedef struct {
	char	glyph;
	int	health;
	int	x, y;
	int	deltaX, deltaY;
} entity;

typedef struct {
	entity	actor;
} player_t;

//================
//
// wad file types
//
//================

typedef struct {
        int             filepos;
        int             size;
        char            name[8];
} lumpinfo_t;


typedef struct {
        char    identification[4];
        int     numlumps;
        int     infotableofs;
} wadinfo_t;

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

					WAD LOADING CODE

============================================================================
*/

int             handle;
int             numlumps;
lumpinfo_t      *lumpinfo;

/*
============
=
= WadInit
=
============
*/

void WadInit(char const *path)
{
        wadinfo_t       wad;
        lumpinfo_t      *lumps;
        char            *pathname;
        int                     i;

        pathname = malloc(strlen(path)+1);
        strcpy (pathname, path);
        handle = open(pathname, O_RDONLY);
//
// read in the header
//
        read (handle, &wad, sizeof(wad));
        if(strncmp(wad.identification, "IWAD", 4))
        {
                if(strncmp(wad.identification, "PWAD", 4))
                { // Bad file id
                        Error("Wad file %s doesn't have IWAD or PWAD id",path);
                }
        }
        wad.numlumps = LittleLong (wad.numlumps);
        wad.infotableofs = LittleLong (wad.infotableofs);

        numlumps = wad.numlumps;

//
// read in the lumpinfo
//
        lseek (handle, wad.infotableofs, SEEK_SET);
        lumpinfo = malloc(wad.numlumps*sizeof(lumpinfo_t));
        lumps = (lumpinfo_t *)lumpinfo;

        read (handle, lumps, wad.numlumps*sizeof(lumpinfo_t));
        for (i=0 ; i<wad.numlumps ; i++, lumps++)
        {
                lumps->filepos = LittleLong (lumps->filepos);
                lumps->size = LittleLong (lumps->size);
        }
}

/*
================
=
= GetLumpNumForName
=
================
*/

int GetLumpNumForName(char const *name)
{
        char name8[9];
        int v1, v2;
        lumpinfo_t *lump_p;

        // Make the name into two integers for easy compares
        strncpy(name8, name, 8);
        name8[8] = 0; // in case the name was a full 8 chars
        strupr(name8); // case insensitive
        v1 = *(int *)name8;
        v2 = *(int *)&name8[4];

        // Scan backwards so patch lump files take precedence
        lump_p = lumpinfo+numlumps;
        while(lump_p-- != lumpinfo)
        {
                if(*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
                {
                        return lump_p-lumpinfo;
                }
        }
        
        Error("Lump %s not found!",name);

	return -1;
}

/*
================
=
= GetLumpData
=
================
*/

void *GetLumpData(int lump)
{
        lumpinfo_t      *inf;
        byte                    *buf;

        inf = lumpinfo+lump;
        buf = malloc (inf->size);

        lseek (handle, inf->filepos, SEEK_SET);
        read (handle, buf, inf->size);

        return buf;
}

/*
================
=
= GetLumpLength
=
================
*/

int GetLumpLength(int lump)
{
	return lumpinfo[lump].size;
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