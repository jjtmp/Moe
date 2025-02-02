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
#define TRACK_IINSREQ 1

#include "Error.h"
#include "EwcArray.h"
#include "EwcHash.h"
#include "EwcTypes.h"
#include "EwcString.h"
#include "Lexer.h"
#include "TypeInfo.h"

class CIRValue;
class CParseContext;
class CSTNode;
class CSymbolTable;
class CWorkspace;
struct GRFUNT;
struct SErrorManager;
struct SOpTypes;
struct SSymbol;
struct SSymbolBase;
struct SUniqueNameSet;

namespace EWC
{
	struct SStringEditBuffer;
}

namespace BCode
{
	struct SValue;
}

// type should only indicate storage type - actual type info should come from STypeInfoLiteral
enum STVALK
{
	STVALK_Nil = -1,
	STVALK_Float,
	STVALK_SignedInt,
	STVALK_UnsignedInt,
	STVALK_String,
	STVALK_ReservedWord,
};


// syntax tree value storage
class CSTValue	// tag = stval
{
public:
						CSTValue()
						:m_stvalk(STVALK_Nil)
						,m_str()
						,m_nUnsigned(0)
						,m_rword(RWORD_Nil)
						,m_litkLex(LITK_Nil)
							{ ; }

	STVALK				m_stvalk;
	EWC::CString		m_str;
	union
	{
		f64					m_g;
		u64					m_nUnsigned;
		s64					m_nSigned;
	};

	RWORD				m_rword;		// only valid iff park == PARK_ReservedWord
	LITK				m_litkLex;		// literal type determined by lexing, 
										// BB - should STVALK should be expanded to make this unnecessary?
};

inline void SetFloatValue(CSTValue * pStval, f64 g)
{
	pStval->m_stvalk = STVALK_Float;
	pStval->m_g = g;
}

inline void SetSignedIntValue(CSTValue * pStval, s64 n)
{
	pStval->m_stvalk = STVALK_SignedInt;
	pStval->m_nSigned = n;
}

inline void SetUnsignedIntValue(CSTValue * pStval, u64 n)
{
	pStval->m_stvalk = STVALK_UnsignedInt;
	pStval->m_nUnsigned = n;
}

inline void SetBoolValue(CSTValue * pStval, bool f)
{
	pStval->m_stvalk = STVALK_UnsignedInt;
	pStval->m_nUnsigned = f;
}



enum PARK : s16 // PARse Kind
{
	PARK_Error,
	PARK_Identifier,
	PARK_ReservedWord,
	PARK_Nop,
	PARK_Literal,
	PARK_AdditiveOp,
	PARK_MultiplicativeOp,
	PARK_ShiftOp,
	PARK_RelationalOp,
	PARK_LogicalAndOrOp,
	PARK_AssignmentOp,
	PARK_UnaryOp,
	PARK_PostfixUnaryOp,	// postfix increment, decrement
	PARK_Uninitializer,

	PARK_Cast,
	PARK_ArrayElement,		// [array, index]
	PARK_MemberLookup,		// [struct, child]
	PARK_ProcedureCall,		// [procedure, arg0, arg1, ...]
	PARK_SpecializedStruct,	// swapped in during typecheck for ProcedureCall nodes that turn out to be instantiated structs SArray(33)

	PARK_List,				// declarations used by structs
	PARK_ParameterList,		// comma separated declarations used by argument lists
	PARK_ExpressionList,	// list of expressions, used by compound literals - doesn't error on rhs only values.
	PARK_GenericTypeSpec,	// list of types to specify a generic procedure/struct instantiation
	PARK_If,
	PARK_Else,

	PARK_ArrayDecl,
	PARK_ReferenceDecl,		// used in type specification, not used for the unary address-of operator
	PARK_QualifierDecl,

	PARK_ProcedureReferenceDecl,
	PARK_Decl,
//	PARK_CompoundDecl,		// comma separated declarations - specialized AST node for future tuple return value support.
	PARK_Typedef,
	PARK_ConstantDecl,
	PARK_ProcedureDefinition,
	PARK_EnumDefinition,
	PARK_StructDefinition,
	PARK_EnumConstant,
	PARK_VariadicArg,
	PARK_CompoundLiteral,		// array/struct literal
	PARK_ArgumentLabel,
	PARK_GenericDecl,
	PARK_GenericStructSpec,		// 
	PARK_TypeArgument,			// raw type, specified to a generic instantiation SFoo(:int)
	PARK_BakedValue,
	
