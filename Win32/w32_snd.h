/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API サウンド ]
 */

#ifdef _WIN32

#ifndef _w32_snd_h_
#define _w32_snd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitSnd(void);
										/* 初期化 */
void FASTCALL CleanSnd(void);
										/* クリーンアップ */
BOOL FASTCALL SelectSnd(HWND hWnd);
										/* セレクト */
void FASTCALL ApplySnd(void);
										/* 適用 */
void FASTCALL PlaySnd(void);
										/* 演奏開始 */
void FASTCALL StopSnd(void);
										/* 演奏停止 */
void FASTCALL ProcessSnd(BOOL bZero);
										/* バッファ充填定時処理 */
int FASTCALL GetLevelSnd(int ch);
										/* サウンドレベル取得 */
void FASTCALL OpenCaptureSnd(char *fname);
										/* WAVキャプチャ開始 */
void FASTCALL CloseCaptureSnd(void);
										/* WAVキャプチャ終了 */

/*
 *	主要ワーク
 */
extern UINT nSampleRate;
										/* サンプルレート(Hz、0で無し) */
extern UINT nSoundBuffer;
										/* サウンドバッファ(ダブル、ms) */
extern UINT nBeepFreq;
										/* BEEP周波数(Hz) */
extern int hWavCapture;
										/* WAVキャプチャファイルハンドル */
extern BOOL bWavCapture;
										/* WAVキャプチャ開始後 */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_snd_h_ */
#endif	/* _WIN32 */
