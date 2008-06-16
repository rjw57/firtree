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

#include "compiler/runtime/glsl/glsl-runtime.h"
#include "compiler/include/main.h"

#include <stdlib.h>

#include <compiler/include/opengl.h>

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

Kernel* g_CheckerKernel = GLSL::CreateKernel(g_CheckerKernelSource);
SamplerParameter* g_CheckerSampler = GLSL::CreateKernelSampler(g_CheckerKernel);

const char* g_SpotKernelSource = 
"    kernel vec4 spotKernel(float dotPitch, __color backColor,"
"                           __color dotColor)"
"    {"
"        vec2 dc = mod(destCoord(), dotPitch) - 0.5*dotPitch;"
"        float discriminant = clamp(length(dc) - 0.3*dotPitch - 0.5, 0.0, 1.0);"
"        return mix(dotColor, backColor, discriminant);"
"    }"
;

Kernel* g_SpotKernel = GLSL::CreateKernel(g_SpotKernelSource);
SamplerParameter* g_SpotSampler = GLSL::CreateKernelSampler(g_SpotKernel);

const char* g_OverKernelSource = 
"    kernel vec4 compositeOver(sampler a, sampler b, float phase)"
"    {"
"        vec4 aOut = sample(a, samplerCoord(a));"
"        vec4 bOut = sample(b, samplerCoord(b));"
"        return aOut + bOut * (1.0 - aOut.a);"
"    }"
;

Kernel* g_OverKernel = GLSL::CreateKernel(g_OverKernelSource);
SamplerParameter* g_OverSampler = GLSL::CreateKernelSampler(g_OverKernel);

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

Kernel* g_RippleKernel = GLSL::CreateKernel(g_RippleKernelSource);
SamplerParameter* g_RippleSampler = GLSL::CreateKernelSampler(g_RippleKernel);

const char* g_GradientKernelSource = 
"    kernel vec4 gradientKernel()"
"    {"
"        float lambda = clamp(destCoord().x / 600.0, 0.0, 1.0);"
"        return mix(vec4(0,0,0,0), vec4(0,1,0,1), lambda);"
"    }"
;

Kernel* g_GradientKernel = GLSL::CreateKernel(g_GradientKernelSource);
SamplerParameter* g_GradientSampler = GLSL::CreateKernelSampler(g_GradientKernel);

Kernel* g_OverKernel2 = GLSL::CreateKernel(g_OverKernelSource);
SamplerParameter* g_OverSampler2 = GLSL::CreateKernelSampler(g_OverKernel2);

