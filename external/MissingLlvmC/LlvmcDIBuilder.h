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

#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LLVMOpaqueDIBuilder * LLVMDIBuilderRef;

LLVMDIBuilderRef LLVMCreateDIBuilder(LLVMModuleRef pMod);
void LLVMDisposeDIBuilder(LLVMDIBuilderRef pDib);
void LLVMDIBuilderFinalize(LLVMDIBuilderRef pDib);

LLVMValueRef LLVMDIBuilderCreateCompileUnit(
				LLVMDIBuilderRef pDib, 
				unsigned nLanguage,
				const char * pChzFilename, 
				const char * pChzDirectory, 
				const char * pChzProducer,
				LLVMBool fIsOptimized,
				const char * pChzFlags,
				unsigned nRuntimeVersion);

LLVMValueRef LLVMDIBuilderCreateFile(LLVMDIBuilderRef pDib, const char * pChzFilename, const char * pChzDirectory);
LLVMValueRef LLVMCreateDebugLocation(int nLine, int nCol, LLVMValueRef pLvalScope);

// Types

LLVMValueRef LLVMDIBuilderCreateNullPtr(LLVMDIBuilderRef pDib);
LLVMValueRef LLVMDIBuilderCreateEnumerator(LLVMDIBuilderRef pDib, const char * pChzName, uint64_t nValue);
LLVMValueRef LLVMDIBuilderCreateBasicType(
				LLVMDIBuilderRef pDib, 
				const char * pChzName,
			    uint64_t cBitSize,
			    uint64_t cBitAlign,
			    unsigned nDwarfEncoding);

LLVMValueRef LLVMDIBuilderCreateQualifiedType(LLVMDIBuilderRef pDib, unsigned nDwarfTag, LLVMValueRef pLvalFromType);
LLVMValueRef LLVMDIBuilderCreatePointerType(
				LLVMDIBuilderRef pDib,
			    LLVMValueRef pLvalPointeeType,
			    uint64_t cBitSize,
			    uint64_t cBitAlign, 
				const char * pChzName);

LLVMValueRef LLVMDIBuilderCreateTypeDef(
				LLVMDIBuilderRef pDib,
			    LLVMValueRef pLvalOriginalType,
				const char * pChzTypedefName,
			    LLVMValueRef pLvalFile,
				unsigned nLine,
			    LLVMValueRef pLvalScope);

LLVMValueRef LLVMDIBuilderCreateMemberType(
			    LLVMDIBuilderRef pDib,
				LLVMValueRef pLvalScope,
				const char * pChzName,
				LLVMValueRef pLvalFile,
			    unsigned nLine,
				uint64_t cBitSize,
				uint64_t cBitAlign,
			    uint64_t dBitOffset,
				unsigned nFlags,
				LLVMValueRef pLvalParentType);

LLVMValueRef LLVMDIBuilderCreateClassType(
			    LLVMDIBuilderRef pDib, 
				LLVMValueRef pLvalScope,
				const char * pChzName,
				LLVMValueRef pLvalFile,
			    unsigned nLine, 
				uint64_t cBitSize, 
				uint64_t cBitAlign,
			    uint64_t dBitOffset,
				unsigned nFlags, 
				LLVMValueRef pLvalDerivedFrom,
			    LLVMValueRef * ppLvalElements, 
				unsigned cElements,
				LLVMValueRef pLvalVTableHolder,
			    LLVMValueRef pLvalTemplateParms);

LLVMValueRef LLVMDIBuilderCreateStructType(
			    LLVMDIBuilderRef pDib, 
				LLVMValueRef pLvalScope, 
				const char * pChzName, 
				LLVMValueRef pLvalFile,
			    unsigned nLine, 
				uint64_t cBitSize, 
				uint64_t cBitAlign, 
				unsigned nFlags,
				LLVMValueRef pLvalDerivedFrom,
			    LLVMValueRef * ppLvalElements, 
				unsigned cElements,
				LLVMValueRef pLvalVTableHolder,
				unsigned nRunTimeLanguage);

LLVMValueRef LLVMDIBuilderCreateArrayType(
				LLVMDIBuilderRef pDib,
				uint64_t cBitArraySize,
				uint64_t cBitAlign,
			    LLVMValueRef pLvalElementType,
			    LLVMValueRef * ppLvalSubscripts,
				unsigned cSubscript);

LLVMValueRef LLVMDIBuilderCreateEnumerationType(
			    LLVMDIBuilderRef pDib, 
				LLVMValueRef pLvalScope, 
				const char * pChzName, 
				LLVMValueRef pLvalFile,
			    unsigned nLine,
				uint64_t cBitSize,
				uint64_t cBitAlign,
			    LLVMValueRef * ppLvalElements, 
				unsigned cElement,
				LLVMValueRef pLvalUnderlyingType);

LLVMValueRef LLVMDIBuilderCreateGlobalVariable(
				LLVMDIBuilderRef pDib,
				LLVMValueRef pLvalScope, 
				const char * pChzName,
				const char * pChzMangled,
				LLVMValueRef pLvalFile,
				unsigned nLine,
				LLVMValueRef pLvalType,
				bool fIsLocalToUnit,
				LLVMValueRef pLvalValue);

LLVMValueRef LLVMDIBuilderCreateLocalVariable(
				LLVMDIBuilderRef pDib,
			    unsigned nDwarTag,
				LLVMValueRef pLvalScope, 
				const char * pChzName,
				LLVMValueRef pLvalFile,
				unsigned nLine,
				LLVMValueRef pLvalType,
				bool fIsPreservedWhenOptimized,
			    unsigned nFlags,
				unsigned iArgument);

LLVMValueRef LLVMDIBuilderCreateFunction(
			    LLVMDIBuilderRef pDib,
				LLVMValueRef pLvalScope,
				const char * pChzName,
				const char * pChzMangled,
				LLVMValueRef pLvalFile,
				unsigned nLine,
				LLVMValueRef pLvalType,
				bool fIsLocalToUnit,
			    bool fIsDefinition,
				unsigned nLineScopeBegin,
				unsigned nDwarfFlags,
				bool fIsOptimized,
			    LLVMValueRef pLvalFunction,
				LLVMValueRef pLvalTemplateParm,
				LLVMValueRef pLvalDecl);

LLVMValueRef LLVMDIBuilderCreateNamespace(
				LLVMDIBuilderRef pDib,
				LLVMValueRef pLvalScope,
				const char * pChzName,
				LLVMValueRef pLvalFile,
				unsigned nLine);

LLVMValueRef LLVMDIBuilderCreateLexicalBlock(
				LLVMDIBuilderRef pDib,
				LLVMValueRef pLvalScope,
				LLVMValueRef pLvalFile,
				unsigned nLine,
				unsigned nCol);

#ifdef __cplusplus
} // extern "C"
#endif