// ---------------------------------------------------------------------------
//	FM Sound Generator - Core Unit
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: fmgen.cpp,v 1	.25 2000/09/08 13:45:56 cisc Exp $
// ----------	-----------------------------------------------------------------
//	参考:             	
//		FM sound generator for M.A.M.E., written by Tatsuyuki Satoh.
//
//	未実装:
//		OPN SSG-Type Envelop
//
//	謎:
//		OPNB の CSM モード(仕様がよくわからない)
//		OPM noise operator
//
//	制限:
//		・同時に存在する全てのオブジェクトにおいてサンプリングレート比
//		  (合成/出力)は等しくなければならない
//		・LFO を使う場合，異なるオブジェクト間においてもスレッドセーフでは
//		  なくなる
//
//	謝辞:
//		Tatsuyuki Satoh さん(fm.c)
//		Hiromitsu Shioya さん(ADPCM-A)
//		DMP-SOFT. さん(OPNB)
//		KAJA さん(test program)
//		ほか掲示板等で様々なご助言，ご支援をお寄せいただいた皆様に
// ---------------------------------------------------------------------------

#include "cisc.h"
#include "fmgen.h"
#include "fmgeninl.h"

#define LOGNAME "fmgen"

// ---------------------------------------------------------------------------
//	X86 版コードを使用する場合，
//	定数の値を sizeof(Channel4) と等しくなるように設定すること．
//
#define CH4S	496				// = sizeof(Channel4)

// ---------------------------------------------------------------------------

#define FM_EGBITS		16

// ---------------------------------------------------------------------------
//	Table/etc
//
namespace FM
{
	static const uint8 notetable[128] =
	{
		 0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  3,  3,  3,  3,  3,  3, 
		 4,  4,  4,  4,  4,  4,  4,  5,  6,  7,  7,  7,  7,  7,  7,  7, 
		 8,  8,  8,  8,  8,  8,  8,  9, 10, 11, 11, 11, 11, 11, 11, 11, 
		12, 12, 12, 12, 12, 12, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 
		16, 16, 16, 16, 16, 16, 16, 17, 18, 19, 19, 19, 19, 19, 19, 19, 
		20, 20, 20, 20, 20, 20, 20, 21, 22, 23, 23, 23, 23, 23, 23, 23, 
		24, 24, 24, 24, 24, 24, 24, 25, 26, 27, 27, 27, 27, 27, 27, 27, 
		28, 28, 28, 28, 28, 28, 28, 29, 30, 31, 31, 31, 31, 31, 31, 31, 
	};
	
	static const int8 dttable[256] =
	{
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		  0,  0,  0,  0,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,
		  4,  6,  6,  6,  8,  8,  8, 10, 10, 12, 12, 14, 16, 16, 16, 16,
		  2,  2,  2,  2,  4,  4,  4,  4,  4,  6,  6,  6,  8,  8,  8, 10,
		 10, 12, 12, 14, 16, 16, 18, 20, 22, 24, 26, 28, 32, 32, 32, 32,
		  4,  4,  4,  4,  4,  6,  6,  6,  8,  8,  8, 10, 10, 12, 12, 14,
		 16, 16, 18, 20, 22, 24, 26, 28, 32, 34, 38, 40, 44, 44, 44, 44,
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		  0,  0,  0,  0, -2, -2, -2, -2, -2, -2, -2, -2, -4, -4, -4, -4,
		 -4, -6, -6, -6, -8, -8, -8,-10,-10,-12,-12,-14,-16,-16,-16,-16,
		 -2, -2, -2, -2, -4, -4, -4, -4, -4, -6, -6, -6, -8, -8, -8,-10,
		-10,-12,-12,-14,-16,-16,-18,-20,-22,-24,-26,-28,-32,-32,-32,-32,
		 -4, -4, -4, -4, -4, -6, -6, -6, -8, -8, -8,-10,-10,-12,-12,-14,
		-16,-16,-18,-20,-22,-24,-26,-28,-32,-34,-38,-40,-44,-44,-44,-44,
	};

	uint	sinetable[1024];
	uint32	tltable[FM_TLPOS + FM_TLENTS];
	int32	cltable[FM_CLENTS*2];
	int		kftable[64];
	
	int		pmtable[2][8][FM_LFOENTS];
	uint	amtable[2][4][FM_LFOENTS];

	static bool tablemade = false;
	static uint currentratio = ~0;