	EWC_MAX_MIN_NIL(PARK)
};

const char * PChzFromPark(PARK park);
const char * PChzFromLitk(LITK litk);
EWC::CString StrFromIdentifier(CSTNode * pStnod);
EWC::CString StrIdentifierFromDecl(CSTNode * pStnodDecl);
EWC::CString StrFromTypeInfo(STypeInfo * pTin);
EWC::CString StrFromSTNode(CSTNode * pTin);



enum FSYM		// SYMbol flags
{
	FSYM_None				= 0x0,
	FSYM_IsBuiltIn			= 0x1,
	FSYM_IsType				= 0x2,	// this is a type declaration (if not set this is a named instance)
	FSYM_VisibleWhenNested	= 0x4,	// types, constants and procedures that are visible in a more deeply nested symbol table
									// - ie. not an instance. Nested procedure should be able to call peer procedure, but not
									//   access variable from parent procedure.
	FSYM_InternalUseOnly	= 0x8,	// type should not be available for user code declarations, just internal compiler use (ie _flag)

	FSYM_All				= 0xF,
};
EWC_DEFINE_GRF(GRFSYM, FSYM, u32);

enum FSYMLOOK	// SYMbol LOOKup flags
{
	FSYMLOOK_None			= 0x0,
	FSYMLOOK_Local			= 0x1,
	FSYMLOOK_Ancestors		= 0x2,
	FSYMLOOK_IgnoreOrder	= 0x4,

	FSYMLOOK_All			= 0x7,
	FSYMLOOK_Default		= FSYMLOOK_Local | FSYMLOOK_Ancestors,
};

EWC_DEFINE_GRF(GRFSYMLOOK, FSYMLOOK, u32);


enum SYMDEP		// SYMbol DEPendency 
{
							// NIL = Haven't determined if used
	SYMDEP_Unused,			// Symbol is not referenced by any live code - no need to codeGen
	SYMDEP_Used,			// Referenced by live code 
	SYMDEP_PublicLinkage,	// public linkage procedures will be considered used during symbol dependency determination

	EWC_MAX_MIN_NIL(SYMDEP)
};

enum SYMK
{
	SYMK_Symbol,
	SYMK_Path,

	EWC_MAX_MIN_NIL(SYMK)
};

struct SSymbolBase // tag = symbase
{
	SYMK					m_symk;
};

struct SSymbol : public SSymbolBase	// tag = sym
{

	GRFSYM					m_grfsym;
	SYMDEP					m_symdep;
	EWC::CString			m_strName;
	CSTNode *				m_pStnodDefinition;

	STypeInfo *				m_pTin;
	SSymbol *				m_pSymPrev;		// list of shadowed symbols in reverse lexical order. 

	EWC::CDynAry<SSymbol *>	m_aryPSymReferencedBy;
	EWC::CDynAry<SSymbol *>	m_aryPSymHasRefTo;			// this symbol has a reference to all of these symbols
};

// symbol path for 'using' aliases

struct SSymbolPath : public SSymbolBase // tag = symp
{
	EWC::CDynAry<SSymbol *>	m_arypSym;					// implicit references followed by actual symbol  
														// foo.m_x is implicitly foo.m_mid.m_base.m_x  [m_mid, m_base, m_x]
};

inline SSymbol * PSymLast(SSymbolBase * pSymbase)
	{

		switch (pSymbase->m_symk)
		{
		case SYMK_Symbol:	return (SSymbol *)pSymbase;
		case SYMK_Path:
			{
				auto pSymp = (SSymbolPath *)pSymbase;
				if (pSymp->m_arypSym.FIsEmpty())
					return nullptr;

				return pSymp->m_arypSym.Last();
			}
		}
		return nullptr;
	}

// node type for mutually exlusive syntax tree nodes 
enum STMAPK // Syntax Tree MAP  Kind
{
	STMAPK_Proc,
	STMAPK_For,
	STMAPK_Decl,
	STMAPK_Enum,
	STMAPK_Struct,

	EWC_MAX_MIN_NIL(STMAPK)
};

struct SSyntaxTreeMap // tag = stmap
{
				SSyntaxTreeMap(STMAPK stmapk)
				:m_stmapk(stmapk)
					{ ; }

	STMAPK		m_stmapk;
};


class CSTDecl : public SSyntaxTreeMap // tag = stdecl
{
public:
	static const STMAPK s_stmapk = STMAPK_Decl;

