/***************************************************************************

	Midway MCR-2 system

    driver by Christopher Kirmse, Aaron Giles

	Games supported:
		* Satan's Hollow
		* Tron
		* Kozmik Krooz'r
		* Domino Man
		* Wacko
		* Two Tigers
		* Journey

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	CPU #1
	========================================================================
	0000-BFFF   R     xxxxxxxx    Program ROM
	C000-C7FF   R/W   xxxxxxxx    NVRAM
	F000-F1FF   R/W   xxxxxxxx    Sprite RAM
	F800-FF7F   R/W   xxxxxxxx    Background video RAM
	FF80-FFFF     W   xxxxxxxx    Palette RAM
	========================================================================
	0000        R     x-xxxxxx    Input ports
	            R     x-------    Service switch (active low)
	            R     --x-----    Tilt
	            R     ---xxx--    External inputs
	            R     ------x-    Right coin
	            R     -------x    Left coin
	0000        W     xxxxxxxx    Data latch OP0 (coin meters, 2 led's and cocktail 'flip')
	0001        R     xxxxxxxx    External inputs
	0002        R     xxxxxxxx    External inputs
	0003        R     xxxxxxxx    DIP switches
	0004        R     xxxxxxxx    External inputs
	0004        W     xxxxxxxx    Data latch OP4 (comm. with external hardware)
	0007        R     xxxxxxxx    Audio status
	001C-001F   W     xxxxxxxx    Audio latches 1-4
	00E0        W     --------    Watchdog reset
	00E8        W     xxxxxxxx    Unknown (written at initialization time)
	00F0-00F3   W     xxxxxxxx    CTC communications
	========================================================================
	Interrupts:
		NMI ???
		INT generated by CTC
	========================================================================


	========================================================================
	CPU #2 (Super Sound I/O)
	========================================================================
	0000-3FFF   R     xxxxxxxx    Program ROM
	8000-83FF   R/W   xxxxxxxx    Program RAM
	9000-9003   R     xxxxxxxx    Audio latches 1-4
	A000          W   xxxxxxxx    AY-8910 #1 control
	A001        R     xxxxxxxx    AY-8910 #1 status
	A002          W   xxxxxxxx    AY-8910 #1 data
	B000          W   xxxxxxxx    AY-8910 #2 control
	B001        R     xxxxxxxx    AY-8910 #2 status
	B002          W   xxxxxxxx    AY-8910 #2 data
	C000          W   xxxxxxxx    Audio status
	E000          W   xxxxxxxx    Unknown
	F000        R     xxxxxxxx    Audio board switches
	========================================================================
	Interrupts:
		NMI ???
		INT generated by external circuitry 780 times/second
	========================================================================

***************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "sndhrdw/mcr.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


static UINT8 wacko_mux_select;



/*************************************
 *
 *	Kozmik Krooz'r input ports
 *
 *************************************/

static READ_HANDLER( kroozr_dial_r )
{
	int dial = readinputport(7);
	int val = readinputport(1);

	val |= (dial & 0x80) >> 1;
	val |= (dial & 0x70) >> 4;

	return val;
}


static READ_HANDLER( kroozr_trakball_x_r )
{
	int val = readinputport(6);

	if (val & 0x02)		/* left */
		return 0x64 - 0x34;
	if (val & 0x01)		/* right */
		return 0x64 + 0x34;
	return 0x64;
}


static READ_HANDLER( kroozr_trakball_y_r )
{
	int val = readinputport(6);

	if (val & 0x08)		/* up */
		return 0x64 - 0x34;
	if (val & 0x04)		/* down */
		return 0x64 + 0x34;
	return 0x64;
}



/*************************************
 *
 *	Wacko input ports
 *
 *************************************/

static WRITE_HANDLER( wacko_mux_select_w )
{
	wacko_mux_select = data & 1;
}


static READ_HANDLER( wacko_trackball_r )
{
	if (!wacko_mux_select)
		return readinputport(1 + offset);
	else
		return readinputport(6 + offset);
}



/*************************************
 *
 *	Two Tigers Yoke input ports
 *
 *************************************/

