
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

#pragma once

#include "CodeGen.h"
#include "EwcArray.h"
#include "EwcHash.h"
#include "EwcString.h"
#include "TypeInfo.h"


typedef struct DCCallVM_ DCCallVM;

namespace BCode
{
	class CVirtualMachine;
	struct SBlock;
	struct SProcedure;

	enum OPK : u8	// tag = Byte Code OPERand Kind
	{
		OPK_Literal,		// literal value stored in the instruction stream
		OPK_LiteralArg,		// literal that needs to have the argument stack frame offset added during FinalizeProc

		OPK_Register,		// stack indexed "register" value (relative to local stack frame)
		OPK_RegisterArg,	// stack indexed "register" value (relative to argument stack frame)

		OPK_GlobalVal,		// global index, lives in the data segment
		OPK_Global,			// global index, lives in the data segment, is expected to be a pointer to the value 
	};

	inline bool FIsLiteral(OPK opk)
		{ return (opk == OPK_Literal) | (opk == OPK_LiteralArg); }
	inline bool FIsRegister(OPK opk)
		{ return (opk == OPK_Register) | (opk == OPK_RegisterArg); }
	inline bool FIsArg(OPK opk)
		{ return (opk == OPK_LiteralArg) | (opk == OPK_RegisterArg); }



	struct SWord	// tag = word
	{
		union
		{
			s8		m_s8;
			s16		m_s16;
			s32		m_s32;
			s64		m_s64;
			u8		m_u8;
			u16		m_u16;
			u32		m_u32;
			u64		m_u64;

			f32		m_f32;
			f64		m_f64;

			void *	m_pV;
		};
	};

	

	// Build time values - baked into the instructions/globals by runtime.
	struct SValue	// tag = val
	{
						SValue(VALK valk)
						:m_valk(valk)
							{ ; }

		VALK			m_valk;
	};

	struct SConstant : public SValue // tag = const
	{
		static const VALK s_valk = VALK_Constant;

						SConstant(VALK valk = VALK_Constant)
						:SValue(VALK_Constant)
							{ ; }

		STypeInfo *		m_pTin;

		OPK				m_opk;
		SLiteralType	m_litty;

		SWord			m_word;
	};

	// BB - should this really be a different struct?
	struct SRegister : public SConstant // tag = reg
	{
		static const VALK s_valk = VALK_BCodeRegister;

						SRegister()
						:SConstant(VALK_BCodeRegister)
							{ ; }
	};

	// Should change BCode::SInstruction to use this rather than cBRegister to make handling 1 bit sign extensions less hacky
	/* enum OPBITS
	{
		OPBITS_1,
		OPBITS_8,
		OPBITS_16,
		OPBITS_32,
		OPBITS_64,
		OPBITS_Nil = -1,
	};*/

	// packed instruction - just the info needed for runtime - not for building bytecode
	struct SInstruction // tag = inst
	{
						SInstruction()
						:m_irop(IROP_Error)
						,m_opkLhs(OPK_Literal)
						,m_opkRhs(OPK_Literal)
						,m_cBRegister(0)
						,m_pred(0)
						,m_iBStackOut(0)
							{ ; }

		IROP			m_irop;
		OPK				m_opkLhs;
		OPK				m_opkRhs;

		u8				m_cBRegister:4;		// operand byte count
		u8				m_pred:4;

		s32				m_iBStackOut;
		SWord			m_wordLhs;
		SWord			m_wordRhs;

	};

	EWC_CASSERT(sizeof(SInstruction) == 24, "runtime instruction packing is wrong");

	// instruction with extra info used during building
	struct SInstructionValue : public SValue // tag = instval
	{
		static const VALK s_valk = VALK_Instruction;

						SInstructionValue()
						:SValue(VALK_Instruction)
						,m_pInst(nullptr)
						,m_pTinOperand(nullptr)
							{ ; }

		bool			FIsError() const
							{ return m_pInst == nullptr || m_pInst->m_irop == IROP_Error; }

		SInstruction *	m_pInst;
		STypeInfo *		m_pTinOperand;
	};




	struct SBranch	// tag = branch
	{
		s32 *		m_pIInstDst;			// opcode referring to branch - finalized to iInstDst
		SBlock *	m_pBlockDest;
	};

