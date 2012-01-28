
//
// rpg.h
//

#ifndef _RPG_H__
#define _RPG_H__

#define VIEW_WIDTH 21
#define VIEW_HEIGHT 21

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
	int	directory;
} player_t;

short LittleShort (short l);
short BigShort (short l);
int LittleLong (int l);
int BigLong (int l);

void strupr (char *s);

void Error (char *error, ...);

#endif