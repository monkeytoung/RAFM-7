/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ OPN(YM2203) ]
 */

#ifndef _opn_h_
#define _opn_h_

/*
 *	定数定義
 */
#define OPN_INACTIVE		0x00			/* インアクティブコマンド */
#define OPN_READDAT			0x01			/* リードデータコマンド */
#define OPN_WRITEDAT		0x02			/* ライトデータコマンド */
#define OPN_ADDRESS			0x03			/* ラッチアドレスコマンド */
#define OPN_READSTAT		0x04			/* リードステータスコマンド */
#define OPN_JOYSTICK		0x09			/* ジョイスティックコマンド */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL opn_init(void);
										/* 初期化 */
void FASTCALL opn_cleanup(void);
										/* クリーンアップ */
void FASTCALL opn_reset(void);
										/* リセット */
BOOL FASTCALL opn_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL opn_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL opn_save(int fileh);
										/* セーブ */
BOOL FASTCALL opn_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BYTE opn_reg[256];
										/* OPNレジスタ */
extern BOOL opn_key[4];
										/* OPNキーオンフラグ */
extern BOOL opn_timera;
										/* タイマーA動作フラグ */
extern BOOL opn_timerb;
										/* タイマーB動作フラグ */
extern DWORD opn_timera_tick;
										/* タイマーA間隔(us) */
extern DWORD opn_timerb_tick;
										/* タイマーB間隔(us) */
extern BYTE opn_scale;
										/* プリスケーラ */
#ifdef __cplusplus
}
#endif

#endif	/* _opn_h_ */