					CSTDecl()
					:SSyntaxTreeMap(s_stmapk)
					,m_fIsBakedConstant(false)
					,m_fHasUsingPrefix(false)
					,m_iStnodIdentifier(-1)
					,m_iStnodType(-1)
					,m_iStnodInit(-1)
					,m_iStnodChildMin(-1)
					,m_iStnodChildMax(-1)
						{ ; }

	bool			m_fIsBakedConstant;
	bool			m_fHasUsingPrefix;
	int				m_iStnodIdentifier;
	int				m_iStnodType;
	int				m_iStnodInit;
	int				m_iStnodChildMin;
	int				m_iStnodChildMax;
};

bool FIsTrimmedGenericParameter(CSTDecl * pStdecl);



class CSTFor : public SSyntaxTreeMap // tag = stfor
{
public:
	static const STMAPK s_stmapk = STMAPK_For;

					CSTFor()
					:SSyntaxTreeMap(s_stmapk)
					,m_iStnodDecl(-1)
					,m_iStnodIterator(-1)
					,m_iStnodInit(-1)
					,m_iStnodBody(-1)
					,m_iStnodPredicate(-1)
					,m_iStnodIncrement(-1)
						{ ; }

	int				m_iStnodDecl;
	int				m_iStnodIterator;	// lhs iterator if not a decl
	int				m_iStnodInit;		// rhs init if not a decl
	int				m_iStnodBody;
	int				m_iStnodPredicate;
	int				m_iStnodIncrement;
};

enum FSTPROC
{
	FSTPROC_IsForeign			= 0x1,	// This is a reference to a foreign procedure
	FSTPROC_UseUnmangledName	= 0x2,	// Don't mangle the procedure name (no overloading)
	FSTPROC_PublicLinkage		= 0x4,	// Expose to the linker, don't cull as unused symbol

	FSTPROC_None				= 0x0,
	FSTPROC_All					= 0x7,
};
EWC_DEFINE_GRF(GRFSTPROC, FSTPROC, u8);

class CSTProcedure : public SSyntaxTreeMap // tag = stproc
{
public:
	static const STMAPK s_stmapk = STMAPK_Proc;

					CSTProcedure()
					:SSyntaxTreeMap(s_stmapk)
					,m_iStnodProcName(-1)
					,m_iStnodParameterList(-1)
					,m_iStnodReturnType(-1)
					,m_iStnodBody(-1)
					,m_iStnodForeignAlias(-1)
					,m_pStnodParentScope(nullptr)
					,m_pTinproc(nullptr)
					,m_grfstproc(FSTPROC_None)
						{ ; }

	int						m_iStnodProcName;
	int						m_iStnodParameterList;
	int						m_iStnodReturnType;
	int						m_iStnodBody;
	int						m_iStnodForeignAlias;
	CSTNode *				m_pStnodParentScope;	// procedure this proc is nested inside (if there is one)
	STypeInfoProcedure *	m_pTinproc;
	GRFSTPROC				m_grfstproc;
};



enum ENUMIMP	// implicit enum members (added as STNodes during parse)
{
	ENUMIMP_NilConstant,	// (enum) -1 or max unsigned (not included in min)
	ENUMIMP_MinConstant,	// (enum) lowest user value
	ENUMIMP_LastConstant,	// (enum) highest user value 
	ENUMIMP_MaxConstant,	// (enum) one past the highest value
	ENUMIMP_None,			// (flagEnum) zero
	ENUMIMP_All,			// (flagEnum) all defined bits in bitmask
	ENUMIMP_Names,
	ENUMIMP_Values,

	ENUMIMP_Max,
	ENUMIMP_Min = 0,
	ENUMIMP_Nil = -1,
};

const char * PChzFromEnumimp(ENUMIMP enumimp);
bool FNeedsImplicitMember(ENUMIMP enumimp, ENUMK enumk);

class CSTEnum : public SSyntaxTreeMap // tag = stenum
{
public:
	static const STMAPK s_stmapk = STMAPK_Enum;

					CSTEnum()
					:SSyntaxTreeMap(s_stmapk)
					,m_enumk(ENUMK_Basic)
					,m_cConstantExplicit(0)
					,m_cConstantImplicit(0)		// just the implicit constants, doesn't include name/value arrays
					,m_iStnodIdentifier(-1)
					,m_iStnodType(-1)
					,m_iStnodConstantList(-1)
						{ 
							for (int enumimp = 0; enumimp < EWC_DIM(m_mpEnumimpIstnod); ++enumimp)
							{
								m_mpEnumimpIstnod[enumimp] = -1;
							}
						}

