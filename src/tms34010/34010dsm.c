/*
 *   A TMS34010 disassembler
 *
 *   This code written by Zsolt Vasvari for the MAME project
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PARAM_WORD(v) ((v) = *(unsigned short *)&p[0], p += 2)
#define PARAM_LONG(v) ((v) = *(unsigned short *)&p[0] + (*(unsigned short *)&p[2] << 16), p += 4)

static char rf;
static long __pc;
static short op,rs,rd;

static char *buffer;
static unsigned char *p;
static char temp[20];


static void print_reg(short reg)
{
	if (reg != 0x0f)
	{
		sprintf(temp, "%c%d", rf, reg);
		strcat(buffer, temp);
	}
	else
	{
		strcat(buffer, "SP");
	}
}

static void print_src_reg(void)
{
	print_reg(rs);
}

static void print_des_reg(void)
{
	print_reg(rd);
}

static void print_src_des_reg(void)
{
    print_src_reg();
	strcat(buffer, ",");
    print_des_reg();
}

static void print_word_parm(void)
{
	unsigned short w;

	PARAM_WORD(w);

	sprintf(temp, "%Xh", w);
	strcat(buffer, temp);
}

static void print_word_parm_1s_comp(void)
{
	unsigned short w;

	PARAM_WORD(w);
	sprintf(temp, "%Xh", ~w & 0xffff);
	strcat(buffer, temp);
}

static void print_long_parm(void)
{
	unsigned long l;

	PARAM_LONG(l);
	sprintf(temp, "%lXh", l);
	strcat(buffer, temp);
}

static void print_long_parm_1s_comp(void)
{
	unsigned long l;

	PARAM_LONG(l);
	sprintf(temp, "%lXh", ~l);
	strcat(buffer, temp);
}

static void print_constant(void)
{
	short constant = (op >> 5) & 0x1f;

	sprintf(temp, "%Xh", constant);
	strcat(buffer, temp);
}

static void print_constant_1_32(void)
{
	short constant = (op >> 5) & 0x1f;
	if (!constant) constant = 0x20;

	sprintf(temp, "%Xh", constant);
	strcat(buffer, temp);
}

static void print_constant_1s_comp(void)
{
	short constant = (~op >> 5) & 0x1f;

	sprintf(temp, "%Xh", constant);
	strcat(buffer, temp);
}

static void print_constant_2s_comp(void)
{
	short constant = 32 - ((op >> 5) & 0x1f);

	sprintf(temp, "%Xh", constant);
	strcat(buffer, temp);
}

static void print_relative(void)
{
	unsigned short l;
	signed short ls;

	PARAM_WORD(l);
	ls = (signed short)l;

	sprintf(temp, "%lXh", __pc + 32 + (ls << 4));
	strcat(buffer, temp);
}

static void print_relative_8bit(void)
{
	signed char ls = (signed char)(op & 0xff);

	sprintf(temp, "%lXh", __pc + 16 + (ls << 4));
	strcat(buffer, temp);
}

static void print_relative_5bit(void)
{
	signed char ls = (signed char)((op >> 5) & 0x1f);
	if (op & 0x0400) ls = -ls;

	sprintf(temp, "%lXh", __pc + 16 + (ls << 4));
	strcat(buffer, temp);
}

static void print_field(void)
{
	sprintf(temp, "%c", (op & 0x200) ? '1' : '0');
	strcat(buffer, temp);
}

static void print_condition_code(void)
{
	switch (op & 0x0f00)
	{
	case 0x0000: strcat(buffer, "  "); break;  /* This is really UC (Unconditional) */
	case 0x0100: strcat(buffer, "P "); break;
	case 0x0200: strcat(buffer, "LS"); break;
	case 0x0300: strcat(buffer, "HI"); break;
	case 0x0400: strcat(buffer, "LT"); break;
	case 0x0500: strcat(buffer, "GE"); break;
	case 0x0600: strcat(buffer, "LE"); break;
	case 0x0700: strcat(buffer, "GT"); break;
	case 0x0800: strcat(buffer, "C "); break;
	case 0x0900: strcat(buffer, "NC"); break;
	case 0x0a00: strcat(buffer, "EQ"); break;
	case 0x0b00: strcat(buffer, "NE"); break;
	case 0x0c00: strcat(buffer, "V "); break;
	case 0x0d00: strcat(buffer, "NV"); break;
	case 0x0e00: strcat(buffer, "N "); break;
	case 0x0f00: strcat(buffer, "NN"); break;
	}
}

