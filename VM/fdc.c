/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ フロッピーディスク コントローラ(MB8877A) ]
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "fdc.h"
#include "mainetc.h"

/*
 *	グローバル ワーク
 */
BYTE fdc_command;					/* FDCコマンド */
BYTE fdc_status;					/* FDCステータス */
BYTE fdc_trkreg;					/* トラックレジスタ */
BYTE fdc_secreg;					/* セクタレジスタ */
BYTE fdc_datareg;					/* データレジスタ */
BYTE fdc_sidereg;					/* サイドレジスタ */
BYTE fdc_drvreg;					/* ドライブ */
BYTE fdc_motor;						/* モータ */
BYTE fdc_drqirq;					/* DRQおよびIRQ */

BYTE fdc_cmdtype;					/* コマンドタイプ */
WORD fdc_totalcnt;					/* トータルカウンタ */
WORD fdc_nowcnt;					/* カレントカウンタ */
BYTE fdc_ready[FDC_DRIVES];			/* レディ状態 */
BOOL fdc_teject[FDC_DRIVES];		/* 一時イジェクト */
BOOL fdc_writep[FDC_DRIVES];		/* ライトプロテクト状態 */
BYTE fdc_track[FDC_DRIVES];			/* 実トラック */

char fdc_fname[FDC_DRIVES][128+1];	/* ファイル名 */
BOOL fdc_fwritep[FDC_DRIVES];		/* ライトプロテクト状態(ファイルレベル) */
BYTE fdc_header[FDC_DRIVES][0x2b0];	/* D77ファイルヘッダ */
BYTE fdc_media[FDC_DRIVES];			/* メディア枚数 */
BYTE fdc_medias[FDC_DRIVES];		/* メディアセレクト状態 */
char fdc_name[FDC_DRIVES][FDC_MEDIAS][17];
BYTE fdc_access[FDC_DRIVES];		/* アクセスLED */

/*
 *	スタティック ワーク
 */
static BYTE fdc_buffer[0x2000];					/* データバッファ */
static BYTE *fdc_dataptr;						/* データポインタ */
static DWORD fdc_seekofs[FDC_DRIVES];			/* シークオフセット */
static DWORD fdc_secofs[FDC_DRIVES];			/* セクタオフセット */
static DWORD fdc_foffset[FDC_DRIVES][FDC_MEDIAS];
static WORD fdc_trklen[FDC_DRIVES];				/* トラックデータ長さ */
static BOOL fdc_seekvct;						/* シーク方向(Trk0:TRUE) */
static BYTE fdc_indexcnt;						/* INDEXホール カウンタ */
static BOOL fdc_boot;							/* ブートフラグ */

/*
 *	プロトタイプ宣言
 */
void FASTCALL fdc_readbuf(int drive);	/* １トラック分読み込み */

/*
 *	FDC
 *	初期化
 */
BOOL FASTCALL fdc_init(void)
{
	int i;

	/* フロッピーファイル関係をリセット */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_ready[i] = FDC_TYPE_NOTREADY;
		fdc_teject[i] = FALSE;
		fdc_fwritep[i] = FALSE;
		fdc_medias[i] = 0;
	}
	/* ファイルオフセットを全てクリア */
	memset(fdc_foffset, 0, sizeof(fdc_foffset));

	return TRUE;
}

/*
 *	FDC
 *	クリーンアップ
 */
void FASTCALL fdc_cleanup(void)
{
	int i;

	/* フロッピーファイル関係をリセット */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_ready[i] = FDC_TYPE_NOTREADY;
	}
}

/*
 *	FDC
 *	リセット
 */
void FASTCALL fdc_reset(void)
{
	/* FDC物理レジスタをリセット */
	fdc_command = 0xff;
	fdc_status = 0;
	fdc_trkreg = 0;
	fdc_secreg = 0;
	fdc_datareg = 0;
	fdc_sidereg = 0;
	fdc_drvreg = 0;
	fdc_motor = 0;

	fdc_cmdtype = 0;
	fdc_seekvct = 0;
	memset(fdc_track, 0, sizeof(fdc_track));
	fdc_dataptr = NULL;
	memset(fdc_access, 0, sizeof(fdc_access));
	fdc_boot = FALSE;

	/* データバッファへ読み込み */
	fdc_readbuf(fdc_drvreg);
}

/*-[ CRC計算 ]--------------------------------------------------------------*/

/*
 *	CRC計算テーブル
 */
static WORD crc_table[256] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

/*
 *	16bit CRCを計算し、セット
 */
static void FASTCALL calc_crc(BYTE *addr, int size)
{
	WORD crc;

	/* 初期化 */
	crc = 0;

	/* 計算 */
	while (size > 0) {
		crc = (WORD)((crc << 8) ^ crc_table[(BYTE)(crc >> 8) ^ (BYTE)*addr++]);
		size--;
	}

	/* 続く２バイトにセット(ビッグエンディアン) */
	*addr++ = (BYTE)(crc >> 8);
	*addr = (BYTE)(crc & 0xff);
}

/*
 *	乱数を計算
 */
static BYTE FASTCALL calc_rand(void)
{
	static WORD rand_s = 0x7f28;
	WORD tmp1, tmp2, tmp3;

	tmp1 = rand_s;
	tmp2 = (WORD)(tmp1 & 255);
	tmp1 = (WORD)((tmp1 << 1) + 1 + rand_s);
	tmp3 = (WORD)(((tmp1 >> 8) + tmp2) & 255);
	tmp1 &= 255;
	rand_s = (WORD)((tmp3 << 8) | tmp1);

	return (BYTE)tmp3;
}

/*-[ ファイル管理 ]---------------------------------------------------------*/

/*
 *	Read Trackデータ作成
 */