	ENUMK			m_enumk;	
	int				m_cConstantExplicit;
	int				m_cConstantImplicit;
	int				m_iStnodIdentifier;
	int				m_iStnodType;
	int				m_iStnodConstantList;
	int				m_mpEnumimpIstnod[ENUMIMP_Max];
};

class CSTStruct : public SSyntaxTreeMap // tag = ststruct
{
public:
	static const STMAPK s_stmapk = STMAPK_Struct;

					CSTStruct()
					:SSyntaxTreeMap(s_stmapk)
					,m_iStnodIdentifier(-1)
					,m_iStnodParameterList(-1)
					,m_iStnodBakedParameterList(-1)
					,m_iStnodDeclList(-1)
						{ ; }

	int				m_iStnodIdentifier;
	int				m_iStnodParameterList;
	int				m_iStnodBakedParameterList;
	int				m_iStnodDeclList;
};



template <typename T>
T PStmapRtiCast(SSyntaxTreeMap * pStmap)
{
	if (pStmap && pStmap->m_stmapk == EWC::SStripPointer<T>::Type::s_stmapk)
		return (T)pStmap;
	return nullptr;
}

template <typename T>
T PStmapDerivedCast(SSyntaxTreeMap * pStmap)
{
EWC_ASSERT(pStmap && pStmap->m_stmapk == EWC::SStripPointer<T>::Type::s_stmapk, "illegal type info derived cast");
	return (T)pStmap;
}

inline SSyntaxTreeMap * PStmapCopy(EWC::CAlloc * pAlloc, SSyntaxTreeMap * pStmapSrc)
{
#define ALLOC_COPY_AND_RETURN(TYPE, PALLOC, SRC)	{auto pStmapDst = EWC_NEW(PALLOC, TYPE) TYPE(); \
													*pStmapDst = *(TYPE *)SRC; \
													return pStmapDst; }

	switch (pStmapSrc->m_stmapk)
	{
		case STMAPK_Proc:	ALLOC_COPY_AND_RETURN(CSTProcedure, pAlloc, pStmapSrc);
		case STMAPK_For:	ALLOC_COPY_AND_RETURN(CSTFor, pAlloc, pStmapSrc);
		case STMAPK_Decl:	ALLOC_COPY_AND_RETURN(CSTDecl, pAlloc, pStmapSrc);
		case STMAPK_Enum:	ALLOC_COPY_AND_RETURN(CSTEnum, pAlloc, pStmapSrc);
		case STMAPK_Struct:	ALLOC_COPY_AND_RETURN(CSTStruct, pAlloc, pStmapSrc);
		default: break;
	}
#undef ALLOC_COPY_AND_RETURN

	EWC_ASSERT(false, "missing STMAPK");
	return nullptr;
}

// Syntax tree string values - used for identifiers, labels and reserved words
class CSTIdentifier // tag = stident
{
public:
						CSTIdentifier()
						:m_str()
							{ ; }

	EWC::CString		m_str;
};

enum STREES : s8
{
	STREES_Parsed,
	STREES_SignatureTypeChecked,	// function's signature has been type checked, but not it's body. it's enough to type check
						//  proc calls, BB - Is this really necessary? should we just mark TypeChecked before we're done?
	STREES_TypeChecked,

	EWC_MAX_MIN_NIL(STREES)
};

enum FSTNOD
{
	FSTNOD_EntryPoint	= 0x1,		// this should be inserted as a top level entry point, not in place (local function)
	FSTNOD_ImplicitMember = 0x2,	// this node was created as an implicit member, did not come directly from the source
	FSTNOD_Fallthrough = 0x4,		// this node (should be a case/default statement) falls through - BB, need a better place to store this
	FSTNOD_CommutativeCall = 0x8,	// this function is an overloaded operator with arguments reversed.
	FSTNOD_NoCodeGeneration = 0x10, // skip this node for codegen - used by generic definitions
	FSTNOD_AssertOnDelete = 0x20,	// debugging tool, assert when deleted

	FSTNOD_None			= 0x0,
	FSTNOD_All			= 0x3F,
};
EWC_DEFINE_GRF(GRFSTNOD, FSTNOD, u8);

