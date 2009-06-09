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

static
vec4 uint32_to_vec(uint32_t pixel) {
    float r = (float)(pixel & (0xff)) / 255.f;
    float g = (float)((pixel>>8) & (0xff)) / 255.f;
    float b = (float)((pixel>>16) & (0xff)) / 255.f;
    float a = (float)((pixel>>24) & (0xff)) / 255.f;
    vec4 rv = { r, g, b, a};
    return rv;
}

static
uint32_t vec_to_uint32(vec4 pix_val) {
    uint32_t rv;
    rv = ((uint32_t)(ELEMENT(pix_val, 0) * 255.f) & 0xff);
    rv |= ((uint32_t)(ELEMENT(pix_val, 1) * 255.f) & 0xff) << 8;
    rv |= ((uint32_t)(ELEMENT(pix_val, 2) * 255.f) & 0xff) << 16;
    rv |= ((uint32_t)(ELEMENT(pix_val, 3) * 255.f) & 0xff) << 24;
    return rv;
}

/* this is the function which renders the buffer. The buffer is
 * an array of packed 32-bit values which are premultiplied. */
void render_buffer_uc_4_p(unsigned char* buffer,
    unsigned int width, unsigned int height,
    unsigned int row_stride, float* extents)
{
    unsigned int row, col;
    float start_x = extents[0];
    float y = extents[1];
    float dx = extents[2] / (float)width;
    float dy = extents[3] / (float)height;

    for(row=0; row<height; ++row, y+=dy) {
        unsigned char* pixel = buffer + (row * row_stride);
        float x = start_x;
        for(col=0; col<width; ++col, pixel+=4, x+=dx) {
            uint32_t in_val = *((uint32_t*)pixel);
            vec4 in_vec = uint32_to_vec(in_val);
            vec2 dest_coord = {x, y};
            vec4 sample_vec = sampler_output(dest_coord);
            float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
            vec4 one_minus_alpha_vec = {
                one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
            vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
            *((uint32_t*)pixel) = vec_to_uint32(out_vec);
        }
    }
}

/* this is the function which renders the buffer. The buffer is
 * an array of packed 32-bit values which are non-premultiplied. */
void render_buffer_uc_4_np(unsigned char* buffer,
    unsigned int width, unsigned int height,
    unsigned int row_stride, float* extents)
{
    unsigned int row, col;
    float start_x = extents[0];
    float y = extents[1];
    float dx = extents[2] / (float)width;
    float dy = extents[3] / (float)height;

    for(row=0; row<height; ++row, y+=dy) {
        unsigned char* pixel = buffer + (row * row_stride);
        float x = start_x;
        for(col=0; col<width; ++col, pixel+=4, x+=dx) {
            uint32_t in_val = *((uint32_t*)pixel);
            vec4 in_vec = premultiply_v4(uint32_to_vec(in_val));
            vec2 dest_coord = {x, y};
            vec4 sample_vec = sampler_output(dest_coord);
            float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
            vec4 one_minus_alpha_vec = {
                one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
            vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
            *((uint32_t*)pixel) = vec_to_uint32(unpremultiply_v4(out_vec));
        }
    }
}

/* this is the function which renders the buffer. The buffer is
 * an array of packed 8-bit values with the alpha value ignored. */
void render_buffer_uc_3_na(unsigned char* buffer,
    unsigned int width, unsigned int height,
    unsigned int row_stride, float* extents)
{
    unsigned int row, col;
    float start_x = extents[0];
    float y = extents[1];
    float dx = extents[2] / (float)width;
    float dy = extents[3] / (float)height;

    for(row=0; row<height; ++row, y+=dy) {
        unsigned char* pixel = buffer + (row * row_stride);
        float x = start_x;
        for(col=0; col<width; ++col, pixel+=3, x+=dx) {
            uint32_t in_val = pixel[0];
            in_val |= pixel[1] << 8;
            in_val |= pixel[2] << 16;
            in_val |= 0xff << 24;
            vec4 in_vec = uint32_to_vec(in_val);
            vec2 dest_coord = {x, y};
            vec4 sample_vec = sampler_output(dest_coord);
            float one_minus_alpha = 1.f - ELEMENT(sample_vec, 3);
            vec4 one_minus_alpha_vec = {
                one_minus_alpha, one_minus_alpha, one_minus_alpha, one_minus_alpha };
            vec4 out_vec = sample_vec + one_minus_alpha_vec * in_vec;
            uint32_t pix_val = vec_to_uint32(unpremultiply_v4(out_vec));
            pixel[0] = (pix_val) & 0xff;
            pixel[1] = (pix_val>>8) & 0xff;
            pixel[2] = (pix_val>>16) & 0xff;
        }
    }
}

/* vim:sw=4:ts=4:cindent:et
 */