//#define g_GlobalSampler g_OverSampler
//#define g_GlobalSampler g_RippleSampler
//#define g_GlobalSampler g_GradientSampler
//#define g_GlobalSampler g_CheckerSampler
//#define g_GlobalSampler g_SpotSampler
#define g_GlobalSampler g_OverSampler2

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
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);      

    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    CHECK( glUseProgramObjectARB(g_ShaderProg) );
    try {
        g_SpotKernel->SetValueForKey(10.f * (1.0f + (float)sin(0.01f*epoch)) + 30.f,
                "dotPitch");
 //       g_SpotKernel->SetValueForKey(30.f, "dotPitch");

        g_RippleKernel->SetValueForKey(-epoch * 0.1f, "phase");

        GLSL::SetGLSLUniformsForSampler(g_GlobalSampler, g_ShaderProg);

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

        g_CheckerKernel->SetValueForKey(10.f, "squareSize");
        g_CheckerKernel->SetValueForKey(backColor, 4, "backColor");
        g_CheckerKernel->SetValueForKey(squareColor, 4, "foreColor");

        g_SpotKernel->SetValueForKey(dotColor, 4, "dotColor");
        g_SpotKernel->SetValueForKey(clearColor, 4, "backColor");

        AffineTransform* spotTrans = g_SpotSampler->GetTransform()->Copy();
        spotTrans->RotateByDegrees(12.f);
        spotTrans->TranslateBy(320.f, 240.f);
        g_SpotSampler->SetTransform(spotTrans);
        spotTrans->Release();

        glGenTextures(1, &g_LenaTexture);
        bool rv = InitialiseTextureFromFile(g_LenaTexture, "lena.png");
        if(!rv)
        {
            fprintf(stderr, "Could not load texture image.\n");
            exit(1);
            return;
        }

        glGenTextures(1, &g_FogTexture);
        rv = InitialiseTextureFromFile(g_FogTexture, "fog.png");
        if(!rv)
        {
            fprintf(stderr, "Could not load fog texture image.\n");
            exit(1);
            return;
        }

        SamplerParameter* lenaSampler = 
            GLSL::CreateTextureSampler(g_LenaTexture);
        SamplerParameter* fogSampler = 
            GLSL::CreateTextureSampler(g_FogTexture);

        AffineTransform* lenaTrans = lenaSampler->GetTransform()->Copy();
        lenaTrans->TranslateBy(-256, -256);
        lenaTrans->ScaleBy(0.5f);
        lenaTrans->RotateByDegrees(45.f);
        lenaTrans->TranslateBy(320.f, 240.f);
        lenaSampler->SetTransform(lenaTrans);
        lenaTrans->Release();

        g_OverKernel->SetValueForKey(lenaSampler, "a");
        g_OverKernel->SetValueForKey(g_CheckerSampler, "b");

        g_RippleKernel->SetValueForKey(g_OverSampler, "a");

        g_OverKernel2->SetValueForKey(g_GradientSampler, "b");
        g_OverKernel2->SetValueForKey(g_RippleSampler, "a");

        lenaSampler->Release();
        fogSampler->Release();
    } catch(Firtree::Exception e) {
        fprintf(stderr, "Error: %s\n", e.GetMessage().c_str());
        exit(1);
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

    if(!GLEW_ARB_shading_language_100 || !GLEW_ARB_shader_objects)
    {
        fprintf(stdout, "OpenGL shading langugage required.\n");
        exit(1);
        return;
    }

    const GLubyte* versionStr = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Shader langugage version supported: %s.\n", versionStr);

    initialize_kernels();

    std::string shaderSource;
    //g_GlobalSampler.BuildGLSL(shaderSource);
    bool retVal = GLSL::BuildGLSLShaderForSampler(shaderSource, g_GlobalSampler);

    if(!retVal)
    {
        fprintf(stderr, "Error compiling kernel:\n%s\n",
                GLSL::GetInfoLogForSampler(g_GlobalSampler));
        exit(3);
    }

    printf("Compiled source:\n%s\n", shaderSource.c_str());

    g_FragShaderObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if(g_FragShaderObj == 0)
    {
        fprintf(stderr, "Error creating shader object.\n");
        return;
    }

    const char* pSrc = shaderSource.c_str();
    CHECK( glShaderSourceARB(g_FragShaderObj, 1, &pSrc, NULL) );
    CHECK( glCompileShaderARB(g_FragShaderObj) );

    GLint status = 0;
    CHECK( glGetObjectParameterivARB(g_FragShaderObj, GL_OBJECT_COMPILE_STATUS_ARB, &status) );
    if(status != GL_TRUE)
    {
        fprintf(stderr, "Error compiling shader:\n");

        GLint logLen = 0;
        CHECK( glGetObjectParameterivARB(g_FragShaderObj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetInfoLogARB(g_FragShaderObj, logLen, &logLen, log) );
        fprintf(stderr, "%s\n", log);
        free(log);
        exit(2);
    }

    CHECK( g_ShaderProg = glCreateProgramObjectARB() );
    CHECK( glAttachObjectARB(g_ShaderProg, g_FragShaderObj) );
    CHECK( glLinkProgramARB(g_ShaderProg) );
    CHECK( glGetObjectParameterivARB(g_ShaderProg, GL_OBJECT_LINK_STATUS_ARB, &status) );
    if(status != GL_TRUE)
    {
        fprintf(stderr, "Error linking shader:\n");

        GLint logLen = 0;
        CHECK( glGetObjectParameterivARB(g_ShaderProg, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetInfoLogARB(g_ShaderProg, logLen, &logLen, log) );
        fprintf(stderr, "%s\n", log);
        free(log);
        exit(2);
    }
}

// vim:sw=4:ts=4:cindent:et
