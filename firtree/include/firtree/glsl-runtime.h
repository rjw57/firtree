// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
// This file defines the interface to the FIRTREE kernels and samplers.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_GLSL_RUNTIME_H
#define FIRTREE_GLSL_RUNTIME_H
//=============================================================================

#include <firtree/kernel.h>
#include <firtree/image.h>

namespace Firtree { namespace GLSL {

struct RenderingContext;

//=============================================================================
bool BuildGLSLShaderForSampler(std::string& dest, Firtree::SamplerParameter* sampler);

//=============================================================================
bool SetGLSLUniformsForSampler(Firtree::SamplerParameter* sampler, unsigned int program);

//=============================================================================
const char* GetInfoLogForSampler(Firtree::SamplerParameter* sampler);

//=============================================================================
Firtree::SamplerParameter* CreateSampler(Image* im);

//=============================================================================
Firtree::Kernel* CreateKernel(const char* source);

//=============================================================================
RenderingContext* CreateRenderingContext(Firtree::SamplerParameter* topLevelSampler);

//=============================================================================
void ReleaseRenderingContext(RenderingContext*);

//=============================================================================
void RenderInRect(RenderingContext* context, const Rect2D& destRect, const Rect2D& srcRect);

//=============================================================================
void RenderAtPoint(RenderingContext* context, const Point2D& location, const Rect2D& srcRect);

} }

//=============================================================================
#endif // FIRTREE_GLSL_RUNTIME_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

