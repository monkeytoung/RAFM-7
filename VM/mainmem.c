/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ メインCPUメモリ ]
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "device.h"
#include "subctrl.h"
#include "ttlpalet.h"
#include "fdc.h"
#include "mainetc.h"
#include "multipag.h"
#include "kanji.h"
#include "tapelp.h"
#include "opn.h"
#include "mmr.h"
#include "apalet.h"
#include "whg.h"

/*
 *	グローバル ワーク
 */
BYTE *mainram_a;						/* RAM (表RAM)    $8000 */
BYTE *mainram_b;						/* RAM (裏RAM)    $7C80 */
BYTE *basic_rom;						/* ROM (F-BASIC)  $7C00 */
BYTE *main_io;							/* メインCPU I/O  $0100 */

BYTE *extram_a;							/* 拡張RAM        $10000 */
BYTE *boot_ram;							/* ブートRAM        $200 */
BYTE *init_rom;							/* イニシエータROM $2000 */

BOOL basicrom_en;						/* BASIC ROMイネーブルフラグ */
BOOL initrom_en;						/* イニシエータROMイネーブルフラグ */
BOOL bootram_rw;						/* ブートRAM 書き込みフラグ */

/*
 *	メインCPUメモリ
 *	初期化
 */
BOOL FASTCALL mainmem_init(void)
{
	int i;
	BYTE *p;

	/* 一度、全てクリア */
	mainram_a = NULL;
	mainram_b = NULL;
	basic_rom = NULL;
	main_io = NULL;
	extram_a = NULL;
	init_rom = NULL;
	boot_ram = NULL;

	/* RAM */
	mainram_a = (BYTE *)malloc(0x8000);
	if (mainram_a == NULL) {
		return FALSE;
	}
	mainram_b = (BYTE *)malloc(0x7c80);
	if (mainram_b == NULL) {
		return FALSE;
	}

	/* BASIC ROM, I/O */
	basic_rom = (BYTE *)malloc(0x7c00);
	if (basic_rom == NULL) {
		return FALSE;
	}
	main_io = (BYTE *)malloc(0x0100);
	if (main_io == NULL) {
		return FALSE;
	}

	/* 拡張RAM、イニシエータROM、ブートRAM */
	extram_a = (BYTE *)malloc(0x10000);
	if (extram_a == NULL) {
		return FALSE;
	}
	init_rom = (BYTE *)malloc(0x2000);
	if (init_rom == NULL) {
		return FALSE;
	}
	boot_ram = (BYTE *)malloc(0x200);
	if (boot_ram == NULL) {
		return FALSE;
	}

	/* ROMファイル読み込み */
	if (!file_load(FBASIC_ROM, basic_rom, 0x7c00)) {
		return FALSE;
	}
	if (!file_load(INITIATE_ROM, init_rom, 0x2000)) {
		return FALSE;
	}

	/* 高速起動モード */
	for (i=0; i<2; i++) {
		p = &init_rom[0x1800 + i * 0x200];
		if (p[0x14f] == 0x26) {
			p[0x14f] = 0x21;
		}
		if (p[0x151] == 0x26) {
			p[0x151] = 0x21;
		}
	}
	p = &init_rom[0x1c00];
	if (p[0x166] == 0x27) {
		p[0x166] = 0x21;
	}
	if (p[0x1d6] == 0x26) {
		p[0x1d6] = 0x21;
	}

	/* イニシエータROM ハードウェアバージョン(FM77AV2相当) */
	memset(&init_rom[0xb0e], 0xff, 3);

	return TRUE;
}

/*
 *	メインCPUメモリ
 *	クリーンアップ
 */
void FASTCALL mainmem_cleanup(void)
{
	ASSERT(mainram_a);
	ASSERT(mainram_b);
	ASSERT(basic_rom);
	ASSERT(main_io);
	ASSERT(extram_a);
	ASSERT(init_rom);
	ASSERT(boot_ram);

	/* 初期化途中で失敗した場合を考慮 */
	if (mainram_a) {
		free(mainram_a);
	}
	if (mainram_b) {
		free(mainram_b);
	}
	if (basic_rom) {
		free(basic_rom);
	}
	if (main_io) {
		free(main_io);
	}
	if (extram_a) {
		free(extram_a);
	}
	if (init_rom) {
		free(init_rom);
	}
	if (boot_ram) {
		free(boot_ram);
	}
}

/*
 *	メインCPUメモリ
 *	リセット
 */
