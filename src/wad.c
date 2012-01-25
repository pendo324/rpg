#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "rpg.h"
#include "wad.h"

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
