/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �T�E���h ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#define DIRECTSOUND_VERSION		0x300	/* DirectX3���w�� */
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
 *	�O���[�o�� ���[�N
 */
UINT nSampleRate;						/* �T���v�����O���[�g */
UINT nSoundBuffer;						/* �T�E���h�o�b�t�@�T�C�Y */
UINT nBeepFreq;							/* BEEP���g�� */
int hWavCapture;						/* WAV�L���v�`���n���h�� */
BOOL bWavCapture;						/* WAV�L���v�`���J�n */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static LPDIRECTSOUND lpds;				/* DirectSound */
static LPDIRECTSOUNDBUFFER lpdsp;		/* DirectSoundBuffer(�v���C�}��) */
static LPDIRECTSOUNDBUFFER lpdsb;		/* DirectSoundBuffer(�Z�J���_��) */
static WORD *lpsbuf;					/* �T�E���h�쐬�o�b�t�@ */
static DWORD *lpstmp;					/* �T�E���h�ꎞ�o�b�t�@ */
static BOOL bNowBank;					/* ���ݍĐ����̃o���N */
static UINT uBufSize;					/* �T�E���h�o�b�t�@�T�C�Y */
static UINT uRate;						/* �������[�g */
static UINT uTick;						/* ���o�b�t�@�T�C�Y�̒��� */
static UINT uFillMs;					/* �P��ɍ������鎞�� */
static UINT uCount;						/* �o�b�t�@�[�U�J�E���^ */
static UINT uBeep;						/* BEEP�g�`�J�E���^ */
static FM::OPN *pOPN[2];				/* OPN�f�o�C�X */
static int nScale[2];					/* OPN�v���X�P�[�� */
static BOOL bInitFlag;					/* �������t���O */
static WORD *pWavCapture;				/* �L���v�`���o�b�t�@(64KB) */
static UINT nWavCapture;				/* �L���v�`���o�b�t�@���f�[�^ */
static DWORD dwWavCapture;				/* �L���v�`���t�@�C���T�C�Y */

/*
 *	������
 */
void FASTCALL InitSnd(void)
{
	/* ���[�N�G���A������ */
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
 *	�N���[���A�b�v
 */
void FASTCALL CleanSnd()
{
	int i;

	/* �T�E���h��~ */
	StopSnd();

	/* OPN����� */
	for (i=0; i<2; i++) {
		if (pOPN[i]) {
			delete pOPN[i];
			pOPN[i] = NULL;
		}
	}

	/* �T�E���h�쐬�o�b�t�@����� */
	if (lpstmp) {
		free(lpstmp);
		lpstmp = NULL;
	}
	if (lpsbuf) {
		free(lpsbuf);
		lpsbuf = NULL;
	}

	/* DirectSoundBuffer����� */
	if (lpdsb) {
		lpdsb->Release();
		lpdsb = NULL;
	}
	if (lpdsp) {
		lpdsp->Release();
		lpdsp = NULL;
	}

	/* DirectSound����� */
	if (lpds) {
		lpds->Release();
		lpds = NULL;
	}

	/* uRate���N���A */
	uRate = 0;

	/* �L���v�`���֘A */
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
 *	���W�X�^�ݒ�
 */
static void FASTCALL SetReg(FM::OPN *pOPN, BYTE *reg)
{
	int i;

	/* SSG */
	for (i=0; i<16; i++) {
		pOPN->SetReg((BYTE)i, reg[i]);
	}

	/* FM�����L�[�I�t */
	for (i=0; i<3; i++) {
		pOPN->SetReg(0x28, (BYTE)i);
	}

	/* FM�������W�X�^ */
	for (i=0x30; i<0xb4; i++) {
		pOPN->SetReg((BYTE)i, reg[i]);
	}
}

/*
 *	�Z���N�g
 */
BOOL FASTCALL SelectSnd(HWND hWnd)
{
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;

	/* assert */
	ASSERT(hWnd);

	/* �N���t���O���Ă� */
	bInitFlag = TRUE;

	/* �p�����[�^��ݒ� */
	uRate = nSampleRate;
	uTick = nSoundBuffer;
	uFillMs = 1;
	uCount = 0;

	/* rate==0�Ȃ�A�������Ȃ� */
	if (uRate == 0) {
		return TRUE;
	}

	/* DiectSound�I�u�W�F�N�g�쐬 */
	if (FAILED(DirectSoundCreate(NULL, &lpds, NULL))) {
		/* �f�t�H���g�f�o�C�X�Ȃ����A�g�p�� */
		return TRUE;
	}

	/* �������x����ݒ�(�D�拦��) */
	if (FAILED(lpds->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
		return FALSE;
	}

	/* �v���C�}���o�b�t�@���쐬 */
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsp, NULL))) {
		return FALSE;
	}

	/* �v���C�}���o�b�t�@�̃t�H�[�}�b�g���w�� */
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

	/* �Z�J���_���o�b�t�@���쐬 */
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
	dsbd.dwBufferBytes = ((dsbd.dwBufferBytes + 1) >> 1) << 1;	// 2�o�C�g���E
	uBufSize = dsbd.dwBufferBytes;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if (FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))) {
		return FALSE;
	}

	/* �T�E���h�o�b�t�@���쐬(�Z�J���_���o�b�t�@�̃n�[�t�T�C�Y������) */
	lpsbuf = (WORD *)malloc(uBufSize / 2);
	if (lpsbuf == NULL) {
		return FALSE;
	}
	memset(lpsbuf, 0, uBufSize / 2);

	/* �ꎞ�o�b�t�@���쐬 */
	lpstmp = (DWORD *)malloc(uBufSize);
	if (lpstmp == NULL) {
		return FALSE;
	}

	/* OPN�f�o�C�X(�W��)���쐬 */
	pOPN[0] = new FM::OPN;
	pOPN[0]->Init(122880, uRate / 10, FALSE, NULL);
	pOPN[0]->Reset();
	pOPN[0]->SetReg(0x27, 0);

	/* OPN�f�o�C�X(WHG)���쐬 */
	pOPN[1] = new FM::OPN;
	pOPN[1]->Init(122880, uRate / 10, FALSE, NULL);
	pOPN[1]->Reset();
	pOPN[1]->SetReg(0x27, 0);

	/* �ăZ���N�g�ɔ����A���W�X�^�ݒ� */
	nScale[0] = 0;
	nScale[1] = 0;
	opn_notify(0x27, 0);
	whg_notify(0x27, 0);
	SetReg(pOPN[0], opn_reg);
	SetReg(pOPN[1], whg_reg);

	/* �L���v�`���֘A */
	if (!pWavCapture) {
		pWavCapture = (WORD *)malloc(sizeof(WORD) * 0x8000);
	}
	ASSERT(hWavCapture == -1);
	ASSERT(!bWavCapture);

	/* �T�E���h�X�^�[�g */
	bNowBank = FALSE;
	PlaySnd();

	return TRUE;
}

/*
 *	�K�p
 */
void FASTCALL ApplySnd(void)
{
	/* �N���������́A���^�[�� */
	if (!bInitFlag) {
		return;
	}

	/* �p�����[�^��v�`�F�b�N */
	if ((uRate == nSampleRate) && (uTick == nSoundBuffer)) {
		return;
	}

	/* ���ɏ������ł��Ă���Ȃ�A��� */
	if (uRate != 0) {
		CleanSnd();
	}

	/* �ăZ���N�g */
	SelectSnd(hMainWnd);
}

/*
 *	���t�J�n
 */
void FASTCALL PlaySnd()
{
	HRESULT hr;
	WORD *ptr1, *ptr2;
	DWORD size1, size2;

	if (lpdsb) {
		/* �o�b�t�@�����ׂăN���A���� */
		if (lpsbuf) {
			memset(lpsbuf, 0, uBufSize / 2);
		}
		if (lpstmp) {
			memset(lpstmp, 0, uBufSize);
		}

		/* ���b�N */
		hr = lpdsb->Lock(0, uBufSize, (void **)&ptr1, &size1,
						(void**)&ptr2, &size2, 0);
		/* �o�b�t�@�������Ă���΁A���X�g�A */
		if (hr == DSERR_BUFFERLOST) {
			lpdsb->Restore();
		}
		/* ���b�N���������ꍇ�̂݁A�Z�b�g */
		if (SUCCEEDED(hr)) {
			if (ptr1) {
				memset(ptr1, 0, size1);
			}
			if (ptr2) {
				memset(ptr2, 0, size2);
			}

			/* �A�����b�N */
			lpdsb->Unlock(ptr1, size1, ptr2, size2);
		}

		/* ���t�J�n */
		lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	}
}

/*
 *	���t��~
 */
void FASTCALL StopSnd()
{
	if (lpdsb) {
		lpdsb->Stop();
	}
}

/*
 *	BEEP����
 */
static void FASTCALL BeepSnd(int *buf, int samples)
{
	int sf;
	int i;

	/* BEEP���o�̓`�F�b�N */
	if (!beep_flag || !speaker_flag) {
		return;
	}

	/* �T���v���������� */
	for (i=0; i<samples; i++) {
		/* ��`�g���쐬 */
		sf = (int)(uBeep * nBeepFreq * 2);
		sf /= (int)uRate;

		/* ��E��ɉ����ăT���v���������� */
		if (sf & 1) {
			*buf += 0x00000c00;
		}
		else {
			*buf -= 0x00000c00;
		}
		buf++;

		/* �J�E���^�A�b�v */
		uBeep++;
		if (uBeep >= uRate) {
			uBeep = 0;
		}
	}
}

/*
 *	�T�E���h�쐬�o�b�t�@�֒ǉ�
 */
static void FASTCALL AddSnd(BOOL bFill, BOOL bZero)
{
	int i;
	int samples;
	int dat;
	WORD highlow;
	WORD *q;

	/* OPN�f�o�C�X���쐬����Ă��Ȃ���΁A�������Ȃ� */
	if (!pOPN[0] && !pOPN[1]) {
		return;
	}

	/* ���ɏ\���ǉ�����Ă���΁A�ǉ����Ȃ� */
	if (uCount == (uBufSize / 4)) {
		return;
	}

	/* �T���v�������� */
	if (bFill) {
		samples = (uBufSize / 4) - uCount;
	}
	else {
		samples = (uBufSize * uFillMs) / (uTick * 4);
		if (samples > (int)(((uBufSize / 4) - uCount))) {
			samples = (uBufSize / 4) - uCount;
		}
	}

	/* �~�L�V���O */
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

	/* �ϊ� */
	q = &lpsbuf[uCount];
	for (i=0; i<samples; i++) {
		dat = (int32)lpstmp[i];
		highlow = (WORD)(dat & 0x0000ffff);

		/* �N���b�s���O */
		if ((dat > 0) && (highlow >= 0x8000)) {
			*q++ = 0x7fff;
			continue;
		}
		if ((dat < 0) && (highlow < 0x8000)) {
			*q++ = 0x8000;
			continue;
		}

		/* �X�g�A */
		*q++ = highlow;
	}

	/* �J�E���g�A�b�v */
	uCount += samples;
}

/*
 *	WAV�L���v�`������
 */
static void FASTCALL WavCapture(void)
{
	UINT nSize;
	WORD *p;

	/* WAV�L���v�`�����łȂ���΁A���^�[�� */
	if (hWavCapture < 0) {
		return;
	}
	ASSERT(pWavCapture);

	/* �|�C���^�A�T�C�Y�������� */
	p = lpsbuf;
	nSize = uBufSize / 2;

	/* bWavCapture��FALSE�Ȃ� */
	if (!bWavCapture) {
		/* ���o���`�F�b�N */
		while (nSize > 0) {
			if (*p != 0) {
				break;
			}
			nSize -= 2;
			p++;
		}
		/* ���� */
		if (nSize == 0) {
			return;
		}
	}

	/* nWavCapture���l�� */
	if ((nWavCapture + nSize) >= 0x8000) {
		/* 32KB�����ς��܂ŃR�s�[ */
		memcpy(&pWavCapture[nWavCapture / 2], p, (0x8000 - nWavCapture));
		nSize -= (0x8000 - nWavCapture);
		p += (0x8000 - nWavCapture) / 2;

		/* �������� */
		file_write(hWavCapture, (BYTE*)pWavCapture, 0x8000);
		dwWavCapture += 0x8000;
		nWavCapture = 0;
	}

	/* �]����o�b�t�@�� */
	memcpy(&pWavCapture[nWavCapture / 2], p, nSize);
	nWavCapture += nSize;

	/* �����Ș^����� */
	bWavCapture = TRUE;
}

/*
 *	�������
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

	/* OPN�f�o�C�X���쐬����Ă��Ȃ���΁A�������Ȃ� */
	if (!pOPN[0] && !pOPN[1]) {
		return;
	}

	/* �������݈ʒu�𓾂� */
	if (FAILED(lpdsb->GetCurrentPosition(&dwPlayC, &dwWriteC))) {
		return;
	}

	/* �������݈ʒu�ƃo���N����A�K�v���𔻒f */
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

	/* �������ޕK�v���Ȃ���΁A�ʏ폈�� */
	if (!bWrite) {
		AddSnd(FALSE, bZero);
		return;
	}

	/* �������݁B�܂��T�E���h�쐬�o�b�t�@��S�����߂� */
	AddSnd(TRUE, bZero);

	/* �����Ń��b�N */
	hr = lpdsb->Lock(dwOffset, uBufSize / 2, (void **)&ptr1, &size1,
						(void**)&ptr2, &size2, 0);

	/* �o�b�t�@�������Ă���΁A���X�g�A */
	if (hr == DSERR_BUFFERLOST) {
		lpdsb->Restore();
	}
	/* ���b�N�������Ȃ���΁A�����Ă��Ӗ����Ȃ� */
	if (FAILED(hr)) {
		return;
	}

	/* �T�E���h�쐬�o�b�t�@���Z�J���_���o�b�t�@ */
	p = lpsbuf;
	memcpy(ptr1, p, size1);
	p += size1;
	if (ptr2) {
		memcpy(ptr2, p, size2);
	}

	/* �A�����b�N */
	lpdsb->Unlock(ptr1, size1, ptr2, size2);

	/* �o���N���]�A�[�U�J�E���^���N���A */
	bNowBank = (!bNowBank);
	uCount = 0;

	/* WAV�L���v�`������ */
	WavCapture();
}

