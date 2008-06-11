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

#include "compiler/include/kernel.h"
#include "compiler/include/main.h"

#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

using namespace Firtree;

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

Kernel g_CheckerKernel(g_CheckerKernelSource);
KernelSamplerParameter g_CheckerSampler(g_CheckerKernel);

const char* g_SpotKernelSource = 
"    kernel vec4 spotKernel(float dotPitch, __color backColor,"
"                           __color dotColor)"
"    {"
"        vec2 dc = mod(destCoord(), dotPitch) - 0.5*dotPitch;"
"        float discriminant = smoothstep(0.3*dotPitch-0.5,"
"               0.3*dotPitch+0.5, length(dc));"
"        discriminant = sqrt(discriminant);"
"        return mix(dotColor, backColor, discriminant);"
"    }"
;

Kernel g_SpotKernel(g_SpotKernelSource);
KernelSamplerParameter g_SpotSampler(g_SpotKernel);

const char* g_OverKernelSource = 
"    kernel vec4 compositeOver(sampler a, sampler b)"
"    {"
"        vec4 aCol = sample(a, samplerCoord(a));"
"        vec4 bCol = sample(b, samplerCoord(b));"
"        return aCol + bCol * (1.0 - aCol.a);"
"    }"
;

Kernel g_OverKernel(g_OverKernelSource);
KernelSamplerParameter g_GlobalSampler(g_OverKernel);

GLenum g_FragShaderObj;
GLuint g_ShaderProg;

#define CHECK(a) do { \
    { do { (a); } while(0); } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) {  \
        fprintf(stderr, "%s:%i: OpenGL Error %s", __FILE__, __LINE__,\
                gluErrorString(_err)); \
    } } while(0) 

void initialise_test(int *argc, char **argv, int window)
{
}

void finalise_test()
{
}

void render(float epoch)
{
    glDisable(GL_DEPTH_TEST);

    CHECK( glUseProgram(g_ShaderProg) );
    try {
        g_SpotKernel.SetValueForKey(10.f * (1.0f + (float)sin(0.01f*epoch)) + 30.f,
                "dotPitch");

        g_GlobalSampler.SetGLSLUniforms(g_ShaderProg);

        glColor3f(1,0,0);
        glBegin(GL_QUADS);
           glVertex2f(0,0);
           glVertex2f(width,0);
           glVertex2f(width,height);
           glVertex2f(0,height);
        glEnd();
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
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

        g_CheckerKernel.SetValueForKey(10.f, "squareSize");
        g_CheckerKernel.SetValueForKey(backColor, 4, "backColor");
        g_CheckerKernel.SetValueForKey(squareColor, 4, "foreColor");

        g_SpotKernel.SetValueForKey(dotColor, 4, "dotColor");
        g_SpotKernel.SetValueForKey(clearColor, 4, "backColor");
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
    }
}

void context_created()
{
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return;
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if(!GLEW_VERSION_2_0)
    {
        fprintf(stdout, "OpenGL 2.0 or greater required.\n");
        exit(1);
        return;
    }

    float angle = 0.2f;
    float spotTransform[] = {
        cos(angle), -sin(angle), -320.f,
        sin(angle),  cos(angle), -240.f,
    };

    g_SpotSampler.SetTransform(spotTransform);
    g_OverKernel.SetValueForKey(g_SpotSampler, "a");
    g_OverKernel.SetValueForKey(g_CheckerSampler, "b");

    std::string shaderSource;
    g_GlobalSampler.BuildGLSL(shaderSource);

    if(!g_GlobalSampler.IsValid())
    {
        fprintf(stderr, "Error compiling kernel.\n%s\n",
                g_GlobalSampler.GetKernel().GetInfoLog());
        exit(3);
    }

    printf("Compiled source:\n%s\n", shaderSource.c_str());

    g_FragShaderObj = glCreateShader(GL_FRAGMENT_SHADER_ARB);
    if(g_FragShaderObj == 0)
    {
        fprintf(stderr, "Error creating shader object.\n");
        return;
    }

    const char* pSrc = shaderSource.c_str();
    CHECK( glShaderSource(g_FragShaderObj, 1, &pSrc, NULL) );
    CHECK( glCompileShader(g_FragShaderObj) );

    int status = 0;
    CHECK( glGetShaderiv(g_FragShaderObj, GL_COMPILE_STATUS, &status) );
    if(status != GL_TRUE)
    {
        fprintf(stderr, "Error compiling shader:\n");

        int logLen = 0;
        CHECK( glGetShaderiv(g_FragShaderObj, GL_INFO_LOG_LENGTH, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetShaderInfoLog(g_FragShaderObj, logLen, &logLen, log) );
        fprintf(stderr, "%s\n", log);
        free(log);
        exit(2);
    }

    CHECK( g_ShaderProg = glCreateProgram() );
    CHECK( glAttachShader(g_ShaderProg, g_FragShaderObj) );
    CHECK( glLinkProgram(g_ShaderProg) );
    CHECK( glGetProgramiv(g_ShaderProg, GL_LINK_STATUS, &status) );
    if(status != GL_TRUE)
    {
        fprintf(stderr, "Error linking shader:\n");

        int logLen = 0;
        CHECK( glGetProgramiv(g_ShaderProg, GL_INFO_LOG_LENGTH, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetProgramInfoLog(g_ShaderProg, logLen, &logLen, log) );
        fprintf(stderr, "%s\n", log);
        free(log);
        exit(2);
    }

    initialize_kernels();
}

// vim:sw=4:ts=4:cindent:et
