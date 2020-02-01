/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ サブCPUコントロール ]
 */

#ifndef _subctrl_h_
#define _subctrl_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL subctrl_init(void);
										/* 初期化 */
void FASTCALL subctrl_cleanup(void);
										/* クリーンアップ */
void FASTCALL subctrl_reset(void);
										/* リセット */
BOOL FASTCALL subctrl_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL subctrl_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL subctrl_save(int fileh);
										/* セーブ */
BOOL FASTCALL subctrl_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BOOL subhalt_flag;
										/* サブHALTフラグ */
extern BOOL subbusy_flag;
										/* サブBUSYフラグ */
extern BOOL subcancel_flag;
										/* サブキャンセルフラグ */
extern BOOL subattn_flag;
										/* サブアテンションフラグ */
extern BYTE shared_ram[0x80];
										/* 共有RAM */
extern BOOL subreset_flag;
										/* サブ再起動フラグ */
extern BOOL mode320;
										/* 320x200モード */
#ifdef __cplusplus
}
#endif

#endif	/* _subctrl_h_ */
