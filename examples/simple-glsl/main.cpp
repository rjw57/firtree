/* 
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * libcga is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libcga is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "common.h"

#include <firtree/opengl.h>
#include <firtree/main.h>
#include <firtree/image.h>

#include <firtree/glsl-runtime.h>

#include <stdlib.h>
#include <string.h>

using namespace Firtree;

GLuint g_LenaTexture = 0;
GLuint g_FogTexture = 0;

const char* g_CheckerKernelSource = 
"    kernel vec4 checkerKernel(float squareSize, __color backColor,"
"                              __color foreColor)"
"    {"
"        vec2 dc = mod(destCoord(), squareSize*2.0);"
"        vec2 discriminant = step(squareSize, dc);"
"        float flag = discriminant.x + discriminant.y - "
"                     2.0*discriminant.x*discriminant.y;"
"        return mix(backColor, foreColor, flag);"
"    }"
;

Kernel* g_CheckerKernel = Kernel::CreateFromSource(g_CheckerKernelSource);

const char* g_SpotKernelSource = 
"    kernel vec4 spotKernel(float dotPitch, __color backColor,"
"                           __color dotColor)"
"    {"
"        vec2 dc = mod(destCoord(), dotPitch) - 0.5*dotPitch;"
"        float discriminant = clamp(length(dc) - 0.3*dotPitch - 0.5, 0.0, 1.0);"
"        return mix(dotColor, backColor, discriminant);"
"    }"
;

Kernel* g_SpotKernel = Kernel::CreateFromSource(g_SpotKernelSource);


const char* g_OverKernelSource = 
"    kernel vec4 compositeOver(sampler a, sampler b, float phase)"
"    {"
"        vec4 aOut = sample(a, samplerCoord(a));"
"        vec4 bOut = sample(b, samplerCoord(b));"
"        return aOut + bOut * (1.0 - aOut.a);"
"    }"
;

Kernel* g_OverKernel = Kernel::CreateFromSource(g_OverKernelSource);

const char* g_RippleKernelSource = 
"    kernel vec4 ripple(sampler a, float phase)"
"    {"
"        vec2 center = vec2(320, 240);"
"        vec3 params = vec3(0.1 /* freq */, phase, 100 /* sigma */);"
"        vec2 delta = destCoord() - center;"
"        float dist = length(delta);"
"        float exponentialTerm = exp(-0.5 * dist*dist / (params.z*params.z));"
"        float heightNorm = 50.0;"
"        float heightDifferential = 0.0;"
"        heightDifferential += params.x*cos(params.x*dist+params.y) *"
"              exponentialTerm;"
"        vec3 normal = normalize(vec3(2.0*delta*heightDifferential, heightNorm));"

"        vec3 incident = vec3(0,0,-1);"
"        vec3 outDir = normalize(reflect(incident, normal*0.5));"
"        delta = 40.0 * outDir.xy;"
"        float specular = max(0.0, dot(normal, normalize(vec3(1,-1,0.1))));"
"        return sample(a, samplerTransform(a, destCoord() + delta)) + "
"               vec4(specular, specular, specular, 0.0);"
"    }"
;

Kernel* g_RippleKernel = Kernel::CreateFromSource(g_RippleKernelSource);

const char* g_GradientKernelSource = 
"    kernel vec4 gradientKernel()"
"    {"
"        float lambda = clamp(destCoord().x / 600.0, 0.0, 1.0);"
"        return mix(vec4(0,0,0,0), vec4(0,1,0,1), lambda);"
"    }"
;

Kernel* g_GradientKernel = Kernel::CreateFromSource(g_GradientKernelSource);

Kernel* g_OverKernel2 = Kernel::CreateFromSource(g_OverKernelSource);

Image* g_LenaImage = NULL;
Image* g_OverImage2 = NULL;
Image* g_GradientImage = NULL;
Image* g_RippleImage = NULL;
Image* g_OverImage = NULL;
Image* g_CheckerImage = NULL;
Image* g_SpotImage = NULL;

