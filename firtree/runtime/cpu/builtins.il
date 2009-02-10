; ModuleID = 'intrinsics'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; EXTERNAL DECLARATIONS

declare float @_ft_expf(float) nounwind readnone
declare float @_ft_exp2f(float) nounwind readnone
declare float @_ft_logf(float) nounwind readnone
declare float @_ft_log2f(float) nounwind readnone
declare float @_ft_tanf(float) nounwind readnone
declare float @_ft_asinf(float) nounwind readnone
declare float @_ft_acosf(float) nounwind readnone
declare float @_ft_atanf(float) nounwind readnone
declare float @_ft_atan2f(float, float) nounwind readnone

;; LLVM INTRINSICS

declare float @llvm.sin.f32(float %Val)
declare <2 x float> @llvm.sin.v2f32(<2 x float> %Val)
declare <3 x float> @llvm.sin.v3f32(<3 x float> %Val)
declare <4 x float> @llvm.sin.v4f32(<4 x float> %Val)

declare float @llvm.cos.f32(float %Val)
declare <2 x float> @llvm.cos.v2f32(<2 x float> %Val)
declare <3 x float> @llvm.cos.v3f32(<3 x float> %Val)
declare <4 x float> @llvm.cos.v4f32(<4 x float> %Val)

declare float @llvm.sqrt.f32(float %Val)
declare <2 x float> @llvm.sqrt.v2f32(<2 x float> %Val)
declare <3 x float> @llvm.sqrt.v3f32(<3 x float> %Val)
declare <4 x float> @llvm.sqrt.v4f32(<4 x float> %Val)

