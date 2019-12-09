#!/bin/sh

gcc -L ../external/MissingLlvmC/build -lMissingLlvmC ./build/CodeGen.o ./build/EwcString.o ./build/Lexer.o ./build/Main.o ./build/Parser.o ./build/TypeCheck.o ./build/UnitTest.o ./build/Util.o ./build/Workspace.o ./build/ByteCode.o -o moe  -L/usr/lib/llvm-7/lib -lLLVM -lstdc++ -lm ../../dyncall-1.0/dyncall/libdyncall_s.a ../../dyncall-1.0/dynload/libdynload_s.a -ldl -lMissingLlvmC


