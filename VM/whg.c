/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ WHG(YM2203) ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "whg.h"
#include "device.h"
#include "mainetc.h"
#include "event.h"

/*
 *	グローバル ワーク
 */
BOOL whg_enable;						/* WHG有効・無効フラグ */
BOOL whg_use;							/* WHG使用フラグ */
BYTE whg_reg[256];						/* WHGレジスタ */
BOOL whg_key[4];						/* WHGキーオンフラグ */
BOOL whg_timera;						/* タイマーA動作フラグ */
BOOL whg_timerb;						/* タイマーB動作フラグ */
DWORD whg_timera_tick;					/* タイマーA間隔 */
DWORD whg_timerb_tick;					/* タイマーB間隔 */
BYTE whg_scale;							/* プリスケーラ */

/*
 *	スタティック ワーク
 */
static BYTE whg_pstate;					/* ポート状態 */
static BYTE whg_selreg;					/* セレクトレジスタ */
static BYTE whg_seldat;					/* セレクトデータ */
static BOOL whg_timera_int;				/* タイマーAオーバーフロー */
static BOOL whg_timerb_int;				/* タイマーBオーバーフロー */
static BOOL whg_timera_en;				/* タイマーAイネーブル */
static BOOL whg_timerb_en;				/* タイマーBイネーブル */

/*
 *	WHG
 *	初期化
 */
BOOL FASTCALL whg_init(void)
{
	memset(whg_reg, 0, sizeof(whg_reg));

	/* 現在は、常にWHG有効 */
	whg_enable = TRUE;
	whg_use = FALSE;

	return TRUE;
}

/*
 *	WHG
 *	クリーンアップ
 */
void FASTCALL whg_cleanup(void)
{
	BYTE i;

	/* PSG */
	for (i=0; i<6; i++) {
		whg_notify(i, 0);
	}
	whg_notify(7, 0xff);

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x7f);
	}

	/* キーオフ */
	for (i=0; i<3; i++) {
		whg_notify(0x28, i);
	}
}

/*
 *	WHG
 *	リセット
 */
void FASTCALL whg_reset(void)
{
	BYTE i;

	/* レジスタクリア、タイマーOFF */
	memset(whg_reg, 0, sizeof(whg_reg));
	whg_timera = FALSE;
	whg_timerb = FALSE;

	/* I/O初期化 */
	whg_pstate = WHG_INACTIVE;
	whg_selreg = 0;
	whg_seldat = 0;

	/* デバイス */
	whg_timera_int = FALSE;
	whg_timerb_int = FALSE;
	whg_timera_tick = 0;
	whg_timerb_tick = 0;
	whg_timera_en = FALSE;
	whg_timerb_en = FALSE;
	whg_scale = 3;

	/* PSG初期化 */
	for (i=0; i<14;i++) {
		if (i == 7) {
			whg_notify(i, 0xff);
			whg_reg[i] = 0xff;
		}
		else {
			whg_notify(i, 0);
		}
	}

	/* MUL,DT */
	for (i=0x30; i<0x40; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0);
	}

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x7f);
		whg_reg[i] = 0x7f;
	}

	/* AR=$1F */
	for (i=0x50; i<0x60; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x1f);
		whg_reg[i] = 0x1f;
	}

	/* その他 */
	for (i=0x60; i<0xb4; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0);
	}

	/* SL,RR */
	for (i=0x80; i<0x90; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0xff);
		whg_reg[i] = 0xff;
	}

	/* キーオフ */
	for (i=0; i<3; i++) {
		whg_notify(0x28, i);
	}

	/* モード */
	whg_notify(0x27, 0);

	/* 使用フラグを下げる */
	whg_use = FALSE;
}

/*
 *	WHG
 *	タイマーAオーバフロー
 */