/*
 *	���x���擾
 */
int FASTCALL GetLevelSnd(int ch)
{
	FM::OPN *p;
	int i;
	double s;
	double t;
	int *buf;

	ASSERT((ch >= 0) && (ch < 12));

	/* OPN,WHG�̋�� */
	if (ch < 6) {
		p = pOPN[0];
	}
	else {
		p = pOPN[1];
		ch -= 6;

		/* WHG�̏ꍇ�A���ۂɎg���Ă��Ȃ����0 */
		if (!whg_enable || !whg_use) {
			return 0;
		}
	}

	/* ���݃`�F�b�N */
	if (!p) {
		return 0;
	}

	/* FM,PSG�̋�� */
	if (ch < 3) {
		/* FM:512�T���v����2��a���v�Z */
		buf = p->rbuf[ch];

		s = 0;
		for (i=0; i<512; i++) {
			t = (double)*buf++;
			t *= t;
			s += t;
		}
		s /= 512;

		/* �[���`�F�b�N */
		if (s == 0) {
			return 0;
		}

		/* log10����� */
		s = log10(s);

		/* FM�����␳ */
		s *= 40.0;
	}
	else {
		/* SSG:512�T���v����2��a���v�Z */
		buf = p->psg.rbuf[ch - 3];

		s = 0;
		for (i=0; i<512; i++) {
			t = (double)*buf++;
			t *= t;
			s += t;
		}
		s /= 512;

		/* �[���`�F�b�N */
		if (s == 0) {
			return 0;
		}

		/* log10����� */
		s = log10(s);

		/* PSG�����␳ */
		s *= 60.0;
	}

	return (int)s;
}

