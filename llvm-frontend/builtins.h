/* Builtins defined by library. */

const char* _firtree_builtins =
    /* Angle and Trigonometric Functions. */
    "__builtin__ float radians(float);\n"
    "__builtin__ vec2 radians(vec2);\n"
    "__builtin__ vec3 radians(vec3);\n"
    "__builtin__ vec4 radians(vec4);\n"

    "__builtin__ float degrees(float);\n"
    "__builtin__ vec2 degrees(vec2);\n"
    "__builtin__ vec3 degrees(vec3);\n"
    "__builtin__ vec4 degrees(vec4);\n"

    "__builtin__ float sin(float);\n"
    "__builtin__ vec2 sin(vec2);\n"
    "__builtin__ vec3 sin(vec3);\n"
    "__builtin__ vec4 sin(vec4);\n"

    "__builtin__ float cos(float);\n"
    "__builtin__ vec2 cos(vec2);\n"
    "__builtin__ vec3 cos(vec3);\n"
    "__builtin__ vec4 cos(vec4);\n"

    "__builtin__ float sin_(float);\n"
    "__builtin__ vec2 sin_(vec2);\n"
    "__builtin__ vec3 sin_(vec3);\n"
    "__builtin__ vec4 sin_(vec4);\n"

    "__builtin__ float cos_(float);\n"
    "__builtin__ vec2 cos_(vec2);\n"
    "__builtin__ vec3 cos_(vec3);\n"
    "__builtin__ vec4 cos_(vec4);\n"

    "__builtin__ float tan(float);\n"
    "__builtin__ vec2 tan(vec2);\n"
    "__builtin__ vec3 tan(vec3);\n"
    "__builtin__ vec4 tan(vec4);\n"

    "__builtin__ float tan_(float);\n"
    "__builtin__ vec2 tan_(vec2);\n"
    "__builtin__ vec3 tan_(vec3);\n"
    "__builtin__ vec4 tan_(vec4);\n"

    "__builtin__ vec2 sincos(float);\n"
    "__builtin__ vec2 cossin(float);\n"

    "__builtin__ vec2 sincos_(float);\n"
    "__builtin__ vec2 cossin_(float);\n"

    "__builtin__ float asin(float);\n"
    "__builtin__ vec2 asin(vec2);\n"
    "__builtin__ vec3 asin(vec3);\n"
    "__builtin__ vec4 asin(vec4);\n"

    "__builtin__ float acos(float);\n"
    "__builtin__ vec2 acos(vec2);\n"
    "__builtin__ vec3 acos(vec3);\n"
    "__builtin__ vec4 acos(vec4);\n"

    "__builtin__ float atan(float);\n"
    "__builtin__ vec2 atan(vec2);\n"
    "__builtin__ vec3 atan(vec3);\n"
    "__builtin__ vec4 atan(vec4);\n"

    "__builtin__ float atan(float,float);\n"
    "__builtin__ vec2 atan(vec2,vec2);\n"
    "__builtin__ vec3 atan(vec3,vec3);\n"
    "__builtin__ vec4 atan(vec4,vec4);\n"

  	// Exponential functions

    "__builtin__ float pow(float,float);\n"
    "__builtin__ vec2 pow(vec2,vec2);\n"
    "__builtin__ vec3 pow(vec3,vec3);\n"
    "__builtin__ vec4 pow(vec4,vec4);\n"

    "__builtin__ float exp(float);\n"
    "__builtin__ vec2 exp(vec2);\n"
    "__builtin__ vec3 exp(vec3);\n"
    "__builtin__ vec4 exp(vec4);\n"

    "__builtin__ float log(float);\n"
    "__builtin__ vec2 log(vec2);\n"
    "__builtin__ vec3 log(vec3);\n"
    "__builtin__ vec4 log(vec4);\n"

    "__builtin__ float exp2(float);\n"
    "__builtin__ vec2 exp2(vec2);\n"
    "__builtin__ vec3 exp2(vec3);\n"
    "__builtin__ vec4 exp2(vec4);\n"

    "__builtin__ float log2(float);\n"
    "__builtin__ vec2 log2(vec2);\n"
    "__builtin__ vec3 log2(vec3);\n"
    "__builtin__ vec4 log2(vec4);\n"

	// Square-root functions.
		
    "__builtin__ float sqrt(float);\n"
    "__builtin__ vec2 sqrt(vec2);\n"
    "__builtin__ vec3 sqrt(vec3);\n"
    "__builtin__ vec4 sqrt(vec4);\n"

    "__builtin__ float inversesqrt(float);\n"
    "__builtin__ vec2 inversesqrt(vec2);\n"
    "__builtin__ vec3 inversesqrt(vec3);\n"
    "__builtin__ vec4 inversesqrt(vec4);\n"

	// Common maths functions.

    "__builtin__ float abs(float);\n"
    "__builtin__ vec2 abs(vec2);\n"
    "__builtin__ vec3 abs(vec3);\n"
    "__builtin__ vec4 abs(vec4);\n"

    "__builtin__ float sign(float);\n"
    "__builtin__ vec2 sign(vec2);\n"
    "__builtin__ vec3 sign(vec3);\n"
    "__builtin__ vec4 sign(vec4);\n"

    "__builtin__ float floor(float);\n"
    "__builtin__ vec2 floor(vec2);\n"
    "__builtin__ vec3 floor(vec3);\n"
    "__builtin__ vec4 floor(vec4);\n"

    "__builtin__ float ceil(float);\n"
    "__builtin__ vec2 ceil(vec2);\n"
    "__builtin__ vec3 ceil(vec3);\n"
    "__builtin__ vec4 ceil(vec4);\n"

    "__builtin__ float fract(float);\n"
    "__builtin__ vec2 fract(vec2);\n"
    "__builtin__ vec3 fract(vec3);\n"
    "__builtin__ vec4 fract(vec4);\n"

    "__builtin__ float mod(float,float);\n"
    "__builtin__ vec2 mod(vec2,float);\n"
    "__builtin__ vec3 mod(vec3,float);\n"
    "__builtin__ vec4 mod(vec4,float);\n"
    "__builtin__ vec2 mod(vec2,vec2);\n"
    "__builtin__ vec3 mod(vec3,vec3);\n"
    "__builtin__ vec4 mod(vec4,vec4);\n"

    "__builtin__ float min(float,float);\n"
    "__builtin__ vec2 min(vec2,float);\n"
    "__builtin__ vec3 min(vec3,float);\n"
    "__builtin__ vec4 min(vec4,float);\n"
    "__builtin__ vec2 min(vec2,vec2);\n"
    "__builtin__ vec3 min(vec3,vec3);\n"
    "__builtin__ vec4 min(vec4,vec4);\n"

    "__builtin__ float max(float,float);\n"
    "__builtin__ vec2 max(vec2,float);\n"
    "__builtin__ vec3 max(vec3,float);\n"
    "__builtin__ vec4 max(vec4,float);\n"
    "__builtin__ vec2 max(vec2,vec2);\n"
    "__builtin__ vec3 max(vec3,vec3);\n"
    "__builtin__ vec4 max(vec4,vec4);\n"

    "__builtin__ float clamp(float,float,float);\n"
    "__builtin__ vec2 clamp(vec2,float,float);\n"
    "__builtin__ vec3 clamp(vec3,float,float);\n"
    "__builtin__ vec4 clamp(vec4,float,float);\n"
    "__builtin__ vec2 clamp(vec2,vec2,vec2);\n"
    "__builtin__ vec3 clamp(vec3,vec3,vec3);\n"
    "__builtin__ vec4 clamp(vec4,vec4,vec4);\n"

    "__builtin__ float mix(float,float,float);\n"
    "__builtin__ vec2 mix(vec2,vec2,float);\n"
    "__builtin__ vec3 mix(vec3,vec3,float);\n"
    "__builtin__ vec4 mix(vec4,vec4,float);\n"
    "__builtin__ vec2 mix(vec2,vec2,vec2);\n"
    "__builtin__ vec3 mix(vec3,vec3,vec3);\n"
    "__builtin__ vec4 mix(vec4,vec4,vec4);\n"

    "__builtin__ float step(float,float);\n"
    "__builtin__ vec2 step(float,vec2);\n"
    "__builtin__ vec3 step(float,vec3);\n"
    "__builtin__ vec4 step(float,vec4);\n"
    "__builtin__ vec2 step(vec2,vec2);\n"
    "__builtin__ vec3 step(vec3,vec3);\n"
    "__builtin__ vec4 step(vec4,vec4);\n"

	/* Geometric functions */
	
    "__builtin__ float length(vec2);\n"
    "__builtin__ float length(vec3);\n"
    "__builtin__ float length(vec4);\n"

    "__builtin__ float dot(vec2,vec2);\n"
    "__builtin__ float dot(vec3,vec3);\n"
    "__builtin__ float dot(vec4,vec4);\n"

    "__builtin__ vec3 cross(vec3,vec3);\n"
	
    "__builtin__ vec2 normalize(vec2);\n"
    "__builtin__ vec3 normalize(vec3);\n"
	"__builtin__ vec4 normalize(vec4);\n"

    "__builtin__ vec2 reflect(vec2,vec2);\n"
    "__builtin__ vec3 reflect(vec3,vec3);\n"
    "__builtin__ vec4 reflect(vec4,vec4);\n"

	/* Kernel functions */

	"__builtin__ float compare(float,float,float);\n"
	"__builtin__ vec2 compare(vec2,vec2,vec2);\n"
	"__builtin__ vec3 compare(vec3,vec3,vec3);\n"
	"__builtin__ vec4 compare(vec4,vec4,vec4);\n"

	"__builtin__ vec4 premultiply(vec4);\n"
	"__builtin__ vec4 unpremultiply(vec4);\n"

	/* Sampler functions */

	"__builtin__ vec2 destCoord();\n"

	/* These three functions are special in that they are implicitly
	 * created by the linker. Then LLVM optimisation magic comes in
	 * and makes them go away, inlining the kernels appropriately. */
	"__builtin__ vec2 samplerTransform(static sampler,vec2);\n"
	"__builtin__ static vec4 samplerExtent(static sampler);\n"
	"__builtin__ vec4 sample(static sampler,vec2);\n"

	"vec2 samplerCoord(static sampler s) {\n"
	"    return samplerTransform(s, destCoord());\n"
	"}\n"
	"vec2 samplerOrigin(static sampler s) {\n"
	"  return samplerExtent(s).xy;\n"
	"}\n"
	"vec2 samplerSize(static sampler s) {\n"
	"  return samplerExtent(s).zw;\n"
	"}\n"

	"";

/* vim:sw=4:ts=4:cindent:noet
 */