	struct SBlock // tag = block
	{
						SBlock()
						:m_iInstFinal(-1)
						,m_pProc(nullptr)
						,m_aryInstval()
						,m_aryInst()
						,m_aryBranch()
							{ ; }

						bool FIsFinalized() const
							{ return m_iInstFinal >= 0; }

		s32									m_iInstFinal;
		SProcedure *						m_pProc;
		EWC::CDynAry<SInstructionValue>		m_aryInstval;
		EWC::CDynAry<SInstruction>			m_aryInst;
		EWC::CDynAry<SBranch>				m_aryBranch;	// outgoing links in control flow graph.
	};

	struct SParameter // tag = param
	{
		s32		m_cB;
		s32		m_cBAlign;
		s32 	m_iBStack;
	};

	// layout data that derived from pTinproc
	struct SProcedureSignature // tag = procsig
	{
								SProcedureSignature(STypeInfoProcedure * pTinproc);

		s32							m_sIBStackVariadic;		// stack location for variadic arguments, -1 if normal proc
		s32							m_cBArgReturn;
		s64							m_cBArgNamed;			// stack space reserved for arguments (does not include variadic args)
		STypeInfoProcedure *		m_pTinproc;

		SParameter *				m_aParamArg;
		SParameter *				m_aParamRet;
	};

	struct SProcedure : public SValue // tag = proc
	{
		static const VALK s_valk = VALK_Procedure;

									SProcedure(EWC::CAlloc * pAlloc);

		SProcedureSignature *				m_pProcsig;

		s64									m_cBStack;		// allocated bytes on stack (iff fIsForeign == false)

		SBlock *							m_pBlockLocals;
		SBlock *							m_pBlockFirst;
		EWC::CDynAry<SBlock *>				m_arypBlock;	// blocks that have written to this procedure 

		EWC::CDynAry<SInstruction>			m_aryInst;
	};

	struct SJumpTargets // tag = jumpt
	{
						SJumpTargets()
						:m_pBlockBreak(nullptr)
						,m_pBlockContinue(nullptr)
							{ ; }

		SBlock * m_pBlockBreak;
		SBlock * m_pBlockContinue;
		EWC::CString	m_strLabel;
	};



	struct SDataBlock // tag = datab
	{
	public:
		size_t			m_iBStart;
		size_t			m_cB;
		size_t			m_cBMax;
		SDataBlock *	m_pDatabNext;
		u8 *			m_pB;
	};

	class CDataSegment // tag = dataseg
	{
	public:
						CDataSegment(EWC::CAlloc * pAlloc)
						:m_pAlloc(pAlloc)
						,m_aryIBPointer(pAlloc, EWC::BK_ByteCode, 128)
						,m_pDatabFirst(nullptr)
						,m_pDatabCur(nullptr)
						,m_cBBlockMin(64 * 1024)
							{ ; }

						~CDataSegment()
						{
							Clear();
						}

		void			Clear();
		size_t			CB();
		u8 *			PBFromIndex(s32 iB);
		u8 *			PBFromGlobal(SConstant * pConstGlob);

		u8 *			PBBakeCopy(EWC::CAlloc * pAlloc, SDataLayout * pDlay);
		void			AllocateDataBlock(size_t cBMin);
		void			AllocateData(size_t cB, size_t cBAlign, u8 ** ppB, s64 * piB, const char * pChzLabel);

		void			AddRelocatedPointer(SDataLayout * pDlay, s32 iBPointer, s32 iBTarget);
		void			DeepCopy(SDataLayout * pDlay, STypeInfo * pTin, s32 iBDst, s32 iBSrc);
		void			AddDeepCopyPointers(SDataLayout * pDlay, STypeInfo * pTin, s32 iBDst, s32 iBSrc);

		EWC::CAlloc *		m_pAlloc;
		EWC::CDynAry<s64>	m_aryIBPointer;		// location of indices that need to be turned into pointers during PBBakeCopy
		SDataBlock *		m_pDatabFirst;
		SDataBlock *		m_pDatabCur;
		size_t				m_cBBlockMin;

	};


	class CBuilder : public CBuilderBase // tag = bcbuild
	{
	public:
		typedef SBlock Block;
		typedef SConstant Constant;
		typedef SConstant Global;
		typedef SInstructionValue Instruction;
		typedef SProcedure Proc;
		typedef SValue Value;
		typedef SValue LValue;
		typedef STypeInfo LType;
		typedef SValue GepIndex;
		typedef SValue ProcArg;