static void FASTCALL fdc_make_track(void)
{
	int i;
	int j;
	int gap3;
	WORD count;
	WORD secs;
	WORD size;
	BOOL flag;
	BYTE *p;
	BYTE *q;

	/* アンフォーマットチェック */
	flag = FALSE;
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		if (fdc_track[fdc_drvreg] > 79) {
			/* アンフォーマット */
			flag = TRUE;
		}
	}
	else {
		if ((fdc_buffer[4] == 0) && (fdc_buffer[5] == 0)) {
			/* アンフォーマット */
			flag = TRUE;
		}
	}

	/* アンフォーマットなら、ランダムデータ作成 */
	if (flag) {
		p = fdc_buffer;
		for (i=0; i<0x1800; i++) {
			*p++ = calc_rand();
		}

		/* データポインタ、カウンタ設定 */
		fdc_dataptr = fdc_buffer;
		fdc_totalcnt = 0x1800;
		fdc_nowcnt = 0;
		return;
	}

	/* セクタ数算出、データ移動 */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		secs = 16;
		q = &fdc_buffer[0x1000];
		memcpy(q, fdc_buffer, 0x1000);
	}
	else {
		secs = (WORD)(fdc_buffer[0x0005] * 256);
		secs += (WORD)(fdc_buffer[0x0004]);
		count = 0;
		/* 全セクタまわって、サイズを計る */
		for (j=0; j<secs; j++) {
			p = &fdc_buffer[count];
			count += (WORD)((p[0x000f] * 256 + p[0x000e]));
			count += (WORD)0x10;
		}
		/* データコピー */
		q = &fdc_buffer[0x2000 - count];
		for (j=(count-1); j>0; j--) {
			q[j] = fdc_buffer[j];
		}
	}

	/* GAP3決定 */
	if (secs <= 5) {
		gap3 = 0x74;
	}
	else {
		if (secs <= 10) {
			gap3 = 0x54;
		}
		else {
			if (secs <= 16) {
				gap3 = 0x33;
			}
			else {
				gap3 = 0x10;
			}
		}
	}

	/* バッファ初期化 */
	p = fdc_buffer;
	count = 0;

	/* GAP0 */
	for (i=0; i<80; i++) {
		*p++ = 0x4e;
	}
	count += (WORD)80;

	/* SYNC */
	for (i=0; i<12; i++) {
		*p++ = 0;
	}
	count += (WORD)12;

	/* INDEX MARK */
	*p++ = 0xc2;
	*p++ = 0xc2;
	*p++ = 0xc2;
	*p++ = 0xfc;
	count += (WORD)4;

	/* GAP1 */
	for (i=0; i<50; i++) {
		*p++ = 0x4e;
	}
	count += (WORD)50;

	/* セクタループ */
	for (j=0; j<secs; j++) {
		/* SYNC */
		for (i=0; i<12; i++) {
			*p++ = 0;
		}
		count += (WORD)12;

		/* ID ADDRESS MARK */
		*p++ = 0xa1;
		*p++ = 0xa1;
		*p++ = 0xa1;
		*p++ = 0xfe;
		count += (WORD)4;

		/* ID */
		if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
			p[0] = fdc_track[fdc_drvreg];
			p[1] = fdc_sidereg;
			p[2] = (BYTE)(j + 1);
			p[3] = 1;
			size = 0x100;
		}
		else {
			memcpy(p, q, 4);
			size = (WORD)(q[0x000f] * 256 + q[0x000e]);
			q += 0x0010;
		}
		calc_crc(p, 4);
		p += (4 + 2);
		count += (WORD)(4 + 2);

		/* GAP2 */
		for (i=0; i<22; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)22;

		/* データ */
		memcpy(p, q, size);
		q += size;
		calc_crc(p, size);
		p += (size + 2);
		count += (WORD)(size + 2);

		/* GAP3 */
		for (i=0; i<gap3; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)gap3;
	}

	/* GAP4 */
	j = (0x1800 - count);
	if (j < 0x1800) {
		for (i=0; i<j; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)j;
	}

	/* データポインタ、カウンタ設定 */
	fdc_dataptr = fdc_buffer;
	fdc_totalcnt = count;
	fdc_nowcnt = 0;
}

/*
 *	IDフィールドをバッファに作る
 *	カウンタ、データポインタを設定
 */
static void FASTCALL fdc_makeaddr(int index)
{
	int i;
	BYTE *p;
	WORD offset;
	WORD size;

	/* 転送のためのテンポラリバッファは、通常バッファの */
	/* 最後尾を潰して設ける */
	p = &fdc_buffer[0x1ff0];

	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* 2Dの場合、C,H,R,Nは確定する */
		p[0] = fdc_track[fdc_drvreg];
		p[1] = fdc_sidereg;
		p[2] = (BYTE)(index + 1);
		p[3] = 1;
	}
	else {
		/* バッファを目的のセクタまで進める */
		offset = 0;
		i = 0;
		while (i < index) {
			/* セクタサイズを取得 */
			size = fdc_buffer[offset + 0x000f];
			size *= (WORD)256;
			size |= fdc_buffer[offset + 0x000e];

			/* 次のセクタへ進める */
			offset += size;
			offset += (WORD)0x10;

			i++;
		}

		/* C,H,R,Nコピー */
		memcpy(p, &fdc_buffer[offset], 4);
	}

	/* CRC */
	calc_crc(p, 4);

	/* データポインタ、カウンタ設定 */
	fdc_dataptr = &fdc_buffer[0x1ff0];
	fdc_totalcnt = 6;
	fdc_nowcnt = 0;
}

/*
 *	インデックスカウンタを次のセクタへ移す
 */
static int FASTCALL fdc_next_index(void)
{
	int secs;

	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);

	/* ディスクタイプをチェック */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* 2Dなら16セクタ固定 */
		fdc_indexcnt = (BYTE)((fdc_indexcnt + 1) & 0x0f);
		if (fdc_track[fdc_drvreg] >= 40) {
			return -1;
		}
		return fdc_indexcnt;
	}

	/* D77 */
	ASSERT(fdc_ready[fdc_drvreg] == FDC_TYPE_D77);
	secs = fdc_buffer[0x0005];
	secs *= 256;
	secs |= fdc_buffer[0x0004];
	if (secs == 0) {
		/* アンフォーマット */
		fdc_indexcnt = (BYTE)((fdc_indexcnt + 1) & 0x0f);
		return -1;
	}
	else {
		fdc_indexcnt++;
		if (fdc_indexcnt >= secs) {
			fdc_indexcnt = 0;
		}
		return fdc_indexcnt;
	}
}

/*
 *	IDマークを探す
 */
