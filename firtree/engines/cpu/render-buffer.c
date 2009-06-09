/* This is not source code per-se, it is more lke documentation.
 * This a C-version of the buffer rendering function which is 
 * acually compiled via llvm-gcc. */

#include <stdint.h>

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

/* Macros to make writing rendering functions easier */
#define START_FUNCTION(name, pix_size) \
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

#define END_FUNCTION } } }

START_FUNCTION(render_buffer_32_abgr_pre, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = uint32_to_vec_abgr(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_abgr(out_vec);
} END_FUNCTION

START_FUNCTION(render_buffer_32_abgr_non, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = premultiply_v4(uint32_to_vec_abgr(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_abgr(unpremultiply_v4(out_vec));
} END_FUNCTION

START_FUNCTION(render_buffer_32_abgr_ign, 4) {
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
} END_FUNCTION

START_FUNCTION(render_buffer_32_argb_pre, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = uint32_to_vec_argb(in_val);
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_argb(out_vec);
} END_FUNCTION

START_FUNCTION(render_buffer_32_argb_non, 4) {
    uint32_t in_val = *((uint32_t*)pixel);
    vec4 in_vec = premultiply_v4(uint32_to_vec_argb(in_val));
    vec2 dest_coord = {x, y};
    vec4 sample_vec = sampler_output(dest_coord);
    float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
    vec4 one_minus_alpha_vec = {
        one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
    vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
    *((uint32_t*)pixel) = vec_to_uint32_argb(unpremultiply_v4(out_vec));
} END_FUNCTION

START_FUNCTION(render_buffer_32_argb_ign, 4) {
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
} END_FUNCTION

START_FUNCTION(render_buffer_24_bgr, 3) {
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
} END_FUNCTION

START_FUNCTION(render_buffer_24_rgb, 3) {
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
} END_FUNCTION

/* vim:sw=4:ts=4:cindent:et
 */
