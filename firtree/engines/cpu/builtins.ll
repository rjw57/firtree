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

;; misc. functions

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