		struct SCodeGenStruct // tag = cgstruct
		{
									SCodeGenStruct()
									:m_pProcInitMethod(nullptr)
									,m_pLtype(nullptr)
									,m_pGlobInit(nullptr)
										{ ; }

			SProcedure *			m_pProcInitMethod;
			STypeInfoStruct *		m_pLtype;			// type reference, here to avoid infinite recursion in
			SConstant *				m_pGlobInit;		// global instance to use when CGINITK_MemcpyGlobal
		};

							CBuilder(CWorkspace * pWork, SDataLayout * pDlay, EWC::CHash<HV, void*> * pHashHvPFnForeign);
							~CBuilder();

		void				Clear();

		void				PrintDump();
		void				FinalizeBuild(CWorkspace * pWork);

		SProcedure *		PProcCreateImplicit(CWorkspace * pWork, STypeInfoProcedure * pTinproc, CSTNode * pStnod);
		SValue *			PValCreateProc(
								CWorkspace * pWork,
								STypeInfoProcedure * pTinproc,
								const EWC::CString & strMangled,
								CSTNode * pStnod,
								CSTNode * pStnodBody,
								EWC::CDynAry<LType *> * parypLtype,
								LType * pLtypeReturn);
		void				SetupParamBlock(
								CWorkspace * pWork,
								Proc * pProc,
								CSTNode * pStnod,
								CSTNode * pStnodParamList, 
								EWC::CDynAry<LType *> * parypLtype);

		void				ActivateProc(SProcedure * pProc, SBlock * pBlock);
		void				FinalizeProc(SProcedure * pProc);

		SBlock *			PBlockCreate(SProcedure * pProc, const char * pChzName = nullptr);
		void				ActivateBlock(SBlock * pBlock);
		void				DeactivateBlock(SBlock * pBlock);

		static LType *		PLtypeFromPTin(STypeInfo * pTin)
								{ return pTin; } 
		LType *				PLtypeVoid();

		Instruction *		PInstCreateNCmp(NPRED npred, SValue * pValLhs, SValue * pValRhs, const char * pChzName = "");
		Instruction *		PInstCreateGCmp(GPRED gpred, SValue * pValLhs, SValue * pValRhs, const char * pChzName = "");

		Instruction *		PInstCreateCall(SValue * pValProc, STypeInfoProcedure * pTinproc, ProcArg ** apLvalArgs, int cpLvalArg);

		void				CreateReturn(SValue ** ppVal, int cpVal, const char * pChzName = "");
		void				CreateBranch(SBlock * pBlock);
		Instruction *		PInstCreateCondBranch(SValue * pValPred, SBlock * pBlockTrue, SBlock * pBlockFalse);
		Instruction *		PInstCreateTraceStore(SValue * pVal, STypeInfo * pTin);

		s32					IBStackAlloc(s64 cB, s64 cBAlign);
		Instruction *		PInstAlloc();

		Instruction *		PInstCreateRaw(IROP irop, s64 cBOperand, SValue * pValLhs, SValue * pValRhs, const char * pChzName = "");
		Instruction *		PInstCreateRaw(IROP irop, SValue * pValLhs, SValue * pValRhs, const char * pChzName = "");
		Instruction *		PInstCreate(IROP irop, SValue * pValLhs, const char * pChzName = nullptr);
		Instruction *		PInstCreate(IROP irop, SValue * pValLhs, SValue * pValRhs, const char * pChzName = nullptr);
		Instruction *		PInstCreateError();

		Instruction *		PInstCreateCast(IROP irop, SValue * pValLhs, STypeInfo * pTinDst, const char * pChzName);
		Instruction *		PInstCreatePtrToInt(SValue * pValOperand, STypeInfoInteger * pTinint, const char * pChzName);
		Instruction *		PInstCreateStore(SValue * pValPT, SValue * pValT);
		Instruction *		PInstCreateStoreToIdx(SValue * pValPT, SValue * pValT, const char * pChzName = "");
		SInstructionValue * PInstCreateStoreToReg(s32 dstIdx, SValue * pValSrc, const char * pChzName = "");
		SValue *			PLvalConstCast(IROP irop, SValue * pValLhs, STypeInfo * pTinDst);
		LValue *			PLvalConstNCmp(NPRED npred, STypeInfo * pTin, LValue * pLvalLhs, LValue * pLvalRhs);
		LValue *			PLvalConstGCmp(GPRED gpred, STypeInfo * pTin, LValue * pLvalLhs, LValue * pLvalRhs);

