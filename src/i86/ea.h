static unsigned EA;
static unsigned EO; /* HJB 12/13/98 effective offset of the address (before segment is added) */

static unsigned EA_000(void) { cycle_count-=7; EO=(WORD)(regs.w[BX]+regs.w[SI]); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_001(void) { cycle_count-=8; EO=(WORD)(regs.w[BX]+regs.w[DI]); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_002(void) { cycle_count-=8; EO=(WORD)(regs.w[BP]+regs.w[SI]); EA=(DefaultBase(SS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_003(void) { cycle_count-=7; EO=(WORD)(regs.w[BP]+regs.w[DI]); EA=(DefaultBase(SS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_004(void) { cycle_count-=5; EO=regs.w[SI]; EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_005(void) { cycle_count-=5; EO=regs.w[DI]; EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_006(void) { cycle_count-=6; EO=FETCHOP; EO+=FETCHOP<<8; EA=(EO+DefaultBase(DS)) & i86_AddrMask; return EA; }
static unsigned EA_007(void) { cycle_count-=5; EO=regs.w[BX]; EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }

static unsigned EA_100(void) { cycle_count-=11; EO=(WORD)(regs.w[BX]+regs.w[SI]+(INT8)FETCHOP); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_101(void) { cycle_count-=12; EO=(WORD)(regs.w[BX]+regs.w[DI]+(INT8)FETCHOP); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_102(void) { cycle_count-=12; EO=(WORD)(regs.w[BP]+regs.w[SI]+(INT8)FETCHOP); EA=(DefaultBase(SS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_103(void) { cycle_count-=11; EO=(WORD)(regs.w[BP]+regs.w[DI]+(INT8)FETCHOP); EA=(DefaultBase(SS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_104(void) { cycle_count-=9; EO=(WORD)(regs.w[SI]+(INT8)FETCHOP); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_105(void) { cycle_count-=9; EO=(WORD)(regs.w[DI]+(INT8)FETCHOP); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_106(void) { cycle_count-=9; EO=(WORD)(regs.w[BP]+(INT8)FETCHOP); EA=(DefaultBase(SS)+EO) & i86_AddrMask; return EA; }
static unsigned EA_107(void) { cycle_count-=9; EO=(WORD)(regs.w[BX]+(INT8)FETCHOP); EA=(DefaultBase(DS)+EO) & i86_AddrMask; return EA; }

static unsigned EA_200(void) { cycle_count-=11; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BX]+regs.w[SI]; EA=(DefaultBase(DS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_201(void) { cycle_count-=12; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BX]+regs.w[DI]; EA=(DefaultBase(DS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_202(void) { cycle_count-=12; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BP]+regs.w[SI]; EA=(DefaultBase(SS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_203(void) { cycle_count-=11; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BP]+regs.w[DI]; EA=(DefaultBase(SS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_204(void) { cycle_count-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[SI]; EA=(DefaultBase(DS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_205(void) { cycle_count-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[DI]; EA=(DefaultBase(DS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_206(void) { cycle_count-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BP]; EA=(DefaultBase(SS)+(WORD)EO) & i86_AddrMask; return EA; }
static unsigned EA_207(void) { cycle_count-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=regs.w[BX]; EA=(DefaultBase(DS)+(WORD)EO) & i86_AddrMask; return EA; }

static unsigned (*GetEA[192])(void)={
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,

	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,

	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207
};