/*
 *	WAV�L���v�`���J�n
 */
void FASTCALL OpenCaptureSnd(char *fname)
{
	WAVEFORMATEX wfex;
	DWORD dwSize;
	int fileh;

	ASSERT(fname);
	ASSERT(hWavCapture < 0);
	ASSERT(!bWavCapture);

	/* �������łȂ���΁A���^�[�� */
	if (!pOPN[0]) {
		return;
	}

	/* �o�b�t�@��������΁A���^�[�� */
	if (!pWavCapture) {
		return;
	}

	/* uBufSize / 2��0x8000�ȉ��łȂ��ƃG���[ */
	if ((uBufSize / 2) > 0x8000) {
		return;
	}

	/* �t�@�C���I�[�v��(�������݃��[�h) */
	fileh = file_open(fname, OPEN_W);
	if (fileh < 0) {
		return;
	}

	/* RIFF�w�b�_�������� */
	if (!file_write(fileh, (BYTE*)"RIFF0123WAVEfmt ", 16)) {
		file_close(fileh);
		return;
	}

	/* WAVEFORMATEX�������� */
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

	/* data�T�u�w�b�_�������� */
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
 *	WAV�L���v�`���I��
 */
void FASTCALL CloseCaptureSnd(void)
{
	DWORD dwLength;

	ASSERT(hWavCapture >= 0);

	/* �o�b�t�@�Ɏc���������������� */
	file_write(hWavCapture, (BYTE*)pWavCapture, nWavCapture);
	dwWavCapture += nWavCapture;
	nWavCapture = 0;

	/* �t�@�C�������O�X���������� */
	file_seek(hWavCapture, 4);
	dwLength = dwWavCapture + sizeof(WAVEFORMATEX) + 20;
	file_write(hWavCapture, (BYTE *)&dwLength, sizeof(dwLength));

	/* data�������O�X���������� */
	file_seek(hWavCapture, sizeof(WAVEFORMATEX) + 24);
	file_write(hWavCapture, (BYTE *)&dwWavCapture, sizeof(dwWavCapture));

	/* �t�@�C���N���[�Y */
	file_close(hWavCapture);

	/* ���[�N�G���A�N���A */
	hWavCapture = -1;
	bWavCapture = FALSE;
}

/*
 *	OPN�o��
 */
extern "C" {
void FASTCALL opn_notify(BYTE reg, BYTE dat)
{
	/* OPN���Ȃ���΁A�������Ȃ� */
	if (!pOPN[0]) {
		return;
	}

	/* �v���X�P�[���𒲐� */
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

	/* �o�� */
	pOPN[0]->SetReg((uint8)reg, (uint8)dat);
}
}

/*
 *	WHG�o��
 */
extern "C" {
void FASTCALL whg_notify(BYTE reg, BYTE dat)
{
	/* WHG���Ȃ���΁A�������Ȃ� */
	if (!pOPN[1]) {
		return;
	}

	/* �v���X�P�[���𒲐� */
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

	/* �o�� */
	pOPN[1]->SetReg((uint8)reg, (uint8)dat);
}
}

#endif	/* _WIN32 */
