/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API サウンド ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#define DIRECTSOUND_VERSION		0x300	/* DirectX3を指定 */
#include <dsound.h>
#include <assert.h>
#include <math.h>
#include "xm7.h"
#include "device.h"
#include "w32.h"
#include "w32_snd.h"
#include "device.h"
#include "mainetc.h"
#include "opn.h"
#include "whg.h"
#include "cisc.h"
#include "opna.h"

/*
 *	グローバル ワーク
 */
UINT nSampleRate;						/* サンプリングレート */
UINT nSoundBuffer;						/* サウンドバッファサイズ */
UINT nBeepFreq;							/* BEEP周波数 */
int hWavCapture;						/* WAVキャプチャハンドル */
BOOL bWavCapture;						/* WAVキャプチャ開始 */

/*
 *	スタティック ワーク
 */
static LPDIRECTSOUND lpds;				/* DirectSound */
static LPDIRECTSOUNDBUFFER lpdsp;		/* DirectSoundBuffer(プライマリ) */
static LPDIRECTSOUNDBUFFER lpdsb;		/* DirectSoundBuffer(セカンダリ) */
static WORD *lpsbuf;					/* サウンド作成バッファ */
static DWORD *lpstmp;					/* サウンド一時バッファ */
static BOOL bNowBank;					/* 現在再生中のバンク */
static UINT uBufSize;					/* サウンドバッファサイズ */
static UINT uRate;						/* 合成レート */
static UINT uTick;						/* 半バッファサイズの長さ */
static UINT uFillMs;					/* １回に合成する時間 */
static UINT uCount;						/* バッファ充填カウンタ */
static UINT uBeep;						/* BEEP波形カウンタ */
static FM::OPN *pOPN[2];				/* OPNデバイス */
static int nScale[2];					/* OPNプリスケーラ */
static BOOL bInitFlag;					/* 初期化フラグ */
static WORD *pWavCapture;				/* キャプチャバッファ(64KB) */
static UINT nWavCapture;				/* キャプチャバッファ実データ */
static DWORD dwWavCapture;				/* キャプチャファイルサイズ */

/*
 *	初期化
 */
