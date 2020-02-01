/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 補助ツール ]
 */

#ifndef _tools_h_
#define _tools_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL make_new_d77(char *fname, char *name);
										/* ブランクディスク作成 */
BOOL make_new_t77(char *fname);
										/* ブランクテープ作成 */
BOOL conv_vfd_to_d77(char *src, char *dst, char *name);
										/* VFD→D77変換 */
BOOL conv_2d_to_d77(char *src, char *dst, char *name);
										/* 2D→D77変換 */
BOOL conv_vtp_to_t77(char *src, char *dst);
										/* VTP→T77変換 */
BOOL capture_to_bmp(char *fname);
										/* 画面キャプチャ(BMP) */
#ifdef __cplusplus
}
#endif

#endif	/* _tools_h_ */
