/* Copyright (C) 2018 Evan Christensen
|
| Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
| documentation files (the "Software"), to deal in the Software without restriction, including without limitation the 
| rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
| persons to whom the Software is furnished to do so, subject to the following conditions:
| 
| The above copyright notice and this permission notice shall be included in all copies or substantial portions of the 
| Software.
| 
| THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
| WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
| COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
| OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "ByteCode.h"
#include "stdio.h"

using namespace EWC;

namespace BCode 
{

u32 CBFromOpsz(OPSZ opsz)
{
	static const u32 s_mpOpszCB [] =
	{
		1,	// OPSZ_8,
		2,	// OPSZ_16,
		4,	// OPSZ_32,
		8,	// OPSZ_64,
	};
	EWC_CASSERT(EWC_DIM(s_mpOpszCB) == OPSZ_Max, "missing size");

	if (!EWC_FVERIFY(opsz < OPSZ_Max))
		return 1;
	return s_mpOpszCB[opsz];
}

SProcedure::SProcedure(EWC::CString strName, EWC::CString strMangled)
:m_strName(strName)
,m_strMangled(strMangled)
,m_cBStack(0)
,m_pBlock(nullptr)
{
}



CBuilder::CBuilder(EWC::CAlloc * pAlloc)
:m_pAlloc(pAlloc)
,m_arypProcManaged(pAlloc, BK_ByteCodeCreator, 256)
,m_pProcCur(nullptr)
,m_pBInstMin(nullptr)
,m_pBInstCur(nullptr)
,m_pBInstMax(nullptr)
{
	static const int s_cBInst = 4 * 1024;
	m_pBInstMin = (u8 *)pAlloc->EWC_ALLOC(s_cBInst, 4);
	m_pBInstCur = m_pBInstMin;
	m_pBInstMax = m_pBInstMin + s_cBInst;
}

SProcedure * CBuilder::PProcCreate(const char * pChzName, const char * pChzMangled)
{
	auto pProc = EWC_NEW(m_pAlloc, SProcedure) SProcedure(pChzName, pChzMangled);
	m_arypProcManaged.Append(pProc);

	return pProc;
}

void CBuilder::BeginProc(SProcedure * pProc)
{
	EWC_ASSERT(!m_pProcCur, "cannot begin procedure %s; previous procedure %s was not ended", 
		m_pProcCur->m_strName.PCoz(), pProc->m_strName.PCoz());
	m_pProcCur = pProc;
}

void CBuilder::EndProc(SProcedure * pProc)
{
	EWC_ASSERT(m_pProcCur == pProc, "mismatch ending procedure %s", pProc->m_strName.PCoz());
	m_pProcCur = nullptr;
}

SBlock * CBuilder::PBlockBegin()
{
	return nullptr;
}

void CBuilder::EndBlock(SBlock * pBlock)
{
}

SRecord CBuilder::RecAddInstOld(OP op, OPSZ opsz, const SRecord & recLhs)
{
	u32 cB = CBFromOpsz(opsz);
	u32 iBStackOut = IBStackAlloc(cB, cB);

	SInstructionOld inst;
	inst.m_op = op;
	inst.m_opsz = opsz;
	inst.m_opkLhs = recLhs.m_opk;
	inst.m_opkRhs = OPK_Nil;
	inst.m_opkOut = OPK_Stack;
	PackInst(&inst, sizeof(SInstructionOld));

	#define MASHTYPE(OPK, OPSZ)				(u32)(OPK | (OPSZ << 16))
	switch (MASHTYPE(recLhs.m_opk, opsz))
	{
		case MASHTYPE(OPK_Literal, OPSZ_8):		PackInst(&recLhs.m_word.m_u8, 1);	break;
		case MASHTYPE(OPK_Literal, OPSZ_16):	PackInst(&recLhs.m_word.m_u16, 2);	break;
		case MASHTYPE(OPK_Literal, OPSZ_32):	PackInst(&recLhs.m_word.m_u32, 4);	break;
		case MASHTYPE(OPK_Literal, OPSZ_64):	PackInst(&recLhs.m_word.m_u64, 8);	break;

		case MASHTYPE(OPK_Stack, OPSZ_8):
		case MASHTYPE(OPK_Stack, OPSZ_16):
		case MASHTYPE(OPK_Stack, OPSZ_32):
		case MASHTYPE(OPK_Stack, OPSZ_64):		PackInst(&recLhs.m_word.m_u32, 4);	break;
		default:
			EWC_ASSERT(false, "unhandled opcode kind");
	}
	#undef MASHTYPE
	
	PackInst(&iBStackOut, 4);
	return RecStack(iBStackOut);
}

SRecord CBuilder::RecAddInstOld(OP op, OPSZ opsz, const SRecord & recLhs, const SRecord & recRhs)
{
	u32 cB = CBFromOpsz(opsz);
	u32 iBStackOut = IBStackAlloc(cB, cB);

	SInstructionOld inst;
	inst.m_op = op;
	inst.m_opsz = opsz;
	inst.m_opkLhs = recLhs.m_opk;
	inst.m_opkRhs = recRhs.m_opk;
	inst.m_opkOut = OPK_Stack;
	PackInst(&inst, sizeof(SInstructionOld));

	#define MASHTYPE(OPK, OPSZ)				(u32)(OPK | (OPSZ << 16))
	switch (MASHTYPE(recLhs.m_opk, opsz))
	{
		case MASHTYPE(OPK_Literal, OPSZ_8):		PackInst(&recLhs.m_word.m_u8, 1);	break;
		case MASHTYPE(OPK_Literal, OPSZ_16):	PackInst(&recLhs.m_word.m_u16, 2);	break;
		case MASHTYPE(OPK_Literal, OPSZ_32):	PackInst(&recLhs.m_word.m_u32, 4);	break;
		case MASHTYPE(OPK_Literal, OPSZ_64):	PackInst(&recLhs.m_word.m_u64, 8);	break;

		case MASHTYPE(OPK_Stack, OPSZ_8):
		case MASHTYPE(OPK_Stack, OPSZ_16):
		case MASHTYPE(OPK_Stack, OPSZ_32):
		case MASHTYPE(OPK_Stack, OPSZ_64):		PackInst(&recLhs.m_word.m_u32, 4);	break;
		default:
			EWC_ASSERT(false, "unhandled opcode kind");
	}

	switch (MASHTYPE(recRhs.m_opk, opsz))
	{
		case MASHTYPE(OPK_Literal, OPSZ_8):		PackInst(&recRhs.m_word.m_u8, 1);	break;
		case MASHTYPE(OPK_Literal, OPSZ_16):	PackInst(&recRhs.m_word.m_u16, 2);	break;
		case MASHTYPE(OPK_Literal, OPSZ_32):	PackInst(&recRhs.m_word.m_u32, 4);	break;
		case MASHTYPE(OPK_Literal, OPSZ_64):	PackInst(&recRhs.m_word.m_u64, 8);	break;

		case MASHTYPE(OPK_Stack, OPSZ_8):
		case MASHTYPE(OPK_Stack, OPSZ_16):
		case MASHTYPE(OPK_Stack, OPSZ_32):
		case MASHTYPE(OPK_Stack, OPSZ_64):		PackInst(&recRhs.m_word.m_u32, 4);	break;
		default:
			EWC_ASSERT(false, "unhandled opcode kind");
	}
	#undef MASHTYPE
	
	PackInst(&iBStackOut, 4);
	return RecStack(iBStackOut);
}

SRecord CBuilder::RecAddInst(OP op, OPSZ opsz, const SRecord & recLhs)
{
	u32 cB = CBFromOpsz(opsz);

	auto pInst = PInstAlloc();
	pInst->m_op = op;
	pInst->m_opsz = opsz;
	pInst->m_opkLhs = recLhs.m_opk;
	pInst->m_opkRhs = OPK_Nil;

	u32 iBStackOut = IBStackAlloc(cB, cB);
	pInst->m_iBStackOut = iBStackOut;

	pInst->m_wordLhs.m_u64 = recLhs.m_word.m_u64;
	pInst->m_wordRhs.m_u64 = 0;
	
	return RecStack(iBStackOut);
}

SRecord CBuilder::RecAddInst(OP op, OPSZ opsz, const SRecord & recLhs, const SRecord & recRhs)
{
	u32 cB = CBFromOpsz(opsz);

	auto pInst = PInstAlloc();
	pInst->m_op = op;
	pInst->m_opsz = opsz;
	pInst->m_opkLhs = recLhs.m_opk;
	pInst->m_opkRhs = OPK_Nil;

	u32 iBStackOut = IBStackAlloc(cB, cB);
	pInst->m_iBStackOut = iBStackOut;

	pInst->m_wordLhs.m_u64 = recLhs.m_word.m_u64;
	pInst->m_wordRhs.m_u64 = recRhs.m_word.m_u64;
	
	return RecStack(iBStackOut);
}

u32	CBuilder::IBStackAlloc(u32 cB, u32 cBAlign)
{
	if (!EWC_FVERIFY(m_pProcCur, "Allocating from the stack without an active procedure"))
		return 0;
	size_t cBMasked = cBAlign - 1;
	u32 cBStack = U32Coerce((m_pProcCur->m_cBStack + cBMasked) & ~cBMasked);
	m_pProcCur->m_cBStack = cBStack + cB;
	return cBStack;
}

SInstruction * CBuilder::PInstAlloc()
{
	auto pBAlign = (u8 *)EWC::PVAlign(m_pBInstCur, EWC_ALIGN_OF(SInstruction));
	ptrdiff_t cBAlign = pBAlign - m_pBInstCur;

	u32 cBTotal = U32Coerce(sizeof(SInstruction) + cBAlign);
	ptrdiff_t cBRemain = m_pBInstMax - m_pBInstCur;
	if (cBRemain < cBTotal)
		return nullptr;

	auto pB = (u8 *)(m_pBInstCur + cBAlign);
	m_pBInstCur += cBTotal;
	return (SInstruction *)pB;
}

void CBuilder::PackInst(const void * pV, u32 cB)
{
	ptrdiff_t cBRemain = m_pBInstMax - m_pBInstCur;
	if (cBRemain < cB)
		return;

	EWC::CopyAB(pV, m_pBInstCur, cB);
	m_pBInstCur += cB;
}

void UnpackInst(u8 ** ppBInst, void * pVOut, u32 cB)
{
	EWC::CopyAB(*ppBInst, pVOut, cB);
	*ppBInst += cB;
}



template <typename T>
static inline T StackFetch(CVirtualMachine * pVm)
{
	auto ibStack = *(u32 *)pVm->m_pBInst;
	pVm->m_pBInst += 4;
	return *(T *)&pVm->m_pBStack[ibStack];
}

static inline void ReadOpcodes(CVirtualMachine * pVm, u32 * piLhs, u32 * piOut)
{
	u32 * pI = (u32*)(pVm->m_pBInst + sizeof(SInstructionOld));
	*piLhs = *pI++;
	*piOut = *pI++;
	pVm->m_pBInst = (u8 *)pI;
}

static inline void ReadOpcodes(CVirtualMachine * pVm, u32 * piLhs, u32 * piRhs, u32 * piOut)
{
	u32 * pI = (u32*)(pVm->m_pBInst + sizeof(SInstructionOld));
	*piLhs = *pI++;
	*piRhs = *pI++;
	*piOut = *pI++;
	pVm->m_pBInst = (u8 *)pI;
}

static inline void LoadWord(CVirtualMachine * pVm, SWord * pWord, u32 iB, OPSZ opsz)
{
	u8 * pB = &pVm->m_pBStack[iB];
	switch (opsz)
	{
		case OPSZ_8:	pWord->m_u8 = *pB;			break;
		case OPSZ_16:	pWord->m_u16 = *(u16*)pB;	break;
		case OPSZ_32:	pWord->m_u32 = *(u32*)pB;	break;
		case OPSZ_64:	pWord->m_u64 = *(u64*)pB;	break;
	}
}

static inline void ReadOpcodesOld(CVirtualMachine * pVm, SInstructionOld * pInst, SWord * pWordLhs, u32 * piOut)
{
	OPSZ opsz = (OPSZ)pInst->m_opsz;
	u32 cB = CBFromOpsz(opsz);
	if (pInst->m_opkLhs == OPK_Stack)
	{
		u32 iBStack;
		UnpackInst(&pVm->m_pBInst, &iBStack, sizeof(iBStack));
		LoadWord(pVm, pWordLhs, iBStack, opsz);
	}
	else
	{
		UnpackInst(&pVm->m_pBInst, &pWordLhs->m_u64, cB);
	}

	UnpackInst(&pVm->m_pBInst, piOut, 4);
}

static inline void ReadOpcodesOld(CVirtualMachine * pVm, SInstructionOld * pInst, SWord * pWordLhs, SWord * pWordRhs, u32 * piOut)
{
	OPSZ opsz = (OPSZ)pInst->m_opsz;
	u32 cB = CBFromOpsz(opsz);
	if (pInst->m_opkLhs == OPK_Stack)
	{
		u32 iBStack;
		UnpackInst(&pVm->m_pBInst, &iBStack, sizeof(iBStack));
		LoadWord(pVm, pWordLhs, iBStack, opsz);
	}
	else
	{
		UnpackInst(&pVm->m_pBInst, &pWordLhs->m_u64, cB);
	}


	if (pInst->m_opkRhs == OPK_Stack)
	{
		u32 iBStack;
		UnpackInst(&pVm->m_pBInst, &iBStack, sizeof(iBStack));
		LoadWord(pVm, pWordRhs, iBStack, opsz);
	}
	else
	{
		UnpackInst(&pVm->m_pBInst, &pWordRhs->m_u64, cB);
	}

	UnpackInst(&pVm->m_pBInst, piOut, 4);
}

static inline void ReadOpcodes(CVirtualMachine * pVm, SInstruction * pInst, SWord * pWordLhs)
{
	if (pInst->m_opkLhs == OPK_Stack)
	{
		OPSZ opsz = (OPSZ)pInst->m_opsz;
		u32 cB = CBFromOpsz(opsz);
		LoadWord(pVm, pWordLhs, pInst->m_wordLhs.m_u32, opsz);
	}
	else
	{
		*pWordLhs = pInst->m_wordLhs;
	}
}

static inline void ReadOpcodes(CVirtualMachine * pVm, SInstruction * pInst, SWord * pWordLhs, SWord * pWordRhs)
{
	OPSZ opsz = (OPSZ)pInst->m_opsz;
	u32 cB = CBFromOpsz(opsz);

	if (pInst->m_opkLhs == OPK_Stack)
	{
		LoadWord(pVm, pWordLhs, pInst->m_wordLhs.m_u32, opsz);
	}
	else
	{
		*pWordLhs = pInst->m_wordLhs;
	}

	if (pInst->m_opkRhs == OPK_Stack)
	{
		LoadWord(pVm, pWordRhs, pInst->m_wordRhs.m_u32, opsz);
	}
	else
	{
		*pWordRhs = pInst->m_wordRhs;
	}
}


template <typename T>
static inline void StackStore(CVirtualMachine * pVm, T val)
{
	auto ibStack = *(u32 *)pVm->m_pBInst;
	pVm->m_pBInst += 4;
	*(T *)&pVm->m_pBStack[ibStack] = val;
}

void ExecuteBytecodeOld(CVirtualMachine * pVm)
{
	#define MASHOP(OP, OPSZ)					(u32)(OP | (OPSZ << 16))
	#define FETCH(IB, TYPE)						*(TYPE *)&pVm->m_pBStack[IB]
	#define STORE(IBOUT, TYPE, VALUE)			*(TYPE *)&pVm->m_pBStack[IBOUT] = VALUE

	u32 iBStackOut;	
	SWord recLhs, recRhs;

	while (1)
	{
		SInstructionOld inst;
		UnpackInst(&pVm->m_pBInst, &inst, sizeof(SInstructionOld));
		if (inst.m_op == OP_Halt)
			return;

		switch (MASHOP(inst.m_op, inst.m_opsz))
		{
		case MASHOP(OP_NAdd, OPSZ_8):	
			{ 
				ReadOpcodesOld(pVm, &inst, &recLhs, &recRhs, &iBStackOut);
				STORE(iBStackOut, u8, recLhs.m_u8 + recRhs.m_u8);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_16):	
			{ 
				ReadOpcodesOld(pVm, &inst, &recLhs, &recRhs, &iBStackOut);
				STORE(iBStackOut, u16, recLhs.m_u16 + recRhs.m_u16);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_32):	
			{ 
				ReadOpcodesOld(pVm, &inst, &recLhs, &recRhs, &iBStackOut);
				STORE(iBStackOut, u32, recLhs.m_u32 + recRhs.m_u32);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_64):	
			{ 
				ReadOpcodesOld(pVm, &inst, &recLhs, &recRhs, &iBStackOut);
				STORE(iBStackOut, u64, recLhs.m_u64 + recRhs.m_u64);
			}	break;

			break;

		case MASHOP(OP_NTrace, OPSZ_8):	
			ReadOpcodesOld(pVm, &inst, &recLhs, &iBStackOut); printf("1byte %d\n", recLhs.m_u8); break;
		case MASHOP(OP_NTrace, OPSZ_16):	
			ReadOpcodesOld(pVm, &inst, &recLhs, &iBStackOut); printf("2byte %d\n", recLhs.m_u16); break;
		case MASHOP(OP_NTrace, OPSZ_32):	
			ReadOpcodesOld(pVm, &inst, &recLhs, &iBStackOut); printf("4byte %d\n", recLhs.m_u32); break;
		case MASHOP(OP_NTrace, OPSZ_64):	
			ReadOpcodesOld(pVm, &inst, &recLhs, &iBStackOut); printf("8byte %lld\n", recLhs.m_u64); break;
		}
	}

	#undef MASHOP
	#undef FETCH
	#undef MASHOP
}

void ExecuteBytecode(CVirtualMachine * pVm)
{
	#define MASHOP(OP, OPSZ)					(u32)(OP | (OPSZ << 16))
	#define FETCH(IB, TYPE)						*(TYPE *)&pVm->m_pBStack[IB]
	#define STORE(IBOUT, TYPE, VALUE)			*(TYPE *)&pVm->m_pBStack[IBOUT] = VALUE

	SWord recLhs, recRhs;

	SInstruction * pInst = (SInstruction *)pVm->m_pBInst;
	while (1)
	{
		if (pInst->m_op == OP_Halt)
			return;

		switch (MASHOP(pInst->m_op, pInst->m_opsz))
		{
		case MASHOP(OP_NAdd, OPSZ_8):	
			{ 
				ReadOpcodes(pVm, pInst, &recLhs, &recRhs);
				STORE(pInst->m_iBStackOut, u8, recLhs.m_u8 + recRhs.m_u8);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_16):	
			{ 
				ReadOpcodes(pVm, pInst, &recLhs, &recRhs);
				STORE(pInst->m_iBStackOut, u16, recLhs.m_u16 + recRhs.m_u16);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_32):	
			{ 
				ReadOpcodes(pVm, pInst, &recLhs, &recRhs);
				STORE(pInst->m_iBStackOut, u32, recLhs.m_u32 + recRhs.m_u32);
			}	break;
		case MASHOP(OP_NAdd, OPSZ_64):	
			{ 
				ReadOpcodes(pVm, pInst, &recLhs, &recRhs);
				STORE(pInst->m_iBStackOut, u64, recLhs.m_u64 + recRhs.m_u64);
			}	break;

			break;

		case MASHOP(OP_NTrace, OPSZ_8):	
			ReadOpcodes(pVm, pInst, &recLhs); printf("1byte %d\n", recLhs.m_u8); break;
		case MASHOP(OP_NTrace, OPSZ_16):	
			ReadOpcodes(pVm, pInst, &recLhs); printf("2byte %d\n", recLhs.m_u16); break;
		case MASHOP(OP_NTrace, OPSZ_32):	
			ReadOpcodes(pVm, pInst, &recLhs); printf("4byte %d\n", recLhs.m_u32); break;
		case MASHOP(OP_NTrace, OPSZ_64):	
			ReadOpcodes(pVm, pInst, &recLhs); printf("8byte %lld\n", recLhs.m_u64); break;
		}
		++pInst;
	}

	#undef MASHOP
	#undef FETCH
	#undef MASHOP
}

CVirtualMachine::CVirtualMachine(u8 * pBInst, u8 * pBStack)
:m_pBInstStart(pBInst)
,m_pBInst(pBInst)
,m_pBStack(pBStack)
{
}

SRecord RecFloat(f64 g)
{
	SRecord rec;
	rec.m_opk	= OPK_Literal;
	rec.m_word.m_f64 = g;
	return rec;
}

SRecord RecSigned(s64 nSigned)
{
	SRecord rec;
	rec.m_opk	= OPK_Literal;
	rec.m_word.m_s64 = nSigned;
	return rec;
}

SRecord RecUnsigned(u64 nUnsigned)
{
	SRecord rec;
	rec.m_opk	= OPK_Literal;
	rec.m_word.m_u64 = nUnsigned;
	return rec;
}

SRecord RecStack(u32 iBStack)
{
	SRecord rec;
	rec.m_opk	= OPK_Stack;
	rec.m_word.m_u32 = iBStack;
	return rec;
}

void BuildTestByteCode(EWC::CAlloc * pAlloc)
{
	CBuilder bcbuild(pAlloc);

	auto pProc = bcbuild.PProcCreate("main", "main");
	
	bcbuild.BeginProc(pProc);
	auto pBlock = bcbuild.PBlockBegin();
	auto recRhs = bcbuild.RecAddInst(OP_NAdd, OPSZ_8, RecSigned(5), RecSigned(3));
	auto recOut = bcbuild.RecAddInst(OP_NAdd, OPSZ_8, recRhs, RecSigned(12));
	(void) bcbuild.RecAddInst(OP_NTrace, OPSZ_8, recOut);
	(void) bcbuild.RecAddInst(OP_Halt, OPSZ_8, RecUnsigned(0));
	bcbuild.EndBlock(pBlock);
	bcbuild.EndProc(pProc);

	u32 cBStackMax = 0;
	for (auto ppProc = bcbuild.m_arypProcManaged.A(); ppProc != bcbuild.m_arypProcManaged.PMac(); ++ppProc)
	{
		cBStackMax = ewcMax(cBStackMax, (*ppProc)->m_cBStack);
	}

	u8 * pBStack = (u8 *)pAlloc->EWC_ALLOC(cBStackMax, 16);
	CVirtualMachine vm(bcbuild.m_pBInstMin, pBStack);

	ExecuteBytecode(&vm);
}

} // namespace BCode 