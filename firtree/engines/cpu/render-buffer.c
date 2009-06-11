/* This is not source code per-se, it is more lke documentation.
 * This a C-version of the buffer rendering function which is 
 * acually compiled via llvm-gcc. */

#include <stdint.h>

typedef enum {
    FIRTREE_FORMAT_ARGB32					= 0x00, 
    FIRTREE_FORMAT_ARGB32_PREMULTIPLIED		= 0x01, 
    FIRTREE_FORMAT_XRGB32					= 0x02, 
    FIRTREE_FORMAT_ABGR32					= 0x03, 
    FIRTREE_FORMAT_ABGR32_PREMULTIPLIED		= 0x04, 
    FIRTREE_FORMAT_XBGR32					= 0x05, 
    FIRTREE_FORMAT_RGB24					= 0x06,
    FIRTREE_FORMAT_BGR24					= 0x07,

    FIRTREE_FORMAT_LAST
} FirtreeEngineBufferFormat;

typedef float vec2 __attribute__ ((vector_size(8)));
typedef float vec4 __attribute__ ((vector_size(16)));

#define ELEMENT(a,i) (((float*)&(a))[i])

/* this is the function which acually calculates the sampler
 * value. */
extern vec4 sampler_output(vec2 dest_coord);

/* leverage some of our builtins. */
extern vec4 premultiply_v4(vec4);
extern vec4 unpremultiply_v4(vec4);

/* Convert data packed into 32-bit ints in the format
 * 0xAABBGGRR to/from (r,g,b,a) vectors. */
static
vec4 uint32_to_vec_abgr(uint32_t pixel) {
    float r = (float)(pixel & (0xff)) / 255.f;
    float g = (float)((pixel>>8) & (0xff)) / 255.f;
    float b = (float)((pixel>>16) & (0xff)) / 255.f;
    float a = (float)((pixel>>24) & (0xff)) / 255.f;
    vec4 rv = { r, g, b, a};
    return rv;
}

static
uint32_t vec_to_uint32_abgr(vec4 pix_val) {
    uint32_t rv;
    rv = ((uint32_t)(ELEMENT(pix_val, 0) * 255.f) & 0xff);
    rv |= ((uint32_t)(ELEMENT(pix_val, 1) * 255.f) & 0xff) << 8;
    rv |= ((uint32_t)(ELEMENT(pix_val, 2) * 255.f) & 0xff) << 16;
    rv |= ((uint32_t)(ELEMENT(pix_val, 3) * 255.f) & 0xff) << 24;
    return rv;
}

/* Convert data packed into 32-bit ints in the format
 * 0xAARRGGBB to/from (r,g,b,a) vectors. */
static
vec4 uint32_to_vec_argb(uint32_t pixel) {
    float b = (float)(pixel & (0xff)) / 255.f;
    float g = (float)((pixel>>8) & (0xff)) / 255.f;
    float r = (float)((pixel>>16) & (0xff)) / 255.f;
    float a = (float)((pixel>>24) & (0xff)) / 255.f;
    vec4 rv = { r, g, b, a};
    return rv;
}

static
uint32_t vec_to_uint32_argb(vec4 pix_val) {
    uint32_t rv;
    rv = ((uint32_t)(ELEMENT(pix_val, 2) * 255.f) & 0xff);
    rv |= ((uint32_t)(ELEMENT(pix_val, 1) * 255.f) & 0xff) << 8;
    rv |= ((uint32_t)(ELEMENT(pix_val, 0) * 255.f) & 0xff) << 16;
    rv |= ((uint32_t)(ELEMENT(pix_val, 3) * 255.f) & 0xff) << 24;
    return rv;
}

/* Image buffer sampler functions */

#define START_SAMPLE_FUNCTION(name, pix_size)                           \
static                                                                  \
vec4 name(uint8_t* buffer,                                               \
        unsigned int width, unsigned int height, unsigned int stride,   \
        vec2 location)                                                  \
{                                                                       \
    vec4 rv = { 0, 0, 0, 0 };                                           \
    int x = (int)(ELEMENT(location, 0));                                \
    int y = (int)(ELEMENT(location, 1));                                \
    if((x < 0) || (x >= width)) { return rv; }                          \
    if((y < 0) || (y >= height)) { return rv; }                         \
    uint8_t* pixel = buffer + (x * pix_size) + (y * stride);            \
    vec4 pixel_vec = { 0, 0, 0, 0 };

#define END_SAMPLE_FUNCTION                                             \
    return pixel_vec;                                                   \
}  

START_SAMPLE_FUNCTION(sample_image_buffer_argb32, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_vec = premultiply_v4(uint32_to_vec_argb(pixel_val));
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_argb32_pre, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_vec = uint32_to_vec_argb(pixel_val);
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_xrgb32, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_val |= 0xff000000;
    pixel_vec = uint32_to_vec_argb(pixel_val);
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_abgr32, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_vec = premultiply_v4(uint32_to_vec_abgr(pixel_val));
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_abgr32_pre, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_vec = uint32_to_vec_abgr(pixel_val);
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_xbgr32, 4) {
    uint32_t pixel_val = *((uint32_t*)pixel);
    pixel_val |= 0xff000000;
    pixel_vec = uint32_to_vec_abgr(pixel_val);
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_rgb24, 3) {
    uint32_t pixel_val = pixel[0];
    pixel_val |= pixel[1] << 8;
    pixel_val |= pixel[2] << 16;
    pixel_val |= 0xff << 24;
    pixel_vec = uint32_to_vec_argb(pixel_val);
} END_SAMPLE_FUNCTION