enum FDBGSTR // DeBuG STRing Flags
{
	FDBGSTR_Name				= 0x1,
	FDBGSTR_Type				= 0x2,
	FDBGSTR_LiteralSize			= 0x4,
	FDBGSTR_UseSizedNumerics	= 0x8, // resolve type aliasing for simple integers - should this be all type aliasing?
	FDBGSTR_NoWhitespace		= 0x10,
	FDBGSTR_Values				= 0x20,
	FDBGSTR_ShowStructArgs		= 0x40,

	FDBGSTR_None				= 0x0,
	FDBGSTR_All					= 0x3F,
};
EWC_DEFINE_GRF(GRFDBGSTR, FDBGSTR, u32);

class CSTNode // tag = stnod
{
public:
						CSTNode(EWC::CAlloc * pAlloc, const SLexerLocation & lexloc);
						~CSTNode();

	int					IAppendChild(CSTNode * pStnodChild)
							{ 
								if (pStnodChild)
								{
									EWC_ASSERT(
										!pStnodChild->m_grfstnod.FIsAnySet(FSTNOD_EntryPoint), 
										"Node marked as EntryPoint being added as child");

									m_arypStnodChild.Append(pStnodChild); 
									return (int)m_arypStnodChild.C() - 1;
								}
								return -1;
							}

	void				ReplaceChild(CSTNode * pStnodOld, CSTNode * pStnodNew);

	void				WriteDebugString(EWC::SStringBuffer * pStrbuf, GRFDBGSTR = FDBGSTR_Name);

	int					CStnodChild() const
							{ return (int)m_arypStnodChild.C(); }
	CSTNode *			PStnodChild(int iStnod)
							{ return m_arypStnodChild[iStnod]; }
	CSTNode *			PStnodChildSafe(int iStnod)
							{ return ((iStnod >= 0) & (iStnod < (int)m_arypStnodChild.C())) ? m_arypStnodChild[iStnod] : nullptr; }

	SSyntaxTreeMap * 	PStmapEnsure(EWC::CAlloc * pAlloc, STMAPK stmapk)
							{
								if (m_pStmap)
								{
									EWC_ASSERT(m_pStmap->m_stmapk == stmapk, "CSTNode stmap collision %d and %d", stmapk, m_pStmap->m_stmapk);
									return m_pStmap;
								}

								switch (stmapk)
								{
									case STMAPK_Proc:	m_pStmap = EWC_NEW(pAlloc, CSTProcedure) CSTProcedure();	break;
									case STMAPK_For:	m_pStmap = EWC_NEW(pAlloc, CSTFor) CSTFor();				break;
									case STMAPK_Decl:	m_pStmap = EWC_NEW(pAlloc, CSTDecl) CSTDecl();				break;
									case STMAPK_Enum:	m_pStmap = EWC_NEW(pAlloc, CSTEnum) CSTEnum();				break;
									case STMAPK_Struct:	m_pStmap = EWC_NEW(pAlloc, CSTStruct) CSTStruct();			break;
									default: 
										EWC_ASSERT(false, "missing STMAPK");
								}
								EWC_ASSERT(m_pStmap->m_stmapk == stmapk, "stmapk error");
								return m_pStmap;
							}
	SSymbol *			PSym() const
							{
								
								if (m_pSymbase)
								{
									return PSymLast(m_pSymbase);
								}
								return nullptr;
							}

	SSymbolPath *			PSymPath() const
							{
								if (m_pSymbase && m_pSymbase->m_symk == SYMK_Path)
									return (SSymbolPath *)m_pSymbase;
								return nullptr;
							}

	template <typename T>
	T *					PStmapEnsure(EWC::CAlloc * pAlloc)
							{ return (T *)PStmapEnsure(pAlloc, T::s_stmapk); }

	TOK						m_tok;
	PARK					m_park;
	STREES					m_strees;
	GRFSTNOD				m_grfstnod;

	CSTValue *				m_pStval;
	CSTIdentifier *			m_pStident;
	SSyntaxTreeMap *		m_pStmap;
	SLexerLocation			m_lexloc;
	STypeInfo *				m_pTin;	
	SOpTypes *				m_pOptype;
	CSymbolTable *			m_pSymtab;
	SSymbolBase *			m_pSymbase;

#if TRACK_IINSREQ
	int						m_iInsreq;
#endif
	EWC::CDynAry<CSTNode *>	m_arypStnodChild;
};

