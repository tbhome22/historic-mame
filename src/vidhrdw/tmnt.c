#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"


#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

static int layer_colorbase[3],sprite_colorbase,bg_colorbase;
static int priorityflag;



/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

/* Missing in Action */

static void mia_tile_callback(int layer,int bank,int *code,int *color)
{
	tile_info.flags = (*color & 0x04) ? TILE_FLIPX : 0;
	if (layer == 0)
	{
		*code |= ((*color & 0x01) << 8);
		*color = layer_colorbase[layer] + ((*color & 0x80) >> 5) + ((*color & 0x10) >> 1);
	}
	else
	{
		*code |= ((*color & 0x01) << 8) | ((*color & 0x18) << 6) | (bank << 11);
		*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
	}
}

static void tmnt_tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x03) << 8) | ((*color & 0x10) << 6) | ((*color & 0x0c) << 9) | (bank << 13);
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}

static void xmen_tile_callback(int layer,int bank,int *code,int *color)
{
	/* flip Y? I sure hope this goes to some inverters on the ROM address lines, */
	/* otherwise it would throw away all my understanding of the 051960 ;-) */
	tile_info.flags = (*color & 0x02) ? TILE_FLIPY : 0;

	if (layer == 0)
		*color = layer_colorbase[layer] + ((*color & 0xf0) >> 4);
	else
		*color = layer_colorbase[layer] + ((*color & 0x7c) >> 2);
}



/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void mia_sprite_callback(int *code,int *color,int *priority)
{
	*color = sprite_colorbase + (*color & 0x0f);
}

static void tmnt_sprite_callback(int *code,int *color,int *priority)
{
	*code |= (*color & 0x10) << 9;
	*color = sprite_colorbase + (*color & 0x0f);
}

static void punkshot_sprite_callback(int *code,int *color,int *priority)
{
	*code |= (*color & 0x10) << 9;
	*priority = 0x20 | ((*color & 0x60) >> 2);
	*color = sprite_colorbase + (*color & 0x0f);
}

/***************************************************************************

  Callbacks for the K053245

***************************************************************************/

static void lgtnfght_sprite_callback(int *code,int *color,int *priority)
{
	*priority = 0x20 | ((*color & 0x60) >> 2);
	*color = sprite_colorbase + (*color & 0x1f);
}



int mia_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 32;
	layer_colorbase[2] = 40;
	sprite_colorbase = 16;
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,mia_tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,REVERSE_PLANE_ORDER,mia_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int tmnt_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 32;
	layer_colorbase[2] = 40;
	sprite_colorbase = 16;
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,REVERSE_PLANE_ORDER,tmnt_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int punkshot_vh_start(void)
{
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,punkshot_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int lgtnfght_vh_start(void)	/* also tmnt2, ssriders */
{
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K053245_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,lgtnfght_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int xmen_vh_start(void)
{
	return (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,xmen_tile_callback));
}

void punkshot_vh_stop(void)
{
	K052109_vh_stop();
	K051960_vh_stop();
}

void lgtnfght_vh_stop(void)
{
	K052109_vh_stop();
	K053245_vh_stop();
}

void xmen_vh_stop(void)
{
	K052109_vh_stop();
}



void tmnt_paletteram_w(int offset,int data)
{
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	WRITE_WORD(&paletteram[offset],newword);

	offset /= 4;
	{
		int palette = ((READ_WORD(&paletteram[offset * 4]) & 0x00ff) << 8)
				+ (READ_WORD(&paletteram[offset * 4 + 2]) & 0x00ff);
		int r = palette & 31;
		int g = (palette >> 5) & 31;
		int b = (palette >> 10) & 31;

		r = (r << 3) + (r >> 2);
		g = (g << 3) + (g >> 2);
		b = (b << 3) + (b >> 2);

		palette_change_color (offset,r,g,b);
	}
}



void tmnt_0a0000_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
	{
		static int last;

		/* bit 0/1 = coin counters */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);	/* 2 players version */

		/* bit 3 high then low triggers irq on sound CPU */
		if (last == 0x08 && (data & 0x08) == 0)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x08;

		/* bit 5 = irq enable */
		interrupt_enable_w(0,data & 0x20);

		/* bit 7 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x80) ? ASSERT_LINE : CLEAR_LINE);

		/* other bits unused */
	}
}

void punkshot_0a0020_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
	{
		static int last;


		/* bit 0 = coin counter */
		coin_counter_w(0,data & 0x01);

		/* bit 2 = trigger irq on sound CPU */
		if (last == 0x04 && (data & 0x04) == 0)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x04;

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
}

void lgtnfght_0a0018_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
	{
		static int last;


		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 2 = trigger irq on sound CPU */
		if (last == 0x00 && (data & 0x04) == 0x04)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x04;

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
}

void ssriders_1c0300_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
	{
		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

		/* other bits unknown (bits 4-6 used in TMNT2) */
	}
}



