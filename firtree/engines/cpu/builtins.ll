; ModuleID = 'builtins'

declare float @llvm.sin.f32( float )
declare <2 x float> @llvm.sin.v2f32( <2 x float> )
declare <3 x float> @llvm.sin.v3f32( <3 x float> )
declare <4 x float> @llvm.sin.v4f32( <4 x float> )

declare float @llvm.cos.f32( float )
declare <2 x float> @llvm.cos.v2f32( <2 x float> )
declare <3 x float> @llvm.cos.v3f32( <3 x float> )
declare <4 x float> @llvm.cos.v4f32( <4 x float> )

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
