/* Copyright (C) 2015 Evan Christensen
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

#pragma once

#include "EwcArray.h"
#include "EwcHash.h"

namespace llvm
{
	class BasicBlock;
	class Constant;
	class Function;
	class Module;
	class Type;
	class Value;
}

class CIRInstruction;
class CSTNode;
class LlvmIRBuilder; // wrapper class to avoid the forward decl template mess.
struct SSymbol;



class CIRBasicBlock		// tag = block
{
public:
						CIRBasicBlock(EWC::CAlloc * pAlloc);
						~CIRBasicBlock();

	//void				Insert(CIRInstruction * pInst, int iInst);
	void				Append(CIRInstruction * pInst);

	llvm::BasicBlock *				m_pLblock;
	EWC::CDynAry<CIRInstruction *>	m_arypInst;		// BB - eventually this should be replaced with something that has 
													// better random insertion performance.
};



enum VALK	// VALue Kind
{
	VALK_Constant,
	VALK_Argument,

	VALK_ProcedureDefinition,

	VALK_Instruction,
	/*
	VALK_TerminatorInst,
	VALK_BinaryOpInst,
	VALK_UnaryOpInst,
	VALK_CastInst,
	VALK_CompareInst,
	VALK_ReturnInst,
	VALK_InvokeInst,

	VALK_EndInst,
	VALK_BeginInst = VALK_TerminatorInst,
	*/

	VALK_Max,
	VALK_Min = 0,
	VALK_Nil = -1,
};

EWC_ENUM_UTILS(VALK);



#define OPCODE_LIST \
		OP(Ret), \
		OP_RANGE(TerminalOp, Ret), \
		\
		OP(Call), \
		OP_RANGE(JumpOp, TerminalOpMax), \
		\
		OP(SAdd), \
		OP(UAdd), \
		OP(GAdd), \
		OP(SSub), \
		OP(USub), \
		OP(GSub), \
		OP(SMul), \
		OP(UMul), \
		OP(GMul), \
		OP(SDiv), \
		OP(UDiv), \
		OP(GDiv), \
		OP_RANGE(BinaryOp, JumpOpMax), \
		\
		OP(Shl), \
		OP(Shr), \
		OP(And), \
		OP(Or), \
		OP(Xor), \
		OP_RANGE(LogicOp, BinaryOpMax), \
		\
		OP(Alloca), \
		OP(Load), \
		OP(Store), \
		OP_RANGE(MemoryOp, LogicOpMax), \
		\
		OP(Trunc), \
		OP(GToS64), \
		OP(GToU64), \
		OP(S64ToG), \
		OP(U64ToG), \
		OP(GTrunc), \
		OP(GExtend), \
		OP(PtrToInt), \
		OP(IntToPtr), \
		OP_RANGE(CastOp, MemoryOpMax), \

#define OP(x) IROP_##x
#define OP_RANGE(range, prev) IROP_##range##Max, IROP_##range##Min = IROP_##prev
	enum IROP
	{
		OPCODE_LIST

		IROP_Max,
		IROP_Min = 0,
		IROP_Nil = -1,
	};
#undef OP
#undef OP_RANGE



class CIRValue		// tag = val
{
public:
						CIRValue(VALK valk);
	virtual				~CIRValue()
							{ ; }

	llvm::Value *		m_pLval;
	CSTNode *			m_pStnod;
	VALK				m_valk;
};



class CIRConstant : public CIRValue // tag = const
{
public:
						CIRConstant()
						:CIRValue(VALK_Constant)
							{ ; }
};



class CIRArgument : public CIRValue // tag = arg
{
public:
						CIRArgument()
						:CIRValue(VALK_Argument)
							{ ; }
};



class CIRInstruction : public CIRValue	// tag = inst
{
public:
						CIRInstruction(IROP irop)
						:CIRValue(VALK_Instruction)
						,m_cpValOperand(0)
						,m_irop(irop)
							{ ; }

	s8					CpValOperandMax();

	s8					m_cpValOperand;		// current opcode count
	IROP				m_irop;

	// NOTE - opcode array needs to be at the end of the struct, we allocate room for extra elements if needed.
	CIRValue *			m_apValOperand[1];	
};


struct SInsertPoint		// tag = inspt
{
						SInsertPoint()
						:m_pBlock(nullptr)
						,m_iInst(0)
							{ ; }

	CIRBasicBlock *		m_pBlock;
	s32					m_iInst;
};

class CIRProcedure	: public CIRValue // tag = proc;
{
public:
						CIRProcedure(EWC::CAlloc * pAlloc)
						:CIRValue(VALK_ProcedureDefinition)
						,m_pAlloc(pAlloc)
						,m_pLfunc(nullptr)
						,m_pBlockEntry(nullptr)
						,m_arypValManaged(pAlloc)
							{ ; }

						~CIRProcedure();

	EWC::CAlloc *		m_pAlloc;
	llvm::Function *	m_pLfunc;		// null if anonymous function
	CIRBasicBlock *		m_pBlockEntry;	

	EWC::CDynAry<CIRValue *>
						m_arypValManaged;
};

class CIRBuilder		// tag = build
{
public:
						CIRBuilder(EWC::CAlloc * pAlloc);
						~CIRBuilder();
	
	void				PrintDump();
	size_t				CChGenerateUniqueName(const char * pChzIn, char * pChzOut, size_t cChMax);

	CIRBasicBlock *		PBlockCreate(CIRProcedure * pProc, const char * pChzName);

	void				ActivateProcedure(CIRProcedure * pProc);
	void				ActivateBlock(CIRBasicBlock * pBlock);
	void				AddManagedVal(CIRValue * pVal);

	CIRInstruction *	PInstCreateNAdd(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName, bool fSigned);
	CIRInstruction *	PInstCreateGAdd(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName);
	CIRInstruction *	PInstCreateNSub(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName, bool fSigned);
	CIRInstruction *	PInstCreateGSub(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName);
	CIRInstruction *	PInstCreateNMul(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName, bool fSigned);
	CIRInstruction *	PInstCreateGMul(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName);
	CIRInstruction *	PInstCreateNDiv(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName, bool fSigned);
	CIRInstruction *	PInstCreateGDiv(CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName);

	CIRInstruction *	PInstCreateRet(CIRValue * pValRhs);
	CIRInstruction *	PInstCreateAlloca(llvm::Type * pLtype, const char * pChzName);

	CIRInstruction *	PInstCreate(IROP irop, CIRValue * pValLhs, CIRValue * pValRhs, const char * pChzName);

	CIRInstruction *	PInstLoadSymbol(SSymbol * pSym, const char * pChzName);
	CIRInstruction *	PInstCreateStore(CIRValue * pValDst, CIRValue * pValSrc);

	llvm::Module *			m_pLmoduleCur;
	LlvmIRBuilder *			m_pLbuild;
	EWC::CAlloc *			m_pAlloc;
	SInsertPoint			m_inspt;

	CIRProcedure *			m_pProcCur;
	CIRBasicBlock *			m_pBlockCur;

	EWC::CHash<HV, u32>		m_hashHvNUnique;	// map for generating unique strings

};

CIRValue * PValGenerate(CIRBuilder * pBuild, CSTNode * pStnod);