void FASTCALL mainmem_reset(void)
{
	/* I/O空間初期化 */
	memset(main_io, 0xff, 0x0100);

	/* イニシエータON、ブートRAM書き込み可 */
	if (fm7_ver >= 2) {
		initrom_en = TRUE;
		bootram_rw = TRUE;
	}
	else {
		initrom_en = FALSE;
		bootram_rw = FALSE;
	}

	/* BASICモードであれば、BASIC ROMをイネーブル */
	if (boot_mode == BOOT_BASIC) {
		basicrom_en = TRUE;
	}
	else {
		basicrom_en = FALSE;
	}

	/* ブートRAM セットアップ */
	/* 一旦すべてクリア(女神転生対策) */
	memset(boot_ram, 0, 0x200);
	if (fm7_ver < 2) {
		if (boot_mode == BOOT_BASIC) {
			memcpy(boot_ram, &init_rom[0x1800], 0x200);
		}
		else {
			memcpy(boot_ram, &init_rom[0x1a00], 0x200);
		}
		boot_ram[0x1fe] = 0xfe;
		boot_ram[0x1ff] = 0x00;
	}
}

/*
 *	メインCPUメモリ
 *	１バイト取得
 */
BYTE FASTCALL mainmem_readb(WORD addr)
{
	BYTE dat;

	/* MMR, TWRチェック */
	if (mmr_flag || twr_flag) {
		/* MMR、TWRを通す */
		if (mmr_extrb(&addr, &dat)) {
			return dat;
		}
	}

	/* メインRAM(表) */
	if (addr < 0x6000) {
		return mainram_a[addr];
	}
	if (addr < 0x8000) {
		if (initrom_en) {
			return init_rom[addr - 0x6000];
		}
		else {
			return mainram_a[addr];
		}
	}

	/* BASIC ROM or メインRAM(裏) */
	if (addr < 0xfc00) {
		if (basicrom_en) {
			return basic_rom[addr - 0x8000];
		}
		else {
			return mainram_b[addr - 0x8000];
		}
	}

	/* メインROMの直後 */
	if (addr < 0xfc80) {
		return mainram_b[addr - 0x8000];
	}

	/* 共有RAM */
	if (addr < 0xfd00) {
		if (subhalt_flag) {
			return shared_ram[(WORD)(addr - 0xfc80)];
		}
		else {
			return 0xff;
		}
	}

	/* ブートRAM */
	if ((addr >= 0xfffe) && initrom_en) {
		return init_rom[addr - 0xe000];
	}
	if (addr >= 0xfe00) {
		return boot_ram[addr - 0xfe00];
	}

	/*
	 *	I/O空間
	 */
	if (mainetc_readb(addr, &dat)) {
		return dat;
	}
	if (ttlpalet_readb(addr, &dat)) {
		return dat;
	}
	if (subctrl_readb(addr, &dat)) {
		return dat;
	}
	if (multipag_readb(addr, &dat)) {
		return dat;
	}
	if (fdc_readb(addr, &dat)) {
		return dat;
	}
	if (kanji_readb(addr, &dat)) {
		return dat;
	}
	if (tapelp_readb(addr, &dat)) {
		return dat;
	}
	if (opn_readb(addr, &dat)) {
		return dat;
	}
	if (mmr_readb(addr, &dat)) {
		return dat;
	}
	if (apalet_readb(addr, &dat)) {
		return dat;
	}
	if (whg_readb(addr, &dat)) {
		return dat;
	}

	return 0xff;
}

/*
 *	メインCPUメモリ
 *	１バイト取得(I/Oなし)
 */
BYTE FASTCALL mainmem_readbnio(WORD addr)
{
	BYTE dat;

	/* MMR, TWRチェック */
	if (mmr_flag || twr_flag) {
		/* MMR、TWRを通す */
		if (mmr_extbnio(&addr, &dat)) {
			return dat;
		}
	}

	/* メインRAM(表) */
	if (addr < 0x6000) {
		return mainram_a[addr];
	}
	if (addr < 0x8000) {
		if (initrom_en) {
			return init_rom[addr - 0x6000];
		}
		else {
			return mainram_a[addr];
		}
	}

	/* BASIC ROM or メインRAM(裏) */
	if (addr < 0xfc00) {
		if (basicrom_en) {
			return basic_rom[addr - 0x8000];
		}
		else {
			return mainram_b[addr - 0x8000];
		}
	}

	/* メインROMの直後 */
	if (addr < 0xfc80) {
		return mainram_b[addr - 0x8000];
	}

	/* 共有RAM */
	if (addr < 0xfd00) {
		if (subhalt_flag) {
			return shared_ram[(WORD)(addr - 0xfc80)];
		}
		else {
			return 0xff;
		}
	}

	/* ブートRAM */
	if ((addr >= 0xfffe) && initrom_en) {
		return init_rom[addr - 0xe000];
	}
	if (addr >= 0xfe00) {
		return boot_ram[addr - 0xfe00];
	}

	/* I/O空間 */
	ASSERT((addr >= 0xfd00) && (addr < 0xfe00));
	return main_io[addr - 0xfd00];
}

