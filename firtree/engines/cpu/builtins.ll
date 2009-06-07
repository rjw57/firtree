; ModuleID = 'builtins'

; Most targets only support 4d and 1d intrinsics and
; so do we.
declare float @llvm.sin.f32( float )
declare <4 x float> @llvm.sin.v4f32( <4 x float> )
declare float @llvm.cos.f32( float )
declare <4 x float> @llvm.cos.v4f32( <4 x float> )

;; sin() intrinsic function 

define float @sin_f(float) {
entry:
	%rv = call float @llvm.sin.f32( float %0 )
	ret float %rv
}

define <2 x float> @sin_v2(<2 x float>) {
entry:
	%ix = extractelement <2 x float> %0, i32 0
	%iy = extractelement <2 x float> %0, i32 1
	%v1 = insertelement <4 x float> zeroinitializer, float %ix, i32 0
	%v2 = insertelement <4 x float> %v1, float %iy, i32 1
	%rv = call <4 x float> @llvm.sin.v4f32( <4 x float> %v2 )
	%ox = extractelement <4 x float> %rv, i32 0
	%oy = extractelement <4 x float> %rv, i32 1
	%rv1 = insertelement <2 x float> zeroinitializer, float %ox, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %oy, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @sin_v3(<3 x float>) {
entry:
	%ix = extractelement <3 x float> %0, i32 0
	%iy = extractelement <3 x float> %0, i32 1
	%iz = extractelement <3 x float> %0, i32 2
	%v1 = insertelement <4 x float> zeroinitializer, float %ix, i32 0
	%v2 = insertelement <4 x float> %v1, float %iy, i32 1
	%v3 = insertelement <4 x float> %v2, float %iz, i32 2
	%rv = call <4 x float> @llvm.sin.v4f32( <4 x float> %v3 )
	%ox = extractelement <4 x float> %rv, i32 0
	%oy = extractelement <4 x float> %rv, i32 1
	%oz = extractelement <4 x float> %rv, i32 2
	%rv1 = insertelement <3 x float> zeroinitializer, float %ox, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %oy, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %oz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @sin_v4(<4 x float>) {
entry:
	%retval = call <4 x float> @llvm.sin.v4f32( <4 x float> %0 )
	ret <4 x float> %retval
}

;; cos() intrinsic function 

define float @cos_f(float) {
entry:
	%rv = call float @llvm.cos.f32( float %0 )
	ret float %rv
}

define <2 x float> @cos_v2(<2 x float>) {
entry:
	%ix = extractelement <2 x float> %0, i32 0
	%iy = extractelement <2 x float> %0, i32 1
	%v1 = insertelement <4 x float> zeroinitializer, float %ix, i32 0
	%v2 = insertelement <4 x float> %v1, float %iy, i32 1
	%rv = call <4 x float> @llvm.cos.v4f32( <4 x float> %v2 )
	%ox = extractelement <4 x float> %rv, i32 0
	%oy = extractelement <4 x float> %rv, i32 1
	%rv1 = insertelement <2 x float> zeroinitializer, float %ox, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %oy, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @cos_v3(<3 x float>) {
entry:
	%ix = extractelement <3 x float> %0, i32 0
	%iy = extractelement <3 x float> %0, i32 1
	%iz = extractelement <3 x float> %0, i32 2
	%v1 = insertelement <4 x float> zeroinitializer, float %ix, i32 0
	%v2 = insertelement <4 x float> %v1, float %iy, i32 1
	%v3 = insertelement <4 x float> %v2, float %iz, i32 2
	%rv = call <4 x float> @llvm.cos.v4f32( <4 x float> %v3 )
	%ox = extractelement <4 x float> %rv, i32 0
	%oy = extractelement <4 x float> %rv, i32 1
	%oz = extractelement <4 x float> %rv, i32 2
	%rv1 = insertelement <3 x float> zeroinitializer, float %ox, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %oy, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %oz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @cos_v4(<4 x float>) {
entry:
	%retval = call <4 x float> @llvm.cos.v4f32( <4 x float> %0 )
	ret <4 x float> %retval
}