void FASTCALL InitSnd(void)
{
	/* ワークエリア初期化 */
	nSampleRate = 44100;
	nSoundBuffer = 80;
	nBeepFreq = 1200;

	hWavCapture = -1;
	bWavCapture = FALSE;
	pWavCapture = NULL;
	nWavCapture = 0;
	dwWavCapture = 0;

	lpds = NULL;
	lpdsp = NULL;
	lpdsb = NULL;
	lpsbuf = NULL;
	lpstmp = NULL;
	bNowBank = FALSE;
	uBufSize = 0;
	uRate = 0;
	uCount = 0;
	uBeep = 0;
	uFillMs = 1;

	pOPN[0] = NULL;
	pOPN[1] = NULL;
	nScale[0] = 0;
	nScale[1] = 0;
	bInitFlag = FALSE;
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanSnd()
{
	int i;

	/* サウンド停止 */
	StopSnd();

	/* OPNを解放 */
	for (i=0; i<2; i++) {
		if (pOPN[i]) {
			delete pOPN[i];
			pOPN[i] = NULL;
		}
	}

	/* サウンド作成バッファを解放 */
	if (lpstmp) {
		free(lpstmp);
		lpstmp = NULL;
	}
	if (lpsbuf) {
		free(lpsbuf);
		lpsbuf = NULL;
	}

	/* DirectSoundBufferを解放 */
	if (lpdsb) {
		lpdsb->Release();
		lpdsb = NULL;
	}
	if (lpdsp) {
		lpdsp->Release();
		lpdsp = NULL;
	}

	/* DirectSoundを解放 */
	if (lpds) {
		lpds->Release();
		lpds = NULL;
	}

	/* uRateをクリア */
	uRate = 0;

	/* キャプチャ関連 */
	if (hWavCapture >= 0) {
		CloseCaptureSnd();
	}
	if (pWavCapture) {
		free(pWavCapture);
		pWavCapture = NULL;
	}
	hWavCapture = -1;
	bWavCapture = FALSE;
}

/*
 *	レジスタ設定
 */
static void FASTCALL SetReg(FM::OPN *pOPN, BYTE *reg)
{
	int i;

	/* SSG */
	for (i=0; i<16; i++) {
		pOPN->SetReg((BYTE)i, reg[i]);
	}

	/* FM音源キーオフ */
	for (i=0; i<3; i++) {
		pOPN->SetReg(0x28, (BYTE)i);
	}

	/* FM音源レジスタ */
	for (i=0x30; i<0xb4; i++) {
		pOPN->SetReg((BYTE)i, reg[i]);
	}
}

/*
 *	セレクト
 */
BOOL FASTCALL SelectSnd(HWND hWnd)
{
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;

	/* assert */
	ASSERT(hWnd);

	/* 起動フラグ立てる */
	bInitFlag = TRUE;

	/* パラメータを設定 */
	uRate = nSampleRate;
	uTick = nSoundBuffer;
	uFillMs = 1;
	uCount = 0;

	/* rate==0なら、何もしない */
	if (uRate == 0) {
		return TRUE;
	}

	/* DiectSoundオブジェクト作成 */
	if (FAILED(DirectSoundCreate(NULL, &lpds, NULL))) {
		/* デフォルトデバイスなしか、使用中 */
		return TRUE;
	}

	/* 協調レベルを設定(優先協調) */
	if (FAILED(lpds->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
		return FALSE;
	}

	/* プライマリバッファを作成 */
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsp, NULL))) {
		return FALSE;
	}

	/* プライマリバッファのフォーマットを指定 */
	memset(&wfex, 0, sizeof(wfex));
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;
	wfex.nSamplesPerSec = uRate;
	wfex.nBlockAlign = 2;
	wfex.nAvgBytesPerSec =
			wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if (FAILED(lpdsp->SetFormat(&wfex))) {
		return FALSE;
	}

	/* セカンダリバッファを作成 */
	memset(&pcmwf, 0, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 1;
	pcmwf.wf.nSamplesPerSec = uRate;
	pcmwf.wf.nBlockAlign = 2;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = (pcmwf.wf.nAvgBytesPerSec * uTick * 2) / 1000;
	dsbd.dwBufferBytes = ((dsbd.dwBufferBytes + 1) >> 1) << 1;	// 2バイト境界
	uBufSize = dsbd.dwBufferBytes;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if (FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))) {
		return FALSE;
	}

	/* サウンドバッファを作成(セカンダリバッファのハーフサイズをつくる) */
	lpsbuf = (WORD *)malloc(uBufSize / 2);
	if (lpsbuf == NULL) {
		return FALSE;
	}
	memset(lpsbuf, 0, uBufSize / 2);

	/* 一時バッファを作成 */
	lpstmp = (DWORD *)malloc(uBufSize);
	if (lpstmp == NULL) {
		return FALSE;
	}

	/* OPNデバイス(標準)を作成 */
	pOPN[0] = new FM::OPN;
	pOPN[0]->Init(122880, uRate / 10, FALSE, NULL);
	pOPN[0]->Reset();
	pOPN[0]->SetReg(0x27, 0);

	/* OPNデバイス(WHG)を作成 */
	pOPN[1] = new FM::OPN;
	pOPN[1]->Init(122880, uRate / 10, FALSE, NULL);
	pOPN[1]->Reset();
	pOPN[1]->SetReg(0x27, 0);

	/* 再セレクトに備え、レジスタ設定 */
	nScale[0] = 0;
	nScale[1] = 0;
	opn_notify(0x27, 0);
	whg_notify(0x27, 0);
	SetReg(pOPN[0], opn_reg);
	SetReg(pOPN[1], whg_reg);

	/* キャプチャ関連 */
	if (!pWavCapture) {
		pWavCapture = (WORD *)malloc(sizeof(WORD) * 0x8000);
	}
	ASSERT(hWavCapture == -1);
	ASSERT(!bWavCapture);

	/* サウンドスタート */
	bNowBank = FALSE;
	PlaySnd();

	return TRUE;
}

/*
 *	適用
 */
void FASTCALL ApplySnd(void)
{
	/* 起動処理時は、リターン */
	if (!bInitFlag) {
		return;
	}

	/* パラメータ一致チェック */
	if ((uRate == nSampleRate) && (uTick == nSoundBuffer)) {
		return;
	}

	/* 既に準備ができているなら、解放 */
	if (uRate != 0) {
		CleanSnd();
	}

	/* 再セレクト */
	SelectSnd(hMainWnd);
}

/*
 *	演奏開始
 */