static READ_HANDLER( twotigra_yoke1_r )
{
	int p1_yoke = readinputport(6);
	if (p1_yoke & 0x10)
	{
		if ((p1_yoke & 0x01) != 0x01) return 0;
		if ((p1_yoke & 0x02) != 0x02) return 255;
		return 100;
	}
	else
	{
		p1_yoke = readinputport(2);
		if (p1_yoke < 0x1b) return 0;
		return p1_yoke - 0x1b;
	}
}


static READ_HANDLER( twotigra_yoke2_r )
{
	int p1_yoke = readinputport(6);
	if (p1_yoke & 0x10)
	{
		if ((p1_yoke & 0x04) != 0x04) return 0;
		if ((p1_yoke & 0x08) != 0x08) return 255;
		return 100;
	}
	else
	{
		p1_yoke = readinputport(1);
		if (p1_yoke < 0x1b) return 0;
		return p1_yoke - 0x1b;
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xf000, 0xf1ff, MRA_RAM },
	{ 0xf800, 0xffff, mcr2_videoram_r },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0xf000, 0xf1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, mcr2_videoram_w, &videoram, &videoram_size },	/* also palette */
MEMORY_END


static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x07, 0x07, ssio_status_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0xf0, 0xf3, z80ctc_0_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, mcr_control_port_w },
	{ 0x1c, 0x1f, ssio_data_w },
	{ 0xe0, 0xe0, watchdog_reset_w },
	{ 0xe8, 0xe8, MWA_NOP },
	{ 0xf0, 0xf3, z80ctc_0_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( shollow )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN2 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( tron )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- controls spinner */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 50, 10, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Coin Meters" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 50, 10, 0, 0 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( kroozr )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- controls firing spinner */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 -- controls joystick x-axis */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 -- controls joystick y-axis */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* dummy extra port for keyboard movement */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* dummy extra port for dial control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 40, 10, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( domino )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )

	PORT_START	/* IN2 unused */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x02, "Skin Color" )
	PORT_DIPSETTING(    0x02, "Light" )
	PORT_DIPSETTING(    0x00, "Dark" )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Coin Meters" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )

	PORT_START	/* IN4 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( wacko )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- controls joystick x-axis */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X, 50, 10, 0, 0 )

	PORT_START	/* IN2 -- controls joystick y-axis */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE, 50, 10, 0, 0 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Coin Meters" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )

	PORT_START	/* IN4 -- 4-way firing joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 -- controls joystick x-axis */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_COCKTAIL, 50, 10, 0, 0 )

	PORT_START	/* IN2 -- controls joystick y-axis */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_COCKTAIL, 50, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( twotiger )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1, "Dogfight Start", KEYCODE_6, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- player 1 spinner */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 10, 10, 0, 0 )

	PORT_START	/* IN2 -- buttons for player 1 & player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 -- player 2 spinner */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 10, 10, 0, 0 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( twotigra )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_START1, "Dogfight Start", KEYCODE_3, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- player 2 yoke */
	//PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 10, 10, 0, 0 )
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0, 255 )

	PORT_START	/* IN2 -- player 1 yoke */
	//PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 10, 10, 0, 0 )
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 255 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Shot Speed" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPNAME( 0x02, 0x00, "Dogfight" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Credits" )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )


	PORT_START	/* IN4 -- buttons for player 1 & player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 fake for yoke */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_DIPNAME( 0x10, 0x10, "Fake Inputs " )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analog" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &mcr_bg_layout,     0, 4 },	/* colors 0-63 */
	{ REGION_GFX2, 0, &mcr_sprite_layout, 0, 4 },	/* colors 0-63 */
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( mcr2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2500000)
	MDRV_CPU_CONFIG(mcr_daisy_chain)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(mcr_interrupt,2)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_30HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mcr)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 30*16)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 0*16, 30*16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(mcr12)
	MDRV_VIDEO_UPDATE(mcr2)

	/* sound hardware */
	MDRV_IMPORT_FROM(mcr_ssio)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( journey )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mcr2)
	MDRV_CPU_REPLACE("main", Z80, 7500000)

	/* video hardware */
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(journey)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( shollow )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sh-pro.00",    0x0000, 0x2000, 0x95e2b800 )
	ROM_LOAD( "sh-pro.01",    0x2000, 0x2000, 0xb99f6ff8 )
	ROM_LOAD( "sh-pro.02",    0x4000, 0x2000, 0x1202c7b2 )
	ROM_LOAD( "sh-pro.03",    0x6000, 0x2000, 0x0a64afb9 )
	ROM_LOAD( "sh-pro.04",    0x8000, 0x2000, 0x22fa9175 )
	ROM_LOAD( "sh-pro.05",    0xa000, 0x2000, 0x1716e2bb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sh-snd.01",    0x0000, 0x1000, 0x55a297cc )
	ROM_LOAD( "sh-snd.02",    0x1000, 0x1000, 0x46fc31f6 )
	ROM_LOAD( "sh-snd.03",    0x2000, 0x1000, 0xb1f4a6a8 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sh-bg.00",     0x00000, 0x2000, 0x3e2b333c )
	ROM_LOAD( "sh-bg.01",     0x02000, 0x2000, 0xd1d70cc4 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sh-fg.00",     0x00000, 0x2000, 0x33f4554e )
	ROM_LOAD( "sh-fg.01",     0x02000, 0x2000, 0xba1a38b4 )
	ROM_LOAD( "sh-fg.02",     0x04000, 0x2000, 0x6b57f6da )
	ROM_LOAD( "sh-fg.03",     0x06000, 0x2000, 0x37ea9d07 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )
ROM_END


ROM_START( shollow2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sh-pro.00",    0x0000, 0x2000, 0x95e2b800 )
	ROM_LOAD( "sh-pro.01",    0x2000, 0x2000, 0xb99f6ff8 )
	ROM_LOAD( "sh-pro.02",    0x4000, 0x2000, 0x1202c7b2 )
	ROM_LOAD( "sh-pro.03",    0x6000, 0x2000, 0x0a64afb9 )
	ROM_LOAD( "sh-pro.04",    0x8000, 0x2000, 0x22fa9175 )
	ROM_LOAD( "sh-pro.05",    0xa000, 0x2000, 0x1716e2bb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "snd-0.a7",     0x0000, 0x1000, 0x9d815bb3 )
	ROM_LOAD( "snd-1.a8",     0x1000, 0x1000, 0x9f253412 )
	ROM_LOAD( "snd-2.a9",     0x2000, 0x1000, 0x7783d6c6 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sh-bg.00",     0x00000, 0x2000, 0x3e2b333c )
	ROM_LOAD( "sh-bg.01",     0x02000, 0x2000, 0xd1d70cc4 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sh-fg.00",     0x00000, 0x2000, 0x33f4554e )
	ROM_LOAD( "sh-fg.01",     0x02000, 0x2000, 0xba1a38b4 )
	ROM_LOAD( "sh-fg.02",     0x04000, 0x2000, 0x6b57f6da )
	ROM_LOAD( "sh-fg.03",     0x06000, 0x2000, 0x37ea9d07 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )
ROM_END


ROM_START( tron )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "scpu_pga.bin", 0x0000, 0x2000, 0x5151770b )
	ROM_LOAD( "scpu_pgb.bin", 0x2000, 0x2000, 0x8ddf8717 )
	ROM_LOAD( "scpu_pgc.bin", 0x4000, 0x2000, 0x4241e3a0 )
	ROM_LOAD( "scpu_pgd.bin", 0x6000, 0x2000, 0x035d2fe7 )
	ROM_LOAD( "scpu_pge.bin", 0x8000, 0x2000, 0x24c185d8 )
	ROM_LOAD( "scpu_pgf.bin", 0xA000, 0x2000, 0x38c4bbaf )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ssi_0a.bin",   0x0000, 0x1000, 0x765e6eba )
	ROM_LOAD( "ssi_0b.bin",   0x1000, 0x1000, 0x1b90ccdd )
	ROM_LOAD( "ssi_0c.bin",   0x2000, 0x1000, 0x3a4bc629 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "scpu_bgg.bin", 0x00000, 0x2000, 0x1a9ed2f5 )
	ROM_LOAD( "scpu_bgh.bin", 0x02000, 0x2000, 0x3220f974 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vg_3.bin",     0x00000, 0x2000, 0xbc036d1d )
	ROM_LOAD( "vg_2.bin",     0x02000, 0x2000, 0x58ee14d3 )
	ROM_LOAD( "vg_1.bin",     0x04000, 0x2000, 0x3329f9d4 )
	ROM_LOAD( "vg_0.bin",     0x06000, 0x2000, 0x9743f873 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.d12",   0x0000, 0x0020, 0xe1281ee9 )
ROM_END


ROM_START( tron2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pro0.d2",      0x0000, 0x2000, 0x0de0471a )
	ROM_LOAD( "scpu_pgb.bin", 0x2000, 0x2000, 0x8ddf8717 )
	ROM_LOAD( "scpu_pgc.bin", 0x4000, 0x2000, 0x4241e3a0 )
	ROM_LOAD( "scpu_pgd.bin", 0x6000, 0x2000, 0x035d2fe7 )
	ROM_LOAD( "scpu_pge.bin", 0x8000, 0x2000, 0x24c185d8 )
	ROM_LOAD( "scpu_pgf.bin", 0xA000, 0x2000, 0x38c4bbaf )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ssi_0a.bin",   0x0000, 0x1000, 0x765e6eba )
	ROM_LOAD( "ssi_0b.bin",   0x1000, 0x1000, 0x1b90ccdd )
	ROM_LOAD( "ssi_0c.bin",   0x2000, 0x1000, 0x3a4bc629 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "scpu_bgg.bin", 0x00000, 0x2000, 0x1a9ed2f5 )
	ROM_LOAD( "scpu_bgh.bin", 0x02000, 0x2000, 0x3220f974 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vg_3.bin",     0x00000, 0x2000, 0xbc036d1d )
	ROM_LOAD( "vg_2.bin",     0x02000, 0x2000, 0x58ee14d3 )
	ROM_LOAD( "vg_1.bin",     0x04000, 0x2000, 0x3329f9d4 )
	ROM_LOAD( "vg_0.bin",     0x06000, 0x2000, 0x9743f873 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.d12",   0x0000, 0x0020, 0xe1281ee9 )
ROM_END


ROM_START( kroozr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "kozmkcpu.2d",  0x0000, 0x2000, 0x61e02045 )
	ROM_LOAD( "kozmkcpu.3d",  0x2000, 0x2000, 0xcaabed57 )
	ROM_LOAD( "kozmkcpu.4d",  0x4000, 0x2000, 0x2bc83fc7 )
	ROM_LOAD( "kozmkcpu.5d",  0x6000, 0x2000, 0xa0ec38c1 )
	ROM_LOAD( "kozmkcpu.6d",  0x8000, 0x2000, 0x7044f2b6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "kozmksnd.7a",  0x0000, 0x1000, 0x6736e433 )
	ROM_LOAD( "kozmksnd.8a",  0x1000, 0x1000, 0xea9cd919 )
	ROM_LOAD( "kozmksnd.9a",  0x2000, 0x1000, 0x9dfa7994 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kozmkcpu.3g",  0x00000, 0x2000, 0xeda6ed2d )
	ROM_LOAD( "kozmkcpu.4g",  0x02000, 0x2000, 0xddde894b )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "kozmkvid.1e",  0x00000, 0x2000, 0xca60e2cc )
	ROM_LOAD( "kozmkvid.1d",  0x02000, 0x2000, 0x4e23b35b )
	ROM_LOAD( "kozmkvid.1b",  0x04000, 0x2000, 0xc6041ba7 )
	ROM_LOAD( "kozmkvid.1a",  0x06000, 0x2000, 0xb57fb0ff )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( domino )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "dmanpg0.bin",  0x0000, 0x2000, 0x3bf3bb1c )
	ROM_LOAD( "dmanpg1.bin",  0x2000, 0x2000, 0x85cf1d69 )
	ROM_LOAD( "dmanpg2.bin",  0x4000, 0x2000, 0x7dd2177a )
	ROM_LOAD( "dmanpg3.bin",  0x6000, 0x2000, 0xf2e0aa44 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "dm-a7.snd",    0x0000, 0x1000, 0xfa982dcc )
	ROM_LOAD( "dm-a8.snd",    0x1000, 0x1000, 0x72839019 )
	ROM_LOAD( "dm-a9.snd",    0x2000, 0x1000, 0xad760da7 )
	ROM_LOAD( "dm-a10.snd",   0x3000, 0x1000, 0x958c7287 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dmanbg0.bin",  0x00000, 0x2000, 0x9163007f )
	ROM_LOAD( "dmanbg1.bin",  0x02000, 0x2000, 0x28615c56 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dmanfg0.bin",  0x00000, 0x2000, 0x0b1f9f9e )
	ROM_LOAD( "dmanfg1.bin",  0x02000, 0x2000, 0x16aa4b9b )
	ROM_LOAD( "dmanfg2.bin",  0x04000, 0x2000, 0x4a8e76b8 )
	ROM_LOAD( "dmanfg3.bin",  0x06000, 0x2000, 0x1f39257e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( wacko )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "wackocpu.2d",  0x0000, 0x2000, 0xc98e29b6 )
	ROM_LOAD( "wackocpu.3d",  0x2000, 0x2000, 0x90b89774 )
	ROM_LOAD( "wackocpu.4d",  0x4000, 0x2000, 0x515edff7 )
	ROM_LOAD( "wackocpu.5d",  0x6000, 0x2000, 0x9b01bf32 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "wackosnd.7a",  0x0000, 0x1000, 0x1a58763f )
	ROM_LOAD( "wackosnd.8a",  0x1000, 0x1000, 0xa4e3c771 )
	ROM_LOAD( "wackosnd.9a",  0x2000, 0x1000, 0x155ba3dd )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wackocpu.3g",  0x00000, 0x2000, 0x33160eb1 )
	ROM_LOAD( "wackocpu.4g",  0x02000, 0x2000, 0xdaf37d7c )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wackovid.1e",  0x00000, 0x2000, 0xdca59be7 )
	ROM_LOAD( "wackovid.1d",  0x02000, 0x2000, 0xa02f1672 )
	ROM_LOAD( "wackovid.1b",  0x04000, 0x2000, 0x7d899790 )
	ROM_LOAD( "wackovid.1a",  0x06000, 0x2000, 0x080be3ad )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( twotiger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2tgrpg0.bin",  0x0000, 0x2000, 0xe77a924b )
	ROM_LOAD( "2tgrpg1.bin",  0x2000, 0x2000, 0x2699ebdc )
	ROM_LOAD( "2tgrpg2.bin",  0x4000, 0x2000, 0xb5ca3f17 )
	ROM_LOAD( "2tgrpg3.bin",  0x6000, 0x2000, 0x8aa82049 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "2tgra7.bin",   0x0000, 0x1000, 0x4620d970 )
	ROM_LOAD( "2tgra8.bin",   0x1000, 0x1000, 0xe95d8cfe )
	ROM_LOAD( "2tgra9.bin",   0x2000, 0x1000, 0x81e6ce0e )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2tgrbg0.bin",  0x00000, 0x2000, 0x52f69068 )
	ROM_LOAD( "2tgrbg1.bin",  0x02000, 0x2000, 0x758d4f7d )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2tgrfg0.bin",  0x00000, 0x2000, 0x4abf3ca0 )
	ROM_LOAD( "2tgrfg1.bin",  0x02000, 0x2000, 0xfbcaffa5 )
	ROM_LOAD( "2tgrfg2.bin",  0x04000, 0x2000, 0x08e3e1a6 )
	ROM_LOAD( "2tgrfg3.bin",  0x06000, 0x2000, 0x9b22697b )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( twotigra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_d2",  0x0000, 0x2000, 0xa682ed24 )
	ROM_LOAD( "cpu_d3",  0x2000, 0x2000, 0x5b48fde9 )
	ROM_LOAD( "cpu_d4",  0x4000, 0x2000, 0xf1ab8c4d )
	ROM_LOAD( "cpu_d5",  0x6000, 0x2000, 0xd7129900 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ssio_a7",   0x0000, 0x1000, 0x64ddc16c )
	ROM_LOAD( "ssio_a8",   0x1000, 0x1000, 0xc3467612 )
	ROM_LOAD( "ssio_a9",   0x2000, 0x1000, 0xc50f7b2d )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2tgrbg0.bin",  0x00000, 0x2000, 0x52f69068 )
	ROM_LOAD( "2tgrbg1.bin",  0x02000, 0x2000, 0x758d4f7d )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_d1",  0x00000, 0x2000, 0xda5f49da )
	ROM_LOAD( "vid_c1",  0x02000, 0x2000, 0x62ed737b )
	ROM_LOAD( "vid_b1",  0x04000, 0x2000, 0x0939921e )
	ROM_LOAD( "vid_a1",  0x06000, 0x2000, 0xef515824 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( journey )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "d2",           0x0000, 0x2000, 0xf2618913 )
	ROM_LOAD( "d3",           0x2000, 0x2000, 0x2f290d2e )
	ROM_LOAD( "d4",           0x4000, 0x2000, 0xcc6c0150 )
	ROM_LOAD( "d5",           0x6000, 0x2000, 0xc3023931 )
	ROM_LOAD( "d6",           0x8000, 0x2000, 0x5d445c99 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "a",            0x0000, 0x1000, 0x2524a2aa )
	ROM_LOAD( "b",            0x1000, 0x1000, 0xb8e35814 )
	ROM_LOAD( "c",            0x2000, 0x1000, 0x09c488cf )
	ROM_LOAD( "d",            0x3000, 0x1000, 0x3d627bee )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g3",           0x00000, 0x2000, 0xc14558de )
	ROM_LOAD( "g4",           0x02000, 0x2000, 0x9104c1d0 )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a7",           0x00000, 0x2000, 0x4ca2bb2d )
	ROM_LOAD( "a8",           0x02000, 0x2000, 0x4fb7925d )
	ROM_LOAD( "a5",           0x04000, 0x2000, 0x560c474f )
	ROM_LOAD( "a6",           0x06000, 0x2000, 0xb1f31583 )
	ROM_LOAD( "a3",           0x08000, 0x2000, 0xf295afda )
	ROM_LOAD( "a4",           0x0a000, 0x2000, 0x765876a7 )
	ROM_LOAD( "a1",           0x0c000, 0x2000, 0x4af986f8 )
	ROM_LOAD( "a2",           0x0e000, 0x2000, 0xb30cd2a7 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( mcr2 )
{
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_write_handler(0, 0x00, 0x00, mcr_control_port_w);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 0;
}


static DRIVER_INIT( domino )
{
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_write_handler(0, 0x01, 0x01, mcr_control_port_w);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 0;
}


static DRIVER_INIT( wacko )
{
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_write_handler(0, 0x04, 0x04, wacko_mux_select_w);
	install_port_read_handler(0, 0x01, 0x02, wacko_trackball_r);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 0;
}


static DRIVER_INIT( kroozr )
{
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_read_handler(0, 0x01, 0x01, kroozr_dial_r);
	install_port_read_handler(0, 0x02, 0x02, kroozr_trakball_x_r);
	install_port_read_handler(0, 0x04, 0x04, kroozr_trakball_y_r);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 0;
}


static DRIVER_INIT( twotigra )
{
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_write_handler(0, 0x00, 0x00, mcr_control_port_w);
	install_port_read_handler(0, 0x01, 0x01, twotigra_yoke2_r);
	install_port_read_handler(0, 0x02, 0x02, twotigra_yoke1_r);

	install_mem_read_handler(0, 0xf800, 0xffff, twotigra_videoram_r);
	install_mem_write_handler(0, 0xf800, 0xffff, twotigra_videoram_w);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 0;
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1981, shollow,  0,        mcr2,    shollow,  mcr2,     ROT90, "Bally Midway", "Satan's Hollow (set 1)" )
GAME( 1981, shollow2, shollow,  mcr2,    shollow,  mcr2,     ROT90, "Bally Midway", "Satan's Hollow (set 2)" )
GAME( 1982, tron,     0,        mcr2,    tron,     mcr2,     ROT90, "Bally Midway", "Tron (set 1)" )
GAME( 1982, tron2,    tron,     mcr2,    tron,     mcr2,     ROT90, "Bally Midway", "Tron (set 2)" )
GAME( 1982, kroozr,   0,        mcr2,    kroozr,   kroozr,   ROT0,  "Bally Midway", "Kozmik Kroozr" )
GAME( 1982, domino,   0,        mcr2,    domino,   domino,   ROT0,  "Bally Midway", "Domino Man" )
GAME( 1982, wacko,    0,        mcr2,    wacko,    wacko,    ROT0,  "Bally Midway", "Wacko" )
GAME( 1984, twotiger, 0,        mcr2,    twotiger, mcr2,     ROT0,  "Bally Midway", "Two Tigers" )
GAME( 1984, twotigra, twotiger, mcr2,    twotigra, twotigra, ROT0,  "Bally Midway", "Two Tigers (dedicated)" )
GAMEX(1983, journey,  0,        journey, domino,   mcr2,     ROT90, "Bally Midway", "Journey", GAME_IMPERFECT_SOUND )
