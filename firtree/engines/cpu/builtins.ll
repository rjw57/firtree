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

define float @round_to_zero( float ) {
entry:
	%I = fptosi float %0 to i32
	%F = sitofp i32 %I to float
	ret float %F
}

;; Return the floating-point remainder of dividing x by y.  The return 
;; value is x - n * y, where n is the quotient of x / y, rounded towards 
;; zero to an integer.

define float @mod_f( float %x, float %y ) {
entry:
	%nf = fdiv float %x, %y
	%nr = call float @round_to_zero( float %nf )
	%ny = mul float %nr, %y
	%rv = sub float %x, %ny
	ret float %rv
}

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

;; exp() intrinsic function 

;; Mapped by the CPU engine into the standard math library's expf() function.
declare float @exp_f( float ) nounwind readnone;

define <2 x float> @exp_v2( <2 x float> ) {
entry:
	%x = extractelement <2 x float> %0, i32 0
	%y = extractelement <2 x float> %0, i32 1
	%ex = call float @exp_f( float %x )
	%ey = call float @exp_f( float %y )
	%rv1 = insertelement <2 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ey, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @exp_v3( <3 x float> ) {
entry:
	%x = extractelement <3 x float> %0, i32 0
	%y = extractelement <3 x float> %0, i32 1
	%z = extractelement <3 x float> %0, i32 2
	%ex = call float @exp_f( float %x )
	%ey = call float @exp_f( float %y )
	%ez = call float @exp_f( float %z )
	%rv1 = insertelement <3 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %ez, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @exp_v4( <4 x float> ) {
entry:
	%x = extractelement <4 x float> %0, i32 0
	%y = extractelement <4 x float> %0, i32 1
	%z = extractelement <4 x float> %0, i32 2
	%w = extractelement <4 x float> %0, i32 3
	%ex = call float @exp_f( float %x )
	%ey = call float @exp_f( float %y )
	%ez = call float @exp_f( float %z )
	%ew = call float @exp_f( float %w )
	%rv1 = insertelement <4 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %ez, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %ew, i32 3
	ret <4 x float> %rv4
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

define float @dot_v2v2( <2 x float>, <2 x float> ) {
entry:
	%mul_val = mul <2 x float> %0, %1
	%x = extractelement <2 x float> %mul_val, i32 0
	%y = extractelement <2 x float> %mul_val, i32 1
	%rv = add float %x, %y
	ret float %rv
}

define float @dot_v3v3( <3 x float>, <3 x float> ) {
entry:
	%mul_val = mul <3 x float> %0, %1
	%x = extractelement <3 x float> %mul_val, i32 0
	%y = extractelement <3 x float> %mul_val, i32 1
	%z = extractelement <3 x float> %mul_val, i32 2
	%rv1 = add float %x, %y
	%rv2 = add float %rv1, %z
	ret float %rv2
}

define float @dot_v4v4( <4 x float>, <4 x float> ) {
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

;; length() intrinsic function 

define float @length_v2( <2 x float> ) {
entry:
	%dotprod = call float @dot_v2v2( <2 x float> %0, <2 x float> %0 )
	%length = call float @sqrt_f( float %dotprod )
	ret float %length
}

define float @length_v3( <3 x float> ) {
entry:
	%dotprod = call float @dot_v3v3( <3 x float> %0, <3 x float> %0 )
	%length = call float @sqrt_f( float %dotprod )
	ret float %length
}

define float @length_v4( <4 x float> ) {
entry:
	%dotprod = call float @dot_v4v4( <4 x float> %0, <4 x float> %0 )
	%length = call float @sqrt_f( float %dotprod )
	ret float %length
}

;; compare (%0 < 0.f ? %1 : %2) for each element

define float @compare_ffff( float, float, float ) {
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

;; atan() intrinsic function 

;; Mapped by the CPU engine into the standard math library's atanf()/atan2f function.
declare float @atan_f( float ) nounwind readnone;
declare float @atan_ff( float, float ) nounwind readnone;

define <2 x float> @atan_v2( <2 x float> ) {
entry:
	%x = extractelement <2 x float> %0, i32 0
	%y = extractelement <2 x float> %0, i32 1
	%ex = call float @atan_f( float %x )
	%ey = call float @atan_f( float %y )
	%rv1 = insertelement <2 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ey, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @atan_v3( <3 x float> ) {
entry:
	%x = extractelement <3 x float> %0, i32 0
	%y = extractelement <3 x float> %0, i32 1
	%z = extractelement <3 x float> %0, i32 2
	%ex = call float @atan_f( float %x )
	%ey = call float @atan_f( float %y )
	%ez = call float @atan_f( float %z )
	%rv1 = insertelement <3 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %ez, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @atan_v4( <4 x float> ) {
entry:
	%x = extractelement <4 x float> %0, i32 0
	%y = extractelement <4 x float> %0, i32 1
	%z = extractelement <4 x float> %0, i32 2
	%w = extractelement <4 x float> %0, i32 3
	%ex = call float @atan_f( float %x )
	%ey = call float @atan_f( float %y )
	%ez = call float @atan_f( float %z )
	%ew = call float @atan_f( float %w )
	%rv1 = insertelement <4 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %ez, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %ew, i32 3
	ret <4 x float> %rv4
}

define <2 x float> @atan_v2v2( <2 x float>, <2 x float> ) {
entry:
	%Yx = extractelement <2 x float> %0, i32 0
	%Yy = extractelement <2 x float> %0, i32 1
	%Xx = extractelement <2 x float> %1, i32 0
	%Xy = extractelement <2 x float> %1, i32 1
	%ex = call float @atan_ff( float %Yx, float %Xx )
	%ey = call float @atan_ff( float %Yy, float %Xy )
	%rv1 = insertelement <2 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ey, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @atan_v3v3( <3 x float>, <3 x float> ) {
entry:
	%Yx = extractelement <3 x float> %0, i32 0
	%Yy = extractelement <3 x float> %0, i32 1
	%Yz = extractelement <3 x float> %0, i32 2
	%Xx = extractelement <3 x float> %1, i32 0
	%Xy = extractelement <3 x float> %1, i32 1
	%Xz = extractelement <3 x float> %1, i32 2
	%ex = call float @atan_ff( float %Yx, float %Xx )
	%ey = call float @atan_ff( float %Yy, float %Xy )
	%ez = call float @atan_ff( float %Yz, float %Xz )
	%rv1 = insertelement <3 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %ez, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @atan_v4v4( <4 x float>, <4 x float> ) {
entry:
	%Yx = extractelement <4 x float> %0, i32 0
	%Yy = extractelement <4 x float> %0, i32 1
	%Yz = extractelement <4 x float> %0, i32 2
	%Yw = extractelement <4 x float> %0, i32 3
	%Xx = extractelement <4 x float> %1, i32 0
	%Xy = extractelement <4 x float> %1, i32 1
	%Xz = extractelement <4 x float> %1, i32 2
	%Xw = extractelement <4 x float> %1, i32 3
	%ex = call float @atan_ff( float %Yx, float %Xx )
	%ey = call float @atan_ff( float %Yy, float %Xy )
	%ez = call float @atan_ff( float %Yz, float %Xz )
	%ew = call float @atan_ff( float %Yw, float %Xw )
	%rv1 = insertelement <4 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %ez, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %ew, i32 3
	ret <4 x float> %rv4
}

;; max() intrinsic function

define float @max_ff( float, float ) {
entry:
	%leq_flag = fcmp ole float %0, %1
	br i1 %leq_flag, label %last, label %first
first:
	ret float %0
last:
	ret float %1
}

;; min() intrinsic function

define float @min_ff( float, float ) {
entry:
	%leq_flag = fcmp ole float %0, %1
	br i1 %leq_flag, label %first, label %last
first:
	ret float %0
last:
	ret float %1
}

;; sign() intrinsic function

define float @sign_f( float ) {
entry:
	%arg0 = bitcast float %0 to i32
	%signbits = and i32 %arg0, 2147483648 ; Pick off sign bit, constant is 0x80000000
	%one = bitcast float 1.0 to i32
	%rv = or i32 %one, %signbits
	%frv = bitcast i32 %rv to float
	ret float %frv
}

define <2 x float> @sign_v2( <2 x float> ) {
entry:
	%x = extractelement <2 x float> %0, i32 0
	%y = extractelement <2 x float> %0, i32 1
	%ex = call float @sign_f( float %x )
	%ey = call float @sign_f( float %y )
	%rv1 = insertelement <2 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <2 x float> %rv1, float %ey, i32 1
	ret <2 x float> %rv2
}

define <3 x float> @sign_v3( <3 x float> ) {
entry:
	%x = extractelement <3 x float> %0, i32 0
	%y = extractelement <3 x float> %0, i32 1
	%z = extractelement <3 x float> %0, i32 2
	%ex = call float @sign_f( float %x )
	%ey = call float @sign_f( float %y )
	%ez = call float @sign_f( float %z )
	%rv1 = insertelement <3 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <3 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <3 x float> %rv2, float %ez, i32 2
	ret <3 x float> %rv3
}

define <4 x float> @sign_v4( <4 x float> ) {
entry:
	%x = extractelement <4 x float> %0, i32 0
	%y = extractelement <4 x float> %0, i32 1
	%z = extractelement <4 x float> %0, i32 2
	%w = extractelement <4 x float> %0, i32 3
	%ex = call float @sign_f( float %x )
	%ey = call float @sign_f( float %y )
	%ez = call float @sign_f( float %z )
	%ew = call float @sign_f( float %w )
	%rv1 = insertelement <4 x float> zeroinitializer, float %ex, i32 0
	%rv2 = insertelement <4 x float> %rv1, float %ey, i32 1
	%rv3 = insertelement <4 x float> %rv2, float %ez, i32 2
	%rv4 = insertelement <4 x float> %rv3, float %ew, i32 3
	ret <4 x float> %rv4
}

;; abs() intrinsic function

define float @abs_f( float ) {
entry:
	%arg0 = bitcast float %0 to i32
	%arg0abs = and i32 %arg0, 2147483647 ; Set the sign bit to zero (constant is 0x7fffffff)
	%abs = bitcast i32 %arg0abs to float
	ret float %abs
}

define <2 x float> @abs_v2( <2 x float> ) {
entry:
	%arg0 = bitcast <2 x float> %0 to <2 x i32>
	%arg0abs = and <2 x i32> %arg0, < i32 2147483647, i32 2147483647 > 
	%abs = bitcast  <2 x i32> %arg0abs to <2 x float>
	ret <2 x float> %abs
}

define <3 x float> @abs_v3( <3 x float> ) {
entry:
	%arg0 = bitcast <3 x float> %0 to <3 x i32>
	%arg0abs = and <3 x i32> %arg0, < i32 2147483647, i32 2147483647, i32 2147483647 > 
	%abs = bitcast  <3 x i32> %arg0abs to <3 x float>
	ret <3 x float> %abs
}

define <4 x float> @abs_v4( <4 x float> ) {
entry:
	%arg0 = bitcast <4 x float> %0 to <4 x i32>
	%arg0abs = and <4 x i32> %arg0, < i32 2147483647, i32 214748364, i32 2147483647, i32 2147483647 > 
	%abs = bitcast  <4 x i32> %arg0abs to <4 x float>
	ret <4 x float> %abs
}