static BOOL FASTCALL fdc_idmark(WORD *p)
{
	WORD offset;
	BYTE dat;

	/* A1 A1 A1 FEもしくはF5 F5 F5 FE */
	offset = *p;

	while (offset < fdc_totalcnt) {
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if (dat == 0xfe) {
			*p = offset;
			return TRUE;
		}
	}

	*p = offset;
	return FALSE;
}

/*
 *	データマークを探す
 */
static BOOL FASTCALL fdc_datamark(WORD *p)
{
	WORD offset;
	BYTE dat;

	/* A1 A1 A1 FBもしくはF5 F5 F5 FB */
	/* deleted data markには未対応 */
	offset = *p;

	while (offset < fdc_totalcnt) {
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if (dat == 0xfb) {
			*p = offset;
			return TRUE;
		}
	}

	*p = offset;
	return FALSE;
}

/*
 *	トラック書き込み終了
 */
static BOOL FASTCALL fdc_writetrk(void)
{
	int total;
	int sectors;
	WORD offset;
	WORD seclen;
	WORD writep;
	int i;
	int handle;

	/* 初期化 */
	total = 0;
	sectors = 0;

	/* セクタ数と、データ部のトータルサイズを数える */
	offset = 0;
	while (offset < fdc_totalcnt) {
		/* IDマークを探す */
		if (!fdc_idmark(&offset)) {
			break;
		}
		/* C,H,R,Nの次が$F7か */
		offset += (WORD)4;
		if (offset >= fdc_totalcnt) {
			return FALSE;
		}
		if (fdc_buffer[offset] != 0xf7) {
			return FALSE;
		}
		offset++;
		/* データマークを探す */
		if (!fdc_datamark(&offset)) {
			return FALSE;
		}
		/* レングスを計算しつつ、$F7を探す */
		seclen = 0;
		while(offset < fdc_totalcnt) {
			if (fdc_buffer[offset] == 0xf7) {
				break;
			}
			offset++;
			seclen++;
		}
		if (offset >= fdc_totalcnt) {
			return FALSE;
		}
		/* セクタok */
		total += seclen;
		sectors++;
		offset++;
	}

	/* 2Dメディアの場合、total=0x1000, sectors=0x10が必須 */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* total, sectorsの検査 */
		if (total != 0x1000) {
			return FALSE;
		}
		if (sectors != 16) {
			return FALSE;
		}

		/* 本来はC,H,Nなどチェックすべき */
		return TRUE;
	}

	/* セクタごとに0x10ぶん、余計にかかる */
	if ((total + (sectors * 0x10)) > fdc_trklen[fdc_drvreg]) {
		return FALSE;
	}

	/* 収まることがわかったので、データを作成する */
	writep = 0;
	offset = 0;
	for (i=0; i<sectors; i++) {
		/* IDマークを見る */
		fdc_idmark(&offset);

		/* 0x10ヘッダの作成 */
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = (BYTE)(sectors & 0xff);
		fdc_buffer[writep++] = (BYTE)(sectors >> 8);
		memset(&fdc_buffer[writep], 0, 8);
		writep += (WORD)8;
		offset++;

		/* セクタレングスは後で書き込む */
		writep += (WORD)2;

		/* データマークを見る */
		fdc_datamark(&offset);

		/* レングスを数えつつコピー */
		seclen = 0;
		while (fdc_buffer[offset] != 0xf7) {
			fdc_buffer[writep++] = fdc_buffer[offset++];
			seclen++;
		}
		offset++;

		/* セクタレングスを設定 */
		fdc_buffer[writep - seclen - 2] = (BYTE)(seclen & 0xff);
		fdc_buffer[writep - seclen - 1] = (BYTE)(seclen >> 8);
	}

	/* ファイルにデータを書きこむ */
	handle = file_open(fdc_fname[fdc_drvreg], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, fdc_seekofs[fdc_drvreg])) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, fdc_buffer, (sectors * 0x10) + total)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	/* 正常終了 */
	return TRUE;
}

/*
 *	セクタ書き込み終了
 */
static BOOL FASTCALL fdc_writesec(void)
{
	DWORD offset;
	int handle;

	/* assert */
	ASSERT(fdc_drvreg < FDC_DRIVES);
	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);
	ASSERT(fdc_dataptr);
	ASSERT(fdc_totalcnt > 0);

	/* オフセット算出 */
	offset = fdc_seekofs[fdc_drvreg];
	offset += fdc_secofs[fdc_drvreg];

	/* 書き込み */
	handle = file_open(fdc_fname[fdc_drvreg], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, fdc_dataptr, fdc_totalcnt)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	return TRUE;
}

/*
 *	トラック、セクタ、サイドと一致するセクタを検索
 *	カウンタ、データポインタを設定
 */
static BYTE FASTCALL fdc_readsec(BYTE track, BYTE sector, BYTE side)
{
	int secs;
	int i;
	WORD offset;
	WORD size;
	BYTE stat;

	/* assert */
	ASSERT(fdc_drvreg < FDC_DRIVES);
	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);

	/* 2Dファイルの場合 */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		if (track != fdc_track[fdc_drvreg]) {
			return FDC_ST_RECNFND;
		}
		if (side != fdc_sidereg) {
			return FDC_ST_RECNFND;
		}
		if ((sector < 1) || (sector > 16)) {
			return FDC_ST_RECNFND;
		}

		/* データポインタ設定 */
		fdc_dataptr = &fdc_buffer[(sector - 1) * 0x0100];
		fdc_secofs[fdc_drvreg] = (sector - 1) * 0x0100;

		/* カウンタ設定 */
		fdc_totalcnt = 0x0100;
		fdc_nowcnt = 0;

		return 0;
	}

	secs = fdc_buffer[0x0005];
	secs *= (WORD)256;
	secs |= fdc_buffer[0x0004];
	if (secs == 0) {
		return FDC_ST_RECNFND;
	}

	offset = 0;
	/* セクタループ */
	for (i=0; i<secs; i++) {
		/* このセクタのサイズを先に取得 */
		size = fdc_buffer[offset + 0x000f];
		size *= (WORD)256;
		size |= fdc_buffer[offset + 0x000e];

		/* C,H,Rが一致するセクタがあるか */
		if (fdc_buffer[offset + 0] != track) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}
		if (fdc_buffer[offset + 1] != side) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}
		if (fdc_buffer[offset + 2] != sector) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}

		/* 単密チェック */
		if (fdc_buffer[offset + 0x0006] != 0) {
			continue;
		}

		/* データポインタ、カウンタ設定 */
		fdc_dataptr = &fdc_buffer[offset + 0x0010];
		fdc_secofs[fdc_drvreg] = offset + 0x0010;
		fdc_totalcnt = size;
		fdc_nowcnt = 0;

		/* ステータス設定 */
		stat = 0;
		if (fdc_buffer[offset + 0x0007] != 0) {
			stat |= FDC_ST_RECTYPE;
		}
		if (fdc_buffer[offset + 0x0008] == 0xb0) {
			stat = FDC_ST_CRCERR;
		}

		return stat;
	}

	/* 全セクタを検査したが、見つからなかった */
	return FDC_ST_RECNFND;
}

