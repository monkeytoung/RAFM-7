/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 論理演算・直線補間 ]
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "xm7.h"
#include "aluline.h"
#include "event.h"
#include "display.h"
#include "subctrl.h"
#include "multipag.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BYTE alu_command;						/* 論理演算 コマンド */
BYTE alu_color;							/* 論理演算 カラー */
BYTE alu_mask;							/* 論理演算 マスクビット */
BYTE alu_cmpstat;						/* 論理演算 比較ステータス */
BYTE alu_cmpdat[8];						/* 論理演算 比較データ */
BYTE alu_disable;						/* 論理演算 禁止バンク */
BYTE alu_tiledat[3];					/* 論理演算 タイルパターン */
BOOL line_busy;							/* 直線補間 BUSY */
WORD line_offset;						/* 直線補間 アドレスオフセット */
WORD line_style;						/* 直線補間 ラインスタイル */
WORD line_x0;							/* 直線補間 X0 */
WORD line_y0;							/* 直線補間 Y0 */
WORD line_x1;							/* 直線補間 X1 */
WORD line_y1;							/* 直線補間 Y1 */
BYTE line_count;						/* 直線補間 カウンタ */

/*
 *	プロトタイプ宣言
 */
static void FASTCALL alu_cmp(WORD addr);	/* コンペア */

/*
 *	論理演算・直線補間
 *	初期化
 */
BOOL FASTCALL aluline_init(void)
{
	return TRUE;
}

/*
 *	論理演算・直線補間
 *	クリーンアップ
 */
void FASTCALL aluline_cleanup(void)
{
}

/*
 *	論理演算・直線補間
 *	リセット
 */
void FASTCALL aluline_reset(void)
{
	/* 全てのレジスタを初期化 */
	alu_command = 0;
	alu_color = 0;
	alu_mask = 0;
	alu_cmpstat = 0;
	memset(alu_cmpdat, 0x80, sizeof(alu_cmpdat));
	alu_disable = 0x08;
	memset(alu_tiledat, 0, sizeof(alu_tiledat));

	line_busy = FALSE;
	line_offset = 0;
	line_style = 0;
	line_x0 = 0;
	line_y0 = 0;
	line_x1 = 0;
	line_y1 = 0;
	line_count = 0;
}

/*-[ 論理演算 ]-------------------------------------------------------------*/

/*
 *	論理演算
 *	VRAM読み出し
 */
static BYTE FASTCALL alu_read(WORD addr)
{
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}

	/* VRAM範囲エラー */
	ASSERT(FALSE);
	return 0xff;
}

/*
 *	論理演算
 *	VRAM書き込み
 */
static void FASTCALL alu_write(WORD addr, BYTE dat)
{
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* フック関数 */
			vram_notify(addr, dat);
		}
		return;
	}
}

/*
 *	論理演算
 *	書き込みサブ(比較書き込み付)
 */
static void FASTCALL alu_writesub(WORD addr, BYTE dat)
{
	BYTE temp;

	/* 常に書き込み可能か */
	if ((alu_command & 0x40) == 0) {
		alu_write(addr, dat);
		return;
	}

	/* イコール書き込みか、NOTイコール書き込みか */
	if (alu_command & 0x20) {
		/* NOTイコールで書き込む */
		temp = alu_read(addr);
		temp &= alu_cmpstat;
		dat &= (BYTE)(~alu_cmpstat);
		alu_write(addr, (BYTE)(temp | dat));
	}
	else {
		/* イコールで書き込む */
		temp = alu_read(addr);
		temp &= (BYTE)(~alu_cmpstat);
		dat &= alu_cmpstat;
		alu_write(addr, (BYTE)(temp | dat));
	}
}

/*
 *	論理演算
 *	PSET
 */
