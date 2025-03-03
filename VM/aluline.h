/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 論理演算・直線補間 ]
 */

#ifndef _aluline_h_
#define _aluline_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL aluline_init(void);
										/* 初期化 */
void FASTCALL aluline_cleanup(void);
										/* クリーンアップ */
void FASTCALL aluline_reset(void);
										/* リセット */
BOOL FASTCALL aluline_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL aluline_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
void FASTCALL aluline_extrb(WORD addr);
										/* VRAMダミーリード */
BOOL FASTCALL aluline_save(int fileh);
										/* セーブ */
BOOL FASTCALL aluline_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BYTE alu_command;
										/* 論理演算 コマンド */
extern BYTE alu_color;
										/* 論理演算 カラー */
extern BYTE alu_mask;
										/* 論理演算 マスクビット */
extern BYTE alu_cmpstat;
										/* 論理演算 比較ステータス */
extern BYTE alu_cmpdat[8];
										/* 論理演算 比較データ */
extern BYTE alu_disable;
										/* 論理演算 禁止バンク */
extern BYTE alu_tiledat[3];
										/* 論理演算 タイルパターン */
extern BOOL line_busy;
										/* 直線補間 BUSY */
extern WORD line_offset;
										/* 直線補間 アドレスオフセット */
extern WORD line_style;
										/* 直線補間 ラインスタイル */
extern WORD line_x0;
										/* 直線補間 X0 */
extern WORD line_y0;
										/* 直線補間 Y0 */
extern WORD line_x1;
										/* 直線補間 X1 */
extern WORD line_y1;
										/* 直線補間 Y1 */
#ifdef __cplusplus
}
#endif

#endif	/* _aluline_h_ */
