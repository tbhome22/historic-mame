/***************************************************************************

  vbzone.c

  Vector video hardware used by Battlezone

***************************************************************************/

#include "driver.h"
#include "vector.h"


#define VECTOR_LIST_SIZE	0x1000	/* Size in bytes of the vector ram area */

/*
 * This initialises the colors for all atari games
 * We must have the basic palette (8 colors) in color_prom[],
 * and the rest is set up to the correct intensity
 * This code is derived from Brad 
 */

void bzone_vh_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int c,i,e;   	/* (c)olor, (i)ntensity, palette (e)ntry */
	for (c=0; c<8; c++)
	{
		for (i=0; i<16; i++)
		{
			e=c+i*8;
			colortable[e]=e;
			palette[3*e  ]=((color_prom[3*c  ]) & 1) * i *16;
			palette[3*e+1]=((color_prom[3*c+1]) & 1) * i *16;
			palette[3*e+2]=((color_prom[3*c+2]) & 1) * i *16;
		}
	}	
}
		

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int bzone_vh_start(void)
{
	if (vg_init (VECTOR_LIST_SIZE, USE_AVG))
		return 1;
	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void bzone_vh_stop(void)
{
	vg_stop ();
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bzone_vh_screenrefresh(struct osd_bitmap *bitmap)
{
/* This routine does nothing - writes to vg_go start the drawing process */
}