	uint	aml, pml;
	int		pmv;
}

namespace FM
{

// ---------------------------------------------------------------------------
//	テーブル作成
//
void MakeTable()
{
	if (tablemade)
		return;

#ifdef FM_USE_X86_CODE
#ifdef _WIN32
	if (CH4S != sizeof(Channel4))
	{
		char buf[40];
		wsprintf(buf, "CH4S incorrect.\nsizeof(Channel4) == %d", sizeof(Channel4));
		MessageBox(0, buf, "FMGen", MB_OK);
	}
#endif
	assert(CH4S == sizeof(Channel4));
#endif

	tablemade = true;
	
	int i;
	for (i=-FM_TLPOS; i<FM_TLENTS; i++)
	{
		tltable[FM_TLPOS + i] = uint(65536. * pow(2.0, i * -16. / FM_TLENTS))-1;
//		LOG2("tltable[%4d] = 0x%.4x\n", i, tltable[FM_TLPOS+i]);
	}
	for (i=0; i<FM_CLENTS; i++)
	{
		int c = int(((1 << (16 + FM_ISHIFT)) - 1) * pow(2.0, -i / 64.));
#if 1
		// 精度抑制, 上位 11 bit, MSB から 15 bit まで
		c &= 0x7fff << (1 + FM_ISHIFT);
		for (int j=16+FM_ISHIFT; j>11; j--)
		{
			if ((1 << j) & c)
			{
				c &= ((1 << 12) - 1) << (j - 11);
				break;
			}
		}
#endif
		cltable[i*2  ] = c;
		cltable[i*2+1] = -c;
//		LOG2("cltable[%4d*2] = %6d\n", i, cltable[i*2]);
	}

	static const double pms[2][8] = 
	{ 
		{ 0, 1/720., 2/720., 3/720.,  4/720.,  6/720., 12/720.,  24/720., },	// OPNA
		{ 0, 1/240., 2/240., 4/240., 10/240., 20/240., 80/240., 140/240., },	// OPM
	};
	//		 3		 6,      12      30       60       240      420		/ 720
	//	1.000963
	//	lfofref[level * max * wave];
	//	pre = lfofref[level][pms * wave >> 8];
	static const uint8 amt[2][4] = 
	{
		{ 31, 6, 4, 3 }, // OPNA
		{ 31, 2, 1, 0 }, //	OPM
	};
	
	for (int type = 0; type < 2; type++)
	{
		for (i=0; i<8; i++)
		{
			double pmb = pms[type][i];
			for (int j=0; j<FM_LFOENTS; j++)
			{
				pmtable[type][i][j] = 
					int(0x10000 * (pow(2.0, pmb * (2*j - FM_LFOENTS+1) / (FM_LFOENTS-1)) - 1));
//				LOG4("pmtable[%d][%d][%.2x] = %5d ", type, i, j, pmtable[type][i][j]);
//				LOG1(" %7.2f\n", log(1. + pmtable[type][i][j] / 65536.) / log(2) * 1200);
			}
		}
		for (i=0; i<4; i++)
		{
			for (int j=0; j<FM_LFOENTS; j++)
			{
				amtable[type][i][j] = ((j * 4) >> amt[type][i]) * 2;
				LOG4("amtable[%d][%d][%.2x] = %5d\n", type, i, j, amtable[type][i][j]);
			}
		}
	}

	double il2 = 128. / log(2.);
	sinetable[0] = 2047;
	for (i=1; i<FM_OPSINENTS/4; i++)
	{
		double s = -log(sin(2. * FM_PI * i / FM_OPSINENTS)) * il2;
		sinetable[i] = Min(int(s), 2047);
	}
	for (i=0; i<FM_OPSINENTS/4; i++)
	{
		uint s = sinetable[i] & ~1;
		sinetable[i]                  = s;
		sinetable[FM_OPSINENTS/2-1-i] = s;
		sinetable[FM_OPSINENTS/2 + i] = s + 1;
		sinetable[FM_OPSINENTS-1 - i] = s + 1;
	}

	for (i=0; i<64; i++)
	{
		kftable[i] = int(0x10000 * pow(2., i/768.)+.5);
	}
}

//	チップのサンプリングレートと生成する音のサンプリングレートとの比を設定
void MakeTimeTable(uint ratio)
{
	if (ratio != currentratio)
	{
		currentratio = ratio;
		Operator::MakeTimeTable(ratio);
	}
}

// ---------------------------------------------------------------------------
//	Operator
//

//	構築
FM::Operator::Operator()
{
	// EG Part
	ar = dr = sr = rr = ksr = 0;
	ams = amtable[0][0];
	mute = false;

	// PG Part
	multiple = 0;
	detune = 0;
	detune2 = 0;
	
	Reset();
}

//	初期化
void FASTCALL FM::Operator::Reset()
{
	// EG part
	tl = tll = 127;
	ShiftPhase(off);
	egstep = 0;

	// PG part
	pgcount = 0;

	// OP part
	out = out2 = 0;

	paramchanged = true;
}


//	クロック・サンプリングレート比に依存するテーブルを作成
void Operator::MakeTimeTable(uint ratio)
{
	int h, l;
	
	// PG Part
	static const float dt2lv[4] = { 1.f, 1.414f, 1.581f, 1.732f };
	for (h=0; h<4; h++)
	{
		double rr = dt2lv[h] * double(ratio) / (1 << (2 + FM_RATIOBITS - FM_PGBITS));
		for (l=0; l<16; l++)
		{
			int mul = l ? l * 2 : 1;
			multable[h][l] = uint(mul * rr);
		}
	}

	// EG
	for (h=1; h<16; h++)
	{
		for (l=0; l<4; l++)
		{
			int m = h == 15 ? 8 : l+4;
			ratetable[h*4+l] = 
				((ratio << (FM_EGBITS - 3 - FM_RATIOBITS)) << Min(h, 11)) * m;
		}
	}
	ratetable[0] = ratetable[1] = ratetable[2] = ratetable[3] = 0;
	ratetable[5] = ratetable[4],  ratetable[7] = ratetable[6];
}

inline void FM::Operator::SetDPBN(uint _dp, uint _bn)
{
	dp = _dp, bn = _bn; paramchanged = true; 
}


//	準備
void FASTCALL Operator::Prepare()
{
	if (paramchanged)
	{
		paramchanged = false;
		//	PG Part
		pgdcount = (dp + dttable[detune + bn]) * multable[detune2][multiple];
		pgdcountl = pgdcount >> 11;

		// EG Part
		ksr = bn >> (3-ks);
		tlout = mute ? 0x3ff : tl * 8;
		
		switch (phase)
		{
		case attack:
			SetEGRate(ar ? Min(63, ar+ksr) : 0);
			break;
		case decay:
			SetEGRate(dr ? Min(63, dr+ksr) : 0);
			eglvnext = sl * 8;
			break;
		case sustain:
			SetEGRate(sr ? Min(63, sr+ksr) : 0);
			break;
		case release:
			SetEGRate(Min(63, rr+ksr));
			break;
		}
		// LFO
		ams = amtable[type][amon ? (ms >> 4) & 3 : 0];
		EGUpdate();
	}
}
//	envelope の phase 変更
void FASTCALL Operator::ShiftPhase(EGPhase nextphase)
{
	switch (nextphase)
	{
	case attack:		// Attack Phase
		tl = tll;
		if ((ar+ksr) < 62)
		{
//			LOG0("@@ATTACK\n");
			SetEGRate(ar ? Min(63, ar+ksr) : 0);
			phase = attack;
			break;
		}
	case decay:			// Decay Phase
		if (sl)
		{
			eglevel = 0;
			eglvnext = sl*8;
			SetEGRate(dr ? Min(63, dr+ksr) : 0);
			phase = decay;
//			LOG0("@@DECAY\n");
			break;
		}
	case sustain:		// Sustain Phase
		eglevel = sl*8;
		eglvnext = 0x400;
		SetEGRate(sr ? Min(63, sr+ksr) : 0);
		phase = sustain;
//		LOG0("@@SUSTAIN\n");
		break;
	
	case release:		// Release Phase
		if (phase == attack || (eglevel < 0x400/* && phase != off*/))
		{
			eglvnext = 0x400;
			SetEGRate(Min(63, rr+ksr));
			phase = release;
//			LOG0("@@RELEASE\n");
			break;
		}
	case off:			// off
	default:
		eglevel = 0x3ff;
		eglvnext = 0x400;
		egout = 0x3ff;
		SetEGRate(0);
//		LOG0("@@OFF\n");
		phase = off;
		break;
	}
}


//	envelope の phase 変更 (for x86 engine)
inline void FASTCALL Operator::ShiftPhase2()
{
	switch (phase)
	{
	case attack:
		if (sl)
		{
			eglevel = 0;
			eglvnext = sl*8;
			SetEGRate(dr ? Min(63, dr+ksr) : 0);
			phase = decay;
			break;
		}
	case decay:
		eglevel = sl*8;
		eglvnext = 0x400;
		SetEGRate(sr ? Min(63, sr+ksr) : 0);
		phase = sustain;
		break;
	
	case sustain:
		if (phase == attack || (eglevel < 0x400 && phase != off))
		{
			eglvnext = 0x400;
			SetEGRate(Min(63, rr+ksr));
			phase = release;
			break;
		}
	case release:
	case off:
	default:
		eglevel = 0x3ff;
		eglvnext = 0x400;
		SetEGRate(0);
		phase = off;
		break;
	}
}


//	Block/F-Num
void Operator::SetFNum(uint f)
{
	dp = (f & 2047) << ((f >> 11) & 7);
	bn = notetable[(f >> 7) & 127];
	paramchanged = true;
//	LOG1("dp = %.8x\n", dp);
}

//	static tables
uint32 Operator::multable[4][16];
Operator::Counter Operator::ratetable[64];

// ---------------------------------------------------------------------------
//	4-op Channel
//
const uint8 Channel4::fbtable[8] = { 31, 7, 6, 5, 4, 3, 2, 1 };

Channel4::Channel4()
{
	SetAlgorithm(0);
	pms = pmtable[0][0];
}

// リセット
void Channel4::Reset()
{
	op[0].Reset();
	op[1].Reset();
	op[2].Reset();
	op[3].Reset();
}

//	Calc の用意
int Channel4::Prepare()
{
	op[0].Prepare();
	op[1].Prepare();
	op[2].Prepare();
	op[3].Prepare();
	
	pms = pmtable[op[0].type][op[0].ms & 7];
	int key = (op[0].IsOn() | op[1].IsOn() | op[2].IsOn() | op[3].IsOn()) ? 1 : 0;
	int lfo = op[0].ms & (op[0].amon | op[1].amon | op[2].amon | op[3].amon ? 0x37 : 7) ? 2 : 0;
	return key | lfo;
}

//	F-Number/BLOCK を設定
void Channel4::SetFNum(uint f)
{
	uint dp = (f & 2047) << ((f >> 11) & 7);
	uint bn = notetable[(f >> 7) & 127];
	op[0].SetDPBN(dp, bn);
	op[1].SetDPBN(dp, bn);
	op[2].SetDPBN(dp, bn);
	op[3].SetDPBN(dp, bn);
}

//	KC/KF を設定
void Channel4::SetKCKF(uint kc, uint kf)
{
#if 1
	// 理論値
	const static uint kctable[16] = 
	{ 
		5197, 5506, 5833, 6180, 6180, 6547, 6937, 7349, 
		7349, 7786, 8249, 8740, 8740, 9259, 9810, 10394, 
	};
#else
	// 引用(X68Sound.dll)
	const static uint kctable[16] = 
	{ 
		5196, 5504, 5832, 6180, 6180, 6548, 6936, 7348, 
		7348, 7784, 8248, 8740, 8740, 9260, 9808, 10392, 
	};
#endif

	int oct = 19 - ((kc >> 4) & 7);
	uint dp = (kctable[kc & 0x0f] * kftable[kf & 0x3f]) >> oct;
	uint bn = (kc >> 2) & 31;
	op[0].SetDPBN(dp, bn);
	op[1].SetDPBN(dp, bn);
	op[2].SetDPBN(dp, bn);
	op[3].SetDPBN(dp, bn);
}

//	キー制御
void Channel4::KeyControl(uint key)
{
	if (key & 0x1) op[0].KeyOn(); else op[0].KeyOff();
	if (key & 0x2) op[1].KeyOn(); else op[1].KeyOff();
	if (key & 0x4) op[2].KeyOn(); else op[2].KeyOff();
	if (key & 0x8) op[3].KeyOn(); else op[3].KeyOff();
}

//	アルゴリズムを設定
void Channel4::SetAlgorithm(uint algo)
{
	static const uint8 table1[8][6] = 
	{
		{ 0, 1, 1, 2, 2, 3 },	{ 1, 0, 0, 1, 1, 2 },
		{ 1, 1, 1, 0, 0, 2 },	{ 0, 1, 2, 1, 1, 2 },
		{ 0, 1, 2, 2, 2, 1 },	{ 0, 1, 0, 1, 0, 1 },
		{ 0, 1, 2, 1, 2, 1 },	{ 1, 0, 1, 0, 1, 0 },
	};
	
	in [0] = &buf[table1[algo][0]];
	out[0] = &buf[table1[algo][1]];
	in [1] = &buf[table1[algo][2]];
	out[1] = &buf[table1[algo][3]];
	in [2] = &buf[table1[algo][4]];
	out[2] = &buf[table1[algo][5]];
	op[0].ResetFB();
}

// ---------------------------------------------------------------------------
//	4-op Channel 
//	１サンプル合成

//	ISample を envelop count (2π) に変換するシフト量
#define IS2EC_SHIFT		((20 + FM_PGBITS) - (16 + FM_ISHIFT))

// 入力: s = 20+FM_PGBITS = 29
#define Sine(s)	sinetable[((s) >> (20+FM_PGBITS-FM_OPSINBITS))&(FM_OPSINENTS-1)]

static inline FM::ISample LogToLin(uint a)
{
#if FM_CLENTS < 0xc00		// 400 for TL, 400 for ENV, 400 for LFO.
	return (a < FM_CLENTS * 2) ? FM::cltable[a] : 0;
#else
	return FM::cltable[a];
#endif
}

void FASTCALL Operator::SetEGRate(uint r)
{
	egstepd = ratetable[r];
	egtransa = Limit(15 - (r>>2), 4, 1);
	egtransd = 16 >> egtransa;
}

inline void FASTCALL Operator::EGUpdate()
{
#if FM_CLENTS < 0xc00
	egout = Min(tlout + eglevel, 0x3ff) * 2;
#else
	egout = (tlout + eglevel) * 2;
#endif
}

//	EG 計算
void FASTCALL FM::Operator::EGCalc()
{
	egstep += 3L << (11 + FM_EGBITS);
	if (phase == attack)
	{
		eglevel -= 1 + (eglevel >> egtransa);
		if (eglevel <= 0)
			ShiftPhase(decay);
	}
	else
	{
		eglevel += egtransd;
		if (eglevel >= eglvnext)
			ShiftPhase(EGPhase(phase+1));
	}
	EGUpdate();
}

//	PG 計算
//	ret:2^(20+PGBITS) / cycle
inline uint32 FASTCALL FM::Operator::PGCalc()
{
	uint32 ret = pgcount;
	pgcount += pgdcount;
	return ret;
}

inline uint32 FASTCALL FM::Operator::PGCalcL()
{
	uint32 ret = pgcount;
	pgcount += pgdcount + ((pgdcountl * pmv) >> 5);
	return ret;
}

//	OP 計算
//	in: ISample (最大 8π)
inline FM::ISample FASTCALL FM::Operator::Calc(ISample in)
{
	if ((egstep -= egstepd) <= 0)
		EGCalc();
	return LogToLin(egout + Sine(PGCalc() + (in << (2 + IS2EC_SHIFT))));
}

inline FM::ISample FASTCALL FM::Operator::CalcL(ISample in)
{
	if ((egstep -= egstepd) <= 0)
		EGCalc();
	return LogToLin(egout + Sine(PGCalcL() + (in << (2 + IS2EC_SHIFT))) + ams[aml]);
}

inline FM::ISample FASTCALL FM::Operator::CalcN(uint noise)
{
	if ((egstep -= egstepd) <= 0)
		EGCalc();
	
	int lv = (egout ^ 0x3ff) << (1 + FM_ISHIFT);
	noise = (noise & 1) - 1;
	return (lv + noise) ^ noise;
}

//	OP (FB) 計算
//	Self Feedback の変調最大 = 4π
inline FM::ISample FASTCALL FM::Operator::CalcFB(uint fb)
{
	if ((egstep -= egstepd) <= 0)
		EGCalc();
	
	ISample in = out + out2;
	out2 = out;
	out = LogToLin(egout + Sine(PGCalc() + ((in << (1 + IS2EC_SHIFT)) >> fb)));
	return out;
}

inline FM::ISample FASTCALL FM::Operator::CalcFBL(uint fb)
{
	if ((egstep -= egstepd) <= 0)
		EGCalc();
	
	ISample in = out + out2;
	out2 = out;
	out = LogToLin(egout + Sine(PGCalcL() + ((in << (1 + IS2EC_SHIFT)) >> fb)) + ams[aml]);
	return out;
}

#undef Sine

#ifndef FM_USE_X86_CODE

//  合成
ISample FASTCALL Channel4::Calc()
{
	buf[1] = buf[2] = buf[3] = 0;

	buf[0] = op[0].CalcFB(fb);
	*out[0] += op[1].Calc(*in[0]);
	*out[1] += op[2].Calc(*in[1]);
	return *out[2] + op[3].Calc(*in[2]);
}

//  合成
ISample FASTCALL Channel4::CalcL()
{
	pmv = pms[pml];
	buf[1] = buf[2] = buf[3] = 0;

	buf[0] = op[0].CalcFBL(fb);
	*out[0] += op[1].CalcL(*in[0]);
	*out[1] += op[2].CalcL(*in[1]);
	return *out[2] + op[3].CalcL(*in[2]);
}

#endif

//  合成
ISample FASTCALL Channel4::CalcN(uint noise)
{
	buf[1] = buf[2] = buf[3] = 0;

	buf[0] = op[0].CalcFB(fb);
	*out[0] += op[1].Calc(*in[0]);
	*out[1] += op[2].Calc(*in[1]);
	return *out[2] + op[3].CalcN(noise);
}

//  合成
ISample FASTCALL Channel4::CalcLN(uint noise)
{
	pmv = pms[pml];
	buf[1] = buf[2] = buf[3] = 0;

	buf[0] = op[0].CalcFBL(fb);
	*out[0] += op[1].CalcL(*in[0]);
	*out[1] += op[2].CalcL(*in[1]);
	return *out[2] + op[3].CalcN(noise);
}



// for compatibility
void FASTCALL Channel4::Calc2(Channel4* ch, ISample* s0, ISample* s1)
{
	ch[0].buf[1] = ch[0].buf[2] = ch[0].buf[3] = 0;
	ch[1].buf[1] = ch[1].buf[2] = ch[1].buf[3] = 0;

	        ch[0].buf[0]  = ch[0].op[0].CalcFB(ch[0].fb);
	        ch[1].buf[0]  = ch[1].op[0].CalcFB(ch[1].fb);
	       *ch[0].out[0] += ch[0].op[1].Calc(*ch[0].in[0]);
	       *ch[1].out[0] += ch[1].op[1].Calc(*ch[1].in[0]);
	       *ch[0].out[1] += ch[0].op[2].Calc(*ch[0].in[1]);
	       *ch[1].out[1] += ch[1].op[2].Calc(*ch[1].in[1]);
	*s0 += *ch[0].out[2] +  ch[0].op[3].Calc(*ch[0].in[2]);
	*s1 += *ch[1].out[2] +  ch[1].op[3].Calc(*ch[1].in[2]);
}

// for compatibility
void FASTCALL Channel4::CalcL2(Channel4* ch, ISample* s0, ISample* s1)
{
	ch[0].buf[1] = ch[0].buf[2] = ch[0].buf[3] = 0;
	ch[1].buf[1] = ch[1].buf[2] = ch[1].buf[3] = 0;

	pmv = ch[0].pms[pml];
	        ch[0].buf[0]  = ch[0].op[0].CalcFBL(ch[0].fb);
	       *ch[0].out[0] += ch[0].op[1].CalcL(*ch[0].in[0]);
	       *ch[0].out[1] += ch[0].op[2].CalcL(*ch[0].in[1]);
	*s0 += *ch[0].out[2] +  ch[0].op[3].CalcL(*ch[0].in[2]);
	
	pmv = ch[1].pms[pml];
	        ch[1].buf[0]  = ch[1].op[0].CalcFBL(ch[1].fb);
	       *ch[1].out[0] += ch[1].op[1].CalcL(*ch[1].in[0]);
	       *ch[1].out[1] += ch[1].op[2].CalcL(*ch[1].in[1]);
	*s1 += *ch[1].out[2] +  ch[1].op[3].CalcL(*ch[1].in[2]);
}

void FASTCALL Channel4::Calc2E(Channel4* ch, ISample* s0, ISample* s1)
{
	Calc2(ch, s0, s1);
}

void FASTCALL Channel4::CalcL2E(Channel4* ch, ISample* s0, ISample* s1)
{
	CalcL2(ch, s0, s1);
}


#ifdef FM_USE_X86_CODE
#include "fmgenx86.h"
#endif



}	// namespace FM