/*
 *	メインCPUメモリ
 *	１バイト書き込み
 */
void FASTCALL mainmem_writeb(WORD addr, BYTE dat)
{
	/* MMR, TWRチェック */
	if (mmr_flag || twr_flag) {

		/* MMR、TWRを通す */
		if (mmr_extwb(&addr, dat)) {
			return;
		}
	}

	/* メインRAM(表) */
	if (addr < 0x6000) {
		mainram_a[addr] = dat;
		return;
	}
	if (addr < 0x8000) {
		if (!initrom_en) {
			mainram_a[addr] = dat;
		}
		return;
	}

	/* BASIC ROM or メインRAM(裏) */
	if (addr < 0xfc00) {
		if (basicrom_en) {
			/* ROM内RCBでBIOSを呼び出すケース */
			return;
		}
		else {
			mainram_b[addr - 0x8000] = dat;
			return;
		}
	}

	/* メインROMの直後 */
	if (addr < 0xfc80) {
		mainram_b[addr - 0x8000] = dat;
		return;
	}

	/* 共有RAM */
	if (addr < 0xfd00) {
		if (subhalt_flag) {
			shared_ram[(WORD)(addr - 0xfc80)] = dat;
			return;
		}
		else {
			/* BASE09対策 */
			return;
		}
	}

	/* ブートROM */
	if (addr >= 0xfe00) {
		if (bootram_rw) {
			boot_ram[addr - 0xfe00] = dat;
			return;
		}
		/* ブートワークRAM、ベクタ */
		if ((addr >= 0xffe0) && (addr < 0xfffe)) {

			boot_ram[addr - 0xfe00] = dat;
		}
		return;
	}

	/*
	 *	I/O空間
	 */
	ASSERT((addr >= 0xfd00) && (addr < 0xfe00));
	main_io[(WORD)(addr - 0xfd00)] = dat;

	if (mainetc_writeb(addr, dat)) {
		return;
	}
	if (ttlpalet_writeb(addr, dat)) {
		return;
	}
	if (subctrl_writeb(addr, dat)) {
		return;
	}
	if (multipag_writeb(addr, dat)) {
		return;
	}
	if (fdc_writeb(addr, dat)) {
		return;
	}
	if (kanji_writeb(addr, dat)) {
		return;
	}
	if (tapelp_writeb(addr, dat)) {
		return;
	}
	if (opn_writeb(addr, dat)) {
		return;
	}
	if (mmr_writeb(addr, dat)) {
		return;
	}
	if (apalet_writeb(addr, dat)) {
		return;
	}
	if (whg_writeb(addr, dat)) {
		return;
	}

	return;
}

/*
 *	メインCPUメモリ
 *	セーブ
 */
BOOL FASTCALL mainmem_save(int fileh)
{
	if (!file_write(fileh, mainram_a, 0x8000)) {
		return FALSE;
	}
	if (!file_write(fileh, mainram_b, 0x7c80)) {
		return FALSE;
	}

	if (!file_write(fileh, main_io, 0x100)) {
		return FALSE;
	}

	if (!file_write(fileh, extram_a, 0x8000)) {
		return FALSE;
	}
	if (!file_write(fileh, &extram_a[0x8000], 0x8000)) {
		return FALSE;
	}
	if (!file_write(fileh, boot_ram, 0x200)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, basicrom_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, initrom_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, bootram_rw)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	メインCPUメモリ
 *	ロード
 */
BOOL FASTCALL mainmem_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_read(fileh, mainram_a, 0x8000)) {
		return FALSE;
	}
	if (!file_read(fileh, mainram_b, 0x7c80)) {
		return FALSE;
	}

	if (!file_read(fileh, main_io, 0x100)) {
		return FALSE;
	}

	if (!file_read(fileh, extram_a, 0x8000)) {
		return FALSE;
	}
	if (!file_read(fileh, &extram_a[0x8000], 0x8000)) {
		return FALSE;
	}
	if (!file_read(fileh, boot_ram, 0x200)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &basicrom_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &initrom_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &bootram_rw)) {
		return FALSE;
	}

	return TRUE;
}
