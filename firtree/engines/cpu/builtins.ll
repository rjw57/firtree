; ModuleID = 'builtins'

declare float @llvm.sin.f32( float )
declare <2 x float> @llvm.sin.v2f32( <2 x float> )
declare <3 x float> @llvm.sin.v3f32( <3 x float> )
declare <4 x float> @llvm.sin.v4f32( <4 x float> )

declare float @llvm.cos.f32( float )
declare <2 x float> @llvm.cos.v2f32( <2 x float> )
declare <3 x float> @llvm.cos.v3f32( <3 x float> )
declare <4 x float> @llvm.cos.v4f32( <4 x float> )

declare float @llvm.sqrt.f32( float )
declare <2 x float> @llvm.sqrt.v2f32( <2 x float> )
declare <3 x float> @llvm.sqrt.v3f32( <3 x float> )
declare <4 x float> @llvm.sqrt.v4f32( <4 x float> )

;; conversion functions to radians.
;; the 0x3F91DF46A0000000 magic number represents 2 * pi / 360

define float @radians_f( float ) {
entry:
	%rv = mul float 0x3F91DF46A0000000, %0
	ret float %rv
}

define <2 x float> @radians_v2( <2 x float> ) {
entry:
	%rv = mul <2 x float> < float 0x3F91DF46A0000000, float 0x3F91DF46A0000000 >, %0
	ret <2 x float> %rv
}

define <3 x float> @radians_v3( <3 x float> ) {
entry:
	%rv = mul <3 x float> < float 0x3F91DF46A0000000, float 0x3F91DF46A0000000, float 0x3F91DF46A0000000 >, %0
	ret <3 x float> %rv
}

define <4 x float> @radians_v4( <4 x float> ) {
entry:
	%rv = mul <4 x float> < float 0x3F91DF46A0000000, float 0x3F91DF46A0000000, float 0x3F91DF46A0000000, float 0x3F91DF46A0000000 >, %0
	ret <4 x float> %rv
}

;; conversion functions to degrees.
;; the 0x404CA5DC20000000 magic number represents 360 / (2 * pi).

define float @degrees_f( float ) {
entry:
	%rv = mul float 0x404CA5DC20000000, %0
	ret float %rv
}

define <2 x float> @degrees_v2( <2 x float> ) {
entry:
	%rv = mul <2 x float> < float 0x404CA5DC20000000, float 0x404CA5DC20000000 >, %0
	ret <2 x float> %rv
}

define <3 x float> @degrees_v3( <3 x float> ) {
entry:
	%rv = mul <3 x float> < float 0x404CA5DC20000000, float 0x404CA5DC20000000, float 0x404CA5DC20000000 >, %0
	ret <3 x float> %rv
}

define <4 x float> @degrees_v4( <4 x float> ) {
entry:
	%rv = mul <4 x float> < float 0x404CA5DC20000000, float 0x404CA5DC20000000, float 0x404CA5DC20000000, float 0x404CA5DC20000000 >, %0
	ret <4 x float> %rv
}

;; sin() intrinsic function 

define float @sin_f(float) {
entry:
	%rv = call float @llvm.sin.f32( float %0 )
	ret float %rv
}

define <2 x float> @sin_v2(<2 x float>) {
entry:
	%rv = call <2 x float> @llvm.sin.v2f32( <2 x float> %0 )
	ret <2 x float> %rv
}

define <3 x float> @sin_v3(<3 x float>) {
entry:
	%rv = call <3 x float> @llvm.sin.v3f32( <3 x float> %0 )
	ret <3 x float> %rv
}

define <4 x float> @sin_v4(<4 x float>) {
entry:
	%rv = call <4 x float> @llvm.sin.v4f32( <4 x float> %0 )
	ret <4 x float> %rv
}

define float @sin__f(float) {
entry:
	%rv = call float @llvm.sin.f32( float %0 )
	ret float %rv
}

define <2 x float> @sin__v2(<2 x float>) {
entry:
	%rv = call <2 x float> @llvm.sin.v2f32( <2 x float> %0 )
	ret <2 x float> %rv
}

define <3 x float> @sin__v3(<3 x float>) {
entry:
	%rv = call <3 x float> @llvm.sin.v3f32( <3 x float> %0 )
	ret <3 x float> %rv
}

define <4 x float> @sin__v4(<4 x float>) {
entry:
	%rv = call <4 x float> @llvm.sin.v4f32( <4 x float> %0 )
	ret <4 x float> %rv
}

;; cos() intrinsic function 

define float @cos_f(float) {
entry:
	%rv = call float @llvm.cos.f32( float %0 )
	ret float %rv
}

define <2 x float> @cos_v2(<2 x float>) {
entry:
	%rv = call <2 x float> @llvm.cos.v2f32( <2 x float> %0 )
	ret <2 x float> %rv
}

define <3 x float> @cos_v3(<3 x float>) {
entry:
	%rv = call <3 x float> @llvm.cos.v3f32( <3 x float> %0 )
	ret <3 x float> %rv
}

define <4 x float> @cos_v4(<4 x float>) {
entry:
	%rv = call <4 x float> @llvm.cos.v4f32( <4 x float> %0 )
	ret <4 x float> %rv
}

define float @cos__f(float) {
entry:
	%rv = call float @llvm.cos.f32( float %0 )
	ret float %rv
}

define <2 x float> @cos__v2(<2 x float>) {
entry:
	%rv = call <2 x float> @llvm.cos.v2f32( <2 x float> %0 )
	ret <2 x float> %rv
}

define <3 x float> @cos__v3(<3 x float>) {
entry:
	%rv = call <3 x float> @llvm.cos.v3f32( <3 x float> %0 )
	ret <3 x float> %rv
}

