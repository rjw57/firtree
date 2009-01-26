/* Builtins defined by library. */

const char* _firtree_builtins =
    /* Angle and Trigonometric Functions. */
    "__builtin__ float radians(float);"
    "__builtin__ vec2 radians(vec2);"
    "__builtin__ vec3 radians(vec3);"
    "__builtin__ vec4 radians(vec4);"

    "__builtin__ float degrees(float);"
    "__builtin__ vec2 degrees(vec2);"
    "__builtin__ vec3 degrees(vec3);"
    "__builtin__ vec4 degrees(vec4);"

    "__builtin__ float sin(float);"
    "__builtin__ vec2 sin(vec2);"
    "__builtin__ vec3 sin(vec3);"
    "__builtin__ vec4 sin(vec4);"

    "__builtin__ float cos(float);"
    "__builtin__ vec2 cos(vec2);"
    "__builtin__ vec3 cos(vec3);"
    "__builtin__ vec4 cos(vec4);"

    "__builtin__ float sin_(float);"
    "__builtin__ vec2 sin_(vec2);"
    "__builtin__ vec3 sin_(vec3);"
    "__builtin__ vec4 sin_(vec4);"

    "__builtin__ float cos_(float);"
    "__builtin__ vec2 cos_(vec2);"
    "__builtin__ vec3 cos_(vec3);"
    "__builtin__ vec4 cos_(vec4);"

    "__builtin__ float tan(float);"
    "__builtin__ vec2 tan(vec2);"
    "__builtin__ vec3 tan(vec3);"
    "__builtin__ vec4 tan(vec4);"

    "__builtin__ float tan_(float);"
    "__builtin__ vec2 tan_(vec2);"
    "__builtin__ vec3 tan_(vec3);"
    "__builtin__ vec4 tan_(vec4);"

    "__builtin__ vec2 sincos(float);"
    "__builtin__ vec2 cossin(float);"

    "__builtin__ vec2 sincos_(float);"
    "__builtin__ vec2 cossin_(float);"

    "__builtin__ float asin(float);"
    "__builtin__ vec2 asin(vec2);"
    "__builtin__ vec3 asin(vec3);"
    "__builtin__ vec4 asin(vec4);"

    "__builtin__ float acos(float);"
    "__builtin__ vec2 acos(vec2);"
    "__builtin__ vec3 acos(vec3);"
    "__builtin__ vec4 acos(vec4);"

    "__builtin__ float atan(float);"
    "__builtin__ vec2 atan(vec2);"
    "__builtin__ vec3 atan(vec3);"
    "__builtin__ vec4 atan(vec4);"

    "__builtin__ float atan(float,float);"
    "__builtin__ vec2 atan(vec2,vec2);"
    "__builtin__ vec3 atan(vec3,vec3);"
    "__builtin__ vec4 atan(vec4,vec4);"

  	// Exponential functions

    "__builtin__ float pow(float);"
    "__builtin__ vec2 pow(vec2);"
    "__builtin__ vec3 pow(vec3);"
    "__builtin__ vec4 pow(vec4);"

    "__builtin__ float exp(float);"
    "__builtin__ vec2 exp(vec2);"
    "__builtin__ vec3 exp(vec3);"
    "__builtin__ vec4 exp(vec4);"

    "__builtin__ float log(float);"
    "__builtin__ vec2 log(vec2);"
    "__builtin__ vec3 log(vec3);"
    "__builtin__ vec4 log(vec4);"

    "__builtin__ float exp2(float);"
    "__builtin__ vec2 exp2(vec2);"
    "__builtin__ vec3 exp2(vec3);"
    "__builtin__ vec4 exp2(vec4);"

    "__builtin__ float log2(float);"
    "__builtin__ vec2 log2(vec2);"
    "__builtin__ vec3 log2(vec3);"
    "__builtin__ vec4 log2(vec4);"

	// Square-root functions.
		
    "__builtin__ float sqrt(float);"
    "__builtin__ vec2 sqrt(vec2);"
    "__builtin__ vec3 sqrt(vec3);"
    "__builtin__ vec4 sqrt(vec4);"

    "__builtin__ float inversesqrt(float);"
    "__builtin__ vec2 inversesqrt(vec2);"
    "__builtin__ vec3 inversesqrt(vec3);"
    "__builtin__ vec4 inversesqrt(vec4);"

	// Common maths functions.

    "__builtin__ float abs(float);"
    "__builtin__ vec2 abs(vec2);"
    "__builtin__ vec3 abs(vec3);"
    "__builtin__ vec4 abs(vec4);"

    "__builtin__ float sign(float);"
    "__builtin__ vec2 sign(vec2);"
    "__builtin__ vec3 sign(vec3);"
    "__builtin__ vec4 sign(vec4);"

    "__builtin__ float floor(float);"
    "__builtin__ vec2 floor(vec2);"
    "__builtin__ vec3 floor(vec3);"
    "__builtin__ vec4 floor(vec4);"

    "__builtin__ float ceil(float);"
    "__builtin__ vec2 ceil(vec2);"
    "__builtin__ vec3 ceil(vec3);"
    "__builtin__ vec4 ceil(vec4);"

    "__builtin__ float fract(float);"
    "__builtin__ vec2 fract(vec2);"
    "__builtin__ vec3 fract(vec3);"
    "__builtin__ vec4 fract(vec4);"

    "__builtin__ float mod(float,float);"
    "__builtin__ vec2 mod(vec2,float);"
    "__builtin__ vec3 mod(vec3,float);"
    "__builtin__ vec4 mod(vec4,float);"
    "__builtin__ vec2 mod(vec2,vec2);"
    "__builtin__ vec3 mod(vec3,vec3);"
    "__builtin__ vec4 mod(vec4,vec4);"

    "__builtin__ float min(float,float);"
    "__builtin__ vec2 min(vec2,float);"
    "__builtin__ vec3 min(vec3,float);"
    "__builtin__ vec4 min(vec4,float);"
    "__builtin__ vec2 min(vec2,vec2);"
    "__builtin__ vec3 min(vec3,vec3);"
    "__builtin__ vec4 min(vec4,vec4);"

    "__builtin__ float max(float,float);"
    "__builtin__ vec2 max(vec2,float);"
    "__builtin__ vec3 max(vec3,float);"
    "__builtin__ vec4 max(vec4,float);"
    "__builtin__ vec2 max(vec2,vec2);"
    "__builtin__ vec3 max(vec3,vec3);"
    "__builtin__ vec4 max(vec4,vec4);"

    "__builtin__ float clamp(float,float,float);"
    "__builtin__ vec2 clamp(vec2,float,float);"
    "__builtin__ vec3 clamp(vec3,float,float);"
    "__builtin__ vec4 clamp(vec4,float,float);"
    "__builtin__ vec2 clamp(vec2,vec2,vec2);"
    "__builtin__ vec3 clamp(vec3,vec3,vec3);"
    "__builtin__ vec4 clamp(vec4,vec4,vec4);"

    "__builtin__ float mix(float,float,float);"
    "__builtin__ vec2 mix(vec2,vec2,float);"
    "__builtin__ vec3 mix(vec3,vec3,float);"
    "__builtin__ vec4 mix(vec4,vec4,float);"
    "__builtin__ vec2 mix(vec2,vec2,vec2);"
    "__builtin__ vec3 mix(vec3,vec3,vec3);"
    "__builtin__ vec4 mix(vec4,vec4,vec4);"

    "__builtin__ float step(float,float);"
    "__builtin__ vec2 step(float,vec2);"
    "__builtin__ vec3 step(float,vec3);"
    "__builtin__ vec4 step(float,vec4);"
    "__builtin__ vec2 step(vec2,vec2);"
    "__builtin__ vec3 step(vec3,vec3);"
    "__builtin__ vec4 step(vec4,vec4);"

	/* Geometric functions */
	
    "__builtin__ float length(float);"
    "__builtin__ float length(vec2);"
    "__builtin__ float length(vec3);"
    "__builtin__ float length(vec4);"

    "__builtin__ float dot(vec2,vec2);"
    "__builtin__ float dot(vec3,vec3);"
    "__builtin__ float dot(vec4,vec4);"

    "__builtin__ vec3 cross(vec3,vec3);"
	
    "__builtin__ float normalize(float);"
    "__builtin__ vec2 normalize(vec2);"
    "__builtin__ vec3 normalize(vec3);"
    "__builtin__ vec4 normalize(vec4);"

	/* Kernel functions */

	"__builtin__ float compare(float,float,float);"
	"__builtin__ vec2 compare(vec2,vec2,vec2);"
	"__builtin__ vec3 compare(vec3,vec3,vec3);"
	"__builtin__ vec4 compare(vec4,vec4,vec4);"

	"__builtin__ vec4 premultiply(vec4);"
	"__builtin__ vec4 unpremultiply(vec4);"

	/* Sampler functions */
	
    "__builtin__ vec2 destCoord();"
    "__builtin__ vec2 samplerCoord(static sampler);"
    "__builtin__ vec2 samplerTransform(static sampler,vec2);"
    "__builtin__ static vec2 samplerOrigin(static sampler);"
    "__builtin__ static vec2 samplerSize(static sampler);"
    "__builtin__ static vec4 samplerExtent(static sampler);"
    "__builtin__ vec4 sample(static sampler,vec2);"

	"";

/* vim:sw=4:ts=4:cindent:noet
 */
