/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ ���ʒ�` ]
 */

#ifndef _xm7_h_
#define _xm7_h_

#include <stdio.h>

/*
 *	�萔�A�^��`
 */

/* �ėp�萔 */
#ifndef FALSE
#define FALSE			0
#define TRUE			(!FALSE)
#endif
#ifndef NULL
#define NULL			((void)0)
#endif

/* �f�f */
#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(exp)		assert(exp)
#else
#define ASSERT(exp)		((void)0)
#endif
#endif

/* �œK�� */
#if defined(_WIN32) && defined(__BORLANDC__)
#define FASTCALL		__fastcall
#else
#define FASTCALL
#endif

/* ��{�^��` */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

/* CPU���W�X�^��` */
#ifdef _WIN32
#pragma pack(push, 1)
#endif
typedef struct {
	BYTE cc;
	BYTE dp;
	union {
		struct {
#ifdef _WIN32
			BYTE b;
			BYTE a;
#endif
#ifdef __MSDOS__
			BYTE b;
			BYTE a;
#endif
#ifdef HUMAN68K
			BYTE a;
			BYTE b;
#endif
#ifdef _XWIN
			BYTE b __attribute__((aligned(1)));
			BYTE a __attribute__((aligned(1)));
#endif
		} h;
		WORD d;
	} acc;
	WORD x;
	WORD y;
	WORD u;
	WORD s;
	WORD pc;
	WORD intr;
	WORD cycle;
	WORD total;
	BYTE FASTCALL (*readmem)(WORD);
	void FASTCALL (*writemem)(WORD, BYTE);
} cpu6809_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

/* ���荞�ݒ�` */
#define INTR_NMI		0x0001			/* NMI���荞�� */
#define INTR_FIRQ		0x0002			/* FIRQ���荞�� */
#define INTR_IRQ		0x0004			/* IRQ���荞�� */

#define INTR_SLOAD		0x0010			/* ���Z�b�g��S��ݒ� */
#define INTR_SYNC_IN	0x0020			/* SYNC���s�� */
#define INTR_SYNC_OUT	0x0040			/* SYNC�I���\ */
#define INTR_CWAI_IN	0x0080			/* CWAI���s��
#define INTR_CWAI_OUT	0x0100			/* CWAI�I���\ */

/* �u���[�N�|�C���g��` */
#define BREAKP_NOTUSE	0				/* ���g�p */
#define BREAKP_ENABLED	1				/* �g�p�� */
#define BREAKP_DISABLED	2				/* �֎~�� */
#define BREAKP_STOPPED	3				/* ��~�� */
#define BREAKP_MAXNUM	8				/* �u���[�N�|�C���g�̌� */
typedef struct {
	int flag;							/* ��̃t���O */
	int cpu;							/* CPU��� */
	WORD addr;							/* �u���[�N�|�C���g�A�h���X */
} breakp_t;

/* �C�x���g��` */
#define EVENT_NOTUSE	0				/* ���g�p */
#define EVENT_ENABLED	1				/* �g�p�� */
#define EVENT_DISABLED	2				/* �֎~�� */
#define EVENT_MAXNUM	16				/* �C�x���g�̌� */
typedef struct {
	int flag;							/* ��̃t���O */
	DWORD current;						/* �J�����g���ԃJ�E���^ */
	DWORD reload;						/* �����[�h���ԃJ�E���^ */
	BOOL FASTCALL (*callback)(void);	/* �R�[���o�b�N�֐� */
} event_t;

/* ���̑��萔 */
#define MAINCPU			0				/* ���C��CPU */
#define SUBCPU			1				/* �T�uCPU */

#define BOOT_BASIC		0				/* BASIC���[�h */
#define BOOT_DOS		1				/* DOS���[�h */

/*
 * ROM�t�@�C������`
 */
#define FBASIC_ROM		"FBASIC30.ROM"
#define SUBSYSC_ROM		"SUBSYS_C.ROM"
#define KANJI_ROM		"KANJI.ROM"
#define INITIATE_ROM	"INITIATE.ROM"
#define SUBSYSA_ROM		"SUBSYS_A.ROM"
#define SUBSYSB_ROM		"SUBSYS_B.ROM"
#define SUBSYSCG_ROM	"SUBSYSCG.ROM"

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */

/* �V�X�e��(system.c) */
BOOL FASTCALL system_init(void);
										/* �V�X�e�� ������ */
void FASTCALL system_cleanup(void);
										/* �V�X�e�� �N���[���A�b�v */
void FASTCALL system_reset(void);
										/* �V�X�e�� ���Z�b�g */
BOOL FASTCALL system_save(char *filename);
										/* �V�X�e�� �Z�[�u */
BOOL FASTCALL system_load(char *filename);
										/* �V�X�e�� ���[�h */

/* �X�P�W���[��(schedule.c) */
BOOL FASTCALL schedule_init(void);
										/* �X�P�W���[�� ������ */
void FASTCALL schedule_cleanup(void);
										/* �X�P�W���[�� �N���[���A�b�v */
void FASTCALL schedule_reset(void);
										/* �X�P�W���[�� ���Z�b�g */
DWORD FASTCALL schedule_exec(DWORD microsec);
										/* ���s */
void FASTCALL schedule_trace(void);
										/* �g���[�X */
BOOL FASTCALL schedule_setbreak(int cpu, WORD addr);
										/* �u���[�N�|�C���g�ݒ� */
BOOL FASTCALL schedule_setevent(int id, DWORD microsec, BOOL FASTCALL (*func)(void));
										/* �C�x���g�ǉ� */
BOOL FASTCALL schedule_delevent(int id);
										/* �C�x���g�폜 */
void FASTCALL schedule_handle(int id, BOOL FASTCALL (*func)(void));
										/* �C�x���g�n���h���ݒ� */
BOOL FASTCALL schedule_save(int fileh);
										/* �X�P�W���[�� �Z�[�u */
BOOL FASTCALL schedule_load(int fileh, int ver);
										/* �X�P�W���[�� ���[�h */

/* �t�A�Z���u��(disasm.c) */
int FASTCALL disline(int cputype, WORD pc, char *buffer);
										/* �P�s�t�A�Z���u�� */

/* ���C��CPU������(mainmem.c) */
BOOL FASTCALL mainmem_init(void);
										/* ���C��CPU������ ������ */
void FASTCALL mainmem_cleanup(void);
										/* ���C��CPU������ �N���[���A�b�v */
void FASTCALL mainmem_reset(void);
										/* ���C��CPU������ ���Z�b�g */
BYTE FASTCALL mainmem_readb(WORD addr);
										/* ���C��CPU������ �ǂݏo�� */
BYTE FASTCALL mainmem_readbnio(WORD addr);
										/* ���C��CPU������ �ǂݏo��(I/O�Ȃ�) */
void FASTCALL mainmem_writeb(WORD addr, BYTE dat);
										/* ���C��CPU������ �������� */
BOOL FASTCALL mainmem_save(int fileh);
										/* ���C��CPU������ �Z�[�u */
BOOL FASTCALL mainmem_load(int fileh, int ver);
										/* ���C��CPU������ ���[�h */

/* �T�uCPU������(submem.c) */
BOOL FASTCALL submem_init(void);
										/* �T�uCPU������ ������ */
void FASTCALL submem_cleanup(void);
										/* �T�uCPU������ �N���[���A�b�v */
void FASTCALL submem_reset(void);
										/* �T�uCPU������ ���Z�b�g */
BYTE FASTCALL submem_readb(WORD addr);
										/* �T�uCPU������ �ǂݏo�� */
BYTE FASTCALL submem_readbnio(WORD addr);
										/* �T�uCPU������ �ǂݏo��(I/O�Ȃ�) */
void FASTCALL submem_writeb(WORD addr, BYTE dat);
										/* �T�uCPU������ �������� */
BOOL FASTCALL submem_save(int fileh);
										/* �T�uCPU������ �Z�[�u */
BOOL FASTCALL submem_load(int fileh, int ver);
										/* �T�uCPU������ ���[�h */

/* ���C��CPU(maincpu.c) */
BOOL FASTCALL maincpu_init(void);
										/* ���C��CPU ������ */
void FASTCALL maincpu_cleanup(void);
										/* ���C��CPU �N���[���A�b�v */
void FASTCALL maincpu_reset(void);
										/* ���C��CPU ���Z�b�g */
void FASTCALL maincpu_execline(void);
										/* ���C��CPU �P�s���s */
void FASTCALL maincpu_exec(void);
										/* ���C��CPU ���s */