void tmnt_priority_w(int offset,int data)
{
	if ((data & 0x00ff0000) == 0)
	{
		/* bit 2/3 = priority; other bits unused */
		/* bit2 = PRI bit3 = PRI2
			  sprite/playfield priority is controlled by these two bits, by bit 3
			  of the background tile color code, and by the SHADOW sprite
			  attribute bit.
			  Priorities are encoded in a PROM (G19 for TMNT). However, in TMNT,
			  the PROM only takes into account the PRI and SHADOW bits.
			  PRI  Priority
			   0   bg fg spr text
			   1   bg spr fg text
			  The SHADOW bit, when set, torns a sprite into a shadow which makes
			  color below it darker (this is done by turning off three resistors
			  in parallel with the RGB output).

			  Note: the background color (color used when all of the four layers
			  are 0) is taken from the *foreground* palette, not the background
			  one as would be more intuitive.
		*/
		priorityflag = (data & 0x0c) >> 2;
	}
}




void mia_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
	if ((priorityflag & 1) == 1) K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,1,0);
	if ((priorityflag & 1) == 0) K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0);
}

void tmnt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
	if ((priorityflag & 1) == 1) K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,1,0);
	if ((priorityflag & 1) == 0) K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0);
}

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

void punkshot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int pri[3],layer[3];


	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);

	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = 0;
	pri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	pri[1] = K053251_get_priority(K053251_CI4);
	layer[2] = 2;
	pri[2] = K053251_get_priority(K053251_CI3);

	sortlayers(layer,pri);

	K052109_tilemap_draw(bitmap,layer[0],TILEMAP_IGNORE_TRANSPARENCY);
	K051960_draw_sprites(bitmap,pri[1]+1,pri[0]);
	K052109_tilemap_draw(bitmap,layer[1],0);
	K051960_draw_sprites(bitmap,pri[2]+1,pri[1]);
	K052109_tilemap_draw(bitmap,layer[2],0);
	K051960_draw_sprites(bitmap,0,pri[2]);
}

void lgtnfght_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int pri[3],layer[3];


	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);

	K052109_tilemap_update();

	palette_init_used_colors();
	K053245_mark_sprites_colors();
	palette_used_colors[16 * bg_colorbase] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = 0;
	pri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	pri[1] = K053251_get_priority(K053251_CI4);
	layer[2] = 2;
	pri[2] = K053251_get_priority(K053251_CI3);

	sortlayers(layer,pri);

	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],&Machine->drv->visible_area);
	K053245_draw_sprites(bitmap,pri[0]+1,0x3f);
	K052109_tilemap_draw(bitmap,layer[0],0);
	K053245_draw_sprites(bitmap,pri[1]+1,pri[0]);
	K052109_tilemap_draw(bitmap,layer[1],0);
	K053245_draw_sprites(bitmap,pri[2]+1,pri[1]);
	K052109_tilemap_draw(bitmap,layer[2],0);
	K053245_draw_sprites(bitmap,0,pri[2]);

#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(spriteram, spriteram_size, 1, fp);
		fclose(fp);
	}
}
#endif
}

void ssriders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;

	for (i = 0;i < 128;i++)
		if ((K053245_word_r(16*i) & 0x8000) && !(K053245_word_r(16*i+2) & 0x8000))
			K053245_word_w(16*i,0xff000000|i);	/* workaround for protection */

	lgtnfght_vh_screenrefresh(bitmap,full_refresh);
}


#define SPRITEWORD(offset) READ_WORD(&spriteram[(offset)])
static void xmen_drawsprites(struct osd_bitmap *bitmap,int min_priority,int max_priority)
{
#define NUM_SPRITES 256
	int offs,pri_code;
	int sortedlist[NUM_SPRITES];

	for (offs = 0;offs < NUM_SPRITES;offs++)
		sortedlist[offs] = -1;

	/* prebuild a sorted table */
	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		sortedlist[SPRITEWORD(offs) & 0x00ff] = offs;
	}

	for (pri_code = NUM_SPRITES-1;pri_code >= 0;pri_code--)
	{
		int ox,oy,col,code,size,w,h,x,y,flipx,flipy,zoomx,zoomy,pri;
		/* sprites can be grouped up to 8x8. The draw order is
			 0  1  4  5 16 17 20 21
			 2  3  6  7 18 19 22 23
			 8  9 12 13 24 25 28 29
			10 11 14 15 26 27 30 31
			32 33 36 37 48 49 52 53
			34 35 38 39 50 51 54 55
			40 41 44 45 56 57 60 61
			42 43 46 47 58 59 62 63
		*/
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };


		offs = sortedlist[pri_code];
		if (offs == -1) continue;

		if ((SPRITEWORD(offs) & 0x8000) == 0) continue;

		pri = (SPRITEWORD(offs+0x0c) & 0x00e0) >> 4;	/* ??????? */
		if (pri < min_priority || pri > max_priority) continue;

		size = (SPRITEWORD(offs) & 0x0f00) >> 8;

		w = 1 << (size & 0x03);
		h = 1 << ((size >> 2) & 0x03);

		code = SPRITEWORD(offs+0x02);

		col = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