define <4 x float> @cos__v4(<4 x float>) {
entry:
	%rv = call <4 x float> @llvm.cos.v4f32( <4 x float> %0 )
	ret <4 x float> %rv
}

;; sincos() intrinsic function 

define <2 x float> @sincos_f(float) {
entry:
	%s = call float @llvm.sin.f32( float %0 )
	%c = call float @llvm.cos.f32( float %0 )
	%rv1 = insertelement <2 x float> zeroinitializer, float %s, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %c, i32 1
	ret <2 x float> %rv2
}

define <2 x float> @sincos__f(float) {
entry:
	%s = call float @llvm.sin.f32( float %0 )
	%c = call float @llvm.cos.f32( float %0 )
	%rv1 = insertelement <2 x float> zeroinitializer, float %s, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %c, i32 1
	ret <2 x float> %rv2
}

;; cossin() intrinsic function 

define <2 x float> @cossin_f(float) {
entry:
	%s = call float @llvm.sin.f32( float %0 )
	%c = call float @llvm.cos.f32( float %0 )
	%rv1 = insertelement <2 x float> zeroinitializer, float %s, i32 1
	%rv2 = insertelement <2 x float> %rv1, float %c, i32 0
	ret <2 x float> %rv2
}

define <2 x float> @cossin__f(float) {
entry:
	%s = call float @llvm.sin.f32( float %0 )
	%c = call float @llvm.cos.f32( float %0 )
	%rv1 = insertelement <2 x float> zeroinitializer, float %s, i32 1
	%rv2 = insertelement <2 x float> %rv1, float %c, i32 0
	ret <2 x float> %rv2
}

;; sqrt() intrinsic function 

define float @sqrt_f(float) {
entry:
	%rv = call float @llvm.sqrt.f32( float %0 )
	ret float %rv
}

define <2 x float> @sqrt_v2(<2 x float>) {
entry:
	%rv = call <2 x float> @llvm.sqrt.v2f32( <2 x float> %0 )
	ret <2 x float> %rv
}

define <3 x float> @sqrt_v3(<3 x float>) {
entry:
	%rv = call <3 x float> @llvm.sqrt.v3f32( <3 x float> %0 )
	ret <3 x float> %rv
}

define <4 x float> @sqrt_v4(<4 x float>) {
entry:
	%rv = call <4 x float> @llvm.sqrt.v4f32( <4 x float> %0 )
	ret <4 x float> %rv
}

;; dot() intrinsic function 

define float @dot_v2( <2 x float>, <2 x float> ) {
entry:
	%mul_val = mul <2 x float> %0, %1
	%x = extractelement <2 x float> %mul_val, i32 0
	%y = extractelement <2 x float> %mul_val, i32 1
	%rv = add float %x, %y
	ret float %rv
}

define float @dot_v3( <3 x float>, <3 x float> ) {
entry:
	%mul_val = mul <3 x float> %0, %1
	%x = extractelement <3 x float> %mul_val, i32 0
	%y = extractelement <3 x float> %mul_val, i32 1
	%z = extractelement <3 x float> %mul_val, i32 2
	%rv1 = add float %x, %y
	%rv2 = add float %rv1, %z
	ret float %rv2
}

define float @dot_v4( <4 x float>, <4 x float> ) {
entry:
	%mul_val = mul <4 x float> %0, %1
	%x = extractelement <4 x float> %mul_val, i32 0
	%y = extractelement <4 x float> %mul_val, i32 1
	%z = extractelement <4 x float> %mul_val, i32 2
	%w = extractelement <4 x float> %mul_val, i32 3
	%rv1 = add float %x, %y
	%rv2 = add float %rv1, %z
	%rv3 = add float %rv2, %w
	ret float %rv3
}

;; compare (%0 < 0.f ? %1 : %2) for each element

define float @compare_f( float, float, float ) {
entry:
	%flag = fcmp olt float %0, zeroinitializer
	br i1 %flag, label %true, label %false
true:
	ret float %1
false:
	ret float %2
}

;; (un)premultiply intrinsic

define <4 x float> @premultiply_v4( <4 x float> ) {
entry:
	%alpha = extractelement <4 x float> %0, i32 3
	%alpha_v1 = insertelement <4 x float> zeroinitializer, float %alpha, i32 0
	%alpha_v2 = insertelement <4 x float> %alpha_v1, float %alpha, i32 1
	%alpha_v3 = insertelement <4 x float> %alpha_v2, float %alpha, i32 2
	%alpha_v4 = insertelement <4 x float> %alpha_v3, float 1.0, i32 3
	%rv = mul <4 x float> %alpha_v4, %0
	ret <4 x float> %rv
}

define <4 x float> @unpremultiply_v4( <4 x float> ) {
entry:
	%alpha = extractelement <4 x float> %0, i32 3
	%is_zero = fcmp oeq float %alpha, zeroinitializer
	br i1 %is_zero, label %pass_through, label %continue
continue:
	%ooalpha = fdiv float %alpha, 1.0
	%alpha_v1 = insertelement <4 x float> zeroinitializer, float %ooalpha, i32 0
	%alpha_v2 = insertelement <4 x float> %alpha_v1, float %ooalpha, i32 1
	%alpha_v3 = insertelement <4 x float> %alpha_v2, float %ooalpha, i32 2
	%alpha_v4 = insertelement <4 x float> %alpha_v3, float 1.0, i32 3
	%rv = mul <4 x float> %alpha_v4, %0
	ret <4 x float> %rv
pass_through:
	ret <4 x float> %0
}
