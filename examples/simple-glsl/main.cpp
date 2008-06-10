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

#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

using namespace Firtree;

const char* g_CheckerKernelSource = 
"    kernel vec4 testKernel(void)"
"    {"
"        float squareSize = 10.0;"
"        vec2 dc = mod(destCoord(), squareSize*2.0);"
"        vec2 discriminant = step(squareSize, dc);"
"        float flag = discriminant.x + discriminant.y - "
"                     2.0*discriminant.x*discriminant.y;"
"        return vec4((0.25 + 0.5*flag) * vec3(1,1,1), 1);"
"    }"
;

const char* g_KernelSource = 
"    kernel vec4 testKernel(void)"
"    {"
"        float dotPitch = 60.0;"
"        vec2 dc = mod(destCoord(), dotPitch) - 0.5*dotPitch;"
"        float discriminant = 1.0 - smoothstep(0.3*dotPitch-0.5,"
"               0.3*dotPitch+0.5, length(dc));"
"        discriminant = sqrt(discriminant);"
"        return vec4(discriminant,discriminant,0,1);"
"    }"
;

Kernel g_FirtreeKernel(g_KernelSource);

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

    glColor3f(1,0,0);
    glBegin(GL_QUADS);
      glVertex2f(0,0);
      glVertex2f(width,0);
      glVertex2f(width,height);
      glVertex2f(0,height);
    glEnd();
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

    KernelSamplerParameter samplerParam("foo", g_FirtreeKernel);
    samplerParam.SetBlockPrefix("foo");
    samplerParam.SetBlockPrefix("toplevel");
    if(!samplerParam.IsValid())
    {
        fprintf(stderr, "Error compiling kernel.\n%s\n",
                samplerParam.GetKernel().GetInfoLog());
        exit(3);
    }

    std::string shaderSource;
    samplerParam.BuildTopLevelGLSL(shaderSource);

    shaderSource += "void main() { vec2 destCoord = gl_FragCoord.xy;\n";

    std::string sampleString;
    samplerParam.BuildSampleGLSL(sampleString, 
            "destCoord", "gl_FragColor");
    shaderSource += sampleString;

    shaderSource += "\n}\n";
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
}

// vim:sw=4:ts=4:cindent:et