CSTValue * PStvalCopy(EWC::CAlloc * pAlloc, CSTValue * pStval);
CSTNode * PStnodCopy(EWC::CAlloc * pAlloc, CSTNode * pStnodSrc, EWC::CHash<CSTNode *, CSTNode *> * pmpPStnodSrcPStnodDst = nullptr);
CSTValue * PStvalExpected(CSTNode * pStnod);

CSTNode ** PPStnodChildFromPark(CSTNode * pStnod, int * pCStnodChild, PARK park);
void PrintStnodName(EWC::SStringBuffer * pStrbuf, CSTNode * pStnod);
void PrintTypeInfo(EWC::SStringBuffer * pStrbuf, STypeInfo * pTin, PARK park, GRFDBGSTR grfdbgstr = FDBGSTR_None);
void PrintLiteral(EWC::SStringBuffer * pStrbuf, CSTNode * pStnodList);
void WriteDebugStringForEntries(CWorkspace * pWork, char * pCh, char * pChEnd, GRFDBGSTR grfdbgstr);
void HideDebugStringForEntries(CWorkspace * pWork, size_t cBHiddenMax);

inline bool FIsReservedWord(CSTNode * pStnod, RWORD rword)
{
	if ((pStnod->m_park != PARK_ReservedWord) | (pStnod->m_pStval == nullptr))
		return false;

	return pStnod->m_pStval->m_rword == rword;
}

inline bool FIsIdentifier(CSTNode * pStnod, const char * pChzIdent)
{
	if ((pStnod->m_park != PARK_Identifier) | (pStnod->m_pStident == nullptr))
		return false;

	return pStnod->m_pStident->m_str == pChzIdent;
}

EWC::CString StrFromIdentifier(CSTNode * pStnod);



SLexerLocation LexlocFromSym(SSymbol * pSym);

enum FSHADOW
{
	FSHADOW_NoShadowing,
	FSHADOW_ShadowingAllowed,
};




class CUniqueTypeRegistry // tag untyper
{
public:
							CUniqueTypeRegistry(EWC::CAlloc * pAlloc);
							~CUniqueTypeRegistry()
								{ Clear(); }

	void					Clear();
	STypeInfo *				PTinMakeUnique(STypeInfo * pTin);
	SCOPID					ScopidAlloc()
								{ m_scopidNext = SCOPID(m_scopidNext + 1); return m_scopidNext; }

	EWC::CAlloc *						m_pAlloc;
	EWC::CHash<u64, STypeInfo *>		m_hashHvPTinUnique;
	SCOPID								m_scopidNext;		// global id for symbol tables, used in for unique types strings
};

class CSymbolTable		// tag = symtab
{
protected:
	friend CSymbolTable * PSymtabNew(EWC::CAlloc *, CSymbolTable *, const EWC::CString &, CUniqueTypeRegistry *, SUniqueNameSet *);

							// protected constructor to force use of CWorkspace::PSymtabNew()
							CSymbolTable(
								const EWC::CString & strNamespace,
								EWC::CAlloc * pAlloc,
								CUniqueTypeRegistry * pUntyper,
								SUniqueNameSet * pUnset)
							:m_strNamespace(strNamespace)
							,m_pAlloc(pAlloc)
							,m_hashHvPSym(pAlloc, EWC::BK_Symbol)
							,m_hashHvPTinBuiltIn(pAlloc, EWC::BK_Symbol)
							,m_hashHvPTinfwd(pAlloc, EWC::BK_Symbol)
							,m_arypTinManaged(pAlloc, EWC::BK_Symbol)
							,m_arypGenmapManaged(pAlloc, EWC::BK_Symbol)
							,m_arypSymGenerics(pAlloc, EWC::BK_Symbol)	
							,m_aryUsing(pAlloc, EWC::BK_Symbol)
							,m_arypSymtabUsedBy(pAlloc, EWC::BK_Symbol)
							,m_pUntyper(pUntyper)
							,m_pUnset(pUnset)
							,m_pSymtabParent(nullptr)
							,m_pSymtabNextManaged(nullptr)
							,m_iNestingDepth(0)
							,m_scopid(pUntyper->ScopidAlloc())
							,m_nVisitId(0)
								{ ; }

public:
	static EWC::CString s_strVoid;
	static EWC::CString s_strChar;
	static EWC::CString s_strBool;
	static EWC::CString s_strInt;
	static EWC::CString s_strUint;
	static EWC::CString s_strSsize;
	static EWC::CString s_strUsize;
	static EWC::CString s_strFloat;
	static EWC::CString s_strDouble;
	static EWC::CString s_strType;
	static EWC::CString s_strS8;
	static EWC::CString s_strS16;
	static EWC::CString s_strS32;
	static EWC::CString s_strS64;
	static EWC::CString s_strU8;
	static EWC::CString s_strU16;
	static EWC::CString s_strU32;
	static EWC::CString s_strU64;
	static EWC::CString s_strF32;
	static EWC::CString s_strF64;

