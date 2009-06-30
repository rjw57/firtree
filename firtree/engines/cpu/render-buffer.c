/* This is not source code per-se, it is more lke documentation.
 * This a C-version of the buffer rendering function which is 
 * acually compiled via llvm-gcc. */

#include <stdint.h>
#include <firtree/firtree-types.h>

/* Vector types */
typedef float vec2 __attribute__ ((vector_size(8)));
typedef float vec4 __attribute__ ((vector_size(16)));

/* Extract an element from a vector. */
#define ELEMENT(a,i) (((float*)&(a))[i])

/* this is the function which acually calculates the sampler
 * value. */
extern vec4 sampler_output(vec2 dest_coord);

/* leverage some of our builtins. */
extern vec4 premultiply_v4(vec4);
extern vec4 unpremultiply_v4(vec4);

/* Unpack a pixel from a memory location into a vector. */
G_INLINE_FUNC
vec4 unpack_pixel(void* p, FirtreeBufferFormat format)
{
    /* The components */
    guint8 x=0, y=0, z=0, w=0;

    switch(format) {
        case FIRTREE_FORMAT_RGB24:
        case FIRTREE_FORMAT_BGR24:
            /* 24-bit value */
            x = ((guint8*)p)[0];
            y = ((guint8*)p)[1];
            z = ((guint8*)p)[2];
            w = 0xff;
            break;

        default: {
            /* 32-bit value */

            /* Load the pixel into a uint32 */
            guint32 pix_val = *((guint32*)p);

#           if G_BYTE_ORDER == G_LITTLE_ENDIAN
                /* value is 0xWWZZYYXX */
                x = pix_val & 0xff;
                y = (pix_val>>8) & 0xff;
                z = (pix_val>>16) & 0xff;
                w = (pix_val>>24) & 0xff;
#           elif G_BYTE_ORDER == G_BIG_ENDIAN
                /* value is 0xXXYYZZWW */
                w = pix_val & 0xff;
                z = (pix_val>>8) & 0xff;
                y = (pix_val>>16) & 0xff;
                x = (pix_val>>24) & 0xff;
#           else
#               error Unknown endian
#           endif
                 }
    }

    float fx = (float)x * (1.f/255.f);
    float fy = (float)y * (1.f/255.f);
    float fz = (float)z * (1.f/255.f);
    float fw = (float)w * (1.f/255.f);

    switch(format) {
        case FIRTREE_FORMAT_ARGB32: {
            vec4 rv = { fy, fz, fw, fx }; /* rgba */
            return premultiply_v4(rv); }
        case FIRTREE_FORMAT_ARGB32_PREMULTIPLIED: {
            vec4 rv = { fy, fz, fw, fx }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_XRGB32: {
            vec4 rv = { fy, fz, fw, 1.f }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_RGBA32: {
            vec4 rv = { fx, fy, fz, fw }; /* rgba */
            return premultiply_v4(rv); }
        case FIRTREE_FORMAT_RGBA32_PREMULTIPLIED: {
            vec4 rv = { fx, fy, fz, fw }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_ABGR32: {
            vec4 rv = { fw, fz, fy, fx }; /* rgba */
            return premultiply_v4(rv); }
        case FIRTREE_FORMAT_ABGR32_PREMULTIPLIED: {
            vec4 rv = { fw, fz, fy, fx }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_XBGR32: {
            vec4 rv = { fw, fz, fy, 1.f }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_BGRA32: {
            vec4 rv = { fz, fy, fx, fw }; /* rgba */
            return premultiply_v4(rv); }
        case FIRTREE_FORMAT_BGRA32_PREMULTIPLIED: {
            vec4 rv = { fz, fy, fx, fw }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_RGB24: {
            vec4 rv = { fx, fy, fz, 1.f }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_BGR24: {
            vec4 rv = { fz, fy, fx, 1.f }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_RGBX32: {
            vec4 rv = { fx, fy, fz, 1.f }; /* rgba */
            return rv; }
        case FIRTREE_FORMAT_BGRX32: {
            vec4 rv = { fz, fy, fx, 1.f }; /* rgba */
            return rv; }
    }

    vec4 zero = {0,0,0,0};
    return zero;
}

/* Unpack a pixel from a memory location into a vector. */
G_INLINE_FUNC
void pack_pixel(vec4 pixel, void* p, FirtreeBufferFormat format)
{
    /* The components */
    guint8 r=0, g=0, b=0, a=0, pr=0, pg=0, pb=0;
    pr = (gint)(ELEMENT(pixel, 0) * 255.f) & 0xff;
    pg = (gint)(ELEMENT(pixel, 1) * 255.f) & 0xff;
    pb = (gint)(ELEMENT(pixel, 2) * 255.f) & 0xff;
    r = (gint)((ELEMENT(pixel, 0) / ELEMENT(pixel, 3)) * 255.f) & 0xff;
    g = (gint)((ELEMENT(pixel, 1) / ELEMENT(pixel, 3)) * 255.f) & 0xff;
    b = (gint)((ELEMENT(pixel, 2) / ELEMENT(pixel, 3)) * 255.f) & 0xff;
    a = (gint)(ELEMENT(pixel, 3) * 255.f) & 0xff;

    guint8 b1=0, b2=0, b3=0, b4=0;

    switch(format) {
        case FIRTREE_FORMAT_ARGB32:
        case FIRTREE_FORMAT_XRGB32:
            b2= r; b3= g; b4= b;
            break;
        case FIRTREE_FORMAT_ARGB32_PREMULTIPLIED:
            b1= a; b2=pr; b3=pg; b4=pb;
            break;
        case FIRTREE_FORMAT_RGBA32:
            b1= r; b2= g; b3= b; b4= a;
            break;
        case FIRTREE_FORMAT_RGBA32_PREMULTIPLIED:
            b1=pr; b2=pg; b3=pb; b4= a;
            break;
        case FIRTREE_FORMAT_ABGR32:
        case FIRTREE_FORMAT_XBGR32:
            b2= b; b3= g; b4= r;
            break;
        case FIRTREE_FORMAT_ABGR32_PREMULTIPLIED:
            b1= a; b2=pb; b3=pg; b4=pr;
            break;
        case FIRTREE_FORMAT_BGRA32:
            b1= b; b2= g; b3= r; b4= a;
            break;
        case FIRTREE_FORMAT_BGRA32_PREMULTIPLIED:
            b1=pb; b2=pg; b3=pr; b4= a;
            break;
        case FIRTREE_FORMAT_RGB24:
            b1=r; b2=g; b3=b;
            break;
        case FIRTREE_FORMAT_BGR24:
            b1=b; b2=g; b3=r;
            break;
        case FIRTREE_FORMAT_RGBX32: 
            b1=r; b2=g; b3=b;
            break;
        case FIRTREE_FORMAT_BGRX32: 
            b1=b; b2=g; b3=r;
            break;
    }

    switch(format) {
        case FIRTREE_FORMAT_RGB24:
        case FIRTREE_FORMAT_BGR24:
            /* 24-bit value */
            ((guint8*)p)[0] = b1;
            ((guint8*)p)[1] = b2;
            ((guint8*)p)[2] = b3;
            break;

        default: {
            /* 32-bit value */
            guint32 pix_val = 0;

#           if G_BYTE_ORDER == G_LITTLE_ENDIAN
                /* value is 0x44332211 */
                pix_val = b1;
                pix_val |= ((guint32)b2)<<8;
                pix_val |= ((guint32)b3)<<16;
                pix_val |= ((guint32)b4)<<24;
#           elif G_BYTE_ORDER == G_BIG_ENDIAN
                /* value is 0x11223344 */
                pix_val = b4;
                pix_val |= ((guint32)b3)<<8;
                pix_val |= ((guint32)b2)<<16;
                pix_val |= ((guint32)b1)<<24;
#           else
#               error Unknown endian
#           endif

            /* Load the pixel into a uint32 */
            *((guint32*)p) = pix_val;
                 }
    }
}

/* Image buffer sampler functions */
#define SAMPLE_FUNCTION(pix_size, format)                               \
G_INLINE_FUNC                                                           \
vec4 sample_##format(uint8_t* buffer,                                   \
        unsigned int width, unsigned int height, unsigned int stride,   \
        vec2 location)                                                  \
{                                                                       \
    vec4 rv = { 0, 0, 0, 0 };                                           \
    int x = (int)(ELEMENT(location, 0));                                \
    int y = (int)(ELEMENT(location, 1));                                \
    if(ELEMENT(location, 0) < 0.f) { x--; } /* implement */             \
    if(ELEMENT(location, 1) < 0.f) { y--; } /* floor()   */             \
    if((x < 0) || (x >= width)) { return rv; }                          \
    if((y < 0) || (y >= height)) { return rv; }                         \
    uint8_t* pixel = buffer + (x * pix_size) + (y * stride);            \
    vec4 pixel_vec = { 0, 0, 0, 0 };                                    \
    pixel_vec = unpack_pixel(pixel, format);                            \
    return pixel_vec;                                                   \
}  

SAMPLE_FUNCTION(4, FIRTREE_FORMAT_ARGB32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_ARGB32_PREMULTIPLIED)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_XRGB32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_RGBA32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_RGBA32_PREMULTIPLIED)

SAMPLE_FUNCTION(4, FIRTREE_FORMAT_ABGR32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_ABGR32_PREMULTIPLIED)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_XBGR32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_BGRA32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_BGRA32_PREMULTIPLIED)

SAMPLE_FUNCTION(3, FIRTREE_FORMAT_RGB24)
SAMPLE_FUNCTION(3, FIRTREE_FORMAT_BGR24)

SAMPLE_FUNCTION(4, FIRTREE_FORMAT_RGBX32)
SAMPLE_FUNCTION(4, FIRTREE_FORMAT_BGRX32)

vec4 sample_image_buffer_nn(uint8_t* buffer,
        FirtreeBufferFormat format, 
        unsigned int width, unsigned int height, unsigned int stride,
        vec2 location)
{
    vec4 default_rv = { 1, 0, 0, 1 };
    switch(format) {
        case FIRTREE_FORMAT_ARGB32:
            return sample_FIRTREE_FORMAT_ARGB32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ARGB32_PREMULTIPLIED:
            return sample_FIRTREE_FORMAT_ARGB32_PREMULTIPLIED(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_XRGB32:
            return sample_FIRTREE_FORMAT_XRGB32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_RGBA32:
            return sample_FIRTREE_FORMAT_RGBA32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_RGBA32_PREMULTIPLIED:
            return sample_FIRTREE_FORMAT_RGBA32_PREMULTIPLIED(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ABGR32:
            return sample_FIRTREE_FORMAT_ABGR32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ABGR32_PREMULTIPLIED:
            return sample_FIRTREE_FORMAT_ABGR32_PREMULTIPLIED(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_XBGR32:
            return sample_FIRTREE_FORMAT_XBGR32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_BGRA32:
            return sample_FIRTREE_FORMAT_BGRA32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_BGRA32_PREMULTIPLIED:
            return sample_FIRTREE_FORMAT_BGRA32_PREMULTIPLIED(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_RGB24:
            return sample_FIRTREE_FORMAT_RGB24(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_BGR24:
            return sample_FIRTREE_FORMAT_BGR24(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_RGBX32:
            return sample_FIRTREE_FORMAT_RGBX32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_BGRX32:
            return sample_FIRTREE_FORMAT_BGRX32(buffer, 
                    width, height, stride, location);
        default:
            /* do nothing */
            break;
    }
    return default_rv;
}

vec4 sample_image_buffer_lerp(uint8_t* buffer,
        FirtreeBufferFormat format, 
        unsigned int width, unsigned int height, unsigned int stride,
        vec2 location)
{
    vec2 offset = {-0.5f, -0.5f};
    location += offset;
    int x = (int)(ELEMENT(location, 0)); /* default rounding is */ 
    int y = (int)(ELEMENT(location, 1)); /* towards zero        */

    /* Correct rounding to rounding down. */
    if(ELEMENT(location, 0) < 0.f) { x--; }
    if(ELEMENT(location, 1) < 0.f) { y--; }

    vec2 bl_loc = { x, y };
    vec4 bl = sample_image_buffer_nn(buffer, format, width, height,
            stride, bl_loc);
    vec2 br_loc = { x+1, y };
    vec4 br = sample_image_buffer_nn(buffer, format, width, height,
            stride, br_loc);
    vec2 tl_loc = { x, y+1 };
    vec4 tl = sample_image_buffer_nn(buffer, format, width, height,
            stride, tl_loc);
    vec2 tr_loc = { x+1, y+1 };
    vec4 tr = sample_image_buffer_nn(buffer, format, width, height,
            stride, tr_loc);

    float lambda_x = ELEMENT(location, 0) - (float)x;
    float lambda_y = ELEMENT(location, 1) - (float)y;

    if(lambda_x<0.f) { lambda_x = -lambda_x; }
    if(lambda_y<0.f) { lambda_y = -lambda_y; }

    vec4 one = { 1, 1, 1, 1 };
    vec4 lambda_x_vec = { lambda_x, lambda_x, lambda_x, lambda_x };
    vec4 lambda_y_vec = { lambda_y, lambda_y, lambda_y, lambda_y };

    vec4 at = lambda_x_vec * tr + (one - lambda_x_vec) * tl;
    vec4 ab = lambda_x_vec * br + (one - lambda_x_vec) * bl;

    vec4 rv = lambda_y_vec * at + (one - lambda_y_vec) * ab;

    return rv;
}

/* Macros to make writing rendering functions easier */
#define RENDER_FUNCTION(pix_size, format)                               \
void render_##format(unsigned char* buffer,                             \
        unsigned int width, unsigned int height,                        \
        unsigned int row_stride, float* extents)                        \
{                                                                       \
    unsigned int row, col;                                              \
    float start_x = extents[0];                                         \
    float y = extents[1];                                               \
    float dx = extents[2] / (float)width;                               \
    float dy = extents[3] / (float)height;                              \
    start_x += 0.5f*dx; y += 0.5f*dy;                                   \
    for(row=0; row<height; ++row, y+=dy) {                              \
        unsigned char* pixel = buffer + (row * row_stride);             \
        float x = start_x;                                              \
        for(col=0; col<width; ++col, pixel+=pix_size, x+=dx) {          \
            vec4 in_vec = unpack_pixel(pixel, format);                  \
            vec2 dest_coord = {x, y};                                   \
            vec4 sample_vec = sampler_output(dest_coord);               \
            float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);       \
            vec4 one_minus_alpha_vec = {                                \
                one_minus_alpha, one_minus_alpha,                       \
                one_minus_alpha, one_minus_alpha };                     \
            vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;   \
            pack_pixel(out_vec, pixel, format);                         \
        }                                                               \
    }                                                                   \
}                                                                       \

RENDER_FUNCTION(4, FIRTREE_FORMAT_ARGB32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_ARGB32_PREMULTIPLIED)
RENDER_FUNCTION(4, FIRTREE_FORMAT_XRGB32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_RGBA32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_RGBA32_PREMULTIPLIED)

RENDER_FUNCTION(4, FIRTREE_FORMAT_ABGR32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_ABGR32_PREMULTIPLIED)
RENDER_FUNCTION(4, FIRTREE_FORMAT_XBGR32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_BGRA32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_BGRA32_PREMULTIPLIED)

RENDER_FUNCTION(3, FIRTREE_FORMAT_RGB24)
RENDER_FUNCTION(3, FIRTREE_FORMAT_BGR24)

RENDER_FUNCTION(4, FIRTREE_FORMAT_RGBX32)
RENDER_FUNCTION(4, FIRTREE_FORMAT_BGRX32)

/* vim:sw=4:ts=4:cindent:et
 */