void FASTCALL PlaySnd()
{
	HRESULT hr;
	WORD *ptr1, *ptr2;
	DWORD size1, size2;

	if (lpdsb) {
		/* バッファをすべてクリアする */
		if (lpsbuf) {
			memset(lpsbuf, 0, uBufSize / 2);
		}
		if (lpstmp) {
			memset(lpstmp, 0, uBufSize);
		}

		/* ロック */
		hr = lpdsb->Lock(0, uBufSize, (void **)&ptr1, &size1,
						(void**)&ptr2, &size2, 0);
		/* バッファが失われていれば、リストア */
		if (hr == DSERR_BUFFERLOST) {
			lpdsb->Restore();
		}
		/* ロック成功した場合のみ、セット */
		if (SUCCEEDED(hr)) {
			if (ptr1) {
				memset(ptr1, 0, size1);
			}
			if (ptr2) {
				memset(ptr2, 0, size2);
			}

			/* アンロック */
			lpdsb->Unlock(ptr1, size1, ptr2, size2);
		}

		/* 演奏開始 */
		lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	}
}

/*
 *	演奏停止
 */
void FASTCALL StopSnd()
{
	if (lpdsb) {
		lpdsb->Stop();
	}
}

/*
 *	BEEP合成
 */
static void FASTCALL BeepSnd(int *buf, int samples)
{
	int sf;
	int i;

	/* BEEP音出力チェック */
	if (!beep_flag || !speaker_flag) {
		return;
	}

	/* サンプル書き込み */
	for (i=0; i<samples; i++) {
		/* 矩形波を作成 */
		sf = (int)(uBeep * nBeepFreq * 2);
		sf /= (int)uRate;

		/* 偶・奇に応じてサンプル書き込み */
		if (sf & 1) {
			*buf += 0x00000c00;
		}
		else {
			*buf -= 0x00000c00;
		}
		buf++;

		/* カウンタアップ */
		uBeep++;
		if (uBeep >= uRate) {
			uBeep = 0;
		}
	}
}

/*
 *	サウンド作成バッファへ追加
 */
static void FASTCALL AddSnd(BOOL bFill, BOOL bZero)
{
	int i;
	int samples;
	int dat;
	WORD highlow;
	WORD *q;

	/* OPNデバイスが作成されていなければ、何もしない */
	if (!pOPN[0] && !pOPN[1]) {
		return;
	}

	/* 既に十分追加されていれば、追加しない */
	if (uCount == (uBufSize / 4)) {
		return;
	}

	/* サンプル数決定 */
	if (bFill) {
		samples = (uBufSize / 4) - uCount;
	}
	else {
		samples = (uBufSize * uFillMs) / (uTick * 4);
		if (samples > (int)(((uBufSize / 4) - uCount))) {
			samples = (uBufSize / 4) - uCount;
		}
	}

	/* ミキシング */
	memset(lpstmp, 0, samples * sizeof(int32));
	if (!bZero) {
		if (pOPN[0]) {
			pOPN[0]->Mix((int32*)lpstmp, samples);
		}
		if (pOPN[1] && whg_use) {
			pOPN[1]->Mix((int32*)lpstmp, samples);
		}
		BeepSnd((int*)lpstmp, samples);
	}

	/* 変換 */
	q = &lpsbuf[uCount];
	for (i=0; i<samples; i++) {
		dat = (int32)lpstmp[i];
		highlow = (WORD)(dat & 0x0000ffff);

		/* クリッピング */
		if ((dat > 0) && (highlow >= 0x8000)) {
			*q++ = 0x7fff;
			continue;
		}
		if ((dat < 0) && (highlow < 0x8000)) {
			*q++ = 0x8000;
			continue;
		}

		/* ストア */
		*q++ = highlow;
	}

	/* カウントアップ */
	uCount += samples;
}

/*
 *	WAVキャプチャ処理
 */
static void FASTCALL WavCapture(void)
{
	UINT nSize;
	WORD *p;

	/* WAVキャプチャ中でなければ、リターン */
	if (hWavCapture < 0) {
		return;
	}
	ASSERT(pWavCapture);

	/* ポインタ、サイズを仮決め */
	p = lpsbuf;
	nSize = uBufSize / 2;

	/* bWavCaptureがFALSEなら */
	if (!bWavCapture) {
		/* 頭出しチェック */
		while (nSize > 0) {
			if (*p != 0) {
				break;
			}
			nSize -= 2;
			p++;
		}
		/* 判定 */
		if (nSize == 0) {
			return;
		}
	}

	/* nWavCaptureを考慮 */
	if ((nWavCapture + nSize) >= 0x8000) {
		/* 32KBいっぱいまでコピー */
		memcpy(&pWavCapture[nWavCapture / 2], p, (0x8000 - nWavCapture));
		nSize -= (0x8000 - nWavCapture);
		p += (0x8000 - nWavCapture) / 2;

		/* 書き込み */
		file_write(hWavCapture, (BYTE*)pWavCapture, 0x8000);
		dwWavCapture += 0x8000;
		nWavCapture = 0;
	}

	/* 余りをバッファへ */
	memcpy(&pWavCapture[nWavCapture / 2], p, nSize);
	nWavCapture += nSize;

	/* 正式な録音状態 */
	bWavCapture = TRUE;
}

