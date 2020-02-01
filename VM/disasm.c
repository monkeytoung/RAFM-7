/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 逆アセンブラ ]
 */

#include <string.h>
#include <assert.h>
#include "xm7.h"

/*
 *	スタティック ワーク
 */
static int cputype;					/* CPU種別 */
static BYTE opc;					/* オペコード */
static WORD pc;						/* 実行前PC */
static WORD addpc;					/* PC加算値(命令長) */
static char linebuf[32];			/* 逆アセンブル出力バッファ */

/*
 *	例外系１(0x00)テーブル
 */
static char *except1_tbl[] = {
	"NEG   ",
	"NEG   ",
	"NGC   ",
	"COM   ",
	"LSR   ",
	"LSR   ",
	"ROR   ",
	"ASR   ",
	"LSL   ",
	"ROL   ",
	"DEC   ",
	"DCC   ",
	"INC   ",
	"TST   ",
	"JMP   ",
	"CLR   "
};

/*
 *	例外系２(0x10)テーブル
 */
static char *except2_tbl[] = {
	NULL,
	NULL,
	"NOP   ",
	"SYNC  ",
	"HALT  ",
	"HALT  ",
	"LBRA  ",
	"LBSR  ",
	"ASLCC ",
	"DAA   ",
	"ORCC  ",
	"NOP   ",
	"ANDCC ",
	"SEX   ",
	"EXG   ",
	"TFR   "
};

/*
 *	ブランチ系(0x20)テーブル
 */
static char *branch_tbl[] = {
	"BRA   ",
	"BRN   ",
	"BHI   ",
	"BLS   ",
	"BCC   ",
	"BCS   ",
	"BNE   ",
	"BEQ   ",
	"BVC   ",
	"BVS   ",
	"BPL   ",
	"BMI   ",
	"BGE   ",
	"BLT   ",
	"BGT   ",
	"BLE   "
};

/*
 *	ロングブランチ系(0x10 0x20)テーブル
 */
static char *lbranch_tbl[] = {
	NULL,
	"LBRN  ",
	"LBHI  ",
	"LBLS  ",
	"LBCC  ",
	"LBCS  ",
	"LBNE  ",
	"LBEQ  ",
	"LBVC  ",
	"LBVS  ",
	"LBPL  ",
	"LBMI  ",
	"LBGE  ",
	"LBLT  ",
	"LBGT  ",
	"LBLE  "
};

/*
 *	LEA、スタック系(0x30)テーブル
 */
static char *leastack_tbl[] = {
	"LEAX  ",
	"LEAY  ",
	"LEAS  ",
	"LEAU  ",
	"PSHS  ",
	"PULS  ",
	"PSHU  ",
	"PULU  ",
	"ANDCC ",
	"RTS   ",
	"ABX   ",
	"RTI   ",
	"CWAI  ",
	"MUL   ",
	"RST   ",
	"SWI   "
};

/*
 *	インヘレントＡ(0x40)テーブル
 */
static char *inha_tbl[] = {
	"NEGA  ",
	"NEGA  ",
	"NGCA  ",
	"COMA  ",
	"LSRA  ",
	"LSRA  ",
	"RORA  ",
	"ASRA  ",
	"LSLA  ",
	"ROLA  ",
	"DECA  ",
	"DCCA  ",
	"INCA  ",
	"TSTA  ",
	"CLCA  ",
	"CLRA  "
};

/*
 *	インヘレントＢ(0x50)テーブル
 */
static char *inhb_tbl[] = {
	"NEGB  ",
	"NEGB  ",
	"NGCB  ",
	"COMB  ",
	"LSRB  ",
	"LSRB  ",
	"RORB  ",
	"ASRB  ",
	"LSLB  ",
	"ROLB  ",
	"DECB  ",
	"DCCB  ",
	"INCB  ",
	"TSTB  ",
	"CLCB  ",
	"CLRB  "
};

/*
 *	インヘレントＭ(0x60, 0x70)テーブル
 */