/*
 *	トラック読み込み
 */
static void FASTCALL fdc_readbuf(int drive)
{
	DWORD offset;
	DWORD len;
	int trkside;
	int handle;

	/* ドライブチェック */
	if ((drive < 0) || (drive >= FDC_DRIVES)) {
		return;
	}

	/* レディチェック */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		return;
	}

	/* インデックスホールカウンタをクリア */
	fdc_indexcnt = 0;

	/* トラック×２＋サイド */
	trkside = fdc_track[drive] * 2 + fdc_sidereg;

	/*
	 * 2Dファイル
	 */
	if (fdc_ready[drive] == FDC_TYPE_2D) {
		/* 範囲チェック */
		if (trkside >= 80) {
			memset(fdc_buffer, 0, 0x1000);
			return;
		}

		/* オフセット算出 */
		offset = (DWORD)trkside;
		offset *= 0x1000;
		fdc_seekofs[drive] = offset;
		fdc_trklen[drive] = 0x1000;

		/* 読み込み */
		memset(fdc_buffer, 0, 0x1000);
		handle = file_open(fdc_fname[drive], OPEN_R);
		if (handle == -1) {
			return;
		}
		if (!file_seek(handle, offset)) {
			file_close(handle);
			return;
		}
		file_read(handle, fdc_buffer, 0x1000);
		file_close(handle);
		return;
	}

	/*
	 * D77ファイル
	 */
	if (trkside >= 84) {
		/* トラックオーバー、セクタ数0 */
		fdc_buffer[4] = 0;
		fdc_buffer[5] = 0;
		return;
	}

	/* ヘッダファイルに従い、オフセット・レングスを算出 */
	offset = *(DWORD *)(&fdc_header[drive][0x0020 + trkside * 4]);
	if (offset == 0) {
		/* 存在しないトラック */
		fdc_buffer[4] = 0;
		fdc_buffer[5] = 0;
		return;
	}

	len = *(DWORD *)(&fdc_header[drive][0x0020 + (trkside + 1) * 4]);
	if (len == 0) {
		/* 最終トラック */
		len = *(DWORD *)(&fdc_header[drive][0x0014]);
	}
	len -= offset;
	if (len > 0x2000) {
		len = 0x2000;
	}
	fdc_seekofs[drive] = offset;
	fdc_trklen[drive] = (WORD)len;

	/* シーク、読み込み */
	memset(fdc_buffer, 0, 0x2000);
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return;
	}
	file_read(handle, fdc_buffer, len);
	file_close(handle);
}

/*
 *	D77ファイル ヘッダ読み込み
 */
static BOOL FASTCALL fdc_readhead(int drive, int index)
{
	int i;
	DWORD offset;
	DWORD temp;
	int handle;

	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((index >= 0) && (index < FDC_MEDIAS));
	ASSERT(fdc_ready[drive] == FDC_TYPE_D77);

	/* オフセット決定 */
	offset = fdc_foffset[drive][index];

	/* シーク、読み込み */
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_read(handle, fdc_header[drive], 0x2b0)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	/* タイプチェック、ライトプロテクト設定 */
	if (fdc_header[drive][0x001b] != 0) {
		/* 2Dでない */
		return FALSE;
	}
	if (fdc_fwritep[drive]) {
		fdc_writep[drive] = TRUE;
	}
	else {
		if (fdc_header[drive][0x001a] & 0x10) {
			fdc_writep[drive] = TRUE;
		}
		else {
			fdc_writep[drive] = FALSE;
		}
	}

	/* トラックオフセットを設定 */
	for (i=0; i<164; i++) {
		temp = 0;
		temp |= fdc_header[drive][0x0020 + i * 4 + 3];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 2];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 1];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 0];

		if (temp != 0) {
			/* データあり */
			temp += offset;
			*(DWORD *)(&fdc_header[drive][0x0020 + i * 4]) = temp;
		}
	}

	/* ディスクサイズ＋オフセット */
	temp = 0;
	temp |= fdc_header[drive][0x001c + 3];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 2];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 1];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 0];
	temp += offset;
	*(DWORD *)(&fdc_header[drive][0x0014]) = temp;

	return TRUE;
}

/*
 *	現在のメディアのライトプロテクトを切り替える
 */
BOOL FASTCALL fdc_setwritep(int drive, BOOL writep)
{
	BYTE header[0x2b0];
	DWORD offset;
	int handle;

	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((writep == TRUE) || (writep == FALSE));

	/* レディでなければならない */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		return FALSE;
	}

	/* ファイルが書き込み不可ならダメ */
	if (fdc_fwritep[drive]) {
		return FALSE;
	}

	/* 読み込み、設定、書き込み */
	offset = fdc_foffset[drive][fdc_media[drive]];
	handle = file_open(fdc_fname[drive], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_read(handle, header, 0x2b0)) {
		file_close(handle);
		return FALSE;
	}
	if (writep) {
		header[0x1a] |= 0x10;
	}
	else {
		header[0x1a] &= ~0x10;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, header, 0x2b0)) {
		file_close(handle);
		return FALSE;
	}

	/* 成功 */
	file_close(handle);
	fdc_writep[drive] = writep;
	return TRUE;
}

/*
 *	メディア番号を設定
 */