		SValue *			PValCreateAlloca(LType * pLtype, const char * pChzName = "");
		Instruction *		PInstCreateMemset(SValue * pValLhs, s64 cBSize, s32 cBAlign, u8 bFill);
		Instruction *		PInstCreateMemcpy(u64 cB, SValue * pValLhs, SValue * pValRhsRef);
		Instruction *		PInstCreateMemcpy(STypeInfo * pTin, SValue * pValLhs, SValue * pValRhsRef);


		Instruction *		PInstCreateGEP(SValue * pValLhs, GepIndex ** apLvalIndices, u32 cpIndices, const char * pChzName);
		GepIndex *			PGepIndex(u64 idx);
	 	GepIndex *			PGepIndexFromValue(SValue * pVal);

		Global *			PGlobCreate(STypeInfo * pTin, const char * pChzName);
		u8 *				PBAllocateGlobalWithPointer(size_t cB, size_t cBAlign, u8 ** ppBPointer, s64 * piBPointer, s64 * piBGlobal, const char * pChzLabel);
		void				SetInitializer(SValue * pValGlob, SValue * pValInit);
		void				SetGlobalInitializer(CWorkspace * pWork, SConstant * pGlob, STypeInfo * pTinGlob, STypeInfoLiteral * pTinlit, CSTNode * pStnodInit);
		void				SetGlobalIsConstant(Global * pGlob, bool fIsConstant)
								{ ; }

		SValue *			PValGenerateCall(
								CWorkspace * pWork,
								CSTNode * pStnod,
								SSymbol * pSym,
								EWC::CDynAry<ProcArg *> * parypArgs,
								bool fIsDirectCall,
								STypeInfoProcedure * pTinproc, 
								VALGENK valgenk);

		static ProcArg *	PProcArg(SValue * pVal);

		Instruction *		PInstCreatePhi(LType * pLtype, const char * pChzName);
		void				AddPhiIncoming(SValue * pInstPhi, SValue * pVal, SBlock * pBlock);
		Instruction *		PInstCreateSwitch(SValue * pVal, SBlock * pBlockElse, u32 cSwitchCase);
		void				AddSwitchCase(Instruction * pInstSwitch, SValue * pValOn, SBlock * pBlock, int iInstCase);

		SConstant *			PConstArg(s64 n, int cBit = 64, bool fIsSigned = true);
		SRegister *			PReg(s64 n, int cBit = 64, bool fIsSigned = true);
		SRegister *			PRegArg(s64 n, int cBit = 64, bool fIsSigned = true);
		SRegister *			PRegArg(s64 n, STypeInfo * pTin);

		SConstant *			PConstPointer(void * pV, STypeInfo * pTin = nullptr);
		//SConstant *			PConstRegAddr(s32 iBStack, int cBitRegister);
		SConstant *			PConstInt(u64 nUnsigned, int cBit = 64, bool fIsSigned = true);
		SConstant *			PConstFloat(f64 g, int cBit = 64);

		LValue *			PLvalConstantInt(u64 nUnsigned, int cBit, bool fIsSigned)
								{ return PConstInt(nUnsigned, cBit, fIsSigned); }
		LValue *			PLvalConstantFloat(f64 g, int cBit)
								{ return PConstFloat(g, cBit); }

		LValue *			PLvalConstantGlobalStringPtr(const char * pChzString, const char * pChzName);
		LValue *			PLvalConstantNull(LType * pLtype);
		LValue *			PLvalConstantArray(LType * pLtypeElement, LValue ** apLval, u32 cpLval);
		LValue *			PLvalConstantStruct(LType * pLtype, LValue ** apLval, u32 cpLval);


		Constant *			PConstEnumLiteral(STypeInfoEnum * pTinenum, CSTValue * pStval);
		SValue *			PValFromSymbol(SSymbol * pSym);
		void				SetSymbolValue(SSymbol * pSym, SValue * pVal);