static void FASTCALL alu_pset(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算カラーデータより、データ作成 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* 演算なし(PSET) */
		mask = alu_read(addr);

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	PRESET
 */
static void FASTCALL alu_preset(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算カラーデータより、データ作成 */
		if (alu_color & bit) {
			dat = 0;
		}
		else {
			dat = 0xff;
		}

		/* 演算なし(PRESET) */
		mask = alu_read(addr);

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	OR
 */
static void FASTCALL alu_or(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算カラーデータより、データ作成 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* 演算 */
		mask = alu_read(addr);
		dat |= mask;

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	AND
 */
static void FASTCALL alu_and(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算カラーデータより、データ作成 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* 演算 */
		mask = alu_read(addr);
		dat &= mask;

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	XOR
 */
static void FASTCALL alu_xor(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算カラーデータより、データ作成 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* 演算 */
		mask = alu_read(addr);
		dat ^= mask;

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	NOT
 */
static void FASTCALL alu_not(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* 演算(NOT) */
		mask = alu_read(addr);
		dat = (BYTE)(~mask);

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	タイルペイント
 */
static void FASTCALL alu_tile(WORD addr)
{
	BYTE bit;
	BYTE mask;
	BYTE dat;
	int i;

	/* VRAMオフセットを取得、初期化 */
	addr &= 0x3fff;
	bit = 0x01;

	/* 比較書き込み時は、先に比較データを取得 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* 無効バンクはスキップ */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* データ作成 */
		dat = alu_tiledat[i];

		/* マスクビットの処理 */
		dat &= (BYTE)(~alu_mask);
		mask = alu_read(addr);
		mask &= alu_mask;
		dat |= mask;

		/* 書き込み */
		alu_writesub(addr, dat);

		/* 次のバンクへ */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	論理演算
 *	コンペア
 */
static void FASTCALL alu_cmp(WORD addr)
{
	BYTE color;
	BYTE bit;
	int i, j;
	BOOL flag;
	BYTE dat;
	BYTE b, r, g;
	BYTE disflag;

	/* アドレスマスク */
	addr &= 0x3fff;

	/* カラーデータ取得 */
	b = alu_read((WORD)(addr + 0x0000));
	r = alu_read((WORD)(addr + 0x4000));
	g = alu_read((WORD)(addr + 0x8000));

	/* バンクディセーブルを考慮する(女神転生対策) */
	disflag = (BYTE)((~alu_disable) & 0x07);

	/* 比較が必要 */
	dat = 0;
	bit = 0x80;
	for (i=0; i<8; i++) {
		/* 色を作成 */
		color = 0;
		if (b & bit) {
			color |= 0x01;
		}
		if (r & bit) {
			color |= 0x02;
		}
		if (g & bit) {
			color |= 0x04;
		}

		/* 8つの色スロットをまわって、どれか一致するものがあるか */
		flag = FALSE;
		for (j=0; j<8; j++) {
			if ((alu_cmpdat[j] & 0x80) == 0) {
				if ((alu_cmpdat[j] & disflag) == (color & disflag)) {
					flag = TRUE;
					break;
				}
			}
		}

		/* イコールで1を設定 */
		if (flag) {
			dat |= bit;
		}

		/* 次へ */
		bit >>= 1;
	}

	/* データ設定 */
	alu_cmpstat = dat;
}

/*-[ 直線補間 ]-------------------------------------------------------------*/

/*
 *	直線補間
 *	点描画
 */
static void FASTCALL aluline_pset(int x, int y)
{
	BYTE temp;
	WORD addr;
	static BYTE table[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	BYTE mask;

	/* 論理演算のデータバスに入るので、論理演算onが必要 */
	if ((alu_command & 0x80) == 0) {
		return;
	}

	/* クリッピング */
	if ((x < 0) || (x >= 640)) {
		return;
	}
	if ((y < 0) || (y >= 200)) {
		return;
	}

	/* 画面モードから、アドレス算出 */
	if (mode320) {
		addr = (WORD)(y * 40 + (x >> 3));
	}
	else {
		addr = (WORD)(y * 80 + (x >> 3));
	}

	/* オフセットを加える */
	addr += line_offset;
	addr &= 0x3fff;

	/* ラインスタイル */
	temp = table[line_count & 0x07];
	if (line_count < 8) {
		temp &= (BYTE)(line_style >> 8);
	}
	else {
		temp &= (BYTE)(line_style & 0xff);
	}

	/* スタイルビットが立っている場合のみ */
	if (temp != 0) {
		/* マスクのバックアップをとる */
		mask = alu_mask;

		/* マスクを設定 */
		alu_mask = (BYTE)(~table[x & 0x07]);

		/* 論理演算を動かす */
		switch (alu_command & 0x07) {
			/* PSET */
			case 0:
				alu_pset(addr);
				break;
			/* 禁止 */
			case 1:
				alu_preset(addr);
				break;
			/* OR */
			case 2:
				alu_or(addr);
				break;
			/* AND */
			case 3:
				alu_and(addr);
				break;
			/* XOR */
			case 4:
				alu_xor(addr);
				break;
			/* NOT */
			case 5:
				alu_not(addr);
				break;
			/* タイルペイント */
			case 6:
				alu_tile(addr);
				break;
			/* コンペア */
			case 7:
				alu_cmp(addr);
				break;
		}

		/* マスクを復元 */
		alu_mask = mask;
	}

	/* カウントアップ */
	line_count++;
	line_count &= 0x0f;
}

/*
 *	直線補間
 *	座標スワップ
 */
static void FASTCALL aluline_swap(int *a, int *b)
{
	int temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

/*
 *	直線補間
 *	Y方向へのばす
 */
static void FASTCALL aluline_nexty(int xp, int yp, int dx, int dy, int dir)
{
	int k1, k2;
	int err;

	/* ステップを決める */
	k1 = dy * 2;
	k2 = k1 - (dx * 2);
	err = k1 - dx;

	/* 描画 */
	aluline_pset(xp, yp);

	/* ループ */
	while (dx > 0) {
		dx --;

		if (err >= 0) {
			yp++;
			err += k2;
		}
		else {
			err += k1;
		}
		xp += dir;
		aluline_pset(xp, yp);
	}
}

/*
 *	直線補間
 *	X方向へのばす
 */
static void FASTCALL aluline_nextx(int xp, int yp, int dx, int dy, int dir)
{
	int k1, k2;
	int err;

	/* ステップを決める */
	k1 = dx * 2;
	k2 = k1 - (dy * 2);
	err = k1 - dy;

	/* 描画 */
	aluline_pset(xp, yp);

	/* ループ */
	while (dy > 0) {
		dy--;

		if (err >= 0) {
			xp += dir;
			err += k2;
		}
		else {
			err += k1;
		}
		yp++;
		aluline_pset(xp, yp);
	}
}

/*
 *	直線描画
 *	参考文献:「Windows95ゲームプログラミング」Bresenhamラインアルゴリズム
 */
static void FASTCALL aluline_line(void)
{
	int x1, x2, y1, y2;
	int dx, dy;
	int step;
	int i;

	/* データ取得 */
	x1 = (int)line_x0;
	x2 = (int)line_x1;
	y1 = (int)line_y0;
	y2 = (int)line_y1;

	/* カウンタ初期化 */
	line_count = 0;

	/* 単一点描画のチェックと処理 */
	if ((x1 == x2) && (y1 == y2)) {
		aluline_pset(x1, y1);
		return;
	}

	/* Xが同一の場合 */
	if (x1 == x2) {
		if (y2 > y1) {
			for (i=y1; i<=y2; i++) {
				aluline_pset(x1, i);
			}
		}
		else {
			for (i=y1; i>=y2; i--) {
				aluline_pset(x1, i);
			}
		}
		return;
	}

	/* Yが同一の場合 */
	if (y1 == y2) {
		if (x2 > x1) {
			for (i=x1; i<=x2; i++) {
				aluline_pset(i, y1);
			}
		}
		else {
			for (i=x1; i>=x2; i--) {
				aluline_pset(i, y1);
			}
		}
		return;
	}

	/* y座標の上下を揃える */
	if (y1 > y2) {
		aluline_swap(&y1, &y2);
		aluline_swap(&x1, &x2);
	}

	/* Δ値を計算 */
	dx = x2 - x1;
	dy = y2 - y1;

	/* 右向き、もしくは左向き */
	if (dx > 0) {
		step = 1;
	}
	else {
		dx = -dx;
		step = -1;
	}

	/* XまたはYどちらかの方向を重視 */
	if (dx > dy) {
		aluline_nexty(x1, y1, dx, dy, step);
	}
	else {
		aluline_nextx(x1, y1, dx, dy, step);
	}
}

/*
 *	直線補間
 *	イベント
 */
static BOOL FASTCALL aluline_event(void)
{
	/* 直線補間はREADY */
	line_busy = FALSE;

	schedule_delevent(EVENT_LINE);
	return TRUE;
}

/*-[ メモリマップドI/O ]----------------------------------------------------*/

/*
 *	論理演算・直線補間
 *	１バイト読み出し
 */
BOOL FASTCALL aluline_readb(WORD addr, BYTE *dat)
{
	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* 論理演算 コマンド */
		case 0xd410:
			*dat = alu_command;
			return TRUE;

		/* 論理演算 カラー */
		case 0xd411:
			*dat = alu_color;
			return TRUE;

		/* 論理演算 マスクビット */
		case 0xd412:
			*dat = alu_mask;
			return TRUE;

		/* 論理演算 比較ステータス */
		case 0xd413:
			*dat = alu_cmpstat;
			return TRUE;

		/* 論理演算 無効バンク */
		case 0xd41b:
			*dat = alu_disable;
			return TRUE;
	}

	/* 論理演算 比較データ */
	if ((addr >= 0xd413) && (addr <= 0xd41a)) {
		*dat = 0xff;
		return TRUE;
	}

	/* 論理演算 タイルパターン */
	if ((addr >= 0xd41c) && (addr <= 0xd41e)) {
		*dat = 0xff;
		return TRUE;
	}

	/* 直線補間 */
	if ((addr >= 0xd420) && (addr <= 0xd42b)) {
		*dat = 0xff;
		return TRUE;
	}

	return FALSE;
}

/*
 *	論理演算・直線補間
 *	１バイト書き込み
 */
BOOL FASTCALL aluline_writeb(WORD addr, BYTE dat)
{
	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* 論理演算 コマンド */
		case 0xd410:
			alu_command = dat;
			return TRUE;

		/* 論理演算 カラー */
		case 0xd411:
			alu_color = dat;
			return TRUE;

		/* 論理演算 マスクビット */
		case 0xd412:
			alu_mask = dat;
			return TRUE;

		/* 論理演算 無効バンク */
		case 0xd41b:
			alu_disable = dat;
			return TRUE;

		/* 直線補間 アドレスオフセット(A1から注意) */
		case 0xd420:
			line_offset &= 0x01fe;
			line_offset |= (WORD)((dat * 512) & 0x3e00);
			return TRUE;
		case 0xd421:
			line_offset &= 0x3e00;
			line_offset |= (WORD)(dat * 2);
			return TRUE;

		/* 直線補間 ラインスタイル */
		case 0xd422:
			line_style &= 0xff;
			line_style |= (WORD)(dat * 256);
			return TRUE;
		case 0xd423:
			line_style &= 0xff00;
			line_style |= (WORD)dat;
			return TRUE;

		/* 直線補間 X0 */
		case 0xd424:
			line_x0 &= 0xff;
			line_x0 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd425:
			line_x0 &= 0xff00;
			line_x0 |= (WORD)dat;
			return TRUE;

		/* 直線補間 Y0 */
		case 0xd426:
			line_y0 &= 0xff;
			line_y0 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd427:
			line_y0 &= 0xff00;
			line_y0 |= (WORD)dat;
			return TRUE;

		/* 直線補間 X1 */
		case 0xd428:
			line_x1 &= 0xff;
			line_x1 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd429:
			line_x1 &= 0xff00;
			line_x1 |= (WORD)dat;
			return TRUE;

		/* 直線補間 Y1 */
		case 0xd42a:
			line_y1 &= 0xff;
			line_y1 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd42b:
			line_y1 &= 0xff00;
			line_y1 |= (WORD)dat;

			/* ここで直線補間スタート */
			aluline_line();

			/* 線は引いたが、しばらくBUSYにしておく */
			line_busy = TRUE;
			schedule_setevent(EVENT_LINE, 20, aluline_event);
			return TRUE;
	}

	/* 論理演算 比較データ */
	if ((addr >= 0xd413) && (addr <= 0xd41a)) {
		alu_cmpdat[addr - 0xd413] = dat;
		return TRUE;
	}

	/* 論理演算 タイルパターン */
	if ((addr >= 0xd41c) && (addr <= 0xd41e)) {
		alu_tiledat[addr - 0xd41c] = dat;
		return TRUE;
	}

	return FALSE;
}

/*
 *	論理演算・直線補間
 *	VRAMダミーリード
 */
void FASTCALL aluline_extrb(WORD addr)
{
	/* 論理演算が有効か */
	if (alu_command & 0x80) {

		/* コマンド別 */
		switch (alu_command & 0x07) {
			/* PSET */
			case 0:
				alu_pset(addr);
				break;
			/* 禁止 */
			case 1:
				alu_preset(addr);
				break;
			/* OR */
			case 2:
				alu_or(addr);
				break;
			/* AND */
			case 3:
				alu_and(addr);
				break;
			/* XOR */
			case 4:
				alu_xor(addr);
				break;
			/* NOT */
			case 5:
				alu_not(addr);
				break;
			/* タイルペイント */
			case 6:
				alu_tile(addr);
				break;
			/* コンペア */
			case 7:
				alu_cmp(addr);
				break;
		}
	}
}

/*-[ ファイルI/O ]----------------------------------------------------------*/

/*
 *	論理演算・直線補間
 *	セーブ
 */
BOOL FASTCALL aluline_save(int fileh)
{
	if (!file_byte_write(fileh, alu_command)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_color)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_mask)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_cmpstat)) {
		return FALSE;
	}
	if (!file_write(fileh, alu_cmpdat, sizeof(alu_cmpdat))) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_disable)) {
		return FALSE;
	}
	if (!file_write(fileh, alu_tiledat, sizeof(alu_tiledat))) {
		return FALSE;
	}

	if (!file_bool_write(fileh, line_busy)) {
		return FALSE;
	}

	if (!file_word_write(fileh, line_offset)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_style)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_x0)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_y0)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_x1)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_y1)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, line_count)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	論理演算・直線補間
 *	ロード
 */
BOOL FASTCALL aluline_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &alu_command)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_color)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_mask)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_cmpstat)) {
		return FALSE;
	}
	if (!file_read(fileh, alu_cmpdat, sizeof(alu_cmpdat))) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_disable)) {
		return FALSE;
	}
	if (!file_read(fileh, alu_tiledat, sizeof(alu_tiledat))) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &line_busy)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &line_offset)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_style)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_x0)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_y0)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_x1)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_y1)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &line_count)) {
		return FALSE;
	}

	/* イベント */
	schedule_handle(EVENT_LINE, aluline_event);

	return TRUE;
}
