#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

#pragma pack(1)

typedef struct {
	char	glyph;		// the char used to display
	int	type;		// what type of tile (i.e. Player Start)
	int	damageTaken;	// how much damage is taken by stepping on tile, default 0
	int	isWalkable;	// tells if the tile can be moved onto or not (walls or floors)
} tile;

#pragma pack()

typedef struct
{
	void	*data;
	int	size;
	int	count;
} object_t;


/*
================
=
= CreateObject
=
================
*/

object_t *CreateObject(int size, int count)
{
	object_t	*object;

	object = (object_t *)malloc(sizeof(object_t));
	object->data = NULL;
	object->count = count;
	object->size = size;

	return object;
}

/*
================
=
= ObjectGetCount
=
================
*/

int ObjectGetCount(object_t *object)
{
	return object->count;
}

/*
================
=
= ObjectGetData
=
================
*/

void *ObjectGetData(object_t *object, int index)
{
	return ((byte *)object->data)+(index*object->size);
}

/*
================
=
= ObjectAddToList
=
================
*/

void ObjectAddToList(object_t *object, void *data)
{
	object->data = realloc(object->data, object->size * (object->count+1));
	memcpy(((byte *)object->data)+(object->count*object->size),data,object->size);
	object->count++;
}

/*
================
=
= main
=
================
*/

int main(int argc, char *argv[])
{
	object_t *tiles;
	FILE *script,*lump;
	tile temp;
	int i,version;

	if (argc == 1 || argc > 2) {
		printf("Usage: gentiles tiles.til\n");
		exit(1);
	}

	tiles = CreateObject(sizeof(tile), 0);

	if ((script = fopen(argv[1], "r")) == NULL) {
		printf("Could not open script file.\n");
		exit(2);
	}

	if (!fscanf(script, "RPG Tiles version %d\n", &version) || version != 1) {
		printf("Not a version 1 RPG tile script.\n");
		exit(3);
	}

	if (fscanf (script,"\ntiles:%d\n",&i) != 1) {
		printf("Couldn't read number of tiles\n");
		exit(4);
	}
	printf ("%i tiles\n", i);

	for (; i > 0; i--) {
		if (fscanf (script,"%c, %d, %d, %d\n",&temp.glyph, &temp.type, &temp.damageTaken, &temp.isWalkable) != 4) {
			printf("Could not read tile definition.\n");
			exit(5);
		}
		ObjectAddToList(tiles,&temp);
	}
	
	if ((lump = fopen("TILES.LMP", "w")) == NULL) {
		printf("Could not output file.\n");
		exit(2);
	}
	
	fwrite(ObjectGetData(tiles,0),sizeof(tile),ObjectGetCount(tiles),lump);
	exit(0);
}
