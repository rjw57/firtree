#!/bin/bash

python codegen/defsgen.py \
	-m firtree \
	-f firtree-overriden.defs \
	-l ../../firtree/libfirtree.so \
	-s firtree -p \
	../../firtree/engines/*/*.h \
	../../firtree/*.h 