;; LIBRARY FUNCTIONS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This is the result of compiling the famous QuakeIII rsqrt function 
;; via clang.
;; From http://www.codemaestro.com/reviews/9
define float @_ft_inversesqrtf(float %number) nounwind {
entry:
	%0 = mul float %number, 5.000000e-01
	%number8 = bitcast float %number to i32
	%1 = ashr i32 %number8, 1	
	%2 = sub i32 1597463007, %1
	%3 = bitcast i32 %2 to float
	%4 = mul float %0, %3	
	%5 = mul float %4, %3
	%6 = sub float 1.500000e+00, %5	
	%7 = mul float %6, %3	
	ret float %7
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Floor and ceiling are tricksy beasts. In general we can get somewhere
;; by truncating.
define i32 @_ft_floori(float %number) nounwind readnone {
entry:
	;; Calculate the truncated value.
	%tmpint = fptosi float %number to i32
	%truncval = sitofp i32 %tmpint to float

	;; Sub 1 from the truncated value
	%lowerval = sub i32 %tmpint, 1

	;; If truncated value is <= to %number, return it, otherwise
	;; return the lower value
	%0 = fcmp ole float %truncval, %number
	%rv = select i1 %0, i32 %tmpint, i32 %lowerval

	ret i32 %rv
}

define float @_ft_floorf(float %number) nounwind readnone {
entry:
	%iv = tail call i32 @_ft_floori(float %number) nounwind readnone
	%rv = sitofp i32 %iv to float
	ret float %rv
}

define i32 @_ft_ceili(float %number) nounwind readnone {
entry:
	;; Calculate the truncated value.
	%tmpint = fptosi float %number to i32
	%truncval = sitofp i32 %tmpint to float

	;; Add 1 to the truncated value
	%higherval = add i32 %tmpint, 1

	;; If truncated value is >= to %number, return it, otherwise
	;; return the higher value
	%0 = fcmp oge float %truncval, %number
	%rv = select i1 %0, i32 %tmpint, i32 %higherval

	ret i32 %rv
}

define float @_ft_ceilf(float %number) nounwind readnone {
entry:
	%iv = tail call i32 @_ft_ceili(float %number) nounwind readnone
	%rv = sitofp i32 %iv to float
	ret float %rv
}

; MATH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @radians_f(float %a) nounwind readnone {
	%rv = mul float %a, 0x3F91DF45A0000000
	ret float %rv
}

define <2 x float> @radians_v2(<2 x float> %a) nounwind readnone {
	%rv = mul <2 x float> %a, < float 0x3F91DF45A0000000, float 0x3F91DF45A0000000 >
	ret <2 x float> %rv
}

define <3 x float> @radians_v3(<3 x float> %a) nounwind readnone {
	%rv = mul <3 x float> %a, < float 0x3F91DF45A0000000, float 0x3F91DF45A0000000, float 0x3F91DF45A0000000 >
	ret <3 x float> %rv
}

define <4 x float> @radians_v4(<4 x float> %a) nounwind readnone {
	%rv = mul <4 x float> %a, < float 0x3F91DF45A0000000, float 0x3F91DF45A0000000, float 0x3F91DF45A0000000, float 0x3F91DF45A0000000  >
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @degrees_f(float %a) nounwind readnone {
	%rv = mul float %a, 0x404CA5DDC0000000
	ret float %rv
}

define <2 x float> @degrees_v2(<2 x float> %a) nounwind readnone {
	%rv = mul <2 x float> %a, < float 0x404CA5DDC0000000, float 0x404CA5DDC0000000 >
	ret <2 x float> %rv
}

define <3 x float> @degrees_v3(<3 x float> %a) nounwind readnone {
	%rv = mul <3 x float> %a, < float 0x404CA5DDC0000000, float 0x404CA5DDC0000000, float 0x404CA5DDC0000000 >
	ret <3 x float> %rv
}

define <4 x float> @degrees_v4(<4 x float> %a) nounwind readnone {
	%rv = mul <4 x float> %a, < float 0x404CA5DDC0000000, float 0x404CA5DDC0000000, float 0x404CA5DDC0000000, float 0x404CA5DDC0000000  >
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @sin__f(float %a) nounwind readnone {
	%rv = tail call float @llvm.sin.f32(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @sin__v2(<2 x float> %a) nounwind readnone {
	%rv = tail call <2 x float> @llvm.sin.v2f32(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @sin__v3(<3 x float> %a) nounwind readnone {
	%rv = tail call <3 x float> @llvm.sin.v3f32(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @sin__v4(<4 x float> %a) nounwind readnone {
	%rv = tail call <4 x float> @llvm.sin.v4f32(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @cos__f(float %a) nounwind readnone {
	%rv = tail call float @llvm.cos.f32(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @cos__v2(<2 x float> %a) nounwind readnone {
	%rv = tail call <2 x float> @llvm.cos.v2f32(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @cos__v3(<3 x float> %a) nounwind readnone {
	%rv = tail call <3 x float> @llvm.cos.v3f32(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @cos__v4(<4 x float> %a) nounwind readnone {
	%rv = tail call <4 x float> @llvm.cos.v4f32(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @tan_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_tanf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @tan_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_tanf(float %x) nounwind readnone
	%ry = tail call float @_ft_tanf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @tan_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_tanf(float %x) nounwind readnone
	%ry = tail call float @_ft_tanf(float %y) nounwind readnone
	%rz = tail call float @_ft_tanf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @tan_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_tanf(float %x) nounwind readnone
	%ry = tail call float @_ft_tanf(float %y) nounwind readnone
	%rz = tail call float @_ft_tanf(float %z) nounwind readnone
	%rw = tail call float @_ft_tanf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @tan__f(float %a) nounwind readnone {
entry:
	%rv = tail call float @tan_f(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @tan__v2(<2 x float> %a) nounwind readnone {
entry:
	%rv = tail call <2 x float> @tan_v2(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @tan__v3(<3 x float> %a) nounwind readnone {
entry:
	%rv = tail call <3 x float> @tan_v3(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @tan__v4(<4 x float> %a) nounwind readnone {
entry:
	%rv = tail call <4 x float> @tan_v4(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define <2 x float> @sincos_f(float %a) nounwind readnone {
	%s = tail call float @llvm.sin.f32(float %a) nounwind readnone
	%c = tail call float @llvm.cos.f32(float %a) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %s, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %c, i32 1
	ret <2 x float> %rv2
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define <2 x float> @cossin_f(float %a) nounwind readnone {
	%s = tail call float @llvm.sin.f32(float %a) nounwind readnone
	%c = tail call float @llvm.cos.f32(float %a) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %c, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %s, i32 1
	ret <2 x float> %rv2
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define <2 x float> @sincos__f(float %a) nounwind readnone {
	%rv = tail call <2 x float> @sincos_f(float %a) nounwind readnone
	ret <2 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define <2 x float> @cossin__f(float %a) nounwind readnone {
	%rv = tail call <2 x float> @cossin_f(float %a) nounwind readnone
	ret <2 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @asin_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_asinf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @asin_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_asinf(float %x) nounwind readnone
	%ry = tail call float @_ft_asinf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @asin_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_asinf(float %x) nounwind readnone
	%ry = tail call float @_ft_asinf(float %y) nounwind readnone
	%rz = tail call float @_ft_asinf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @asin_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_asinf(float %x) nounwind readnone
	%ry = tail call float @_ft_asinf(float %y) nounwind readnone
	%rz = tail call float @_ft_asinf(float %z) nounwind readnone
	%rw = tail call float @_ft_asinf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @acos_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_acosf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @acos_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_acosf(float %x) nounwind readnone
	%ry = tail call float @_ft_acosf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @acos_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_acosf(float %x) nounwind readnone
	%ry = tail call float @_ft_acosf(float %y) nounwind readnone
	%rz = tail call float @_ft_acosf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @acos_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_acosf(float %x) nounwind readnone
	%ry = tail call float @_ft_acosf(float %y) nounwind readnone
	%rz = tail call float @_ft_acosf(float %z) nounwind readnone
	%rw = tail call float @_ft_acosf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @atan_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_atanf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @atan_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_atanf(float %x) nounwind readnone
	%ry = tail call float @_ft_atanf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @atan_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_atanf(float %x) nounwind readnone
	%ry = tail call float @_ft_atanf(float %y) nounwind readnone
	%rz = tail call float @_ft_atanf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @atan_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_atanf(float %x) nounwind readnone
	%ry = tail call float @_ft_atanf(float %y) nounwind readnone
	%rz = tail call float @_ft_atanf(float %z) nounwind readnone
	%rw = tail call float @_ft_atanf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @atan_ff(float %a, float %b) nounwind readnone {
entry:
	%rv = tail call float @_ft_atan2f(float %a, float %b) nounwind readnone
	ret float %rv
}

define <2 x float> @atan_v2v2(<2 x float> %a, <2 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <2 x float> %a, i32 0
	%y1 = extractelement <2 x float> %a, i32 1
	%x2 = extractelement <2 x float> %b, i32 0
	%y2 = extractelement <2 x float> %b, i32 1
	%rx = tail call float @_ft_atan2f(float %x1, float %x2) nounwind readnone
	%ry = tail call float @_ft_atan2f(float %y1, float %y2) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @atan_v3v3(<3 x float> %a, <3 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <3 x float> %a, i32 0
	%y1 = extractelement <3 x float> %a, i32 1
	%z1 = extractelement <3 x float> %a, i32 2
	%x2 = extractelement <3 x float> %b, i32 0
	%y2 = extractelement <3 x float> %b, i32 1
	%z2 = extractelement <3 x float> %b, i32 2
	%rx = tail call float @_ft_atan2f(float %x1, float %x2) nounwind readnone
	%ry = tail call float @_ft_atan2f(float %y1, float %y2) nounwind readnone
	%rz = tail call float @_ft_atan2f(float %z1, float %z2) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @atan_v4v4(<4 x float> %a, <4 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <4 x float> %a, i32 0
	%y1 = extractelement <4 x float> %a, i32 1
	%z1 = extractelement <4 x float> %a, i32 2
	%w1 = extractelement <4 x float> %a, i32 3
	%x2 = extractelement <4 x float> %b, i32 0
	%y2 = extractelement <4 x float> %b, i32 1
	%z2 = extractelement <4 x float> %b, i32 2
	%w2 = extractelement <4 x float> %b, i32 3
	%rx = tail call float @_ft_atan2f(float %x1, float %x2) nounwind readnone
	%ry = tail call float @_ft_atan2f(float %y1, float %y2) nounwind readnone
	%rz = tail call float @_ft_atan2f(float %z1, float %z2) nounwind readnone
	%rw = tail call float @_ft_atan2f(float %w1, float %w2) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @exp_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_expf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @exp_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_expf(float %x) nounwind readnone
	%ry = tail call float @_ft_expf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @exp_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_expf(float %x) nounwind readnone
	%ry = tail call float @_ft_expf(float %y) nounwind readnone
	%rz = tail call float @_ft_expf(float %z) nounwind readnone
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
	%rx = tail call float @_ft_expf(float %x) nounwind readnone
	%ry = tail call float @_ft_expf(float %y) nounwind readnone
	%rz = tail call float @_ft_expf(float %z) nounwind readnone
	%rw = tail call float @_ft_expf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @log_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_logf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @log_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_logf(float %x) nounwind readnone
	%ry = tail call float @_ft_logf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @log_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_logf(float %x) nounwind readnone
	%ry = tail call float @_ft_logf(float %y) nounwind readnone
	%rz = tail call float @_ft_logf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @log_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_logf(float %x) nounwind readnone
	%ry = tail call float @_ft_logf(float %y) nounwind readnone
	%rz = tail call float @_ft_logf(float %z) nounwind readnone
	%rw = tail call float @_ft_logf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @exp2_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_exp2f(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @exp2_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_exp2f(float %x) nounwind readnone
	%ry = tail call float @_ft_exp2f(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @exp2_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_exp2f(float %x) nounwind readnone
	%ry = tail call float @_ft_exp2f(float %y) nounwind readnone
	%rz = tail call float @_ft_exp2f(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @exp2_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_exp2f(float %x) nounwind readnone
	%ry = tail call float @_ft_exp2f(float %y) nounwind readnone
	%rz = tail call float @_ft_exp2f(float %z) nounwind readnone
	%rw = tail call float @_ft_exp2f(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @log2_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_log2f(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @log2_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_log2f(float %x) nounwind readnone
	%ry = tail call float @_ft_log2f(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @log2_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_log2f(float %x) nounwind readnone
	%ry = tail call float @_ft_log2f(float %y) nounwind readnone
	%rz = tail call float @_ft_log2f(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @log2_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_log2f(float %x) nounwind readnone
	%ry = tail call float @_ft_log2f(float %y) nounwind readnone
	%rz = tail call float @_ft_log2f(float %z) nounwind readnone
	%rw = tail call float @_ft_log2f(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @sqrt_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @llvm.sqrt.f32(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @sqrt_v2(<2 x float> %a) nounwind readnone {
entry:
	%rv = tail call <2 x float> @llvm.sqrt.v2f32(<2 x float> %a) nounwind readnone
	ret <2 x float> %rv
}

define <3 x float> @sqrt_v3(<3 x float> %a) nounwind readnone {
entry:
	%rv = tail call <3 x float> @llvm.sqrt.v3f32(<3 x float> %a) nounwind readnone
	ret <3 x float> %rv
}

define <4 x float> @sqrt_v4(<4 x float> %a) nounwind readnone {
entry:
	%rv = tail call <4 x float> @llvm.sqrt.v4f32(<4 x float> %a) nounwind readnone
	ret <4 x float> %rv
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @inversesqrt_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_inversesqrtf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @inversesqrt_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_inversesqrtf(float %x) nounwind readnone
	%ry = tail call float @_ft_inversesqrtf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @inversesqrt_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_inversesqrtf(float %x) nounwind readnone
	%ry = tail call float @_ft_inversesqrtf(float %y) nounwind readnone
	%rz = tail call float @_ft_inversesqrtf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @inversesqrt_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_inversesqrtf(float %x) nounwind readnone
	%ry = tail call float @_ft_inversesqrtf(float %y) nounwind readnone
	%rz = tail call float @_ft_inversesqrtf(float %z) nounwind readnone
	%rw = tail call float @_ft_inversesqrtf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @floor_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_floorf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @floor_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_floorf(float %x) nounwind readnone
	%ry = tail call float @_ft_floorf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @floor_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_floorf(float %x) nounwind readnone
	%ry = tail call float @_ft_floorf(float %y) nounwind readnone
	%rz = tail call float @_ft_floorf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @floor_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_floorf(float %x) nounwind readnone
	%ry = tail call float @_ft_floorf(float %y) nounwind readnone
	%rz = tail call float @_ft_floorf(float %z) nounwind readnone
	%rw = tail call float @_ft_floorf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define float @ceil_f(float %a) nounwind readnone {
entry:
	%rv = tail call float @_ft_ceilf(float %a) nounwind readnone
	ret float %rv
}

define <2 x float> @ceil_v2(<2 x float> %a) nounwind readnone {
entry:
	%x = extractelement <2 x float> %a, i32 0
	%y = extractelement <2 x float> %a, i32 1
	%rx = tail call float @_ft_ceilf(float %x) nounwind readnone
	%ry = tail call float @_ft_ceilf(float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @ceil_v3(<3 x float> %a) nounwind readnone {
entry:
	%x = extractelement <3 x float> %a, i32 0
	%y = extractelement <3 x float> %a, i32 1
	%z = extractelement <3 x float> %a, i32 2
	%rx = tail call float @_ft_ceilf(float %x) nounwind readnone
	%ry = tail call float @_ft_ceilf(float %y) nounwind readnone
	%rz = tail call float @_ft_ceilf(float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @ceil_v4(<4 x float> %a) nounwind readnone {
entry:
	%x = extractelement <4 x float> %a, i32 0
	%y = extractelement <4 x float> %a, i32 1
	%z = extractelement <4 x float> %a, i32 2
	%w = extractelement <4 x float> %a, i32 3
	%rx = tail call float @_ft_ceilf(float %x) nounwind readnone
	%ry = tail call float @_ft_ceilf(float %y) nounwind readnone
	%rz = tail call float @_ft_ceilf(float %z) nounwind readnone
	%rw = tail call float @_ft_ceilf(float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; returns minimum of a and b.
define float @min_ff(float %a, float %b) nounwind readnone {
entry:
	%0 = fcmp olt float %a, %b
	%iftmp = select i1 %0, float %a, float %b
	ret float %iftmp
}

define <2 x float> @min_v2f(<2 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <2 x float> %v, i32 0
	%y = extractelement <2 x float> %v, i32 1
	%rx = tail call float @min_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @min_ff(float %scalar, float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @min_v3f(<3 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <3 x float> %v, i32 0
	%y = extractelement <3 x float> %v, i32 1
	%z = extractelement <3 x float> %v, i32 2
	%rx = tail call float @min_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @min_ff(float %scalar, float %y) nounwind readnone
	%rz = tail call float @min_ff(float %scalar, float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @min_v4f(<4 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <4 x float> %v, i32 0
	%y = extractelement <4 x float> %v, i32 1
	%z = extractelement <4 x float> %v, i32 2
	%w = extractelement <4 x float> %v, i32 3
	%rx = tail call float @min_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @min_ff(float %scalar, float %y) nounwind readnone
	%rz = tail call float @min_ff(float %scalar, float %z) nounwind readnone
	%rw = tail call float @min_ff(float %scalar, float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

define <2 x float> @min_v2v2(<2 x float> %a, <2 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <2 x float> %a, i32 0
	%y1 = extractelement <2 x float> %a, i32 1
	%x2 = extractelement <2 x float> %b, i32 0
	%y2 = extractelement <2 x float> %b, i32 1
	%rx = tail call float @min_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @min_ff(float %y1, float %y2) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @min_v3v3(<3 x float> %a, <3 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <3 x float> %a, i32 0
	%y1 = extractelement <3 x float> %a, i32 1
	%z1 = extractelement <3 x float> %a, i32 2
	%x2 = extractelement <3 x float> %b, i32 0
	%y2 = extractelement <3 x float> %b, i32 1
	%z2 = extractelement <3 x float> %b, i32 2
	%rx = tail call float @min_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @min_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @min_ff(float %z1, float %z2) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @min_v4v4(<4 x float> %a, <4 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <4 x float> %a, i32 0
	%y1 = extractelement <4 x float> %a, i32 1
	%z1 = extractelement <4 x float> %a, i32 2
	%w1 = extractelement <4 x float> %a, i32 3
	%x2 = extractelement <4 x float> %b, i32 0
	%y2 = extractelement <4 x float> %b, i32 1
	%z2 = extractelement <4 x float> %b, i32 2
	%w2 = extractelement <4 x float> %b, i32 3
	%rx = tail call float @min_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @min_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @min_ff(float %z1, float %z2) nounwind readnone
	%rw = tail call float @min_ff(float %w1, float %w2) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; returns maximum of a and b.
define float @max_ff(float %a, float %b) nounwind readnone {
entry:
	%0 = fcmp olt float %a, %b
	%iftmp = select i1 %0, float %b, float %a
	ret float %iftmp
}

define <2 x float> @max_v2f(<2 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <2 x float> %v, i32 0
	%y = extractelement <2 x float> %v, i32 1
	%rx = tail call float @max_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @max_ff(float %scalar, float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @max_v3f(<3 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <3 x float> %v, i32 0
	%y = extractelement <3 x float> %v, i32 1
	%z = extractelement <3 x float> %v, i32 2
	%rx = tail call float @max_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @max_ff(float %scalar, float %y) nounwind readnone
	%rz = tail call float @max_ff(float %scalar, float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @max_v4f(<4 x float> %v, float %scalar) nounwind readnone {
entry:
	%x = extractelement <4 x float> %v, i32 0
	%y = extractelement <4 x float> %v, i32 1
	%z = extractelement <4 x float> %v, i32 2
	%w = extractelement <4 x float> %v, i32 3
	%rx = tail call float @max_ff(float %scalar, float %x) nounwind readnone
	%ry = tail call float @max_ff(float %scalar, float %y) nounwind readnone
	%rz = tail call float @max_ff(float %scalar, float %z) nounwind readnone
	%rw = tail call float @max_ff(float %scalar, float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

define <2 x float> @max_v2v2(<2 x float> %a, <2 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <2 x float> %a, i32 0
	%y1 = extractelement <2 x float> %a, i32 1
	%x2 = extractelement <2 x float> %b, i32 0
	%y2 = extractelement <2 x float> %b, i32 1
	%rx = tail call float @max_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @max_ff(float %y1, float %y2) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @max_v3v3(<3 x float> %a, <3 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <3 x float> %a, i32 0
	%y1 = extractelement <3 x float> %a, i32 1
	%z1 = extractelement <3 x float> %a, i32 2
	%x2 = extractelement <3 x float> %b, i32 0
	%y2 = extractelement <3 x float> %b, i32 1
	%z2 = extractelement <3 x float> %b, i32 2
	%rx = tail call float @max_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @max_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @max_ff(float %z1, float %z2) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @max_v4v4(<4 x float> %a, <4 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <4 x float> %a, i32 0
	%y1 = extractelement <4 x float> %a, i32 1
	%z1 = extractelement <4 x float> %a, i32 2
	%w1 = extractelement <4 x float> %a, i32 3
	%x2 = extractelement <4 x float> %b, i32 0
	%y2 = extractelement <4 x float> %b, i32 1
	%z2 = extractelement <4 x float> %b, i32 2
	%w2 = extractelement <4 x float> %b, i32 3
	%rx = tail call float @max_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @max_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @max_ff(float %z1, float %z2) nounwind readnone
	%rw = tail call float @max_ff(float %w1, float %w2) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Returns 0.0 if x < edge, 1.0 otherwise
define float @step_ff(float %edge, float %x) nounwind readnone {
entry:
	%0 = fcmp olt float %x, %edge
	%iftmp = select i1 %0, float 0.0, float 1.0
	ret float %iftmp
}

define <2 x float> @step_fv2(float %edge, <2 x float> %v) nounwind readnone {
entry:
	%x = extractelement <2 x float> %v, i32 0
	%y = extractelement <2 x float> %v, i32 1
	%rx = tail call float @step_ff(float %edge, float %x) nounwind readnone
	%ry = tail call float @step_ff(float %edge, float %y) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @step_fv3(float %edge, <3 x float> %v) nounwind readnone {
entry:
	%x = extractelement <3 x float> %v, i32 0
	%y = extractelement <3 x float> %v, i32 1
	%z = extractelement <3 x float> %v, i32 2
	%rx = tail call float @step_ff(float %edge, float %x) nounwind readnone
	%ry = tail call float @step_ff(float %edge, float %y) nounwind readnone
	%rz = tail call float @step_ff(float %edge, float %z) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @step_fv4(float %edge, <4 x float> %v) nounwind readnone {
entry:
	%x = extractelement <4 x float> %v, i32 0
	%y = extractelement <4 x float> %v, i32 1
	%z = extractelement <4 x float> %v, i32 2
	%w = extractelement <4 x float> %v, i32 3
	%rx = tail call float @step_ff(float %edge, float %x) nounwind readnone
	%ry = tail call float @step_ff(float %edge, float %y) nounwind readnone
	%rz = tail call float @step_ff(float %edge, float %z) nounwind readnone
	%rw = tail call float @step_ff(float %edge, float %w) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

define <2 x float> @step_v2v2(<2 x float> %a, <2 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <2 x float> %a, i32 0
	%y1 = extractelement <2 x float> %a, i32 1
	%x2 = extractelement <2 x float> %b, i32 0
	%y2 = extractelement <2 x float> %b, i32 1
	%rx = tail call float @step_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @step_ff(float %y1, float %y2) nounwind readnone
	%rv1 = insertelement <2 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ry, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @step_v3v3(<3 x float> %a, <3 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <3 x float> %a, i32 0
	%y1 = extractelement <3 x float> %a, i32 1
	%z1 = extractelement <3 x float> %a, i32 2
	%x2 = extractelement <3 x float> %b, i32 0
	%y2 = extractelement <3 x float> %b, i32 1
	%z2 = extractelement <3 x float> %b, i32 2
	%rx = tail call float @step_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @step_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @step_ff(float %z1, float %z2) nounwind readnone
	%rv1 = insertelement <3 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %rz, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @step_v4v4(<4 x float> %a, <4 x float> %b) nounwind readnone {
entry:
	%x1 = extractelement <4 x float> %a, i32 0
	%y1 = extractelement <4 x float> %a, i32 1
	%z1 = extractelement <4 x float> %a, i32 2
	%w1 = extractelement <4 x float> %a, i32 3
	%x2 = extractelement <4 x float> %b, i32 0
	%y2 = extractelement <4 x float> %b, i32 1
	%z2 = extractelement <4 x float> %b, i32 2
	%w2 = extractelement <4 x float> %b, i32 3
	%rx = tail call float @step_ff(float %x1, float %x2) nounwind readnone
	%ry = tail call float @step_ff(float %y1, float %y2) nounwind readnone
	%rz = tail call float @step_ff(float %z1, float %z2) nounwind readnone
	%rw = tail call float @step_ff(float %w1, float %w2) nounwind readnone
	%rv1 = insertelement <4 x float> zeroinitializer, float %rx, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ry, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %rz, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %rw, i32 3
	ret <4 x float> %rv4
}

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

; COLOR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define <4 x float> @unpremultiply_v4(<4 x float> %a) nounwind readnone {
	%alpha = extractelement <4 x float> %a, i32 3
	%iszero = fcmp oeq float zeroinitializer, %alpha
	br i1 %iszero, label %early_out, label %cont
early_out:
	ret <4 x float> zeroinitializer
cont:
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%sampler = type < { 
	<4 x float>*,	; Pointer to data.
	i32,		; Width
	i32		; Height
} >

define <2 x float> @_ft_apply_matrix(<2 x float> %in, <3 x float> %row1,
		<3 x float> %row2) {
	%x = extractelement <2 x float> %in, i32 0
	%y = extractelement <2 x float> %in, i32 1

	%inh1 = insertelement <3 x float> <float 0.0, float 0.0, float 1.0>, float %x, i32 0
	%inh = insertelement <3 x float> %inh1, float %y, i32 1

	%ox = call float @dot_v3v3(<3 x float> %inh, <3 x float> %row1) nounwind readnone
	%oy = call float @dot_v3v3(<3 x float> %inh, <3 x float> %row2) nounwind readnone

	%rv1 = insertelement <2 x float> zeroinitializer, float %ox, i32 0
	%rv = insertelement <2 x float> %rv1, float %oy, i32 1

	ret <2 x float> %rv
}

;; This global is defined by the CPU runtime to be an array
;; of sampler structures.
@_ft_sampler_table = external global [0 x %sampler]

;define <4 x float> @foo_ft_sample(i32 %samp, i32 %x, i32 %y) nounwind readonly {
;	%xf1 = sitofp i32 %x to float
;	%yf1 = sitofp i32 %y to float
;	%xf = fdiv float %xf1, 300.0
;	%yf = fdiv float %yf1, 300.0
;	%rv1 = insertelement <4 x float> <float 0.0, float 0.0, float 0.0, float 1.0>, float %xf, i32 0
;	%rv2 = insertelement <4 x float> %rv1, float %yf, i32 1
;	ret <4 x float> %rv1
;}

define <4 x float> @_ft_sample(i32 %samp, i32 %x, i32 %y) nounwind readonly {
	;; Extract sampler info.
	%pw = getelementptr [0 x %sampler]* @_ft_sampler_table, i32 0,
		i32 %samp, i32 1
	%w = load i32* %pw
	%ph = getelementptr [0 x %sampler]* @_ft_sampler_table, i32 0,
		i32 %samp, i32 2
	%h = load i32* %ph

	;; Early out if we're outside image.
	%xlt0 = icmp slt i32 %x, zeroinitializer
	br i1 %xlt0, label %early_out, label %cont1
cont1:
	%ylt0 = icmp slt i32 %y, zeroinitializer
	br i1 %ylt0, label %early_out, label %cont2
cont2:
	%xgew = icmp sge i32 %x, %w
	br i1 %xgew, label %early_out, label %cont3
cont3:
	%ygeh = icmp sge i32 %y, %h
	br i1 %ygeh, label %early_out, label %cont4
cont4:

	%idx1 = mul i32 %w, %y
	%idx2 = add i32 %idx1, %x

	%pdata = getelementptr [0 x %sampler]* @_ft_sampler_table, i32 0,
		i32 %samp, i32 0
	%data = load <4 x float>** %pdata

	%pval = getelementptr <4 x float>* %data, i32 %idx2
	%val = load <4 x float>* %pval

	ret <4 x float> %val

early_out:
	; Return a transparent border.
	ret <4 x float> <float 0.0, float 0.0, float 0.0, float 0.0>
}

define <4 x float> @nearestSample(i32 %sampidx, <2 x float> %coord) nounwind readonly {
	;; Round image co-ordinate.
	%xf = extractelement <2 x float> %coord, i32 0
	%yf = extractelement <2 x float> %coord, i32 1

	%x = tail call i32 @_ft_floori(float %xf) nounwind readnone
	%y = tail call i32 @_ft_floori(float %yf) nounwind readnone

	%rv = tail call <4 x float> @_ft_sample(i32 %sampidx, i32 %x, i32 %y) nounwind readonly

	ret <4 x float> %rv
}

define <4 x float> @linearSample(i32 %samp_idx, <2 x float> %coord) nounwind readonly {
	;; Round image co-ordinate.
	%xflt = extractelement <2 x float> %coord, i32 0
	%yflt = extractelement <2 x float> %coord, i32 1

	%xf = tail call i32 @_ft_floori(float %xflt) nounwind readnone
	%yf = tail call i32 @_ft_floori(float %yflt) nounwind readnone
	%xc = add i32 %xf, 1
	%yc = add i32 %yf, 1

	%xfflt = sitofp i32 %xf to float
	%yfflt = sitofp i32 %yf to float
	%xcflt = sitofp i32 %xc to float
	%ycflt = sitofp i32 %yc to float

	%dx = sub float %xflt, %xfflt
	%dy = sub float %yflt, %yfflt

	%omdx = sub float 1.0, %dx
	%omdy = sub float 1.0, %dy

	%dxv1 = insertelement <4 x float> zeroinitializer, float %dx, i32 0
	%dxv2 = insertelement <4 x float> %dxv1, float %dx, i32 1
	%dxv3 = insertelement <4 x float> %dxv2, float %dx, i32 2
	%dxv = insertelement <4 x float> %dxv3, float %dx, i32 3

	%dyv1 = insertelement <4 x float> zeroinitializer, float %dy, i32 0
	%dyv2 = insertelement <4 x float> %dyv1, float %dy, i32 1
	%dyv3 = insertelement <4 x float> %dyv2, float %dy, i32 2
	%dyv = insertelement <4 x float> %dyv3, float %dy, i32 3

	%omdxv1 = insertelement <4 x float> zeroinitializer, float %omdx, i32 0
	%omdxv2 = insertelement <4 x float> %omdxv1, float %omdx, i32 1
	%omdxv3 = insertelement <4 x float> %omdxv2, float %omdx, i32 2
	%omdxv = insertelement <4 x float> %omdxv3, float %omdx, i32 3

	%omdyv1 = insertelement <4 x float> zeroinitializer, float %omdy, i32 0
	%omdyv2 = insertelement <4 x float> %omdyv1, float %omdy, i32 1
	%omdyv3 = insertelement <4 x float> %omdyv2, float %omdy, i32 2
	%omdyv = insertelement <4 x float> %omdyv3, float %omdy, i32 3

	%ff = tail call <4 x float> @_ft_sample(i32 %samp_idx,
		i32 %xf, i32 %yf) nounwind readonly
	%cf = tail call <4 x float> @_ft_sample(i32 %samp_idx,
		i32 %xc, i32 %yf) nounwind readonly
	%fc = tail call <4 x float> @_ft_sample(i32 %samp_idx,
		i32 %xf, i32 %yc) nounwind readonly
	%cc = tail call <4 x float> @_ft_sample(i32 %samp_idx,
		i32 %xc, i32 %yc) nounwind readonly

	%y1a = mul <4 x float> %omdyv, %ff
	%y1b = mul <4 x float> %dyv, %fc
	%y1 = add <4 x float> %y1a, %y1b

	%y2a = mul <4 x float> %omdyv, %cf
	%y2b = mul <4 x float> %dyv, %cc
	%y2 = add <4 x float> %y2a, %y2b

	%rva = mul <4 x float> %omdxv, %y1
	%rvb = mul <4 x float> %dxv, %y2
	%rv = add <4 x float> %rva, %rvb

	ret <4 x float> %rv
}

;define <4 x float> @texSample(i32, <2 x float>) nounwind readnone {
;	ret <4 x float> <float 0.5, float 0.0, float 0.5, float 0.5>
;}

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