static BOOL FASTCALL whg_timera_event(void)
{
	/* イネーブルか */
	if (whg_enable && whg_timera_en) {
		/* オーバーフローアクションが有効か */
		if (whg_timera) {
			whg_timera = FALSE;
			whg_timera_int = TRUE;

			/* 割り込みをかける */
			whg_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* CSM音声合成モードでのキーオン */
	if (whg_enable) {
		whg_notify(0xff, 0);
	}

	/* タイマーは回し続ける */
	return TRUE;
}

/*
 *	WHG
 *	タイマーAインターバル算出
 */
static void FASTCALL whg_timera_calc(void)
{
	DWORD t;
	BYTE temp;

	t = whg_reg[0x24];
	t *= 4;
	temp = (BYTE)(whg_reg[0x25] & 3);
	t |= temp;
	t &= 0x3ff;
	t = (1024 - t);
	t *= whg_scale;
	t *= 12;
	t *= 10000;
	t /= 12288;

	/* タイマー値を設定 */
	if (whg_timera_tick != t) {
		whg_timera_tick = t;
		schedule_setevent(EVENT_WHG_A, whg_timera_tick, whg_timera_event);
	}
}

/*
 *	WHG
 *	タイマーBオーバフロー
 */
static BOOL FASTCALL whg_timerb_event(void)
{
	/* イネーブルか */
	if (whg_enable && whg_timerb_en) {
		/* オーバーフローアクションが有効か */
		if (whg_timerb) {
			/* フラグ変更 */
			whg_timerb = FALSE;
			whg_timerb_int = TRUE;

			/* 割り込みをかける */
			whg_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* タイマーは回し続ける */
	return TRUE;
}

/*
 *	WHG
 *	タイマーBインターバル算出
 */
static void FASTCALL whg_timerb_calc(void)
{
	DWORD t;

	t = whg_reg[0x26];
	t = (256 - t);
	t *= 192;
	t *= whg_scale;
	t *= 10000;
	t /= 12288;

	/* タイマー値を設定 */
	if (t != whg_timerb_tick) {
		whg_timerb_tick = t;
		schedule_setevent(EVENT_WHG_B, whg_timerb_tick, whg_timerb_event);
	}
}

/*
 *	WHG
 *	レジスタアレイより読み出し
 */
static BYTE FASTCALL whg_readreg(BYTE reg)
{
	/* FM音源部は読み出せない */
	if (reg >= 0x10) {
		return 0xff;
	}

	return whg_reg[reg];
}

/*
 *	WHG
 *	レジスタアレイへ書き込み
 */
static void FASTCALL whg_writereg(BYTE reg, BYTE dat)
{
	/* フラグオン */
	whg_use = TRUE;

	/* タイマー処理 */
	/* このレジスタは非常に難しい。良く分からないまま扱っている人が大半では？ */
	if (reg == 0x27) {
		/* オーバーフローフラグのクリア */
		if (dat & 0x10) {
			whg_timera_int = FALSE;
		}
		if (dat & 0x20) {
			whg_timerb_int = FALSE;
		}

		/* 両方落ちたら、割り込みを落とす */
		if (!whg_timera_int && !whg_timerb_int) {
			whg_irq_flag = FALSE;
			maincpu_irq();
		}

		/* タイマーA */
		if (dat & 0x01) {
			/* 0→1でタイマー値をロード、それ以外でもタイマーon */
			if ((whg_reg[0x27] & 0x01) == 0) {
				whg_timera_calc();
			}
			whg_timera_en = TRUE;
		}
		else {
			whg_timera_en = FALSE;
		}
		if (dat & 0x04) {
			whg_timera = TRUE;
		}
		else {
			whg_timera = FALSE;
		}

		/* タイマーB */
		if (dat & 0x02) {
			/* 0→1でタイマー値をロード、それ以外でもタイマーon */
			if ((whg_reg[0x27] & 0x02) == 0) {
				whg_timerb_calc();
			}
			whg_timerb_en = TRUE;
		}
		else {
			whg_timerb_en = FALSE;
		}
		if (dat & 0x08) {
			whg_timerb = TRUE;
		}
		else {
			whg_timerb = FALSE;
		}

		/* データ記憶 */
		whg_reg[reg] = dat;

		/* モードのみ出力 */
		whg_notify(0x27, (BYTE)(dat & 0xc0));
		return;
	}

	/* データ記憶 */
	whg_reg[reg] = dat;

	switch (reg) {
		/* プリスケーラ１ */
		case 0x2d:
			if (whg_scale != 3) {
				whg_scale = 6;
				whg_timerb_calc();
				whg_timerb_calc();
			}
			return;

		/* プリスケーラ２ */
		case 0x2e:
			whg_scale = 3;
			whg_timerb_calc();
			whg_timerb_calc();
			return;

		/* プリスケーラ３ */
		case 0x2f:
			whg_scale = 2;
			whg_timerb_calc();
			whg_timerb_calc();
			return;

		/* タイマーA(us単位で計算) */
		case 0x24:
			whg_timera_calc();
			return;

		case 0x25:
			whg_timera_calc();
			return;

		/* タイマーB(us単位で計算) */
		case 0x26:
			whg_timerb_calc();
			return;
	}

	/* 出力先を絞る */
	if ((reg >= 14) && (reg <= 0x26)) {
		return;
	}
	if ((reg >= 0x29) && (reg <= 0x2f)) {
		return;
	}

	/* キーオン */
	if (reg == 0x28) {
		if (dat >= 16) {
			whg_key[dat & 0x03] = TRUE;
		}
		else {
			whg_key[dat & 0x03] = FALSE;
		}
	}

	/* 出力 */
	whg_notify(reg, dat);
}

/*
 *	WHG
 *	１バイト読み出し
 */
BOOL FASTCALL whg_readb(WORD addr, BYTE *dat)
{
	/* バージョン、有効フラグをチェック、無効なら何もしない */
	if (!whg_enable || (fm7_ver < 2)) {
		return FALSE;
	}

	switch (addr) {
		/* コマンドレジスタは読み出し禁止 */
		case 0xfd45:
			*dat = 0xff;
			return TRUE;

		/* データレジスタ */
		case 0xfd46:
			switch (whg_pstate) {
				/* 通常コマンド */
				case WHG_INACTIVE:
				case WHG_READDAT:
				case WHG_WRITEDAT:
				case WHG_ADDRESS:
					*dat = whg_seldat;
					break;

				/* ステータス読み出し */
				case WHG_READSTAT:
					*dat = 0;
					if (whg_timera_int) {
						*dat |= 0x01;
					}
					if (whg_timerb_int) {
						*dat |= 0x02;
					}
					break;

				/* ジョイスティック読み取り */
				case WHG_JOYSTICK:
					if (whg_selreg == 14) {
						/* ジョイスティックは未接続として扱う */
						*dat = 0xff;
					}
					else {
						/* レジスタが14でなければ、FF以外を返す */
						*dat = 0;
					}
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	WHG
 *	１バイト書き込み
 */
BOOL FASTCALL whg_writeb(WORD addr, BYTE dat)
{
	/* バージョン、有効フラグをチェック、無効なら何もしない */
	if (!whg_enable || (fm7_ver < 2)) {
		return FALSE;
	}

	switch (addr) {
		/* WHGコマンドレジスタ */
		case 0xfd45:
			switch (dat & 0x0f) {
				/* インアクティブ(動作定義なし、データレジスタ書き換え可) */
				case WHG_INACTIVE:
					whg_pstate = WHG_INACTIVE;
					break;
				/* データ読み出し */
				case WHG_READDAT:
					whg_pstate = WHG_READDAT;
					whg_seldat = whg_readreg(whg_selreg);
					break;
				/* データ書き込み */
				case WHG_WRITEDAT:
					whg_pstate = WHG_WRITEDAT;
					whg_writereg(whg_selreg, whg_seldat);
					break;
				/* ラッチアドレス */
				case WHG_ADDRESS:
					whg_pstate = WHG_ADDRESS;
					whg_selreg = whg_seldat;
					break;
				/* リードステータス */
				case WHG_READSTAT:
					whg_pstate = WHG_READSTAT;
					break;
				/* ジョイスティック読み取り */
				case WHG_JOYSTICK:
					whg_pstate = WHG_JOYSTICK;
					break;
			}
			return TRUE;

		/* データレジスタ */
		case 0xfd46:
			whg_seldat = dat;
			/* インアクティブ以外の場合は、所定の動作を行う */
			switch (whg_pstate){
				/* データ書き込み */
				case WHG_WRITEDAT:
					whg_writereg(whg_selreg, whg_seldat);
					break;
				/* ラッチアドレス */
				case WHG_ADDRESS:
					whg_selreg = whg_seldat;
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	WHG
 *	セーブ
 */
BOOL FASTCALL whg_save(int fileh)
{
	if (!file_bool_write(fileh, whg_enable)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_use)) {
		return FALSE;
	}

	if (!file_write(fileh, whg_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, whg_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, whg_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_scale)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, whg_pstate)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_selreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_seldat)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb_en)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	WHG
 *	ロード
 */
BOOL FASTCALL whg_load(int fileh, int ver)
{
	int i;

	/* ファイルバージョン3で追加 */
	if (ver < 3) {
		return TRUE;
	}

	if (!file_bool_read(fileh, &whg_enable)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_use)) {
		return FALSE;
	}

	if (!file_read(fileh, whg_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &whg_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &whg_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_scale)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &whg_pstate)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_selreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_seldat)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb_en)) {
		return FALSE;
	}

	/* WHGレジスタ復旧 */
	whg_notify(0x27, whg_reg[0x27]);
	whg_notify(0x28, 0);
	whg_notify(0x28, 1);
	whg_notify(0x28, 2);

	whg_notify(8, 0);
	whg_notify(9, 0);
	whg_notify(10, 0);
	for (i=0; i<14; i++) {
		if ((i < 8) || (i > 10)) {
			whg_notify((BYTE)i, whg_reg[i]);
		}
	}

	for (i=0x30; i<0xb4; i++) {
		if ((i & 0x03) != 3) {
			whg_notify((BYTE)i, whg_reg[i]);
		}
	}

	/* イベント */
	schedule_handle(EVENT_WHG_A, whg_timera_event);
	schedule_handle(EVENT_WHG_B, whg_timerb_event);

	return TRUE;
}
