#!/bin/sh

../source/moe -nolink helloworld.moe

[ $? = 0 ] || exit

gcc helloworld.o ../lib/Basic/source/Basic.o -o helloworld -lm

[ $? = 0 ] || exit 

./helloworld

