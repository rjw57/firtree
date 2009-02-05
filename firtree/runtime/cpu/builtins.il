; ModuleID = 'intrinsics'

; MATH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
declare float @expf(float) nounwind readnone

define float @exp_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @expf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @exp_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @expf(float %x) nounwind readnone
	%ry = tail call float @expf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @exp_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @expf(float %x) nounwind readnone
	%ry = tail call float @expf(float %y) nounwind readnone
	%rz = tail call float @expf(float %y) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @exp_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @expf(float %x) nounwind readnone
	%ry = tail call float @expf(float %y) nounwind readnone
	%rz = tail call float @expf(float %y) nounwind readnone
	%rw = tail call float @expf(float %y) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
declare float @sqrtf(float) nounwind readnone

define float @sqrt_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @sqrtf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @sqrt_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @sqrtf(float %x) nounwind readnone
	%ry = tail call float @sqrtf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @sqrt_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @sqrtf(float %x) nounwind readnone
	%ry = tail call float @sqrtf(float %y) nounwind readnone
	%rz = tail call float @sqrtf(float %y) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @sqrt_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @sqrtf(float %x) nounwind readnone
	%ry = tail call float @sqrtf(float %y) nounwind readnone
	%rz = tail call float @sqrtf(float %y) nounwind readnone
	%rw = tail call float @sqrtf(float %y) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @dot_v2v2(<2 x float> %a, <2 x float> %b) nounwind readnone {
entry:
	%mul = mul <2 x float>  %a, %b
	%x = extractelement <2 x float> %mul, i32 0
	%y = extractelement <2 x float> %mul, i32 1
	%rv1 = add float %x, %y
	ret float %rv1
}

define float @dot_v3v3(<3 x float> %a, <3 x float> %b) nounwind readnone {
entry:
	%mul = mul <3 x float>  %a, %b
	%x = extractelement <3 x float> %mul, i32 0
	%y = extractelement <3 x float> %mul, i32 1
	%z = extractelement <3 x float> %mul, i32 2
	%rv1 = add float %x, %y
	%rv2 = add float %rv1, %z
	ret float %rv2
}

define float @dot_v4v4(<4 x float> %a, <4 x float> %b) nounwind readnone {
entry:
	%mul = mul <4 x float>  %a, %b
	%x = extractelement <4 x float> %mul, i32 0
	%y = extractelement <4 x float> %mul, i32 1
	%z = extractelement <4 x float> %mul, i32 2
	%w = extractelement <4 x float> %mul, i32 3
	%rv1 = add float %x, %y
	%rv2 = add float %rv1, %z
	%rv3 = add float %rv2, %w
	ret float %rv3
}

; GEOM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @length_v2(<2 x float> %a) nounwind readnone {
entry:
	%sqln = tail call float @dot_v2v2(<2 x float> %a, <2 x float> %a) nounwind readnone
	%ln = tail call float @sqrt_f(float %sqln) nounwind readnone
	ret float %ln
}

define float @length_v3(<3 x float> %a) nounwind readnone {
entry:
	%sqln = tail call float @dot_v3v3(<3 x float> %a, <3 x float> %a) nounwind readnone
	%ln = tail call float @sqrt_f(float %sqln) nounwind readnone
	ret float %ln
}

define float @length_v4(<4 x float> %a) nounwind readnone {
entry:
	%sqln = tail call float @dot_v4v4(<4 x float> %a, <4 x float> %a) nounwind readnone
	%ln = tail call float @sqrt_f(float %sqln) nounwind readnone
	ret float %ln
}

; TRIG

declare float @llvm.sin.f32(float %Val)
declare <2 x float> @llvm.sin.v2f32(<2 x float> %Val)
declare <3 x float> @llvm.sin.v3f32(<3 x float> %Val)
declare <4 x float> @llvm.sin.v4f32(<4 x float> %Val)

define float @sin_f(float %a) nounwind readnone {
	%rv = tail call float @llvm.sin.f32(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @sin_v2(<2 x float> %a) nounwind readnone {
	%rv = tail call <2 x float> @llvm.sin.v2f32(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @sin_v3(<3 x float> %a) nounwind readnone {
	%rv = tail call <3 x float> @llvm.sin.v3f32(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @sin_v4(<4 x float> %a) nounwind readnone {
	%rv = tail call <4 x float> @llvm.sin.v4f32(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

declare float @llvm.cos.f32(float %Val)
declare <2 x float> @llvm.cos.v2f32(<2 x float> %Val)
declare <3 x float> @llvm.cos.v3f32(<3 x float> %Val)
declare <4 x float> @llvm.cos.v4f32(<4 x float> %Val)

define float @cos_f(float %a) nounwind readnone {
	%rv = tail call float @llvm.cos.f32(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @cos_v2(<2 x float> %a) nounwind readnone {
	%rv = tail call <2 x float> @llvm.cos.v2f32(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @cos_v3(<3 x float> %a) nounwind readnone {
	%rv = tail call <3 x float> @llvm.cos.v3f32(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @cos_v4(<4 x float> %a) nounwind readnone {
	%rv = tail call <4 x float> @llvm.cos.v4f32(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

; COLOR

define <4 x float> @premultiply_v4(<4 x float> %a) nounwind readnone {
	%alpha = extractelement <4 x float> %a, i32 3
	%r = extractelement <4 x float> %a, i32 0
	%g = extractelement <4 x float> %a, i32 1
	%b = extractelement <4 x float> %a, i32 2
	%rm = mul float %r, %alpha
	%gm = mul float %g, %alpha
	%bm = mul float %b, %alpha
	%rv1 = insertelement <4 x float> %a, float %rm, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %gm, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %bm, i32 2
	ret <4 x float> %rv3
}

define <4 x float> @unpremultiply_v4(<4 x float> %a) nounwind readnone {
	%alpha = extractelement <4 x float> %a, i32 3
	%r = extractelement <4 x float> %a, i32 0
	%g = extractelement <4 x float> %a, i32 1
	%b = extractelement <4 x float> %a, i32 2
	%rm = fdiv float %r, %alpha
	%gm = fdiv float %g, %alpha
	%bm = fdiv float %b, %alpha
	%rv1 = insertelement <4 x float> %a, float %rm, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %gm, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %bm, i32 2
	ret <4 x float> %rv3
}

;; This function does the actual work calling the kernel and blending the
;; result.

declare <4 x float> @kernel(<2 x float>) nounwind readnone

define void @doit(<2 x float>* %pcoord, <4 x float>* %pout) {
entry:
	%coord = load <2 x float>* %pcoord
	%oldval = load <4 x float>* %pout
	%result = call <4 x float> @kernel(<2 x float> %coord)

	;; Perform an OVER composite.
	%alpha_a = extractelement <4 x float> %result, i32 3
	%one_minus_a = sub float 1.0, %alpha_a

	%omav1 = insertelement <4 x float> zeroinitializer, float %one_minus_a, i32 0
	%omav2 = insertelement <4 x float> %omav1, float %one_minus_a, i32 1
	%omav3 = insertelement <4 x float> %omav2, float %one_minus_a, i32 2
	%omav4 = insertelement <4 x float> %omav3, float %one_minus_a, i32 3

	%tmp1 = mul <4 x float> %omav4, %oldval
	%tmp2 = add <4 x float> %tmp1, %result

	store <4 x float> %tmp2, <4 x float>* %pout
	ret void
}