/*
 *	定期処理
 */
void FASTCALL ProcessSnd(BOOL bZero)
{
	DWORD dwPlayC, dwWriteC;
	BOOL bWrite;
	DWORD dwOffset;
	WORD *ptr1, *ptr2;
	DWORD size1, size2;
	WORD *p;
	HRESULT hr;

	/* OPNデバイスが作成されていなければ、何もしない */
	if (!pOPN[0] && !pOPN[1]) {
		return;
	}

	/* 書き込み位置を得る */
	if (FAILED(lpdsb->GetCurrentPosition(&dwPlayC, &dwWriteC))) {
		return;
	}

	/* 書き込み位置とバンクから、必要性を判断 */
	bWrite = FALSE;
	if (bNowBank) {
		if (dwPlayC >= (uBufSize / 2)) {
			dwOffset = 0;
			bWrite = TRUE;
		}
	}
	else {
		if (dwPlayC < (uBufSize / 2)) {
			dwOffset = uBufSize / 2;
			bWrite = TRUE;
		}
	}

	/* 書き込む必要がなければ、通常処理 */
	if (!bWrite) {
		AddSnd(FALSE, bZero);
		return;
	}

	/* 書き込み。まずサウンド作成バッファを全部埋める */
	AddSnd(TRUE, bZero);

	/* 次いでロック */
	hr = lpdsb->Lock(dwOffset, uBufSize / 2, (void **)&ptr1, &size1,
						(void**)&ptr2, &size2, 0);

	/* バッファが失われていれば、リストア */
	if (hr == DSERR_BUFFERLOST) {
		lpdsb->Restore();
	}
	/* ロック成功しなければ、続けても意味がない */
	if (FAILED(hr)) {
		return;
	}

	/* サウンド作成バッファ→セカンダリバッファ */
	p = lpsbuf;
	memcpy(ptr1, p, size1);
	p += size1;
	if (ptr2) {
		memcpy(ptr2, p, size2);
	}

	/* アンロック */
	lpdsb->Unlock(ptr1, size1, ptr2, size2);

	/* バンク反転、充填カウンタをクリア */
	bNowBank = (!bNowBank);
	uCount = 0;

	/* WAVキャプチャ処理 */
	WavCapture();
}

/*
 *	レベル取得
 */
int FASTCALL GetLevelSnd(int ch)
{
	FM::OPN *p;
	int i;
	double s;
	double t;
	int *buf;

	ASSERT((ch >= 0) && (ch < 12));

	/* OPN,WHGの区別 */
	if (ch < 6) {
		p = pOPN[0];
	}
	else {
		p = pOPN[1];
		ch -= 6;

		/* WHGの場合、実際に使われていなければ0 */
		if (!whg_enable || !whg_use) {
			return 0;
		}
	}

	/* 存在チェック */
	if (!p) {
		return 0;
	}

	/* FM,PSGの区別 */
	if (ch < 3) {
		/* FM:512サンプルの2乗和を計算 */
		buf = p->rbuf[ch];

		s = 0;
		for (i=0; i<512; i++) {
			t = (double)*buf++;
			t *= t;
			s += t;
		}
		s /= 512;

		/* ゼロチェック */
		if (s == 0) {
			return 0;
		}

		/* log10を取る */
		s = log10(s);

		/* FM音源補正 */
		s *= 40.0;
	}
	else {
		/* SSG:512サンプルの2乗和を計算 */
		buf = p->psg.rbuf[ch - 3];

		s = 0;
		for (i=0; i<512; i++) {
			t = (double)*buf++;
			t *= t;
			s += t;
		}
		s /= 512;

		/* ゼロチェック */
		if (s == 0) {
			return 0;
		}

		/* log10を取る */
		s = log10(s);

		/* PSG音源補正 */
		s *= 60.0;
	}

	return (int)s;
}

