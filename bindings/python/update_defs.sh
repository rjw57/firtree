#!/bin/bash

python codegen/defsgen.py \
	-m firtree \
	-f firtree-core-overriden.defs \
	-l ../../firtree/libfirtree-core-0.1.so \
	-s firtree-core -p \
	../../firtree/firtree-affine-transform.h \
	../../firtree/firtree-buffer-sampler.h \
	../../firtree/firtree-debug.h \
	../../firtree/firtree-kernel.h \
	../../firtree/firtree-kernel-sampler.h \
	../../firtree/firtree-sampler.h \
	../../firtree/firtree-types.h \
	../../firtree/firtree-vector.h

python codegen/defsgen.py \
	-m firtree \
	-f firtree-engine-cpu-overriden.defs \
	-l ../../firtree/engines/cpu/libfirtree-engine-cpu-0.1.so \
	-s firtree-engine-cpu -p \
	../../firtree/engines/cpu/*.h 

python codegen/defsgen.py \
	-m firtree \
	-l ../../firtree/libfirtree-core-0.1.so \
	-s firtree-cairo -p \
	../../firtree/firtree-cairo-surface-sampler.h

python codegen/defsgen.py \
	-m firtree \
	-l ../../firtree/libfirtree-core-0.1.so \
	-s firtree-clutter -p \
	../../firtree/firtree-cogl-texture-sampler.h

python codegen/defsgen.py \
	-m firtree \
	-l ../../firtree/libfirtree-core-0.1.so \
	-s firtree-gdk-pixbuf -p \
	../../firtree/firtree-pixbuf-sampler.h

