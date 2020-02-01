/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ WHG(YM2203) ]
 */

#ifndef _whg_h_
#define _whg_h_

/*
 *	定数定義
 */
#define WHG_INACTIVE		0x00			/* インアクティブコマンド */
#define WHG_READDAT			0x01			/* リードデータコマンド */
#define WHG_WRITEDAT		0x02			/* ライトデータコマンド */
#define WHG_ADDRESS			0x03			/* ラッチアドレスコマンド */
#define WHG_READSTAT		0x04			/* リードステータスコマンド */
#define WHG_JOYSTICK		0x09			/* ジョイスティックコマンド */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL whg_init(void);
										/* 初期化 */
void FASTCALL whg_cleanup(void);
										/* クリーンアップ */
void FASTCALL whg_reset(void);
										/* リセット */
BOOL FASTCALL whg_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL whg_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL whg_save(int fileh);
										/* セーブ */
BOOL FASTCALL whg_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BOOL whg_enable;
										/* WHG有効・無効フラグ */
extern BOOL whg_use;
										/* WHG使用フラグ */
extern BYTE whg_reg[256];
										/* WHGレジスタ */
extern BOOL whg_key[4];
										/* WHGキーオンフラグ */
extern BOOL whg_timera;
										/* タイマーA動作フラグ */
extern BOOL whg_timerb;
										/* タイマーB動作フラグ */
extern DWORD whg_timera_tick;
										/* タイマーA間隔(us) */
extern DWORD whg_timerb_tick;
										/* タイマーB間隔(us) */
extern BYTE whg_scale;
										/* プリスケーラ */
#ifdef __cplusplus
}
#endif

#endif	/* _whg_h_ */