Image* g_GlobalImage = NULL;

GLRenderer* g_GLRenderingContext = NULL;

GLenum g_FragShaderObj;
GLuint g_ShaderProg;

char g_ProgramPath[4096];

void initialise_test(int *argc, char **argv, int window)
{
    strncpy(g_ProgramPath, argv[0], 4096);

    int i = strlen(g_ProgramPath) - 1;
    if(i >= 0)
    {
        for(; i>0; i--)
        {
            if(g_ProgramPath[i] == '/')
            {
                g_ProgramPath[i] = '\0';
                break;
            }
        }
    }

    if(i < 0) 
    {
        g_ProgramPath[0] = '\0';
    }

    printf("Prog path: %s\n", g_ProgramPath);
}

void finalise_test()
{
    printf("Cleaning up...\n");

    FIRTREE_SAFE_RELEASE(g_RippleKernel);
    FIRTREE_SAFE_RELEASE(g_SpotKernel);
    FIRTREE_SAFE_RELEASE(g_GradientImage);
    FIRTREE_SAFE_RELEASE(g_CheckerImage);
    FIRTREE_SAFE_RELEASE(g_OverKernel2);
    FIRTREE_SAFE_RELEASE(g_RippleImage);
    FIRTREE_SAFE_RELEASE(g_SpotImage);
    FIRTREE_SAFE_RELEASE(g_OverImage);
        
    FIRTREE_SAFE_RELEASE(g_GlobalImage);

    FIRTREE_SAFE_RELEASE(g_GLRenderingContext);

    printf("Allocated object count at exit (should be zero): %lu\n",
            ReferenceCounted::GetGlobalObjectCount());
}

void key_pressed(unsigned char key, int x, int y)
{
    printf("KEYPRESS: %i\n", key);

    if(key == 100 /* 'd' */)
    {
        static int flag = 0;

        if(flag)
        {
            g_RippleKernel->SetValueForKey(g_OverImage, "a");
        } else {
            g_RippleKernel->SetValueForKey(g_SpotImage, "a");
        }

        flag = ~flag;
    }

    if(key == 115 /* 's' */)
    {
        static int flag = 0;

        if(flag)
        {
            // Swap checkerborad for gradient
            g_OverKernel2->SetValueForKey(g_GradientImage, "b");
        } else {
            // Swap gradient for checkerboard
            g_OverKernel2->SetValueForKey(g_CheckerImage, "b");
        }

        flag = ~flag;
    }
}

void render(float epoch)
{
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    try {
        g_SpotKernel->SetValueForKey(10.f * (1.0f + (float)sin(0.01f*epoch)) + 30.f,
                "dotPitch");
 //       g_SpotKernel->SetValueForKey(30.f, "dotPitch");

        g_RippleKernel->SetValueForKey(-epoch * 0.1f, "phase");

        Rect2D outQuad(0,0,width,height);
        g_GLRenderingContext->RenderAtPoint(g_GlobalImage,
                Point2D(0,0), outQuad);
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
        throw e;
    }
}

