// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: fmgen.h,v 1.22 2000/09/08 13:45:56 cisc Exp $

#ifndef FM_GEN_H
#define FM_GEN_H

#include "cisc.h"

// ---------------------------------------------------------------------------
//	�o�̓T���v���̌^
//
#define FM_SAMPLETYPE	int32				// int16 or int32

// ---------------------------------------------------------------------------
//	�萔���̂P
//	�ÓI�e�[�u���̃T�C�Y

#define FM_LFOBITS		8					// �ύX�s��
#define FM_TLBITS		7

// ---------------------------------------------------------------------------

#define FM_TLENTS		(1 << FM_TLBITS)
#define FM_LFOENTS		(1 << FM_LFOBITS)
#define FM_TLPOS		(FM_TLENTS/4)

// ---------------------------------------------------------------------------

namespace FM
{	
	//	Types ----------------------------------------------------------------
	typedef FM_SAMPLETYPE	Sample;
	typedef int32 			ISample;

	enum OpType { typeN=0, typeM=1 };
	
	//	Tables (�O���[�o���Ȃ��̂� asm ����Q�Ƃ������̓�) -----------------
	void MakeTable();
	void MakeTimeTable(uint ratio);
	extern uint32 tltable[];
	extern int32  cltable[];
	extern uint32 dltable[];
	extern int    pmtable[2][8][FM_LFOENTS];
	extern uint   amtable[2][4][FM_LFOENTS];
	extern uint   aml, pml;
	extern int    pmv;		// LFO �ω����x��

	void StoreSample(ISample& dest, int data);

	//	Operator -------------------------------------------------------------
	class Operator
	{
	public:
		Operator();
		static void MakeTable();
		static void	MakeTimeTable(uint ratio);

		ISample	FASTCALL Calc(ISample in);
		ISample	FASTCALL CalcL(ISample in);
		ISample FASTCALL CalcFB(uint fb);
		ISample FASTCALL CalcFBL(uint fb);
		ISample FASTCALL CalcN(uint noise);
		void FASTCALL	Prepare();
		void FASTCALL	KeyOn();
		void FASTCALL	KeyOff();
		void FASTCALL	Reset();
		void FASTCALL	ResetFB();
		int FASTCALL		IsOn();

		void	SetDT(uint dt);
		void	SetDT2(uint dt2);
		void	SetMULTI(uint multi);
		void	SetTL(uint tl, bool csm);
		void	SetKS(uint ks);
		void	SetAR(uint ar);
		void	SetDR(uint dr);
		void	SetSR(uint sr);
		void	SetRR(uint rr);
		void	SetSL(uint sl);
		void	SetSSGEC(uint ssgec);
		void	SetFNum(uint fnum);
		void	SetDPBN(uint dp, uint bn);
		void	SetMode(bool modulator);
		void	SetAMON(bool on);
		void	SetMS(uint ms);
		void	Mute(bool);

		static void SetAML(uint l);
		static void SetPML(uint l);

	private:
		typedef uint32 Counter;

		ISample	out, out2;

	//	Phase Generator ------------------------------------------------------
		uint32 FASTCALL	PGCalc();
		uint32 FASTCALL	PGCalcL();

		uint	dp;			// ��P
		uint	detune;		// Detune
		uint	detune2;	// DT2
		uint	multiple;	// Multiple
		uint32	pgcount;	// Phase ���ݒl
		uint32	pgdcount;	// Phase �����l
		int32	pgdcountl;	// Phase �����l >> x

	//	Envelope Generator ---------------------------------------------------
		enum	EGPhase { next, attack, decay, sustain, release, off };

		void FASTCALL	EGCalc();
		void FASTCALL	ShiftPhase(EGPhase nextphase);
		void FASTCALL	ShiftPhase2();
		void FASTCALL	SetEGRate(uint);
		void FASTCALL	EGUpdate();

		OpType	type;		// OP �̎�� (M, N...)
		uint	bn;			// Block/Note
		int		eglevel;	// EG �̏o�͒l
		int		eglvnext;	// ���� phase �Ɉڂ�l
		int32	egstep;		// EG �̎��̕ψڂ܂ł̎���
		int32	egstepd;	// egstep �̎��ԍ���
		int		egtransa;	// EG �ω��̊��� (for attack)
		int		egtransd;	// EG �ω��̊��� (for decay)
		int		egout;		// EG+TL �����킹���o�͒l
		int		tlout;		// TL ���̏o�͒l
		int		pmd;		// PM depth
		int		amd;		// AM depth

		uint	ksr;		// key scale rate
		EGPhase	phase;
		uint*	ams;
		uint8	ms;

		bool	key;		// current key state

		uint8	tl;			// Total Level	 (0-127)
		uint8	tll;		// Total Level Latch (for CSM mode)
		uint8	ar;			// Attack Rate   (0-63)
		uint8	dr;			// Decay Rate    (0-63)
		uint8	sr;			// Sustain Rate  (0-63)
		uint8	sl;			// Sustain Level (0-127)
		uint8	rr;			// Release Rate  (0-63)
		uint8	ks;			// Keyscale      (0-3)
		uint8	ssgtype;	// SSG-Type Envelop Control

		bool	amon;		// enable Amplitude Modulation
		bool	paramchanged;	// �p�����[�^���X�V���ꂽ
		bool	mute;

	//	Tables ---------------------------------------------------------------
		enum TableIndex { dldecay = 0, dlattack = 0x400, };

		static Counter ratetable[64];
		static uint32 multable[4][16];

	//	friends --------------------------------------------------------------
		friend class Channel4;
		friend void __stdcall FM_NextPhase(Operator* op);
	};

	//	4-op Channel ---------------------------------------------------------
	class Channel4
	{
	public:
		Channel4();
		void SetType(OpType type);

		ISample FASTCALL Calc();
		ISample FASTCALL CalcL();
		ISample FASTCALL CalcN(uint noise);
		ISample FASTCALL CalcLN(uint noise);
		void SetFNum(uint fnum);
		void SetFB(uint fb);
		void SetKCKF(uint kc, uint kf);
		void SetAlgorithm(uint algo);
		int Prepare();
		void KeyControl(uint key);
		void Reset();
		void SetMS(uint ms);
		void Mute(bool);

		static void FASTCALL Calc2 (Channel4* ch, ISample* s0, ISample* s1);
		static void FASTCALL Calc2E(Channel4* ch, ISample* s0, ISample* s1);
		static void FASTCALL CalcL2(Channel4* ch, ISample* s0, ISample* s1);
		static void FASTCALL CalcL2E(Channel4* ch, ISample* s0, ISample* s1);

	private:
		static const uint8 fbtable[8];
		uint	fb;
		int		buf[4];
		int*	in[3];			// �e OP �̓��̓|�C���^
		int*	out[3];			// �e OP �̏o�̓|�C���^
		int*	pms;

	public:
		Operator op[4];
	};
}

#endif // FM_GEN_H