/*
 *	WAVキャプチャ開始
 */
void FASTCALL OpenCaptureSnd(char *fname)
{
	WAVEFORMATEX wfex;
	DWORD dwSize;
	int fileh;

	ASSERT(fname);
	ASSERT(hWavCapture < 0);
	ASSERT(!bWavCapture);

	/* 合成中でなければ、リターン */
	if (!pOPN[0]) {
		return;
	}

	/* バッファが無ければ、リターン */
	if (!pWavCapture) {
		return;
	}

	/* uBufSize / 2が0x8000以下でないとエラー */
	if ((uBufSize / 2) > 0x8000) {
		return;
	}

	/* ファイルオープン(書き込みモード) */
	fileh = file_open(fname, OPEN_W);
	if (fileh < 0) {
		return;
	}

	/* RIFFヘッダ書き込み */
	if (!file_write(fileh, (BYTE*)"RIFF0123WAVEfmt ", 16)) {
		file_close(fileh);
		return;
	}

	/* WAVEFORMATEX書き込み */
	dwSize = sizeof(wfex);
	if (!file_write(fileh, (BYTE*)&dwSize, sizeof(dwSize))) {
		file_close(fileh);
		return;
	}
	memset(&wfex, 0, sizeof(wfex));
	wfex.cbSize = sizeof(wfex);
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;
	wfex.nSamplesPerSec = uRate;
	wfex.nBlockAlign = 2;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if (!file_write(fileh, (BYTE *)&wfex, sizeof(wfex))) {
		file_close(fileh);
		return;
	}

	/* dataサブヘッダ書き込み */
	if (!file_write(fileh, (BYTE *)"data0123", 8)) {
		file_close(fileh);
		return;
	}

	/* ok */
	nWavCapture = 0;
	dwWavCapture = 0;
	bWavCapture = FALSE;
	hWavCapture = fileh;
}

/*
 *	WAVキャプチャ終了
 */
void FASTCALL CloseCaptureSnd(void)
{
	DWORD dwLength;

	ASSERT(hWavCapture >= 0);

	/* バッファに残った分を書き込み */
	file_write(hWavCapture, (BYTE*)pWavCapture, nWavCapture);
	dwWavCapture += nWavCapture;
	nWavCapture = 0;

	/* ファイルレングスを書き込む */
	file_seek(hWavCapture, 4);
	dwLength = dwWavCapture + sizeof(WAVEFORMATEX) + 20;
	file_write(hWavCapture, (BYTE *)&dwLength, sizeof(dwLength));

	/* data部レングスを書き込む */
	file_seek(hWavCapture, sizeof(WAVEFORMATEX) + 24);
	file_write(hWavCapture, (BYTE *)&dwWavCapture, sizeof(dwWavCapture));

	/* ファイルクローズ */
	file_close(hWavCapture);

	/* ワークエリアクリア */
	hWavCapture = -1;
	bWavCapture = FALSE;
}

/*
 *	OPN出力
 */
extern "C" {
void FASTCALL opn_notify(BYTE reg, BYTE dat)
{
	/* OPNがなければ、何もしない */
	if (!pOPN[0]) {
		return;
	}

	/* プリスケーラを調整 */
	if (opn_scale != nScale[0]) {
		nScale[0] = opn_scale;

		switch (opn_scale) {
			case 2:
				pOPN[0]->SetReg(0x2f, 0);
				break;
			case 3:
				pOPN[0]->SetReg(0x2e, 0);
				break;
			case 6:
				pOPN[0]->SetReg(0x2d, 0);
				break;
		}
	}

	/* 出力 */
	pOPN[0]->SetReg((uint8)reg, (uint8)dat);
}
}

/*
 *	WHG出力
 */
extern "C" {
void FASTCALL whg_notify(BYTE reg, BYTE dat)
{
	/* WHGがなければ、何もしない */
	if (!pOPN[1]) {
		return;
	}

	/* プリスケーラを調整 */
	if (opn_scale != nScale[1]) {
		nScale[1] = opn_scale;

		switch (opn_scale) {
			case 2:
				pOPN[1]->SetReg(0x2f, 0);
				break;
			case 3:
				pOPN[1]->SetReg(0x2e, 0);
				break;
			case 6:
				pOPN[1]->SetReg(0x2d, 0);
				break;
		}
	}

	/* 出力 */
	pOPN[1]->SetReg((uint8)reg, (uint8)dat);
}
}

#endif	/* _WIN32 */