	class CSymbolIterator // tag = symiter
	{
	public:
							CSymbolIterator()
							:m_pSymtab(nullptr)
							,m_pSym(nullptr)
							,m_grfsymlook(FSYMLOOK_Default)
								{ ; }


							CSymbolIterator(
								CSymbolTable * pSymtab,
								const EWC::CString & str,
								const SLexerLocation & lexloc,
								GRFSYMLOOK grfsymlook);

		SSymbol *			PSymNext();
		bool				FIsDone() const
								{ return (m_pSymtab == nullptr) | (m_pSym == nullptr); }

		CSymbolTable *		m_pSymtab;
		SSymbol *			m_pSym;
		SLexerLocation		m_lexloc;
		GRFSYMLOOK			m_grfsymlook;
	};

	struct SUsing // tag = using
	{
										SUsing()
										:m_pSymtab(nullptr)
										,m_pStnod(nullptr)
										,m_hashHvPSymp()
											{ ; }

										~SUsing();

		CSymbolTable *					m_pSymtab;
		CSTNode *						m_pStnod;

		EWC::CHash<HV, SSymbolPath *>	m_hashHvPSymp;			// a cache of the symbol paths found from this 
																// 'using' statement. Added lazily
	};

							~CSymbolTable();

	void					AddBuiltInSymbols(CWorkspace * pWork);
	SSymbol *				PSymEnsure(
								SErrorManager * pErrman,
								const EWC::CString & strName,
								CSTNode * pStnodDefinition,
								GRFSYM grfsym = FSYM_None, 
	 							FSHADOW fshadow = FSHADOW_ShadowingAllowed);

	void					AddUsingScope(
								SErrorManager * pErrman,
								CSymbolTable * pSymtabNew,
								CSTNode * pStnodSource
							);

	SSymbol *				PSymNewUnmanaged(const EWC::CString & strName, CSTNode * pStnodDefinition, GRFSYM grfsym);
	SSymbol * 				PSymGenericInstantiate(SSymbol * pSym, STypeInfo * pTinInstance);

	SSymbol *				PSymLookup(
								const EWC::CString & str,
								const SLexerLocation & lexloc, 
								GRFSYMLOOK grfsymlook = FSYMLOOK_Default,
								CSymbolTable ** ppSymtabOut = nullptr);

	STypeInfo *				PTinBuiltin(const EWC::CString & str);
	STypeInfoQualifier *	PTinqualBuiltinConst(const EWC::CString & str)
								{
									return PTinqualWrap(PTinBuiltin(str), FQUALK_Const);
								}

	STypeInfoLiteral *		PTinlitFromLitk(LITK litk);
	STypeInfoLiteral *		PTinlitFromLitk(LITK litk, int cBit, bool fIsSigned);
	STypeInfoPointer *		PTinptrAllocate(STypeInfo * pTinPointedTo, bool fIsImplicitRef = false);
	STypeInfoQualifier *	PTinqualEnsure(STypeInfo * pTinTarget, GRFQUALK grfqualk);
	STypeInfoQualifier *	PTinqualWrap(STypeInfo * pTinTarget, GRFQUALK grfqualk);

	template <typename T>
	T *						PTinMakeUnique(T * pTin)
								{ 
									return (T *)m_pUntyper->PTinMakeUnique(pTin);
								}

	void					AddBuiltInType(SErrorManager * pErrman, SLexer * pLex, STypeInfo * pTin, GRFSYM grfsym = FSYM_None);
	void					AddManagedTin(STypeInfo * pTin);
	void					AddManagedSymtab(CSymbolTable * pSymtab);

	void					PrintDump();

	static void				StaticStringInit();
	static void				StaticStringShutdown();

	EWC::CString					m_strNamespace;			// unique name for this symbol table's scope
	EWC::CAlloc *					m_pAlloc;
	EWC::CHash<HV, SSymbol *>		m_hashHvPSym;			// All the symbols defined within this scope, a full lookup requires
															//  walking up the parent list
	EWC::CHash<HV, STypeInfo *>		m_hashHvPTinBuiltIn;	// Builtin Types declared in this scope