void initialize_kernels()
{
    try {
        // All colors have pre-multiplied alpha.
        static float squareColor[] = {0.75, 0.75, 0.75, 0.75};
        static float backColor[] = {0.25, 0.25, 0.25, 0.25};
        static float dotColor[] = {0.7, 0.0, 0.0, 0.7};
        static float clearColor[] = {0.0, 0.0, 0.0, 0.0};

        Image* tmpim = NULL;
        AffineTransform* t = NULL;

        g_LenaImage = NULL;
        g_GlobalImage = NULL;

        tmpim = Image::CreateFromKernel(g_OverKernel2);
        t = AffineTransform::RotationByDegrees(-20.f);
        t->TranslateBy(0,150);
        g_OverImage2 = Image::CreateFromImageWithTransform(tmpim, t);
        FIRTREE_SAFE_RELEASE(t);

        g_GlobalImage = g_OverImage2;
        g_GlobalImage->Retain();

        FIRTREE_SAFE_RELEASE(tmpim);

        g_GradientImage = Image::CreateFromKernel(g_GradientKernel);

        g_RippleImage = Image::CreateFromKernel(g_RippleKernel);

        g_OverImage = Image::CreateFromKernel(g_OverKernel);

        tmpim = Image::CreateFromKernel(g_CheckerKernel);
        t = AffineTransform::RotationByDegrees(30.f);
        g_CheckerImage = Image::CreateFromImageWithTransform(tmpim, t);
        FIRTREE_SAFE_RELEASE(t);
        FIRTREE_SAFE_RELEASE(tmpim);

        g_SpotImage = Image::CreateFromKernel(g_SpotKernel);

        g_CheckerKernel->SetValueForKey(10.f, "squareSize");
        g_CheckerKernel->SetValueForKey(backColor, 4, "backColor");
        g_CheckerKernel->SetValueForKey(squareColor, 4, "foreColor");

        g_SpotKernel->SetValueForKey(dotColor, 4, "dotColor");
        g_SpotKernel->SetValueForKey(clearColor, 4, "backColor");

        char fileName[4096];
        snprintf(fileName, 4096, "%s/%s", g_ProgramPath, "../lena.png");
        Image* lenaImage = Image::CreateFromFile(fileName);
        //printf("Created lena: %p\n", lenaImage);

        snprintf(fileName, 4096, "%s/%s", g_ProgramPath, "../fog.png");
        Image* fogImage = Image::CreateFromFile(fileName);
        //printf("Created foo: %p\n", fogImage);

        AffineTransform* lenaTrans = AffineTransform::Identity();
        lenaTrans->TranslateBy(-256, -256);
        lenaTrans->ScaleBy(0.5f);
        lenaTrans->RotateByDegrees(45.f);
        lenaTrans->TranslateBy(320.f, 240.f);
        g_LenaImage = Image::CreateFromImageWithTransform(lenaImage, lenaTrans);

        FIRTREE_SAFE_RELEASE(lenaTrans);
        FIRTREE_SAFE_RELEASE(lenaImage);

        g_OverKernel->SetValueForKey(g_LenaImage, "a");
        g_OverKernel->SetValueForKey(fogImage, "b");

        FIRTREE_SAFE_RELEASE(fogImage);

        g_RippleKernel->SetValueForKey(g_OverImage, "a");

        g_OverKernel2->SetValueForKey(g_GradientImage, "b");
        g_OverKernel2->SetValueForKey(g_RippleImage, "a");

        // Don't release these since we want to access them
        // in the render loop.
        // FIRTREE_SAFE_RELEASE(g_RippleKernel);
        // FIRTREE_SAFE_RELEASE(g_SpotKernel);
        // FIRTREE_SAFE_RELEASE(g_GradientImage);
        // FIRTREE_SAFE_RELEASE(g_CheckerImage);
        // FIRTREE_SAFE_RELEASE(g_OverKernel2);
        // FIRTREE_SAFE_RELEASE(g_RippleImage);
        // FIRTREE_SAFE_RELEASE(g_SpotImage);
        // FIRTREE_SAFE_RELEASE(g_OverImage);

        FIRTREE_SAFE_RELEASE(fogImage);
        FIRTREE_SAFE_RELEASE(g_LenaImage);
        FIRTREE_SAFE_RELEASE(g_OverImage2);
        FIRTREE_SAFE_RELEASE(g_GradientKernel);
        FIRTREE_SAFE_RELEASE(g_OverKernel);
        FIRTREE_SAFE_RELEASE(g_CheckerKernel);
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
        exit(1);
    }
}

void context_created()
{
    try{
        initialize_kernels();
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
        exit(1);
    }

    g_GLRenderingContext = GLRenderer::Create();
}

// vim:sw=4:ts=4:cindent:et