static char *inhm_tbl[] = {
	"NEG   ",
	NULL,
	NULL,
	"COM   ",
	"LSR   ",
	NULL,
	"ROR   ",
	"ASR   ",
	"LSL   ",
	"ROL   ",
	"DEC   ",
	"DCC   ",
	"INC   ",
	"TST   ",
	NULL,
	"CLR   "
};

/*
 *	Ａレジスタ、Ｘレジスタ系(0x80)テーブル
 */
static char *regax_tbl[] = {
	"SUBA  ",
	"CMPA  ",
	"SBCA  ",
	"SUBD  ",
	"ANDA  ",
	"BITA  ",
	"LDA   ",
	"STA   ",
	"EORA  ",
	"ADCA  ",
	"ORA   ",
	"ADDA  ",
	"CMPX  ",
	"JSR   ",
	"LDX   ",
	"STX   "
};

/*
 *	Ｂレジスタ、Ｄレジスタ、Ｕレジスタ系(0xc0)テーブル
 */
static char *regbdu_tbl[] = {
	"SUBB  ",
	"CMPB  ",
	"SBCB  ",
	"ADDD  ",
	"ANDB  ",
	"BITB  ",
	"LDB   ",
	"STB   ",
	"EORB  ",
	"ADCB  ",
	"ORB   ",
	"ADDB  ",
	"LDD   ",
	"STD   ",
	"LDU   ",
	"STU   "
};

/*
 *	TFR/EXGテーブル
 */
static char *tfrexg_tbl[] = {
	"D",
	"X",
	"Y",
	"U",
	"S",
	"PC",
	NULL,
	NULL,
	"A",
	"B",
	"CC",
	"DP",
	NULL,
	NULL
};

/*
 *	PSH/PULテーブル
 */
static char *pshpul_tbl[] = {
	"CC",
	"A",
	"B",
	"DP",
	"X",
	"Y",
	"S/U",
	"PC"
};

/*
 *	インデックステーブル
 */
static char *idx_tbl[] = {
	"X",
	"Y",
	"U",
	"S"
};

/*-[ 汎用サブ ]-------------------------------------------------------------*/

/*
 *	データフェッチ
 */