static void print_reg_list(int rev)
{
	unsigned short l,i;

	PARAM_WORD(l);

	for (i = 0; i  < 16; i++)
	{
		if (rev)
		{
			if (l & 0x8000)
			{
				strcat(buffer, ",");
				print_reg(i);
			}

			l <<= 1;
		}
		else
		{
			if (l & 0x01)
			{
				strcat(buffer, ",");
				print_reg(i);
			}

			l >>= 1;
		}
	}
}


int Dasm34010 (unsigned char *pBase, char *buff, int _pc)
{
	short bad = 0;
	short subop;
	p = pBase;

	__pc = _pc;
	buffer = buff;

	PARAM_WORD(op);

	subop = (op & 0x01e0);
	rs = (op >> 5) & 0x0f;		    /* Source register */
	rd =  op & 0x0f;		        /* Destination register */
	rf = ((op & 0x10) ? 'B' : 'A');	/* Register file */

	switch (op & 0xfe00)
	{
	case 0x0000:
		switch (subop)
		{
		case 0x0020:
			sprintf (buffer, "REV    ");
			print_des_reg();
			break;

		case 0x0100:
			sprintf (buffer, "EMU    ");
			break;

		case 0x0120:
			sprintf (buffer, "EXGPC  ");
			print_des_reg();
			break;

		case 0x0140:
			sprintf (buffer, "GETPC  ");
			print_des_reg();
			break;

		case 0x0160:
			sprintf (buffer, "JUMP   ");
			print_des_reg();
			break;

		case 0x0180:
			sprintf (buffer, "GETST  ");
			print_des_reg();
			break;

		case 0x01a0:
			sprintf (buffer, "PUTST  ");
			print_des_reg();
			break;

		case 0x01c0:
			sprintf (buffer, "POPST  ");
			break;

		case 0x01e0:
			sprintf (buffer, "PUSHST ");
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0200:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "NOP    ");
			break;

		case 0x0120:
			sprintf (buffer, "CLRC   ");
			break;

		case 0x0140:
			sprintf (buffer, "MOVB   @");
			print_long_parm();
			strcat(buffer, ",@");
			print_long_parm();
			break;

		case 0x0160:
			sprintf (buffer, "DINT   ");
			break;

		case 0x0180:
			sprintf (buffer, "ABS    ");
			print_des_reg();
			break;

		case 0x01a0:
			sprintf (buffer, "NEG    ");
			print_des_reg();
			break;

		case 0x01c0:
			sprintf (buffer, "NEGB   ");
			print_des_reg();
			break;

		case 0x01e0:
			sprintf (buffer, "NOT    ");
			print_des_reg();
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0400:
	case 0x0600:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "SEXT   ");
			print_des_reg();
			strcat(buffer, ",");
			print_field();
			break;

		case 0x0120:
			sprintf (buffer, "ZEXT   ");
			print_des_reg();
			strcat(buffer, ",");
			print_field();
			break;

		case 0x0140:
		case 0x0160:
			sprintf (buffer, "SETF   %Xh,%X,",
				     (op & 0x1f) ? op & 0x1f : 0x20,
					 (op >> 5) & 1);
			print_field();
			break;

		case 0x0180:
			sprintf (buffer, "MOVE   ");
			print_des_reg();
			strcat(buffer, ",@");
			print_long_parm();
			strcat(buffer, ",");
			print_field();
			break;

		case 0x01a0:
			sprintf (buffer, "MOVE   @");
			print_long_parm();
			strcat(buffer, ",");
			print_des_reg();
			strcat(buffer, ",");
			print_field();
			break;

		case 0x01c0:
			sprintf (buffer, "MOVE   @");
			print_long_parm();
			strcat(buffer, ",@");
			print_long_parm();
			strcat(buffer, ",");
			print_field();
			break;

		case 0x01e0:
			if (op & 0x200)
			{
				sprintf (buffer, "MOVE   @");
				print_long_parm();
				strcat(buffer, ",");
				print_des_reg();
			}
			else
			{
				sprintf (buffer, "MOVB   ");
				print_des_reg();
				strcat(buffer, ",@");
				print_long_parm();
			}
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0800:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "TRAP   %Xh", op & 0x1f);
			break;

		case 0x0120:
			sprintf (buffer, "CALL   ");
			print_des_reg();
			break;

		case 0x0140:
			sprintf (buffer, "RETI   ");
			break;

		case 0x0160:
			sprintf (buffer, "RETS   ");
			if (op & 0x1f)
			{
				sprintf(temp, "%Xh", op & 0x1f);
				strcat(buffer, temp);
			}
			break;

		case 0x0180:
			sprintf (buffer, "MMTM   ");
			print_des_reg();
			print_reg_list(1);
			break;

		case 0x01a0:
			sprintf (buffer, "MMFM   ");
			print_des_reg();
			print_reg_list(0);
			break;

		case 0x01c0:
			sprintf (buffer, "MOVI   ");
			print_word_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x01e0:
			sprintf (buffer, "MOVI   ");
			print_long_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0a00:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "ADDI   ");
            print_word_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x0120:
			sprintf (buffer, "ADDI   ");
            print_long_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x0140:
			sprintf (buffer, "CMPI   ");
            print_word_parm_1s_comp();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x0160:
			sprintf (buffer, "CMPI   ");
            print_long_parm_1s_comp();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x0180:
			sprintf (buffer, "ANDI   ");
            print_long_parm_1s_comp();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x01a0:
			sprintf (buffer, "ORI    ");
            print_long_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x01c0:
			sprintf (buffer, "XORI   ");
            print_long_parm();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x01e0:
			sprintf (buffer, "SUBI   ");
            print_word_parm_1s_comp();
			strcat(buffer, ",");
			print_des_reg();
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0c00:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "SUBI   ");
            print_long_parm_1s_comp();
			strcat(buffer, ",");
			print_des_reg();
			break;

		case 0x0120:
			sprintf (buffer, "CALLR  ");
			print_relative();
			break;

		case 0x0140:
			sprintf (buffer, "CALLA  ");
			print_long_parm();
			break;

		case 0x0160:
			sprintf (buffer, "EINT   ");
			break;

		case 0x0180:
			sprintf (buffer, "DSJ    ");
			print_des_reg();
			strcat(buffer, ",");
			print_relative();
			break;

		case 0x01a0:
			sprintf (buffer, "DSJEQ  ");
			print_des_reg();
			strcat(buffer, ",");
			print_relative();
			break;

		case 0x01c0:
			sprintf (buffer, "DSJNE  ");
			print_des_reg();
			strcat(buffer, ",");
			print_relative();
			break;

		case 0x01e0:
			sprintf (buffer, "SETC   ");
			break;

		default:
			bad = 1;
		}
		break;


	case 0x0e00:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "PIXBLT L,L");
			break;

		case 0x0120:
			sprintf (buffer, "PIXBLT L,XY");
			break;

		case 0x0140:
			sprintf (buffer, "PIXBLT XY,L");
			break;

		case 0x0160:
			sprintf (buffer, "PIXBLT XY,XY");
			break;

		case 0x0180:
			sprintf (buffer, "PIXBLT B,L");
			break;

		case 0x01a0:
			sprintf (buffer, "PIXBLT B,XY");
			break;

		case 0x01c0:
			sprintf (buffer, "FILL   L");
			break;

		case 0x01e0:
			sprintf (buffer, "FILL   XY");
			break;

		default:
			bad = 1;
		}
		break;


	case 0x1000:
	case 0x1200:
		if ((op & 0x03e0) != 0x0020)
		{
			sprintf (buffer, "ADDK   ");
			print_constant_1_32();
			strcat(buffer, ",");
		}
		else
		{
			sprintf (buffer, "INC    ");
		}
		print_des_reg();

		break;


	case 0x1400:
	case 0x1600:
		if ((op & 0x03e0) != 0x0020)
		{
			sprintf (buffer, "SUBK   ");
			print_constant_1_32();
			strcat(buffer, ",");
		}
		else
		{
			sprintf (buffer, "DEC    ");
		}
		print_des_reg();

		break;


	case 0x1800:
	case 0x1a00:
		sprintf (buffer, "MOVK   ");
		print_constant_1_32();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x1c00:
	case 0x1e00:
		sprintf (buffer, "BTST   ");
		print_constant_1s_comp();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x2000:
	case 0x2200:
		sprintf (buffer, "SLA    ");
		print_constant();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x2400:
	case 0x2600:
		sprintf (buffer, "SLL    ");
		print_constant();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x2800:
	case 0x2a00:
		sprintf (buffer, "SRA    ");
		print_constant_2s_comp();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x2c00:
	case 0x2e00:
		sprintf (buffer, "SRL    ");
		print_constant_2s_comp();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x3000:
	case 0x3200:
		sprintf (buffer, "RL     ");
		print_constant();
		strcat(buffer, ",");
		print_des_reg();
		break;


	case 0x3800:
	case 0x3a00:
	case 0x3c00:
	case 0x3e00:
		sprintf (buffer, "DSJS   ");
		print_des_reg();
		strcat(buffer, ",");
		print_relative_5bit();
		break;


	case 0x4000:
		sprintf (buffer, "ADD    ");
		print_src_des_reg();
		break;


	case 0x4200:
		sprintf (buffer, "ADDC   ");
		print_src_des_reg();
		break;


	case 0x4400:
		sprintf (buffer, "SUB    ");
		print_src_des_reg();
		break;


	case 0x4600:
		sprintf (buffer, "SUBB   ");
		print_src_des_reg();
		break;


	case 0x4800:
		sprintf (buffer, "CMP    ");
		print_src_des_reg();
		break;


	case 0x4a00:
		sprintf (buffer, "BTST   ");
		print_src_des_reg();
		break;


	case 0x4c00:
	case 0x4e00:
		sprintf (buffer, "MOVE   ");

		if (!(op & 0x0200))
		{
			print_src_des_reg();
		}
		else
		{
			print_src_reg();
			strcat(buffer, ",");

			if (rf == 'A')
			{
				rf = 'B';
			}
			else
			{
				rf = 'A';
			}

			print_des_reg();
		}
		break;


	case 0x5000:
		sprintf (buffer, "AND    ");
		print_src_des_reg();
		break;


	case 0x5200:
		sprintf (buffer, "ANDN   ");
		print_src_des_reg();
		break;


	case 0x5400:
		sprintf (buffer, "OR     ");
		print_src_des_reg();
		break;


	case 0x5600:
		if (rs != rd)
		{
			sprintf (buffer, "XOR    ");
			print_src_des_reg();
		}
		else
		{
			sprintf (buffer, "CLR    ");
			print_des_reg();
		}
		break;


	case 0x5800:
		sprintf (buffer, "DIVS   ");
		print_src_des_reg();
		break;


	case 0x5a00:
		sprintf (buffer, "DIVU   ");
		print_src_des_reg();
		break;


	case 0x5c00:
		sprintf (buffer, "MPYS   ");
		print_src_des_reg();
		break;


	case 0x5e00:
		sprintf (buffer, "MPYU   ");
		print_src_des_reg();
		break;


	case 0x6000:
		sprintf (buffer, "SLA    ");
		print_src_des_reg();
		break;


	case 0x6200:
		sprintf (buffer, "SLL    ");
		print_src_des_reg();
		break;


	case 0x6400:
		sprintf (buffer, "SRA    ");
		print_src_des_reg();
		break;


	case 0x6600:
		sprintf (buffer, "SRL    ");
		print_src_des_reg();
		break;


	case 0x6800:
		sprintf (buffer, "RL     ");
		print_src_des_reg();
		break;


	case 0x6a00:
		sprintf (buffer, "LMO    ");
		print_src_des_reg();
		break;


	case 0x6c00:
		sprintf (buffer, "MODS   ");
		print_src_des_reg();
		break;


	case 0x6e00:
		sprintf (buffer, "MODU   ");
		print_src_des_reg();
		break;


	case 0x8000:
	case 0x8200:
		sprintf (buffer, "MOVE   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0x8400:
	case 0x8600:
		sprintf (buffer, "MOVE   *");
		print_src_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0x8800:
	case 0x8a00:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0x8c00:
		sprintf (buffer, "MOVB   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		break;


	case 0x8e00:
		sprintf (buffer, "MOVB   *");
		print_src_des_reg();
		break;


	case 0x9000:
	case 0x9200:
		sprintf (buffer, "MOVE   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, "+,");
		print_field();
		break;


	case 0x9400:
	case 0x9600:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, "+,");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0x9800:
	case 0x9a00:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, "+,*");
		print_des_reg();
		strcat(buffer, "+,");
		print_field();
		break;


	case 0x9c00:
		sprintf (buffer, "MOVB   *");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		break;


	case 0xa000:
	case 0xa200:
		sprintf (buffer, "MOVE   ");
		print_src_reg();
		strcat(buffer, ",-*");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0xa400:
	case 0xa600:
		sprintf (buffer, "MOVE   -*");
		print_src_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0xa800:
	case 0xaa00:
		sprintf (buffer, "MOVE   -*");
		print_src_reg();
		strcat(buffer, ",-*");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0xac00:
		sprintf (buffer, "MOVB   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, ")");
		break;


	case 0xae00:
		sprintf (buffer, "MOVB   *");
		print_src_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),");
		print_des_reg();
		break;


	case 0xb000:
	case 0xb200:
		sprintf (buffer, "MOVE   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),");
		print_field();
		break;


	case 0xb400:
	case 0xb600:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),");
		print_des_reg();
		strcat(buffer, ",");
		print_field();
		break;


	case 0xb800:
	case 0xba00:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),*");
		print_des_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),");
		print_field();
		break;


	case 0xbc00:
		sprintf (buffer, "MOVB   *");
		print_src_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),*");
		print_des_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, ")");
		break;


	case 0xc000:
	case 0xc200:
	case 0xc400:
	case 0xc600:
	case 0xc800:
	case 0xca00:
	case 0xcc00:
	case 0xce00:
		if ((op & 0x00ff) == 0x80)
		{
			sprintf (buffer, "JA");
		}
		else
		{
			sprintf (buffer, "JR");
		}

		print_condition_code();
		strcat (buffer, "   ");

		switch (op & 0x00ff)
		{
		case 0x00:
			print_relative();
			break;

		case 0x80:
			print_long_parm();
			break;

		default:
			print_relative_8bit();
		}
		break;


	case 0xd000:
	case 0xd200:
		sprintf (buffer, "MOVE   *");
		print_src_reg();
		strcat(buffer, "(");
		print_word_parm();
		strcat(buffer, "),*");
		print_des_reg();
		strcat(buffer, "+,");
		print_field();
		break;


	case 0xd400:
	case 0xd600:
		switch (subop)
		{
		case 0x0000:
			sprintf (buffer, "MOVE   @");
			print_long_parm();
			strcat(buffer, ",*");
			print_des_reg();
			strcat(buffer, "+,");
			print_field();
			break;

		case 0x0100:
			sprintf (buffer, "EXGF   ");
			print_des_reg();
			strcat(buffer, ",");
			print_field();
			break;

		default:
			bad = 1;
		}
		break;


	case 0xde00:
		switch (subop)
		{
		case 0x0100:
			sprintf (buffer, "LINE   0");
			break;

		case 0x0180:
			sprintf (buffer, "LINE   1");
			break;

		default:
			bad = 1;
		}
		break;

	case 0xe000:
		sprintf (buffer, "ADDXY  ");
		print_src_des_reg();
		break;


	case 0xe200:
		sprintf (buffer, "SUBXY  ");
		print_src_des_reg();
		break;


	case 0xe400:
		sprintf (buffer, "CMPXY  ");
		print_src_des_reg();
		break;


	case 0xe600:
		sprintf (buffer, "CPW    ");
		print_src_des_reg();
		break;


	case 0xe800:
		sprintf (buffer, "CVXYL  ");
		print_src_des_reg();
		break;


	case 0xec00:
		sprintf (buffer, "MOVX   ");
		print_src_des_reg();
		break;


	case 0xee00:
		sprintf (buffer, "MOVY   ");
		print_src_des_reg();
		break;


	case 0xf000:
		sprintf (buffer, "PIXT   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		strcat(buffer, ",XY");
		break;


	case 0xf200:
		sprintf (buffer, "PIXT   *");
		print_src_reg();
		strcat(buffer, ",XY,");
		print_des_reg();
		break;


	case 0xf400:
		sprintf (buffer, "PIXT   *");
		print_src_reg();
		strcat(buffer, ",XY,*");
		print_des_reg();
		strcat(buffer, ",XY");
		break;


	case 0xf600:
		sprintf (buffer, "DRAV   ");
		print_src_des_reg();
		break;


	case 0xf800:
		sprintf (buffer, "PIXT   ");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		break;


	case 0xfa00:
		sprintf (buffer, "PIXT   *");
		print_src_des_reg();
		break;


	case 0xfc00:
		sprintf (buffer, "PIXT   *");
		print_src_reg();
		strcat(buffer, ",*");
		print_des_reg();
		break;

	default:
		bad = 1;
	}

	if (bad)
	{
		sprintf (buffer, "DW     %04Xh", op & 0xffff);
	}

	return (p - pBase)<<3;
}