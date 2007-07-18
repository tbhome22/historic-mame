#include "driver.h"
#include "video/taitoic.h"

#define TC0100SCN_GFX_NUM 1


/**********************************************************/

void asuka_core_video_start(running_machine *machine, int x_offs,int buffering)
{
	PC090OJ_vh_start(0,0,8,buffering);	/* gfxset, x offset, y offset, buffering */
	TC0100SCN_vh_start(machine,1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,0);
	TC0110PCR_vh_start();
}

VIDEO_START( asuka )
{
	asuka_core_video_start(machine, 0,0);
}

VIDEO_START( galmedes )
{
	asuka_core_video_start(machine, 1,0);
}

VIDEO_START( cadash )
{
	asuka_core_video_start(machine, 1,1);
}

/**************************************************************
                 SPRITE READ AND WRITE HANDLERS
**************************************************************/

WRITE16_HANDLER( asuka_spritectrl_w )
{
	/* Bits 2-5 are color bank; in asuka games bit 0 is global priority */
	PC090OJ_sprite_ctrl = ((data & 0x3c) >> 2) | ((data & 0x1) << 15);
}


/**************************************************************
                        SCREEN REFRESH
**************************************************************/

VIDEO_UPDATE( asuka )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update(machine);

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[2],0,4);

	/* Sprites may be over or under top bg layer */
	PC090OJ_draw_sprites(machine,bitmap,cliprect,2);
	return 0;
}


VIDEO_UPDATE( bonzeadv )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update(machine);

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(machine,bitmap,cliprect,0,layer[2],0,4);

	/* Sprites are always over both bg layers */
	PC090OJ_draw_sprites(machine,bitmap,cliprect,0);
	return 0;
}