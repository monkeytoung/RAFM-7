/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ カセットテープ＆プリンタ ]
 */

#ifndef _tapelp_h_
#define _tapelp_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL tapelp_init(void);
										/* 初期化 */
void FASTCALL tapelp_cleanup(void);
										/* クリーンアップ */
void FASTCALL tapelp_reset(void);
										/* リセット */
BOOL FASTCALL tapelp_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL tapelp_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL tapelp_save(int fileh);
										/* セーブ */
BOOL FASTCALL tapelp_load(int fileh, int ver);
										/* ロード */
void FASTCALL lp_setfile(char *fname);
										/* プリンタファイル設定 */
void FASTCALL tape_setfile(char *fname);
										/* テープファイル設定 */
void FASTCALL tape_setrec(BOOL flag);
										/* テープ録音フラグ設定 */
void FASTCALL tape_rew(void);
										/* テープ巻き戻し */
void FASTCALL tape_ff(void);
										/* テープ早送り */

/*
 *	主要ワーク
 */
extern BOOL tape_in;
										/* テープ 入力データ */
extern BOOL tape_out;
										/* テープ 出力データ */
extern BOOL tape_motor;
										/* テープ モータ */
extern BOOL tape_rec;
										/* テープ 録音中 */
extern BOOL tape_writep;
										/* テープ 録音不可 */
extern WORD tape_count;
										/* テープ カウンタ */
extern BYTE tape_subcnt;
										/* テープ サブカウント */
extern int tape_fileh;
										/* テープ ファイルハンドル */
extern DWORD tape_offset;
										/* テープ　ファイルオフセット */
extern char tape_fname[128+1];
										/* テープ ファイルネーム */

extern BYTE lp_data;
										/* プリンタ 出力データ */
extern BOOL lp_busy;
										/* プリンタ BUSYフラグ */
extern BOOL lp_error;
										/* プリンタ エラーフラグ */
extern BOOL lp_pe;
										/* プリンタ PEフラグ */
extern BOOL lp_ackng;
										/* プリンタ ACKフラグ */
extern BOOL lp_online;
										/* プリンタ オンライン */
extern BOOL lp_strobe;
										/* プリンタ ストローブ */
extern int lp_fileh;
										/* プリンタ ファイルハンドル */
#ifdef __cplusplus
}
#endif

#endif	/* _tapelp_h_ */