		SCodeGenStruct *	PCgstructEnsure(STypeInfoStruct * pTinstruct);
		SProcedureSignature * 
							PProcsigEnsure(STypeInfoProcedure * pTinproc);

		void				AddManagedVal(SValue * pVal);
		void				SwapToVm(CVirtualMachine * pVm);

		EWC::CAlloc *						m_pAlloc;
		CSymbolTable *						m_pSymtab;
		CIRBuilderErrorContext *			m_pBerrctx;
		SDataLayout *						m_pDlay;
		EWC::CHash<HV, SProcedure *>		m_hashHvMangledPProc;
		EWC::CDynAry<SBlock *>				m_arypBlockManaged;
		EWC::CDynAry<SJumpTargets>			m_aryJumptStack;
		EWC::CDynAry<SValue *>				m_arypValManaged;
		EWC::CDynAry<SProcedure *>			m_arypProcManaged;
		CDataSegment						m_dataseg;
		EWC::CHash<SSymbol *, SValue *>		m_hashPSymPVal;
		EWC::CHash<STypeInfoLiteral *, SConstant *>					m_hashPTinlitPGlob;
		EWC::CHash<STypeInfoStruct *, SCodeGenStruct *>				m_hashPTinstructPCgstruct;
		EWC::CHash<STypeInfoProcedure *, SProcedureSignature *>		m_hashPTinprocPProcsig;
		EWC::CHash<HV, void *> *			m_phashHvPFnForeign;

		EWC::CBlockList<SConstant, 255>		m_blistConst; // constants / registers used during code generation


		SProcedure *						m_pProcCur;
		SBlock *							m_pBlockCur;
	};


#define DEBUG_PROC_CALL 1
#if DEBUG_PROC_CALL
	struct SDebugCall // tag = debcall
	{
		STypeInfoProcedure *	m_pTinproc;
		SInstruction **			m_ppInstCall;
		u8 *					m_pBReturnStorage;
		u8 *					m_pBStackSrc; // calling stack frame
		u8 *					m_pBStackArg;
		u8 *					m_pBStackDst;
	};
#endif

	// Do-nothing struct used as a proxy for LLVM debug info values
	struct SStub
	{
		int m_pad;
	};



	class CVirtualMachine	// tag = vm
	{
	public:
						CVirtualMachine(u8 * pBStack, u8 * pBStackMax, CBuilder * pBuild);
						~CVirtualMachine()
							{ Clear(); }

		void			Clear();


		EWC::CAlloc *	m_pAlloc;
		SDataLayout *	m_pDlay;
		u8 *			m_pBStackMin;
		u8 *			m_pBStackMax;
		u8 *			m_pBStack;			// current stack bottom (grows down)
		u8 *			m_pBGlobal;			// global data segment
		SProcedure *	m_pProcCurDebug;	// current procedure being executed (not available in release)
		DCCallVM *		m_pDcvm;
		EWC::SStringBuffer *
						m_pStrbuf;			
		s32				m_iInstSource;		// instruction index of branch that jumped to this block (for phi nodes)

		EWC::CDynAry<SBlock *>				m_arypBlockManaged;
		EWC::CDynAry<SProcedure *>			m_arypProcManaged;
		EWC::CHash<HV, SProcedure *>		m_hashHvMangledPProc;
		EWC::CHash<STypeInfoProcedure *, SProcedureSignature *>	
											m_hashPTinprocPProcsig;

#if DEBUG_PROC_CALL
		EWC::CDynAry<SDebugCall> 		m_aryDebCall;
#endif 
	};

	SProcedure * PProcLookup(CVirtualMachine * pVm, HV hv);

	bool LoadForeignLibraries(CWorkspace * pWork, EWC::CHash<HV, void*> * pHashHvPFn, EWC::CDynAry<void *> * parypDll);
	void UnloadForeignLibraries(EWC::CDynAry<void *> * paryDll);

	void ExecuteBytecode(CVirtualMachine * pVm, SProcedure * pProc);
	void BuildTestByteCode(CWorkspace * pWork, EWC::CAlloc * pAlloc);

} // namespace BCode

void BuildStubDataLayout(SDataLayout * pDlay);
void CalculateByteSizeAndAlign(SDataLayout * pDlay, STypeInfo * pTin, u64 * pcB, u64 * pcBAlign);