static BYTE FASTCALL fetch(void)
{
	BYTE dat;

	switch (cputype) {
		case MAINCPU:
			dat = mainmem_readbnio((WORD)(pc + addpc));
			addpc++;
			break;
		case SUBCPU:
			dat = submem_readbnio((WORD)(pc + addpc));
			addpc++;
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	return dat;
}

/*
 *	データ読み出し
 */
static BYTE FASTCALL read_byte(WORD addr)
{
	BYTE dat;

	switch (cputype) {
		case MAINCPU:
			dat = mainmem_readbnio(addr);
			break;
		case SUBCPU:
			dat = submem_readbnio(addr);
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	return dat;
}

/*
 *	１６進１桁セット サブ
 */
static void FASTCALL sub1hex(BYTE dat, char *buffer)
{
	char buf[2];

	/* assert */
	ASSERT(buffer);

	buf[0] = (char)(dat + 0x30);
	if (dat > 9) {
		buf[0] = (char)(dat + 0x37);
	}

	buf[1] = '\0';
	strcat(buffer, buf);
}

/*
 *	１６進２桁セット サブ
 */
static void FASTCALL sub2hex(BYTE dat, char *buffer)
{
	sub1hex((BYTE)(dat >> 4), buffer);
	sub1hex((BYTE)(dat & 0x0f), buffer );
}

/*
 *	１６進２桁セット
 */
static void FASTCALL set2hex(BYTE dat)
{
	strcat(linebuf, "$");

	sub2hex(dat, linebuf);
}

/*
 *	１６進４桁セット サブ
 */
static void FASTCALL sub4hex(WORD dat, char *buffer)
{
	sub2hex((BYTE)(dat >> 8), buffer);
	sub2hex((BYTE)(dat & 0xff), buffer);
}

/*
 *	１６進４桁セット
 */
static void FASTCALL set4hex(WORD dat)
{
	strcat(linebuf, "$");

	sub2hex((BYTE)(dat >> 8), linebuf);
	sub2hex((BYTE)(dat & 0xff), linebuf);
}

/*
 *	１０進２桁セット
 */
static void FASTCALL set2dec(BYTE dat)
{
	char buf[2];

	buf[1] = '\0';

	/* 上位桁 */
	buf[0] = (char)(dat / 10);
	if (buf[0] > 0) {
		buf[0] += (char)0x30;
		strcat(linebuf, buf);
	}

	/* 下位桁 */
	buf[0] = (char)(dat % 10);
	buf[0] += (char)0x30;
	strcat(linebuf, buf);
}

/*
 *	未定義
 */
static void FASTCALL notdef(void)
{
	strcat(linebuf, "?");
}

/*-[ アドレッシングモード処理 ]---------------------------------------------*/

/*
 *	リラティブモード(１バイト)
 */
static void FASTCALL rel1(void)
{
	BYTE opr;

	/* オペランド取得 */
	opr = fetch();

	/* セット */
	if (opr <= 0x7f) {
		set4hex((WORD)(pc + addpc + (BYTE)opr));
	}
	else {
		set4hex((WORD)(pc + addpc - (BYTE)(~opr + 1)));
	}
}

/*
 *	リラティブモード(２バイト)
 */
static void FASTCALL rel2(void)
{
	WORD dat;

	/* オペランド取得 */
	dat = (WORD)(fetch() << 8);
	dat |= (WORD)fetch();

	/* セット */
	if (dat <= 0x7fff) {
		set4hex((WORD)(pc + addpc + dat));
	}
	else {
		set4hex((WORD)(pc + addpc - (~dat + 1)));
	}
}

/*
 *	ダイレクトモード
 */
static void FASTCALL direct(void)
{
	BYTE opr;
	
	/* オペランド取得 */
	opr = fetch();

	/* セット */
	strcat(linebuf, "<");
	set2hex(opr);
}

/*
 *	エクステンドモード
 */
static void FASTCALL extend(void)
{
	WORD dat;

	/* オペランド取得 */
	dat = (WORD)(fetch() << 8);
	dat |= (WORD)fetch();

	/* セット */
	set4hex(dat);
}

/*
 *	イミディエイトモード(１バイト)
 */
static void FASTCALL imm1(void)
{
	BYTE opr;

	/* オペランド取得 */
	opr = fetch();

	/* セット */
	strcat(linebuf, "#");
	set2hex(opr);
}

/*
 *	イミディエイトモード(２バイト)
 */
static void FASTCALL imm2(void)
{
	WORD dat;

	/* オペランド取得 */
	dat = (WORD)(fetch() << 8);
	dat |= (WORD)(fetch());

	/* セット */
	strcat(linebuf, "#");
	set4hex(dat);
}

/*
 *	インデックスモード
 */
static void FASTCALL idx(void)
{
	BYTE opr;
	BYTE high, low;
	BYTE offset;
	WORD woffset;

	/* オペランド取得 */
	opr = fetch();
	high = (BYTE)(opr & 0xf0);
	low = (BYTE)(opr & 0x0f);

	/* 0x00～0x7fは5bitオフセット */
	if (opr < 0x80) {
		if (opr & 0x10) {
			/* マイナス */
			offset = (BYTE)(~((opr & 0x0f) | 0xf0) + 1);
			strcat(linebuf, "-");
			set2dec(offset);
		}
		else {
			/* プラス */
			offset = low;
			set2dec(offset);
		}

		strcat(linebuf, ",");

		/* X, Y, U, S */
		offset = (BYTE)((opr & 0x60) >> 5);
		ASSERT(offset <= 3);
		strcat(linebuf, idx_tbl[offset]);
		return;
	}

	/* 0x80以上で、下位が0,1,2,3はオートインクリメントorデクリメント */
	if (low < 4) {
		if (high & 0x10) {
			if ((low == 0) || (low == 2)) {
				notdef();
				return;
			}
			strcat(linebuf, "[");
		}
		strcat(linebuf, ",");

		/* オートデクリメント */
		if (low >= 2) {
			strcat(linebuf, "-");
		}
		if (low == 3) {
			strcat(linebuf, "-");
		}

		/* X, Y, U, S */
		offset = (BYTE)((opr & 0x60) >> 5);
		ASSERT(offset <= 3);
		strcat(linebuf, idx_tbl[offset]);

		/* オートインクリメント */
		if (low < 2) {
			strcat(linebuf, "+");
		}
		if (low == 1) {
			strcat(linebuf, "+");
		}

		if (high & 0x10) {
			strcat(linebuf, "]");
		}
		return;
	}

	/* 下位4,5,6,Bはレジスタオフセット */
	if ((low == 4) || (low == 5) || (low == 6) || (low == 11)) {
		if (high & 0x10) {
			strcat(linebuf, "[");
		}

		switch (low) {
			case 4:
				strcat(linebuf, ",");
				break;
			case 5:
				strcat(linebuf, "B,");
				break;
			case 6:
				strcat(linebuf, "A,");
				break;
			case 11:
				strcat(linebuf, "D,");
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		/* X, Y, U, S */
		offset = (BYTE)((opr & 0x60) >> 5);
		ASSERT(offset <= 3);
		strcat(linebuf, idx_tbl[offset]);

		if (high & 0x10) {
			strcat(linebuf, "]");
		}
		return;
	}

	/* 下位8,Cは8bitオフセット */
	if ((low == 8) || (low == 12)) {
		if (high & 0x10) {
			strcat(linebuf, "[");
		}

		offset = fetch();

		/* X, Y, U, S, PCR */
		if (low == 12) {
			if (offset >= 0x80) {
				woffset = (WORD)(0xff00 + offset);
			}
			else {
				woffset = offset;
			}
			set4hex((WORD)(pc + addpc + woffset));
			strcat(linebuf, ",");
			strcat(linebuf, "PCR");
		}
		else {
			if (offset >= 0x80) {
				offset = (BYTE)(~offset + 1);
				strcat(linebuf, "-");
			}
			set2hex(offset);
			strcat(linebuf, ",");
			offset = (BYTE)((opr & 0x60) >> 5);
			ASSERT(offset <= 3);
			strcat(linebuf, idx_tbl[offset]);
		}

		if (high & 0x10) {
			strcat(linebuf, "]");
		}
		return;
	}

	/* 下位9,Dは16bitオフセット */
	if ((low == 9) || (low == 13)) {
		if (high & 0x10) {
			strcat(linebuf, "[");
		}

		woffset = (WORD)fetch();
		woffset = (WORD)((woffset << 8) + (WORD)fetch());

		/* X, Y, U, S, PCR */
		if (low == 13) {
			set4hex((WORD)(woffset + pc + addpc));
			strcat(linebuf, ",");
			strcat(linebuf, "PCR");
		}
		else {
			if (woffset >= 0x8000) {
				woffset = (WORD)(~woffset + 1);
				strcat(linebuf, "-");
			}
			set4hex(woffset);
			strcat(linebuf, ",");
			offset = (BYTE)((opr & 0x60) >> 5);
			ASSERT(offset <= 3);
			strcat(linebuf, idx_tbl[offset]);
		}

		if (high & 0x10) {
			strcat(linebuf, "]");
		}
		return;
	}

	/* 0x9f,0xbf,0xdf,0xffは例外[addr] */
	if ((opr == 0x9f) || (opr == 0xbf) || (opr == 0xdf) || (opr == 0xff)) {
		strcat(linebuf, "[");
		woffset = (WORD)fetch();
		woffset = (WORD)((woffset << 8) + (WORD)fetch());
		set4hex(woffset);
		strcat(linebuf, "]");
		return;
	}

	/* それ以外は未定義 */
	notdef();
}

/*
 *	TFR,EXG
 */
static void FASTCALL tfrexg(void)
{
	BYTE opr;

	/* オペランド取得 */
	opr = fetch();

	/* 作成 */
	if (tfrexg_tbl[(opr & 0xf0) >> 4] == NULL) {
		linebuf[0] = '\0';
		notdef();
		return;
	}
	strcat(linebuf, tfrexg_tbl[(opr & 0xf0) >> 4]);
	strcat(linebuf, ",");
	if (tfrexg_tbl[opr & 0x0f] == NULL) {
		linebuf[0] = '\0';
		notdef();
		return;
	}
 	strcat(linebuf, tfrexg_tbl[opr & 0x0f]);
}

/*
 *	PSH,PUL
 */
static void FASTCALL pshpul(void)
{
	BYTE opr;
	char sreg[2];
	int i;
	int flag;

	/* オペランド取得 */
	opr = fetch();

	/* S,Uを決定する */
	if (linebuf[3] == 'S') {
		sreg[0] = 'U';
	}
	else {
		sreg[0] = 'S';
	}
	sreg[1] = '\0';

	/* 8回評価 */
	flag = FALSE;
	for (i=0; i<8; i++) {
		if (opr & 0x01) {
			if (flag == TRUE) {
				strcat(linebuf, ",");
			}
			if (i == 6) {
				/* S,U */
				strcat(linebuf, sreg);
			}
			else {
				/* それ以外 */
				strcat(linebuf, pshpul_tbl[i]);
			}
			flag = TRUE;
		}
		opr >>= 1;
	}
}

/*-[ オペコード処理 ]-------------------------------------------------------*/

/*
 *	ページ２(0x10)
 */
static void FASTCALL page2(void)
{
	BYTE high, low;

	/* オペコードを再度取得 */
	opc = fetch();
	high = (BYTE)(opc & 0xf0);
	low = (BYTE)(opc & 0x0f);

	/* 0x3fはSWI2 */
	if (opc == 0x3f) {
		strcat(linebuf, "SWI2  ");
		return;
	}

	/* 0x20台はロングブランチ */
	if (high == 0x20) {
		/* 未定義チェック */
		if (lbranch_tbl[low] == NULL) {
			notdef();
			return;
		}
		strcat(linebuf, lbranch_tbl[low]);
		rel2();
		return;
	}

	/* 0xc0以上はSレジスタ */
	if (opc >= 0xc0) {
		if (low <= 0x0d) {
			notdef();
			return;
		}
		if (opc == 0xcf) {
			notdef();
			return;
		}
		/* 0x?eはLDS、0x?fはSTS */
		if (low == 0x0e) {
			strcat(linebuf, "LDS   ");
		}
		if (low == 0x0f) {
			strcat(linebuf, "STS   ");
		}
		/* LDS,STS アドレッシングモード */
		switch (high) {
			case 0xc0:
				imm2();
				break;
			case 0xd0:
				direct();
				break;
			case 0xe0:
				idx();
				break;
			case 0xf0:
				extend();
				break;
			default:
				ASSERT(FALSE);
				break;
		}
		return;
	}

	/* 0x80以上はCMPD, CMPY, LDY, STY */
	if (opc >= 0x80) {
		switch (low) {
			case 0x03:
				strcat(linebuf, "CMPD  ");
				break;
			case 0x0c:
				strcat(linebuf, "CMPY  ");
				break;
			case 0x0e:
				strcat(linebuf, "LDY   ");
				break;
			case 0x0f:
				if (high == 0x80) {
					notdef();
					return;
				}
				else {
					strcat(linebuf, "STY   ");
				}
				break;
			default:
				notdef();
				return;
		}

		/* アドレッシングモード */
		switch (high) {
			case 0x80:
				imm2();
				break;
			case 0x90:
				direct();
				break;
			case 0xa0:
				idx();
				break;
			case 0xb0:
				extend();
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		return;
	}

	/* それ以外は未定義 */
	notdef();
}

/*
 *	ページ３(0x11)
 */
static void FASTCALL page3(void)
{
	BYTE high, low;

	/* オペコードを再度取得 */
	opc = fetch();
	high = (BYTE)(opc & 0xf0);
	low = (BYTE)(opc & 0x0f);

	/* 0x3fはSWI3 */
	if (opc == 0x3f) {
		strcat(linebuf, "SWI3  ");
		return;
	}

	/* 上位が8,9,A,B */
	if ((high >= 0x80) && (high <= 0xb0)) {
		/* 下位チェック */
		switch (low) {
			case 3:
				strcat(linebuf, "CMPU  ");
				break;
			case 12:
				strcat(linebuf, "CMPS  ");
				break;
			default:
				notdef();
				return;
		}

		/* アドレッシングモード */
		switch (high) {
			case 0x80:
				imm2();
				break;
			case 0x90:
				direct();
				break;
			case 0xa0:
				idx();
				break;
			case 0xb0:
				extend();
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		return;
	}

	/* それ以外は未定義 */
	notdef();
}

/*
 *	例外系１(0x00)
 */
static void FASTCALL except1(void)
{
	/* 未定義チェック */
	if (except1_tbl[opc & 0x0f] == NULL) {
		notdef();
		return;
	}
	strcat(linebuf, except1_tbl[opc & 0x0f]);

	/* すべてダイレクト */
	direct();
}

/*
 *	例外系２(0x10)
 */
static void FASTCALL except2(void)
{
	/* 0x10, 0x11で始まるページ */
	if (opc == 0x10) {
		page2();
		return;
	}
	if (opc == 0x11) {
		page3();
		return;
	}

	/* 未定義チェック */
	if (except2_tbl[opc & 0x0f] == NULL) {
		notdef();
		return;
	}
	strcat(linebuf, except2_tbl[opc & 0x0f]);

	/* 0x16, 0x17はロングブランチ */
	if ((opc == 0x16) || (opc == 0x17)) {
		rel2();
		return;
	}

	/* 0x1a, 0x1cはイミディエイト */
	if ((opc == 0x1a) || (opc == 0x1c)) {
		imm1();
		return;
	}

	/* 0x1e, 0x1fはTFR/EXG */
	if ((opc == 0x1e) || (opc == 0x1f)) {
		tfrexg();
		return;
	}
}

/*
 *	ブランチ系(0x20)
 */
static void FASTCALL branch(void)
{
	strcat(linebuf, branch_tbl[opc - 0x20]);
	rel1();
}

/*
 *	LEA,スタック系(0x30)
 */
static void FASTCALL leastack(void)
{
	/* オペコード */
	if (leastack_tbl[opc & 0x0f] == NULL) {
		notdef();
		return;
	}
	strcat(linebuf, leastack_tbl[opc & 0x0f]);

	/* LEAはインデックスのみ */
	if (opc < 0x34) {
		idx();
		return;
	}

	/* PSH,PULは専用 */
	if (opc < 0x38) {
		pshpul();
		return;
	}

	/* 隠し命令ANDCCはイミディエイト */
	if (opc == 0x38) {
		imm1();
	}
}

/*
 *	インヘレントA,B,M系(0x40,0x50,0x60,0x70)
 */
static void FASTCALL inhabm(void)
{
	/* 0x6e, 0x7eは特別 */
	if (opc == 0x6e) {
		strcat(linebuf, "JMP   ");
		idx();
		return;
	}
	if (opc == 0x7e) {
		strcat(linebuf, "JMP   ");
		extend();
		return;
	}

	switch (opc >> 4) {
		/* Ａレジスタ */
		case 0x4:
			if (inha_tbl[opc & 0x0f] == NULL) {
				notdef();
				return;
			}
			strcat(linebuf, inha_tbl[opc & 0x0f]);
			break;
		/* Ｂレジスタ */
		case 0x5:
			if (inhb_tbl[opc & 0x0f] == NULL) {
				notdef();
				return;
			}
			strcat(linebuf, inhb_tbl[opc & 0x0f]);
			break;
		/* メモリ、インデックス */
		case 0x6:
			if (inhm_tbl[opc & 0x0f] == NULL) {
				notdef();
				return;
			}
			strcat(linebuf, inhm_tbl[opc & 0x0f]);
			idx();
			break;
		/* メモリ、エクステンド */
		case 0x7:
			if (inhm_tbl[opc & 0x0f] == NULL) {
				notdef();
				return;
			}
			strcat(linebuf, inhm_tbl[opc & 0x0f]);
			extend();
			break;
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	Ａレジスタ、Ｘレジスタ系(0x80, 0x90, 0xa0, 0xb0)
 */
static void FASTCALL regax(void)
{
	/* 0x87, 0x8fは隠し命令 */
	if (opc == 0x87) {
		strcat(linebuf, "FLAG  ");
		imm1();
		return;
	}
	if (opc == 0x8f) {
		strcat(linebuf, "FLAG  ");
		imm2();
		return;
	}

	/* 0x8dはBSR */
	if (opc == 0x8d) {
		strcat(linebuf, "BSR   ");
		rel1();
		return;
	}

	/* それ以外 */
	strcat(linebuf, regax_tbl[opc & 0x0f]);

	/* アドレッシングモード別 */
	switch (opc >> 4) {
		case 0x8:
			if ((opc == 0x83) || (opc == 0x8c) || (opc == 0x8e)) {
				imm2();
			}
			else {
				imm1();
			}
			break;
		case 0x9:
			direct();
			break;
		case 0xa:
			idx();
			break;
		case 0xb:
			extend();
			break;
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	Ｂレジスタ、Ｄレジスタ、Ｕレジスタ系(0xc0, 0xd0, 0xe0, 0xf0)
 */
static void FASTCALL regbdu(void)
{
	/* 0xc7, 0xcd, 0xcfは隠し命令 */
	if (opc == 0xc7) {
		strcat(linebuf, "FLAG  ");
		imm1();
		return;
	}
	if (opc == 0xcd) {
		strcat(linebuf, "HALT  ");
		return;
	}
	if (opc == 0xcf) {
		strcat(linebuf, "FLAG  ");
		imm2();
		return;
	}

	/* それ以外 */
	strcat(linebuf, regbdu_tbl[opc & 0x0f]);

	/* アドレッシングモード別 */
	switch (opc >> 4) {
		case 0xc:
			if ((opc == 0xc3) || (opc == 0xcc) || (opc == 0xce)) {
				imm2();
			}
			else {
				imm1();
			}
			break;
		case 0xd:
			direct();
			break;
		case 0xe:
			idx();
			break;
		case 0xf:
			extend();
			break;
		default:
			ASSERT(FALSE);
			break;
	}
}

/*-[ メイン ]---------------------------------------------------------------*/

/*
 *	逆アセンブラ本体
 */
int FASTCALL disline(int cpu, WORD pcreg, char *buffer)
{
	int i;
	int j;

	/* assert */
	ASSERT((cpu == MAINCPU) || (cpu == SUBCPU));
	ASSERT(buffer);

	/* 初期設定 */
	cputype = cpu;
	pc = pcreg;
	addpc = 0;
	linebuf[0] = '\0';

	/* 先頭のバイトを読み出し */
	opc = fetch();

	/* グループ別 */
	switch ((int)(opc >> 4)) {
		case 0x0:
			except1();	
			break;
		case 0x1:
			except2();
			break;
		case 0x2:
			branch();
			break;
		case 0x3:
			leastack();
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			inhabm();
			break;
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			regax();
			break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			regbdu();
			break;
		default:
			ASSERT(FALSE);
	}

	/* 命令データをセット */
	buffer[0] = '\0';

	sub4hex(pcreg, buffer);
	strcat(buffer, " ");
	for (i=0; i<addpc; i++) {
		sub2hex(read_byte((WORD)(pcreg + i)), buffer);
		strcat(buffer, " ");
	}

	/* bufferが20バイト+'\0'になるよう調整する */
	j = strlen(buffer);
	if (j < 20) {
		for (i=0; i<20 - j; i++) {
			buffer[i + j] = ' ';
		}
		buffer[i + j] = '\0';
	}

	strcat(buffer, linebuf);
 	return (int)addpc;
}