/* this bit is used in a few places but cannot be just shadow, it's used */
/* for normal sprites too. */
//			shadow = SPRITEWORD(offs+0x0c) & 0x0080;
#if 0
if (keyboard_pressed(KEYCODE_Q) && (SPRITEWORD(offs+0x0c) & 0x8000)) col = rand();
if (keyboard_pressed(KEYCODE_W) && (SPRITEWORD(offs+0x0c) & 0x4000)) col = rand();
if (keyboard_pressed(KEYCODE_E) && (SPRITEWORD(offs+0x0c) & 0x2000)) col = rand();
if (keyboard_pressed(KEYCODE_R) && (SPRITEWORD(offs+0x0c) & 0x1000)) col = rand();
if (keyboard_pressed(KEYCODE_T) && (SPRITEWORD(offs+0x0c) & 0x0800)) col = rand();
if (keyboard_pressed(KEYCODE_Y) && (SPRITEWORD(offs+0x0c) & 0x0400)) col = rand();
if (keyboard_pressed(KEYCODE_A) && (SPRITEWORD(offs+0x0c) & 0x0200)) col = rand();
if (keyboard_pressed(KEYCODE_S) && (SPRITEWORD(offs+0x0c) & 0x0100)) col = rand();
if (keyboard_pressed(KEYCODE_D) && (SPRITEWORD(offs+0x0c) & 0x0080)) col = rand();
if (keyboard_pressed(KEYCODE_F) && (SPRITEWORD(offs+0x0c) & 0x0040)) col = rand();
if (keyboard_pressed(KEYCODE_G) && (SPRITEWORD(offs+0x0c) & 0x0020)) col = rand();
#endif
		ox = ((SPRITEWORD(offs+0x06) + 111) & 0x3ff);
		oy = 512-144 - (SPRITEWORD(offs+0x04) & 0x3ff);
		if (oy < -512) oy += 1024;

		/* zoom control:
		   0x40 = normal scale
		  <0x40 enlarge (0x20 = double size)
		  >0x40 reduce (0x80 = half size)
		*/
		zoomy = SPRITEWORD(offs+0x08);
		if (zoomy > 0x2000) continue;
		if (zoomy) zoomy = (0x400000+zoomy/2) / zoomy;
		else zoomy = 2 * 0x400000;
		if ((SPRITEWORD(offs) & 0x8000) == 0)
		{
			zoomx = SPRITEWORD(offs+0x0a);
			if (zoomx > 0x2000) continue;
			if (zoomx) zoomx = (0x400000+zoomx/2) / zoomx;
			else zoomx = 2 * 0x400000;
		}
		else zoomx = zoomy;

		/* the coordinates given are for the *center* of the sprite */
		ox -= (zoomx * w) >> 13;
		oy -= (zoomy * h) >> 13;

		flipx = SPRITEWORD(offs) & 0x1000;
		flipy = SPRITEWORD(offs) & 0x2000;

		if (zoomx == 0x10000 && zoomy == 0x10000)
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy;

				sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 16 * x;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfx(bitmap,Machine->gfx[0],
							c,
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			for (y = 0;y < h;y++)
			{
				int sx,sy,zw,zh;

				sy = oy + ((zoomy * y + (1<<11)) >> 12);
				zh = (oy + ((zoomy * (y+1) + (1<<11)) >> 12)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoomx * x + (1<<11)) >> 12);
					zw = (ox + ((zoomx * (x+1) + (1<<11)) >> 12)) - sx;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfxzoom(bitmap,Machine->gfx[0],
							c,
							col,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
}

void xmen_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,i;
	int pri[3],layer[3];


	bg_colorbase       = K053251_get_palette_index(K053251_CI4);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI2);

	K052109_tilemap_update();

	palette_init_used_colors();

	/* palette remapping first */
	{
		unsigned short palette_map[128];
		int color;

		memset (palette_map, 0, sizeof (palette_map));

		/* sprites */
		for (offs = 0;offs < spriteram_size;offs += 32)
		{
			color = sprite_colorbase + (SPRITEWORD(offs+0x0c) & 0x001f);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] |= PALETTE_COLOR_VISIBLE;
			}
		}
	}
	palette_used_colors[16 * bg_colorbase+1] |= PALETTE_COLOR_VISIBLE;
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = 0;
	pri[0] = K053251_get_priority(K053251_CI3);
	layer[1] = 1;
	pri[1] = K053251_get_priority(K053251_CI0);
	layer[2] = 2;
	pri[2] = K053251_get_priority(K053251_CI2);

	sortlayers(layer,pri);

	/* note the '+1' in the background color!!! */
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase+1],&Machine->drv->visible_area);
	xmen_drawsprites(bitmap,pri[0]+1,0x3f);
	K052109_tilemap_draw(bitmap,layer[0],0);
	xmen_drawsprites(bitmap,pri[1]+1,pri[0]);
	K052109_tilemap_draw(bitmap,layer[1],0);
	xmen_drawsprites(bitmap,pri[2]+1,pri[1]);
	K052109_tilemap_draw(bitmap,layer[2],0);
	xmen_drawsprites(bitmap,0,pri[2]);

#if 0
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("SPRITE.DMP", "w+b");
	if (fp)
	{
		fwrite(spriteram, spriteram_size, 1, fp);
		fclose(fp);
	}
}
#endif
}