BOOL FASTCALL fdc_setmedia(int drive, int index)
{
	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((index >= 0) && (index < FDC_MEDIAS));

	/* レディ状態か */
	if (fdc_ready[drive] == 0) {
		return FALSE;
	}

	/* 2Dファイルの場合、index = 0か */
	if ((fdc_ready[drive] == FDC_TYPE_2D) && (index != 0)) {
		return FALSE;
	}

	/* index > 0 なら、fdc_foffsetを調べて>0が必要 */
	if (index > 0) {
		if (fdc_foffset[drive][index] == 0) {
			return FALSE;
		}
	}

	/* D77ファイルの場合、ヘッダ読み込み */
	if (fdc_ready[drive] == FDC_TYPE_D77) {
		/* ライトプロテクトは内部で設定 */
		if (!fdc_readhead(drive, index)) {
			return FALSE;
		}
	}
	else {
		/* 2Dファイルなら、ファイル属性に従う */
		fdc_writep[drive] = fdc_fwritep[drive];
	}

	/* 一時イジェクトを強制解除 */
	fdc_teject[fdc_drvreg] = FALSE;

	/* データバッファ読み込み、ワークセーブ */
	fdc_readbuf(fdc_drvreg);
	fdc_media[drive] = (BYTE)index;

	return TRUE;
}

/*
 *	D77ファイル解析、メディア数および名称取得
 */
static int FASTCALL fdc_chkd77(int drive)
{
	int i;
	int handle;
	int count;
	DWORD offset;
	DWORD len;
	BYTE buf[0x20];

	/* 初期化 */
	for (i=0; i<FDC_MEDIAS; i++) {
		fdc_foffset[drive][i] = 0;
		fdc_name[drive][i][0] = '\0';
	}
	count = 0;
	offset = 0;

	/* ファイルオープン */
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return count;
	}

	/* メディアループ */
	while (count < FDC_MEDIAS) {
		/* シーク */
		if (!file_seek(handle, offset)) {
			file_close(handle);
			return count;
		}

		/* 読み込み */
		if (!file_read(handle, buf, 0x0020)) {
			file_close(handle);
			return count;
		}

		/* チェック */
		if (buf[0x1b] != 0) {
			file_close(handle);
			return count;
		}

		/* ok,ファイル名、オフセット格納 */
		buf[17] = '\0';
		memcpy(fdc_name[drive][count], buf, 17);
		fdc_foffset[drive][count] = offset;

		/* next処理 */
		len = 0;
		len |= buf[0x1f];
		len *= 256;
		len |= buf[0x1e];
		len *= 256;
		len |= buf[0x1d];
		len *= 256;
		len |= buf[0x1c];
		offset += len;
		count++;
	}

	/* 最大メディア枚数に達した */
	file_close(handle);
	return count;
}

/*
 *	ディスクファイルを設定
 */
int FASTCALL fdc_setdisk(int drive, char *fname)
{
	BOOL writep;
	int handle;
	DWORD fsize;
	int count;

	ASSERT((drive >= 0) && (drive < FDC_DRIVES));

	/* ノットレディにする場合 */
	if (fname == NULL) {
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		fdc_fname[drive][0] = '\0';
		return 1;
	}

	/* ファイルをオープンし、ファイルサイズを調べる */
	strcpy(fdc_fname[drive], fname);
	writep = FALSE;
	handle = file_open(fname, OPEN_RW);
	if (handle == -1) {
		handle = file_open(fname, OPEN_R);
		if (handle == -1) {
			return 0;
		}
		writep = TRUE;
	}
	fsize = file_getsize(handle);
	file_close(handle);

	/*
	 * 2Dファイル
	 */
	if (fsize == 327680) {
		/* タイプ、書き込み属性設定 */
		fdc_ready[drive] = FDC_TYPE_2D;
		fdc_fwritep[drive] = writep;

		/* メディア設定 */
		if (!fdc_setmedia(drive, 0)) {
			fdc_ready[drive] = FDC_TYPE_NOTREADY;
			return 0;
		}

		/* 成功。一時イジェクト解除 */
		fdc_teject[drive] = FALSE;
		fdc_medias[drive] = 1;
		return 1;
	}

	/*
	 * D77ファイル
	 */
	fdc_ready[drive] = FDC_TYPE_D77;
	fdc_fwritep[drive] = writep;

	/* ファイル検査 */
	count = fdc_chkd77(drive);
	if (count == 0){
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		return 0;
	}

	/* メディア設定 */
	if (!fdc_setmedia(drive, 0)) {
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		return 0;
	}

	/* 成功。一時イジェクト解除 */
	fdc_teject[drive] = FALSE;
	fdc_medias[drive] = (BYTE)count;
	return count;
}

/*-[ FDCコマンド ]----------------------------------------------------------*/

/*
 *	FDC ステータス作成
 */
static void FASTCALL fdc_make_stat(void)
{
	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		fdc_status |= FDC_ST_NOTREADY;
		return;
	}

	/* 有効なドライブ */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_NOTREADY) {
		fdc_status |= FDC_ST_NOTREADY;
	}
	else {
		fdc_status &= (BYTE)(~FDC_ST_NOTREADY);
	}

	/* 一時イジェクト */
	if (fdc_teject[fdc_drvreg]) {
		fdc_status |= FDC_ST_NOTREADY;
	}

	/* ライトプロテクト(Read系コマンドは0) */
	if ((fdc_cmdtype == 2) || (fdc_cmdtype == 4) || (fdc_cmdtype == 6)) {
		fdc_status &= ~FDC_ST_WRITEP;
	}
	else {
		if (fdc_writep[fdc_drvreg] && !(fdc_status & FDC_ST_NOTREADY)) {
			fdc_status |= FDC_ST_WRITEP;
		}
		else {
			fdc_status &= ~FDC_ST_WRITEP;
		}
	}

	/* TYPE I のみ */
	if (fdc_cmdtype != 1) {
		return;
	}

	/* TRACK00 */
	if (fdc_track[fdc_drvreg] == 0) {
		fdc_status |= FDC_ST_TRACK00;
	}
	else {
		fdc_status &= ~FDC_ST_TRACK00;
	}

	/* index */
	if (!(fdc_status & FDC_ST_NOTREADY)) {
		if (fdc_indexcnt == 0) {
			fdc_status |= FDC_ST_INDEX;
		}
		else {
			fdc_status &= ~FDC_ST_INDEX;
		}
		fdc_next_index();
	}
}

