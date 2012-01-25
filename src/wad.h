
//================
//
// wad file types
//
//================

#ifndef _WAD_H__
#define _WAD_H__

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

void WadInit(char const *path);
int GetLumpNumForName(char const *name);
void *GetLumpData(int lump);
int GetLumpLength(int lump);

#endif

