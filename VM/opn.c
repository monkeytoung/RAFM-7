/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ OPN(YM2203) ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "opn.h"
#include "device.h"
#include "mainetc.h"
#include "event.h"
#include "whg.h"

/*
 *	グローバル ワーク
 */
BYTE opn_reg[256];						/* OPNレジスタ */
BOOL opn_key[4];						/* OPNキーオンフラグ */
BOOL opn_timera;						/* タイマーA動作フラグ */
BOOL opn_timerb;						/* タイマーB動作フラグ */
DWORD opn_timera_tick;					/* タイマーA間隔 */
DWORD opn_timerb_tick;					/* タイマーB間隔 */
BYTE opn_scale;							/* プリスケーラ */

/*
 *	スタティック ワーク
 */
static BYTE opn_pstate;					/* ポート状態 */
static BYTE opn_selreg;					/* セレクトレジスタ */
static BYTE opn_seldat;					/* セレクトデータ */
static BOOL opn_timera_int;				/* タイマーAオーバーフロー */
static BOOL opn_timerb_int;				/* タイマーBオーバーフロー */
static BOOL opn_timera_en;				/* タイマーAイネーブル */
static BOOL opn_timerb_en;				/* タイマーBイネーブル */

/*
 *	OPN
 *	初期化
 */
BOOL FASTCALL opn_init(void)
{
	memset(opn_reg, 0, sizeof(opn_reg));

	return TRUE;
}

/*
 *	OPN
 *	クリーンアップ
 */
void FASTCALL opn_cleanup(void)
{
	BYTE i;

	/* PSG */
	for (i=0; i<6; i++) {
		opn_notify(i, 0);
	}
	opn_notify(7, 0xff);

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x7f);
	}

	/* キーオフ */
	for (i=0; i<3; i++) {
		opn_notify(0x28, i);
	}
}

/*
 *	OPN
 *	リセット
 */
void FASTCALL opn_reset(void)
{
	BYTE i;

	/* レジスタクリア、タイマーOFF */
	memset(opn_reg, 0, sizeof(opn_reg));
	opn_timera = FALSE;
	opn_timerb = FALSE;

	/* I/O初期化 */
	opn_pstate = OPN_INACTIVE;
	opn_selreg = 0;
	opn_seldat = 0;

	/* デバイス */
	opn_timera_int = FALSE;
	opn_timerb_int = FALSE;
	opn_timera_tick = 0;
	opn_timerb_tick = 0;
	opn_timera_en = FALSE;
	opn_timerb_en = FALSE;
	opn_scale = 3;

	/* PSG初期化 */
	for (i=0; i<14;i++) {
		if (i == 7) {
			opn_notify(i, 0xff);
			opn_reg[i] = 0xff;
		}
		else {
			opn_notify(i, 0);
		}
	}

	/* MUL,DT */
	for (i=0x30; i<0x40; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0);
	}

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x7f);
		opn_reg[i] = 0x7f;
	}

	/* AR=$1F */
	for (i=0x50; i<0x60; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x1f);
		opn_reg[i] = 0x1f;
	}

	/* その他 */
	for (i=0x60; i<0xb4; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0);
	}

	/* SL,RR */
	for (i=0x80; i<0x90; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0xff);
		opn_reg[i] = 0xff;
	}

	/* キーオフ */
	for (i=0; i<3; i++) {
		opn_notify(0x28, i);
	}

	/* モード */
	opn_notify(0x27, 0);
}

/*
 *	OPN
 *	タイマーAオーバフロー
 */