/*
 *	TYPE I
 *	RESTORE
 */
static void FASTCALL fdc_restore(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* トラック比較、リストア、読み込み */
	if (fdc_track[fdc_drvreg] != 0) {
		fdc_track[fdc_drvreg] = 0;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = TRUE;

	/* 自動アップデート */
	fdc_trkreg = 0;

	/* FDC割り込み */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* ステータス生成 */
	fdc_status = FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* アクセス(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	SEEK
 */
static void FASTCALL fdc_seek(void)
{
	BYTE target;

	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* 先にベリファイ */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
			/* 自動修復(らぷてっく対策) */
			fdc_trkreg = fdc_track[fdc_drvreg];
		}
	}

	/* 相対シーク */
	if (fdc_datareg > fdc_trkreg) {
		fdc_seekvct = FALSE;
		target = (BYTE)(fdc_track[fdc_drvreg] + fdc_datareg -  fdc_trkreg);
		if (target > 41) {
			target = 41;
		}
	}
	else {
		fdc_seekvct = TRUE;
		target = (BYTE)(fdc_track[fdc_drvreg] + fdc_datareg -  fdc_trkreg);
		if (target > 41) {
			target = 0;
		}
	}

	/* トラック比較、シーク、読み込み */
	if (fdc_track[fdc_drvreg] != target) {
		fdc_track[fdc_drvreg] = target;
		fdc_readbuf(fdc_drvreg);
	}

	/* アップデート */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC割り込み */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* ステータス生成 */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* アクセス(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP IN
 */
static void FASTCALL fdc_step_in(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* 先にベリファイ */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
		}
	}

	/* ステップ、読み込み */
	if (fdc_track[fdc_drvreg] < 41) {
		fdc_track[fdc_drvreg]++;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = FALSE;

	/* アップデート */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC割り込み */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* ステータス生成 */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* アクセス(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP OUT
 */
static void FASTCALL fdc_step_out(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* 先にベリファイ */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
		}
	}

	/* ステップ、読み込み */
	if (fdc_track[fdc_drvreg] != 0) {
		fdc_track[fdc_drvreg]--;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = TRUE;

	/* アップデート */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC割り込み */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* ステータス生成 */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* アクセス(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP
 */
static void FASTCALL fdc_step(void)
{
	if (fdc_seekvct) {
		fdc_step_out();
	}
	else {
		fdc_step_in();
	}
}

/*
 *	TYPE II, III
 *	READ/WRITE サブ
 */
static BOOL FASTCALL fdc_rw_sub(void)
{
	fdc_status = 0;

	/* ドライブチェック */
	if (fdc_drvreg >= FDC_DRIVES) {
		fdc_make_stat();
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return FALSE;
	}

	/* NOT READYチェック */
	if ((fdc_ready[fdc_drvreg] == FDC_TYPE_NOTREADY) || fdc_teject[fdc_drvreg]) {
		fdc_make_stat();
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return FALSE;
	}

	return TRUE;
}

/*
 *	TYPE II
 *	READ DATA
 */
static void FASTCALL fdc_read_data(void)
{
	BYTE stat;

	/* TYPE II, Read */
	fdc_cmdtype = 2;

	/* 基本チェック */
	if (!fdc_rw_sub()) {
		return;
	}

	/* ブートフラグ */
	if (!fdc_boot && (fdc_drvreg > 0)) {
		/* 擬似的に、NOT READYを作る */
		fdc_status |= 0x80;

		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* セクタ検索 */
	if (fdc_command & 0x02) {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, (BYTE)((fdc_command & 0x08) >> 3));
	}
	else {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, fdc_sidereg);
	}

	/* 先にステータスを設定する */
	fdc_status = stat;

	/* RECORD NOT FOUND ? */
	if (fdc_status & FDC_ST_RECNFND) {
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* 最初のデータを設定 */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* ブートフラグ設定 */
	if ((fdc_trkreg == 0) && (fdc_secreg == 1) && (fdc_sidereg == 0)) {
		fdc_boot = TRUE;
	}

	/* アクセス(READ) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_READ;
}

/*
 *	TYPE II
 *	WRITE DATA
 */
static void FASTCALL fdc_write_data(void)
{
	BYTE stat;

	/* TYPE II, Write */
	fdc_cmdtype = 3;

	/* 基本チェック */
	if (!fdc_rw_sub()) {
		return;
	}

	/* WRITE PROTECTチェック */
	if (fdc_writep[fdc_drvreg] != 0) {
		fdc_make_stat();
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* セクタ検索 */
	if (fdc_command & 0x02) {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, (BYTE)((fdc_command & 0x08) >> 3));
	}
	else {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, fdc_sidereg);
	}

	/* 先にステータスを設定する */
	fdc_status = stat;

	/* RECORD NOT FOUND ? */
	if (fdc_status & FDC_ST_RECNFND) {
		fdc_status = FDC_ST_RECNFND;
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* DRQ */
	fdc_status = FDC_ST_BUSY | FDC_ST_DRQ;
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* アクセス(WRITE) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_WRITE;
}

/*
 *	TYPE III
 *	READ ADDRESS
 */
static void FASTCALL fdc_read_addr(void)
{
	int idx;

	/* TYPE III, Read Address */
	fdc_cmdtype = 4;

	/* 基本チェック */
	if (!fdc_rw_sub()) {
		return;
	}

	/* トラック先頭からの相対セクタ番号を取得 */
	idx = fdc_next_index();

	/* アンフォーマットチェック */
	if (idx == -1) {
		fdc_status = FDC_ST_RECNFND;
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* データ作成 */
	fdc_makeaddr(idx);

	/* 最初のデータを設定 */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* ここで、セクタレジスタにトラックを設定 */
	fdc_secreg = fdc_track[fdc_drvreg];

	/* アクセス(READ) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_READ;
}

/*
 *	TYPE III
 *	READ TRACK
 */
static void FASTCALL fdc_read_track(void)
{
	/* TYPE III, Read Track */
	fdc_cmdtype = 6;

	/* 基本チェック */
	if (!fdc_rw_sub()) {
		return;
	}

	/* データ作成 */
	fdc_make_track();

	/* 最初のデータを設定 */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;
}

/*
 *	TYPE III
 *	WRITE TRACK
 */
static void FASTCALL fdc_write_track(void)
{
	fdc_status = 0;

	/* TYPE III, Write */
	fdc_cmdtype = 5;

	/* 基本チェック */
	if (!fdc_rw_sub()) {
		return;
	}

	/* WRITE PROTECTチェック */
	if (fdc_writep[fdc_drvreg] != 0) {
		fdc_make_stat();
		/* FDC割り込み */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* DRQ */
	fdc_status = FDC_ST_BUSY | FDC_ST_DRQ;
	mainetc_fdc();
	fdc_drqirq = 0x80;

	fdc_dataptr = fdc_buffer;
	fdc_totalcnt = 0x1800;
	fdc_nowcnt = 0;

	/* アクセス(WRITE) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_WRITE;
}

/*
 *	TYPE IV
 *	FORCE INTERRUPT
 */
static void FASTCALL fdc_force_intr(void)
{
	/* ステータス生成 */
	if (fdc_status & 0x01) {
		fdc_status &= ~FDC_ST_BUSY;
		/* 実行中のコマンドと同じステータスを示す */
	}
	else {
		/* INDEX, TRACK00, HEAD ENGAGED, WRTE PROTECT, NOT READY */
		fdc_cmdtype = 1;
		fdc_status = 0;
		fdc_indexcnt = 0;
		fdc_make_stat();
	}

	/* いずれかが立っていれば、IRQ発生(実装としては不十分) */
	if (fdc_command & 0x0f) {
		fdc_drqirq = 0x40;
		mainetc_fdc();
	}

	/* アクセス停止 */
	fdc_readbuf(fdc_drvreg);
	fdc_dataptr = NULL;
	fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
}

/*
 *	コマンド処理
 */
static void FASTCALL fdc_process_cmd(void)
{
	BYTE high;

	high = (BYTE)(fdc_command >> 4);

	/* データ転送を実行していれば、即時止める */
	fdc_dataptr = NULL;

	/* 分岐 */
	switch (high) {
		/* restore */
		case 0x00:
			fdc_restore();
			break;
		/* seek */
		case 0x01:
			fdc_seek();
			break;
		/* step */
		case 0x02:
		case 0x03:
			fdc_step();
			break;
		/* step in */
		case 0x04:
		case 0x05:
			fdc_step_in();
			break;
		/* step out */
		case 0x06:
		case 0x07:
			fdc_step_out();
			break;
		/* read data */
		case 0x08:
		case 0x09:
			fdc_read_data();
			break;
		/* write data */
		case 0x0a:
		case 0x0b:
			fdc_write_data();
			break;
		/* read address */
		case 0x0c:
			fdc_read_addr();
			break;
		/* force interrupt */
		case 0x0d:
			fdc_force_intr();
			break;
		/* read track */
		case 0x0e:
			fdc_read_track();
			break;
		/* write track */
		case 0x0f:
			fdc_write_track();
			break;
		/* それ以外 */
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	FDC
 *	１バイト読み出し
 */
BOOL FASTCALL fdc_readb(WORD addr, BYTE *dat)
{
	switch (addr) {
		/* ステータスレジスタ */
		case 0xfd18:
			fdc_make_stat();
			*dat = fdc_status;
			/* BUSY処理 */
			if ((fdc_status & FDC_ST_BUSY) && (fdc_dataptr == NULL)) {
				fdc_status &= ~FDC_ST_BUSY;
				fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
				return TRUE;
			}
			/* FDC割り込みを、ここで止める */
			mfd_irq_flag = FALSE;
			return TRUE; 

		/* トラックレジスタ */
		case 0xfd19:
			*dat = fdc_trkreg;
			return TRUE;

		/* セクタレジスタ */
		case 0xfd1a:
			*dat = fdc_secreg;
			return TRUE;

		/* データレジスタ */
		case 0xfd1b:
			*dat = fdc_datareg;
			/* カウンタ処理 */
			if (fdc_dataptr) {
				fdc_nowcnt++;
				/* DRQ,IRQ処理 */
				if (fdc_nowcnt == fdc_totalcnt) {
					fdc_status &= ~FDC_ST_BUSY;
					fdc_status &= ~FDC_ST_DRQ;
					if ((fdc_cmdtype == 2) && (fdc_command & 0x10)) {
						/* マルチセクタ処理 */
						/* チャンピオンプロレススペシャル対策 */
						fdc_dataptr = NULL;
						fdc_secreg++;
						fdc_read_data();
						return TRUE;
					}
					/* シングルセクタ */
					fdc_dataptr = NULL;
					mainetc_fdc();
					fdc_drqirq = 0x40;
					fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
					/* Read Trackは必ずLOST DATA、バッファ修復 */
					if (fdc_cmdtype == 6) {
						fdc_status |= FDC_ST_LOSTDATA;
						fdc_readbuf(fdc_drvreg);
					}
				}
				else {
					fdc_datareg = fdc_dataptr[fdc_nowcnt];
					mainetc_fdc();
					fdc_drqirq = 0x80;
				}
			}
			return TRUE;

		/* ヘッドレジスタ */
		case 0xfd1c:
			*dat = (BYTE)(fdc_sidereg | 0xfe);
			return TRUE;

		/* ドライブレジスタ */
		case 0xfd1d:
			if (fdc_motor) {
				*dat = (BYTE)(0xbc | fdc_drvreg);
			}
			else {
				*dat = (BYTE)(0x3c | fdc_drvreg);
			}
			return TRUE;

		/* DRQ,IRQ */
		case 0xfd1f:
			*dat = (BYTE)(fdc_drqirq | 0x3f);
			fdc_drqirq = 0;
			/* データ転送中で無ければBUSY終了(太陽の神殿対策) */
			if (fdc_dataptr == NULL) {
				fdc_status &= ~FDC_ST_BUSY;
				fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	FDC
 *	１バイト書き込み
 */
BOOL FASTCALL fdc_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* コマンドレジスタ */
		case 0xfd18:
			fdc_command = dat;
			fdc_process_cmd();
			return TRUE;

		/* トラックレジスタ */
		case 0xfd19:
			fdc_trkreg = dat;
			return TRUE;

		/* セクタレジスタ */
		case 0xfd1a:
			fdc_secreg = dat;
			return TRUE;

		/* データレジスタ */
		case 0xfd1b:
			fdc_datareg = dat;
			/* カウンタ処理 */
			if (fdc_dataptr) {
				fdc_dataptr[fdc_nowcnt] = fdc_datareg;
				fdc_nowcnt++;
				/* DRQ,IRQ処理 */
				if (fdc_nowcnt == fdc_totalcnt) {
					fdc_status &= ~FDC_ST_BUSY;
					fdc_status &= ~FDC_ST_DRQ;
					if (fdc_cmdtype == 3) {
						if (!fdc_writesec()) {
							fdc_status |= FDC_ST_WRITEFAULT;
						}
					}
					if (fdc_cmdtype == 5) {
						if (!fdc_writetrk()) {
							fdc_status |= FDC_ST_WRITEFAULT;
						}
						/* フォーマットのため使用したバッファを修復 */
						fdc_readbuf(fdc_drvreg);
					}
					if ((fdc_cmdtype == 3) && (fdc_command & 0x10)) {
						/* マルチセクタ処理 */
						fdc_dataptr = NULL;
						fdc_secreg++;
						fdc_write_data();
						return TRUE;
					}
					fdc_dataptr = NULL;
					mainetc_fdc();
					fdc_drqirq = 0x40;
					fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
				}
				else {
					mainetc_fdc();
					fdc_drqirq = 0x80;
				}
			}
			return TRUE;

		/* ヘッドレジスタ */
		case 0xfd1c:
			if ((dat & 0x01) != fdc_sidereg) {
				fdc_sidereg = (BYTE)(dat & 0x01);
				fdc_readbuf(fdc_drvreg);
			}
			return TRUE;

		/* ドライブレジスタ */
		case 0xfd1d:
			/* ドライブ変更なら、fdc_readbuf */
			if (fdc_drvreg != (dat & 0x03)) {
				fdc_drvreg = (BYTE)(dat & 0x03);
				fdc_readbuf(fdc_drvreg);
			}
			fdc_motor = (BYTE)(dat & 0x80);
			/* ドライブ無しなら、モータ止める */
			if (fdc_drvreg >= FDC_DRIVES) {
				fdc_motor = 0;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	FDC
 *	セーブ
 */
BOOL FASTCALL fdc_save(int fileh)
{
	int i;

	/* ファイル関係を先に持ってくる */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_ready[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_write(fileh, (BYTE*)fdc_fname[i], 128 + 1)) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_media[i])) {
			return FALSE;
		}
	}

	/* Ver4拡張 */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_bool_write(fileh, fdc_teject[i]));
	}

	/* ファイルステータス */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_track[i])) {
			return FALSE;
		}
	}
	if (!file_write(fileh, fdc_buffer, 0x2000)) {
		return FALSE;
	}
	/* fdc_dataptrは環境に依存するデータポインタ */
	if (!file_word_write(fileh, (WORD)(fdc_dataptr - &fdc_buffer[0]))) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_write(fileh, fdc_seekofs[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_write(fileh, fdc_secofs[i])) {
			return FALSE;
		}
	}

	/* I/O */
	if (!file_byte_write(fileh, fdc_command)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_status)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_trkreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_secreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_datareg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_sidereg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_drvreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_motor)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_drqirq)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_cmdtype)) {
		return FALSE;
	}

	/* その他 */
	if (!file_word_write(fileh, fdc_totalcnt)) {
		return FALSE;
	}
	if (!file_word_write(fileh, fdc_nowcnt)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, fdc_seekvct)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_indexcnt)) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_access[i])) {
			return FALSE;
		}
	}
	if (!file_bool_write(fileh, fdc_boot)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	FDC
 *	ロード
 */
BOOL FASTCALL fdc_load(int fileh, int ver)
{
	int i;
	BYTE ready[FDC_DRIVES];
	char fname[FDC_DRIVES][128 + 1];
	BYTE media[FDC_DRIVES];
	WORD offset;

	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	/* ファイル関係を先に持ってくる */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &ready[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_read(fileh, (BYTE*)fname[i], 128 + 1)) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &media[i])) {
			return FALSE;
		}
	}

	/* 再マウントを試みる */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_setdisk(i, NULL);
		if (ready[i] != FDC_TYPE_NOTREADY) {
			fdc_setdisk(i, fname[i]);
			if (fdc_ready[i] != FDC_TYPE_NOTREADY) {
				if (fdc_medias[i] >= (media[i] + 1)) {
					fdc_setmedia(i, media[i]);
				}
			}
		}
	}

	/* Ver4拡張 */
	if (ver >= 4) {
		for (i=0; i<FDC_DRIVES; i++) {
			if (!file_bool_read(fileh, &fdc_teject[i])) {
				return FALSE;
			}
		}
	}
	else {
		memset(fdc_teject, 0, sizeof(fdc_teject));
	}

	/* ファイルステータス */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &fdc_track[i])) {
			return FALSE;
		}
	}
	if (!file_read(fileh, fdc_buffer, 0x2000)) {
		return FALSE;
	}
	/* fdc_dataptrは環境に依存するデータポインタ */
	if (!file_word_read(fileh, &offset)) {
		return FALSE;
	}
	fdc_dataptr = &fdc_buffer[offset];
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_read(fileh, &fdc_seekofs[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_read(fileh, &fdc_secofs[i])) {
			return FALSE;
		}
	}

	/* I/O */
	if (!file_byte_read(fileh, &fdc_command)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_status)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_trkreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_secreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_datareg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_sidereg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_drvreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_motor)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_drqirq)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_cmdtype)) {
		return FALSE;
	}

	/* その他 */
	if (!file_word_read(fileh, &fdc_totalcnt)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &fdc_nowcnt)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &fdc_seekvct)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_indexcnt)) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &fdc_access[i])) {
			return FALSE;
		}
	}
	if (!file_bool_read(fileh, &fdc_boot)) {
		return FALSE;
	}

	return TRUE;
}
