/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ サブCPUメモリ ]
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "subctrl.h"
#include "device.h"
#include "keyboard.h"
#include "multipag.h"
#include "aluline.h"

/*
 *	グローバル ワーク
 */
BYTE *vram_c;						/* VRAM(タイプC) $C000 */
BYTE *subrom_c;						/* ROM (タイプC) $2800 */
BYTE *sub_ram;						/* コンソールRAM $1680 */
BYTE *sub_io;						/* サブCPU I/O   $0100 */

BYTE *vram_b;						/* VRAM(タイプB) $C000 */
BYTE *subrom_a;						/* ROM (タイプA) $2000 */
BYTE *subrom_b;						/* ROM (タイプB) $2000 */
BYTE *subromcg;						/* ROM (フォント)$2000 */

BYTE subrom_bank;					/* サブシステムROMバンク */
BYTE cgrom_bank;					/* CGROMバンク */

/*
 *	サブCPUメモリ
 *	初期化
 */
BOOL FASTCALL submem_init(void)
{
	/* 一度、全てクリア */
	vram_c = NULL;
	subrom_c = NULL;
	vram_b = NULL;
	subrom_a = NULL;
	subrom_b = NULL;
	subromcg = NULL;
	sub_ram = NULL;
	sub_io = NULL;

	/* メモリ確保(タイプC) */
	vram_c = (BYTE *)malloc(0x18000);
	if (vram_c == NULL) {
		return FALSE;
	}
	subrom_c = (BYTE *)malloc(0x2800);
	if (subrom_c == NULL) {
		return FALSE;
	}

	/* メモリ確保(タイプA,B) */
	vram_b = vram_c + 0xc000;
	subrom_a = (BYTE *)malloc(0x2000);
	if (subrom_a == NULL) {
		return FALSE;
	}
	subrom_b = (BYTE *)malloc(0x2000);
	if (subrom_b == NULL) {
		return FALSE;
	}
	subromcg = (BYTE *)malloc(0x2000);
	if (subromcg == NULL) {
		return FALSE;
	}

	/* メモリ確保(共通) */
	sub_ram = (BYTE *)malloc(0x1680);
	if (sub_ram == NULL) {
		return FALSE;
	}
	sub_io = (BYTE *)malloc(0x0100);
	if (sub_io == NULL) {
		return FALSE;
	}

	/* ROMファイル読み込み */
	if (!file_load(SUBSYSC_ROM, subrom_c, 0x2800)) {
		return FALSE;
	}
	if (!file_load(SUBSYSA_ROM, subrom_a, 0x2000)) {
		return FALSE;
	}
	if (!file_load(SUBSYSB_ROM, subrom_b, 0x2000)) {
		return FALSE;
	}
	if (!file_load(SUBSYSCG_ROM, subromcg, 0x2000)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	サブCPUメモリ
 *	クリーンアップ
 */
void FASTCALL submem_cleanup(void)
{
	ASSERT(vram_c);
	ASSERT(subrom_c);
	ASSERT(subrom_a);
	ASSERT(subrom_b);
	ASSERT(subromcg);
	ASSERT(sub_ram);
	ASSERT(sub_io);

	/* 初期化途中で失敗した場合を考慮 */
	if (vram_c) {
		free(vram_c);
	}
	if (subrom_c) {
		free(subrom_c);
	}

	if (subrom_a) {
		free(subrom_a);
	}
	if (subrom_b) {
		free(subrom_b);
	}
	if (subromcg) {
		free(subromcg);
	}

	if (sub_ram) {
		free(sub_ram);
	}
	if (sub_io) {
		free(sub_io);
	}
}

/*
 *	サブCPUメモリ
 *	リセット
 */
void FASTCALL submem_reset(void)
{
	/* VRAMアクティブページ */
	vram_aptr = vram_c;
	vram_active = 0;

	/* バンククリア */
	subrom_bank = 0;
	cgrom_bank = 0;

	/* I/O空間 クリア */
	memset(sub_io, 0xff, 0x0100);
}

/*
 *	サブCPUメモリ
 *	１バイト取得
 */
BYTE FASTCALL submem_readb(WORD addr)
{
	BYTE dat;

	/* VRAM */
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}

	/* ワークRAM */
	if (addr < 0xd380) {
		return sub_ram[addr - 0xc000];
	}

	/* 共有RAM */
	if (addr < 0xd400) {
		ASSERT(!subhalt_flag);
		return shared_ram[(WORD)(addr - 0xd380)];
	}

	/* サブROM */
	if (addr >= 0xe000) {
		ASSERT(subrom_bank <= 2);
		switch (subrom_bank) {
			/* タイプC */
			case 0:
				return subrom_c[addr - 0xd800];
			/* タイプA */
			case 1:
				return subrom_a[addr - 0xe000];
			/* タイプB */
			case 2:
				return subrom_b[addr - 0xe000];
		}
	}

	/* CGROM */
	if (addr >= 0xd800) {
		ASSERT(cgrom_bank <= 3);
		return subromcg[cgrom_bank * 0x0800 + (addr - 0xd800)];
	}

	/* ワークRAM */
	if (addr >= 0xd500){
		return sub_ram[(addr - 0xd500) + 0x1380];
	}

	/*
	 *	サブI/O
	 */

	/* ディスプレイ */
	if (display_readb(addr, &dat)) {
		return dat;
	}
	/* キーボード */
	if (keyboard_readb(addr, &dat)) {
		return dat;
	}
	/* 論理演算・直線補間 */
	if (aluline_readb(addr, &dat)) {
		return dat;
	}

	return 0xff;
}

/*
 *	サブCPUメモリ
 *	１バイト取得(I/Oなし)
 */
BYTE FASTCALL submem_readbnio(WORD addr)
{
	/* VRAM */
	if (addr < 0xc000) {
		return vram_aptr[addr];
	}

	/* ワークRAM */
	if (addr < 0xd380) {
		return sub_ram[addr - 0xc000];
	}

	/* 共有RAM */
	if (addr < 0xd400) {
		return shared_ram[(WORD)(addr - 0xd380)];
	}

	/* サブI/O */
	if (addr < 0xd500) {
		return sub_io[addr - 0xd400];
	}

	/* ワークRAM */
	if (addr < 0xd800){
		return sub_ram[(addr - 0xd500) + 0x1380];
	}

	/* サブROM */
	if (addr >= 0xe000) {
		ASSERT(subrom_bank <= 2);
		switch (subrom_bank) {
			/* タイプC */
			case 0:
				return subrom_c[addr - 0xd800];
			/* タイプA */
			case 1:
				return subrom_a[addr - 0xe000];
			/* タイプB */
			case 2:
				return subrom_b[addr - 0xe000];
		}
	}

	/* CGROM */
	if (addr >= 0xd800) {
		ASSERT(cgrom_bank <= 3);
		return subromcg[cgrom_bank * 0x0800 + (addr - 0xd800)];
	}

	/* ここには来ない */
	ASSERT(FALSE);
	return 0;
}

/*
 *	サブCPUメモリ
 *	１バイト書き込み
 */
void FASTCALL submem_writeb(WORD addr, BYTE dat)
{
	/* VRAM(タイプC) */
	if (addr < 0x4000) {
		if (!(multi_page & 0x01)) {
			/* ALUはメモリ書き込みでも有効。(少年マイクの一人旅) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0x8000) {
		if (!(multi_page & 0x02)) {
			/* ALUはメモリ書き込みでも有効。(少年マイクの一人旅) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0xc000) {
		if (!(multi_page & 0x04)) {
			/* ALUはメモリ書き込みでも有効。(少年マイクの一人旅) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}

	/* ワークRAM */
	if (addr < 0xd380) {
		sub_ram[addr - 0xc000] = dat;
		return;
	}

	/* 共有RAM */
	if (addr < 0xd400) {
		shared_ram[(WORD)(addr - 0xd380)] = dat;
		return;
	}

	/* ワークRAM */
	if ((addr >= 0xd500) && (addr < 0xd800)) {
		sub_ram[(addr - 0xd500) + 0x1380] = dat;
		return;
	}

	/* CGROM、サブROM→書き込みできない */
	if (addr >= 0xd800) {
		/* YAMAUCHIで乗っ取った後、NMI割り込みハンドラで書き込み */
		/* Thunder Force対策 */
		return;
	}

	/*
	 *	サブI/O
	 */
	sub_io[addr - 0xd400] = dat;

	/* ディスプレイ */
	if (display_writeb(addr, dat)) {
		return;
	}
	/* キーボード */
	if (keyboard_writeb(addr, dat)) {
		return;
	}
	/* 論理演算・直線補間 */
	if (aluline_writeb(addr, dat)) {
		return;
	}
}

/*
 *	サブCPUメモリ
 *	セーブ
 */
BOOL FASTCALL submem_save(int fileh)
{
	if (!file_write(fileh, vram_c, 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0x6000], 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0xc000], 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0x12000], 0x6000)) {
		return FALSE;
	}

	if (!file_write(fileh, sub_ram, 0x1680)) {
		return FALSE;
	}

	if (!file_write(fileh, sub_io, 0x100)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, subrom_bank)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, cgrom_bank)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	サブCPUメモリ
 *	ロード
 */
BOOL FASTCALL submem_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_read(fileh, vram_c, 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0x6000], 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0xc000], 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0x12000], 0x6000)) {
		return FALSE;
	}

	if (!file_read(fileh, sub_ram, 0x1680)) {
		return FALSE;
	}

	if (!file_read(fileh, sub_io, 0x100)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &subrom_bank)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &cgrom_bank)) {
		return FALSE;
	}

	return TRUE;
}
