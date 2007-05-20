

bool isMMX() {

	//CPUID命令を使用できるCPUかどうか
	//::OutputDebugString("CPUID命令の使用可否を判定します。\n");
	unsigned long	flg_cpuid=0, tmp=0;
	_asm {
		pushfd
		pop	eax
		mov	tmp, eax
		xor	eax, 00200000h
		push	eax
		popfd
		pushfd
		pop	eax
		mov	flg_cpuid, eax
	}
	if( flg_cpuid != tmp ) {
		//::OutputDebugString("CPUID命令を実行します。\n");
		unsigned long	cpuid_result = 0;
		_asm {
			mov EAX, 1
			_emit	0fh			;CPUID命令<1999.04.24修正>
			_emit	0a2h		;	〃
			mov cpuid_result, EDX
		}
		if( cpuid_result & 0x00800000 ) {
			//::OutputDebugString("MMX命令セットは使用可能です。\n");
			return true;
		}
	}
	//::OutputDebugString("MMX命令セットは使用できません。\n");
	return false;
}

bool	gMMX = isMMX();
//bool	gMMX = false;


void	memcpy8(void* iDst, const void* iSrc, size_t iLen) {
	for (size_t i=0 ; i<iLen ; i++)
		((unsigned char*)iDst)[i]=((unsigned char*)iSrc)[i];
}

void	memcpy32(void* iDst, const void* iSrc, size_t iLen) {
	if (iLen==0)
		return;
	int	iTail = iLen%4;
	int	iBody = iLen-iTail;
	for (size_t i=0 ; i<iLen/4 ; i++)
		((unsigned int*)iDst)[i]=((unsigned int*)iSrc)[i];
	for (i=iBody ; i<iLen ; i++)
		((unsigned char*)iDst)[i]=((unsigned char*)iSrc)[i];
}

void	memcpyMMX(void* iDst, const void* iSrc, size_t iLen) {
	if (iLen==0)
		return;
	const int	iTail = iLen%8;
	const int	iBody = iLen-iTail;
	iLen /= 8;

	_asm {
		mov		ecx, iLen
		mov		eax, iDst
		mov		ebx, iSrc
	looptop:
		movq	mm0, [ebx]
		movq	[eax], mm0
		add		eax,8		// 8byte = 64bits
		add		ebx,8
		loop	looptop
	}
	_asm	emms

	if (iTail>0)
		memcpy8((char*)iDst+iBody, (char*)iSrc+iBody, iTail);
}

void	memcpyx(void* iDst, const void* iSrc, size_t iLen) {
	if ( iLen < 8 )
		memcpy8(iDst, iSrc, iLen);
	else if ( gMMX )
		memcpyMMX(iDst, iSrc, iLen);
	else
		memcpy32(iDst, iSrc, iLen);
}


