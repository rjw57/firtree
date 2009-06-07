/* This is not source code per-se, it is more lke documentation.
 * This a C-version of the buffer rendering function which is 
 * acually read from render-buffer.llvm. */

typedef float vec2 __attribute__ ((vector_size(8)));
typedef float vec4 __attribute__ ((vector_size(16)));

#define ELEMENT(a,i) (((float*)&(a))[i])

/* this is the function which acually calculates the sampler
 * value. */
extern vec4 sample(vec2 dest_coord);

/* this is the function which renders the buffer. */
void render_buffer_uc_4(unsigned char* buffer,
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
            vec2 dest_coord;
            ELEMENT(dest_coord, 0) = x;
            ELEMENT(dest_coord, 1) = y;
            vec4 pix_val = sample(dest_coord);
            pixel[0] = ELEMENT(pix_val, 0) * 255.f;
            pixel[1] = ELEMENT(pix_val, 1) * 255.f;
            pixel[2] = ELEMENT(pix_val, 2) * 255.f;
            pixel[3] = ELEMENT(pix_val, 3) * 255.f;
        }
    }
}

/* vim:sw=4:ts=4:cindent:et
 */