void FASTCALL maincpu_firq(void);
										/* ���C��CPU FIRQ���荞�ݗv�� */
void FASTCALL maincpu_irq(void);
										/* ���C��CPU IRQ���荞�ݗv�� */
BOOL FASTCALL maincpu_save(int fileh);
										/* ���C��CPU �Z�[�u */
BOOL FASTCALL maincpu_load(int fileh, int ver);
										/* ���C��CPU ���[�h */

/* �T�uCPU(subcpu.c) */
BOOL FASTCALL subcpu_init(void);
										/* �T�uCPU ������ */
void FASTCALL subcpu_cleanup(void);
										/* �T�uCPU �N���[���A�b�v */
void FASTCALL subcpu_reset(void);
										/* �T�uCPU ���Z�b�g */
void FASTCALL subcpu_execline(void);
										/* �T�uCPU �P�s���s */
void FASTCALL subcpu_exec(void);
										/* �T�uCPU ���s */
void FASTCALL subcpu_nmi(void);
										/* �T�uCPU NMI���荞�ݗv�� */
void FASTCALL subcpu_firq(void);
										/* �T�uCPU FIRQ���荞�ݗv�� */
void FASTCALL subcpu_irq(void);
										/* �T�uCPU IRQ���荞�ݗv�� */
BOOL FASTCALL subcpu_save(int fileh);
										/* �T�uCPU �Z�[�u */
BOOL FASTCALL subcpu_load(int fileh, int ver);
										/* �T�uCPU ���[�h */

/*
 *	CPU�A���̑���v���[�N�G���A
 */
extern cpu6809_t maincpu;
										/* ���C��CPU */
extern cpu6809_t subcpu;
										/* �T�uCPU */
extern int fm7_ver;
										/* ����o�[�W���� */
extern breakp_t breakp[BREAKP_MAXNUM];
										/* �u���[�N�|�C���g */
extern event_t event[EVENT_MAXNUM];
										/* �C�x���g */
extern BOOL run_flag;
										/* ����t���O */
extern BOOL stopreq_flag;
										/* ��~�v���t���O */
extern DWORD main_speed;
										/* ���C��CPU�X�s�[�h */
extern DWORD mmr_speed;
										/* ���C��CPU(MMR)�X�s�[�h */
extern BOOL cycle_steel;
										/* �T�C�N���X�`�[���t���O */

/*
 *	������
 */
extern BYTE *mainram_a;
										/* RAM (�\RAM)      $8000 */
extern BYTE *mainram_b;
										/* RAM (��RAM)      $7C80 */
extern BYTE *basic_rom;
										/* ROM (F-BASIC)    $7C00 */
extern BYTE *main_io;
										/* ���C��CPU I/O    $0100 */
extern BYTE *vram_c;
										/* VRAM(�^�C�vC)    $C000 */
extern BYTE *subrom_c;
										/* ROM (�^�C�vC)    $2800 */
extern BYTE *sub_ram;
										/* �R���\�[��RAM    $1680 */
extern BYTE *sub_io;
										/* �T�uCPU I/O      $0100 */

extern BYTE *extram_a;
										/* RAM (FM77AV)    $10000 */
extern BYTE *boot_ram;
										/* BOOT (RAM)        $200 */
extern BYTE *init_rom;
										/* �C�j�V�G�[�gROM  $2000 */

extern BYTE *subrom_a;
										/* ROM (�^�C�vA)    $2000 */
extern BYTE *subrom_b;
										/* ROM (�^�C�vB)    $2000 */
extern BYTE *subromcg;
										/* ROM (CG)         $2000 */
extern BYTE *vram_b;
										/* VRAM (�^�C�vA,B) $C000 */

extern BOOL boot_mode;
										/* �N�����[�h */
extern BOOL basicrom_en;
										/* F-BASIC 3.0 ROM �C�l�[�u�� */
extern BOOL initrom_en;
										/* �C�j�V�G�[�^ROM �C�l�[�u�� */
extern BOOL bootram_rw;
										/* �u�[�gRAM �������݉\ */
extern BYTE subrom_bank;
										/* �T�u�V�X�e��ROM�o���N */
extern BYTE cgrom_bank;
										/* CGROM�o���N */

#ifdef __cplusplus
}
#endif

#endif	/* _xm7_h_ */