START_SAMPLE_FUNCTION(sample_image_buffer_bgr24, 3) {
    uint32_t pixel_val = pixel[0];
    pixel_val |= pixel[1] << 8;
    pixel_val |= pixel[2] << 16;
    pixel_val |= 0xff << 24;
    pixel_vec = uint32_to_vec_abgr(pixel_val);
} END_SAMPLE_FUNCTION

vec4 sample_image_buffer_nn(uint8_t* buffer,
        FirtreeEngineBufferFormat format, 
        unsigned int width, unsigned int height, unsigned int stride,
        vec2 location)
{
    vec4 default_rv = { 1, 0, 0, 1 };
    switch(format) {
        case FIRTREE_FORMAT_ARGB32:
            return sample_image_buffer_argb32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ARGB32_PREMULTIPLIED:
            return sample_image_buffer_argb32_pre(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_XRGB32:
            return sample_image_buffer_xrgb32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ABGR32:
            return sample_image_buffer_abgr32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_ABGR32_PREMULTIPLIED:
            return sample_image_buffer_abgr32_pre(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_XBGR32:
            return sample_image_buffer_xbgr32(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_RGB24:
            return sample_image_buffer_rgb24(buffer, 
                    width, height, stride, location);
        case FIRTREE_FORMAT_BGR24:
            return sample_image_buffer_bgr24(buffer, 
                    width, height, stride, location);
        default:
            /* do nothing */
            break;
    }
    return default_rv;
}

vec4 sample_image_buffer_lerp(uint8_t* buffer,
        FirtreeEngineBufferFormat format, 
        unsigned int width, unsigned int height, unsigned int stride,
        vec2 location)
{
    vec2 offset = {-0.5f, -0.5f};
    location += offset;
    int x = (int)(ELEMENT(location, 0)); /* default rounding is */ 
    int y = (int)(ELEMENT(location, 1)); /* towards zero        */

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
#define START_RENDER_FUNCTION(name, pix_size) \
    void name(unsigned char* buffer,    \
        unsigned int width, unsigned int height,    \
        unsigned int row_stride, float* extents)    \
    {    \
        unsigned int row, col;    \
        float start_x = extents[0];    \
        float y = extents[1];    \
        float dx = extents[2] / (float)width;    \
        float dy = extents[3] / (float)height;    \
    \
        for(row=0; row<height; ++row, y+=dy) {    \
            unsigned char* pixel = buffer + (row * row_stride);    \
            float x = start_x;    \
            for(col=0; col<width; ++col, pixel+=pix_size, x+=dx) {

#define END_RENDER_FUNCTION } } }

START_RENDER_FUNCTION(render_buffer_32_abgr_pre, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = uint32_to_vec_abgr(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_abgr(out_vec);
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_32_abgr_non, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = premultiply_v4(uint32_to_vec_abgr(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_abgr(unpremultiply_v4(out_vec));
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_32_abgr_ign, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    in_val |= (0xff << 24);
    vec4 in_vec = premultiply_v4(uint32_to_vec_abgr(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_abgr(unpremultiply_v4(out_vec));
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_32_argb_pre, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = uint32_to_vec_argb(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_argb(out_vec);
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_32_argb_non, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = premultiply_v4(uint32_to_vec_argb(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_argb(unpremultiply_v4(out_vec));
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_32_argb_ign, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    in_val |= (0xff << 24);
    vec4 in_vec = premultiply_v4(uint32_to_vec_argb(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_argb(unpremultiply_v4(out_vec));
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_24_bgr, 3) {
    uint32_t in_val = pixel[0];
    in_val |= pixel[1] << 8;
    in_val |= pixel[2] << 16;
    in_val |= 0xff << 24;
    vec4 in_vec = uint32_to_vec_abgr(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    uint32_t pix_val = vec_to_uint32_abgr(unpremultiply_v4(out_vec));
    pixel[0] = (pix_val) & 0xff;
    pixel[1] = (pix_val>>8) & 0xff;
    pixel[2] = (pix_val>>16) & 0xff;
} END_RENDER_FUNCTION

START_RENDER_FUNCTION(render_buffer_24_rgb, 3) {
    uint32_t in_val = pixel[0];
    in_val |= pixel[1] << 8;
    in_val |= pixel[2] << 16;
    in_val |= 0xff << 24;
    vec4 in_vec = uint32_to_vec_argb(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    uint32_t pix_val = vec_to_uint32_argb(unpremultiply_v4(out_vec));
    pixel[0] = (pix_val) & 0xff;
    pixel[1] = (pix_val>>8) & 0xff;
    pixel[2] = (pix_val>>16) & 0xff;
} END_RENDER_FUNCTION

/* vim:sw=4:ts=4:cindent:et
 */
