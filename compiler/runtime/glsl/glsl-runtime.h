/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2008 Rich Wareham <srichwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//=============================================================================
// This file defines the interface to the FIRTREE kernels and samplers.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_GLSL_RUNTIME_H
#define FIRTREE_GLSL_RUNTIME_H
//=============================================================================

#include <compiler/include/kernel.h>

namespace Firtree { namespace GLSL {

struct RenderingContext;

//=============================================================================
bool BuildGLSLShaderForSampler(std::string& dest, Firtree::SamplerParameter* sampler);

//=============================================================================
bool SetGLSLUniformsForSampler(Firtree::SamplerParameter* sampler, unsigned int program);

//=============================================================================
const char* GetInfoLogForSampler(Firtree::SamplerParameter* sampler);

//=============================================================================
Firtree::SamplerParameter* CreateTextureSampler(unsigned int texObj);

//=============================================================================
Firtree::SamplerParameter* CreateKernelSampler(Firtree::Kernel* kernel);

//=============================================================================
Firtree::Kernel* CreateKernel(const char* source);

//=============================================================================
RenderingContext* CreateRenderingContext(Firtree::SamplerParameter* topLevelSampler);

//=============================================================================
void ReleaseRenderingContext(RenderingContext*);

//=============================================================================
void RenderInRect(RenderingContext* context, const Rect2D& destRect);

} }

//=============================================================================
#endif // FIRTREE_GLSL_RUNTIME_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