static BOOL FASTCALL opn_timera_event(void)
{
	/* イネーブルか */
	if (opn_timera_en) {
		/* オーバーフローアクションが有効か */
		if (opn_timera) {
			opn_timera = FALSE;
			opn_timera_int = TRUE;

			/* 割り込みをかける */
			opn_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* CSM音声合成モードでのキーオン */
	opn_notify(0xff, 0);

	/* タイマーは回し続ける */
	return TRUE;
}

/*
 *	OPN
 *	タイマーAインターバル算出
 */
static void FASTCALL opn_timera_calc(void)
{
	DWORD t;
	BYTE temp;

	t = opn_reg[0x24];
	t *= 4;
	temp = (BYTE)(opn_reg[0x25] & 3);
	t |= temp;
	t &= 0x3ff;
	t = (1024 - t);
	t *= opn_scale;
	t *= 12;
	t *= 10000;
	t /= 12288;

	/* タイマー値を設定 */
	if (opn_timera_tick != t) {
		opn_timera_tick = t;
		schedule_setevent(EVENT_OPN_A, opn_timera_tick, opn_timera_event);
	}
}

/*
 *	OPN
 *	タイマーBオーバフロー
 */
static BOOL FASTCALL opn_timerb_event(void)
{
	/* イネーブルか */
	if (opn_timerb_en) {
		/* オーバーフローアクションが有効か */
		if (opn_timerb) {
			/* フラグ変更 */
			opn_timerb = FALSE;
			opn_timerb_int = TRUE;

			/* 割り込みをかける */
			opn_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* タイマーは回し続ける */
	return TRUE;
}

/*
 *	OPN
 *	タイマーBインターバル算出
 */
static void FASTCALL opn_timerb_calc(void)
{
	DWORD t;

	t = opn_reg[0x26];
	t = (256 - t);
	t *= 192;
	t *= opn_scale;
	t *= 10000;
	t /= 12288;

	/* タイマー値を設定 */
	if (t != opn_timerb_tick) {
		opn_timerb_tick = t;
		schedule_setevent(EVENT_OPN_B, opn_timerb_tick, opn_timerb_event);
	}
}

/*
 *	OPN
 *	レジスタアレイより読み出し
 */
static BYTE FASTCALL opn_readreg(BYTE reg)
{
	/* FM音源部は読み出せない */
	if (reg >= 0x10) {
		return 0xff;
	}

	return opn_reg[reg];
}

/*
 *	OPN
 *	レジスタアレイへ書き込み
 */
static void FASTCALL opn_writereg(BYTE reg, BYTE dat)
{
	/* タイマー処理 */
	/* このレジスタは非常に難しい。良く分からないまま扱っている人が大半では？ */
	if (reg == 0x27) {
		/* オーバーフローフラグのクリア */
		if (dat & 0x10) {
			opn_timera_int = FALSE;
		}
		if (dat & 0x20) {
			opn_timerb_int = FALSE;
		}

		/* 両方落ちたら、割り込みを落とす */
		if (!opn_timera_int && !opn_timerb_int) {
			opn_irq_flag = FALSE;
			maincpu_irq();
		}

		/* タイマーA */
		if (dat & 0x01) {
			/* 0→1でタイマー値をロード、それ以外でもタイマーon */
			if ((opn_reg[0x27] & 0x01) == 0) {
				opn_timera_calc();
			}
			opn_timera_en = TRUE;
		}
		else {
			opn_timera_en = FALSE;
		}
		if (dat & 0x04) {
			opn_timera = TRUE;
		}
		else {
			opn_timera = FALSE;
		}

		/* タイマーB */
		if (dat & 0x02) {
			/* 0→1でタイマー値をロード、それ以外でもタイマーon */
			if ((opn_reg[0x27] & 0x02) == 0) {
				opn_timerb_calc();
			}
			opn_timerb_en = TRUE;
		}
		else {
			opn_timerb_en = FALSE;
		}
		if (dat & 0x08) {
			opn_timerb = TRUE;
		}
		else {
			opn_timerb = FALSE;
		}

		/* データ記憶 */
		opn_reg[reg] = dat;

		/* モードのみ出力 */
		opn_notify(0x27, (BYTE)(dat & 0xc0));
		return;
	}

	/* データ記憶 */
	opn_reg[reg] = dat;

	switch (reg) {
		/* プリスケーラ１ */
		case 0x2d:
			if (opn_scale != 3) {
				opn_scale = 6;
				opn_timerb_calc();
				opn_timerb_calc();
			}
			return;

		/* プリスケーラ２ */
		case 0x2e:
			opn_scale = 3;
			opn_timerb_calc();
			opn_timerb_calc();
			return;

		/* プリスケーラ３ */
		case 0x2f:
			opn_scale = 2;
			opn_timerb_calc();
			opn_timerb_calc();
			return;

		/* タイマーA(us単位で計算) */
		case 0x24:
			opn_timera_calc();
			return;

		case 0x25:
			opn_timera_calc();
			return;

		/* タイマーB(us単位で計算) */
		case 0x26:
			opn_timerb_calc();
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
			opn_key[dat & 0x03] = TRUE;
		}
		else {
			opn_key[dat & 0x03] = FALSE;
		}
	}

	/* 出力 */
	opn_notify(reg, dat);
}

/*
 *	OPN
 *	１バイト読み出し
 */
BOOL FASTCALL opn_readb(WORD addr, BYTE *dat)
{
	switch (addr) {
		/* コマンドレジスタは読み出し禁止 */
		case 0xfd0d:
		case 0xfd15:
			*dat = 0xff;
			return TRUE;

		/* データレジスタ */
		case 0xfd0e:
		case 0xfd16:
			switch (opn_pstate) {
				/* 通常コマンド */
				case OPN_INACTIVE:
				case OPN_READDAT:
				case OPN_WRITEDAT:
				case OPN_ADDRESS:
					*dat = opn_seldat;
					break;

				/* ステータス読み出し */
				case OPN_READSTAT:
					*dat = 0;
					if (opn_timera_int) {
						*dat |= 0x01;
					}
					if (opn_timerb_int) {
						*dat |= 0x02;
					}
					break;

				/* ジョイスティック読み取り */
				case OPN_JOYSTICK:
					if (opn_selreg == 14) {
						if ((opn_reg[15] & 0xf0) == 0x20) {
							/* ジョイスティック１ */
							*dat = (BYTE)(~joy_request(0) | 0xc0);
							break;
						}
						if ((opn_reg[15] & 0xf0) == 0x50) {
							/* ジョイスティック２ */
							*dat = (BYTE)(~joy_request(1) | 0xc0);
							break;
						}
						/* それ以外 */
						*dat = 0xff;
					}
					else {
						/* レジスタが14でなければ、FF以外を返す */
						/* HOW MANY ROBOT対策 */
						*dat = 0;
					}
					break;
			}
			return TRUE;

		/* 拡張割り込みステータス */
		case 0xfd17:
			if (opn_timera_int || opn_timerb_int) {
				*dat = 0xf7;
			}
			else {
				*dat = 0xff;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	OPN
 *	１バイト書き込み
 */
BOOL FASTCALL opn_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* OPNコマンドレジスタ */
		case 0xfd0d:
		case 0xfd15:
			switch (dat & 0x0f) {
				/* インアクティブ(動作定義なし、データレジスタ書き換え可) */
				case OPN_INACTIVE:
					opn_pstate = OPN_INACTIVE;
					break;
				/* データ読み出し */
				case OPN_READDAT:
					opn_pstate = OPN_READDAT;
					opn_seldat = opn_readreg(opn_selreg);
					break;
				/* データ書き込み */
				case OPN_WRITEDAT:
					opn_pstate = OPN_WRITEDAT;
					opn_writereg(opn_selreg, opn_seldat);
					break;
				/* ラッチアドレス */
				case OPN_ADDRESS:
					opn_pstate = OPN_ADDRESS;
					opn_selreg = opn_seldat;
					break;
				/* リードステータス */
				case OPN_READSTAT:
					opn_pstate = OPN_READSTAT;
					break;
				/* ジョイスティック読み取り */
				case OPN_JOYSTICK:
					opn_pstate = OPN_JOYSTICK;
					break;
			}
			return TRUE;

		/* データレジスタ */
		case 0xfd0e:
		case 0xfd16:
			opn_seldat = dat;
			/* インアクティブ以外の場合は、所定の動作を行う */
			switch (opn_pstate){
				/* データ書き込み */
				case OPN_WRITEDAT:
					opn_writereg(opn_selreg, opn_seldat);
					break;
				/* ラッチアドレス */
				case OPN_ADDRESS:
					opn_selreg = opn_seldat;
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	OPN
 *	セーブ
 */
BOOL FASTCALL opn_save(int fileh)
{
	if (!file_write(fileh, opn_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, opn_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, opn_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_scale)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, opn_pstate)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_selreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_seldat)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb_en)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	OPN
 *	ロード
 */
BOOL FASTCALL opn_load(int fileh, int ver)
{
	int i;

	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_read(fileh, opn_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &opn_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &opn_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_scale)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &opn_pstate)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_selreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_seldat)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb_en)) {
		return FALSE;
	}

	/* OPNレジスタ復旧 */
	opn_notify(0x27, opn_reg[0x27]);
	opn_notify(0x28, 0);
	opn_notify(0x28, 1);
	opn_notify(0x28, 2);

	opn_notify(8, 0);
	opn_notify(9, 0);
	opn_notify(10, 0);
	for (i=0; i<14; i++) {
		if ((i < 8) || (i > 10)) {
			opn_notify((BYTE)i, opn_reg[i]);
		}
	}

	for (i=0x30; i<0xb4; i++) {
		if ((i & 0x03) != 3) {
			opn_notify((BYTE)i, opn_reg[i]);
		}
	}

	/* イベント */
	schedule_handle(EVENT_OPN_A, opn_timera_event);
	schedule_handle(EVENT_OPN_B, opn_timerb_event);

	return TRUE;
}