	EWC::CHash<HV, STypeInfoForwardDecl *>
									m_hashHvPTinfwd;		// all pending forward declarations
	EWC::CDynAry<STypeInfo *>		m_arypTinManaged;		// all type info structs that need to be deleted.
	EWC::CDynAry<SGenericMap *>		m_arypGenmapManaged;	// generic mappings used by instantiated generic structs
	EWC::CDynAry<SSymbol *>			m_arypSymGenerics;		// symbol copies for generics, not mapped to an identifier
	EWC::CDynAry<SUsing>			m_aryUsing;				// symbol tables with members pushed into this table's scope
	EWC::CDynAry<CSymbolTable *>	m_arypSymtabUsedBy;		// symbol tables that refer to this one via a using statement
	CUniqueTypeRegistry *			m_pUntyper;				// hashes to find unique type instances
	SUniqueNameSet *				m_pUnset;				// set of unique names for implict names (created during parse)
	EWC::CDynAry<STypeInfoLiteral *>	
									m_mpLitkArypTinlit[LITK_Max];

	CSymbolTable *					m_pSymtabParent;

	CSymbolTable *					m_pSymtabNextManaged;	// next table in the global list
	s32								m_iNestingDepth;					
	SCOPID							m_scopid;				// unique table id, for unique type strings
	u64								m_nVisitId;				// id to check if this table has been visited during collision check
};

SSymbolBase * PSymbaseLookup(CSymbolTable * pSymtab, const EWC::CString & str, const SLexerLocation & lexloc, GRFSYMLOOK grfsymlook);


class CParseContext // tag = parctx
{
public:
						CParseContext(EWC::CAlloc * pAlloc, CWorkspace * pWork)
						:m_pAlloc(pAlloc)
						,m_pWork(pWork)
						,m_pSymtab(nullptr)
						,m_pSymtabGeneric(nullptr)
						,m_pStnodScope(nullptr)
						,m_grfsymlook(FSYMLOOK_Default)
							{ ; }

	EWC::CAlloc * 		m_pAlloc;
	CWorkspace *		m_pWork;
	CSymbolTable *		m_pSymtab;
	CSymbolTable *		m_pSymtabGeneric;	// symbol table for child generic types/values
											// ie. 'TakeFoo proc (foo : SFoo($T))' needs to add symbol 'D' to the symtable for TakeFoo
	CSTNode *			m_pStnodScope;		// current containg scope
	GRFSYMLOOK			m_grfsymlook;
};

void			PushSymbolTable(CParseContext * pParctx, CSymbolTable * pSymtab, const SLexerLocation & lexloc);
CSymbolTable *	PSymtabPop(CParseContext * pParctx);

STypeInfoStruct * PTinstructAlloc(CSymbolTable * pSymtab, EWC::CString strIdent, size_t cField, size_t cGenericParam);
STypeInfoProcedure * PTinprocAlloc(CSymbolTable * pSymtab, size_t cParam, size_t cReturn, const char * pCozName);

STypeInfo * PTinQualifyAfterAssignment(STypeInfo * pTin, CSymbolTable * pSymtab, STypeInfo * pTinDst);
STypeInfo * PTinStripQualifiers(STypeInfo * pTin);
STypeInfo * PTinStripQualifiers(STypeInfo * pTin, GRFQUALK & pGrfqualk);
STypeInfo * PTinStripQualifiersAndPointers(STypeInfo * pTin);
STypeInfo * PTinStripEnumToLoose(STypeInfo * pTin);

const char * PCozOverloadNameFromTok(TOK tok);
ERRID ErridCheckOverloadSignature(TOK tok, STypeInfoProcedure * pTinproc, SErrorManager * pErrman, SLexerLocation * pLexloc);
bool FAllowsCommutative(PARK park);

void ParseGlobalScope(CWorkspace * pWork, SLexer * pLex, GRFUNT grfunt);
CSTNode * PStnodAllocateIdentifier(EWC::CAlloc * pAlloc, const SLexerLocation & lexloc, const EWC::CString & strIdent);



enum IVALK // Instance VALue Kind
{
	IVALK_Error,	
	IVALK_Type,		// not an expression value: either a type or Type.m_nonConstantMember
	IVALK_RValue,	// has a value, but does not correspond to a memory location
	IVALK_LValue,	// has an assignable value

	EWC_MAX_MIN_NIL(IVALK)
};


IVALK IvalkCompute(CSTNode * pStnod);

#include "Generics.inl"
