*
* FM-7 EMULATOR "XM7"
*
* Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
* [ ���[�e�B���e�BIPL ]
*
		ORG	$0100

*
* �v���O�����X�^�[�g
*
* BASIC���[�h��	$0100����u�[�g
* DOS���[�h��	$0300����u�[�g
* �ƂȂ邽�߁A�|�W�V�����C���f�B�y���f���g�ɏ����Ă���
*
START		BRA	MAIN
		NOP

*
* DPB (720KB 2DD)
*
		FCC	/MSDOS5.0/
		FCB	$00,$02,$02,$01,$00
		FCB	$02,$70,$00,$A0,$05
		FCB	$F9
		FCB	$03,$00,$09,$00,$02
		FCB	$00,$00,$00,$00,$00
		FCB	$00,$00,$00,$00
		FCB	$00,$00
		FCB	$00,$00
		FCB	$00,$00,$00
		FCC	/NO NAME    /
		FCC	/FAT12   /

*
* ���C��
*
MAIN		ORCC	#$50
* ������
		LDS	#$0F00
		STA	$FD0F
* �{�̓ǂݍ��݁B$1000����$1000�o�C�g
		LDA	#$FD
		TFR	A,DP
		LDB	#$08
LOOP		LEAX	RCB,PCR
		PSHS	B
		JSR	$FE08
		PULS	B
		TSTA
		BNE	ERROR
* ���̃Z�N�^��
		INC	2,X
		INC	2,X
		INC	5,X
		DECB
		BNE	LOOP

*
* �{�̂փW�����v
*
		JMP	$1000

*
* �ǂݍ��݃G���[(BEEP��炷)
*
ERROR		LDA	#$81
		STA	$FD03
		BRA	*

*
* RCB
*
RCB		FCB	$0A,$00		* DREAD
		FDB	$1000		* �o�b�t�@�A�h���X
		FCB	$00		* �g���b�N
		FCB	$02		* �Z�N�^
		FCB	$00		* �T�C�h
		FCB	$00		* �h���C�u

*
* ���܂�f�[�^
*
OMAKE		FCB	$00,$00,$00,$00
		FCB	$00,$00,$00,$00
		FCB	$00,$00
		FCC	/���̃v���O������XM7�̂��߂�/
		FCC	/���N(1999�N)�ɍ쐬�������̂ł��B/
		FCC	/�����Ƃ��āAMS-DOS���ʌ݊���/
		FCC	/�ȒP�ȃt�@�C���V�X�e���������Ă���/
		FCC	/Win9x�Ō݊����̂��߃T�|�[�g����Ă���A360KB/
		FCC	/���f�B�A�̍쐬�y�уt�@�C����������/
		FCC	/���ł��܂��B�Ȃ�/
		FCC	/�J���ɂ�MS-DOS��ŃN���X�A�Z���u��/
		FCC	/AS63.EXE���g�p���܂����B�g���₷��/
		FCC	/�A�Z���u����FSW�Œ񋟒��������/
		FCC	/�̕��Ɋ��ӂ��܂��B/
		FCC	/1999.7.31 written by �o�h�D/
		FCC	/(ytanaka@ipc-tokai.or.jp)/

*
* �v���O�����I��
*
		END
