# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

SET(MESA_SOURCES
    # Grammar parsing
	shader/grammar/grammar_mesa.c 

    # Shader language
    shader/slang/slang_builtin.c
    shader/slang/slang_codegen.c
    shader/slang/slang_compile.c
    shader/slang/slang_compile_function.c
    shader/slang/slang_compile_operation.c
    shader/slang/slang_compile_struct.c
    shader/slang/slang_compile_variable.c
    shader/slang/slang_emit.c
    shader/slang/slang_ir.c
    shader/slang/slang_label.c
    shader/slang/slang_library_noise.c
    shader/slang/slang_link.c
    shader/slang/slang_log.c
    shader/slang/slang_mem.c
    shader/slang/slang_preprocess.c
    shader/slang/slang_print.c
    shader/slang/slang_simplify.c
    shader/slang/slang_storage.c
    shader/slang/slang_typeinfo.c
    shader/slang/slang_utility.c
    shader/slang/slang_vartable.c

    # Shader support
    shader/arbprogparse.c   shader/nvvertparse.c       shader/prog_print.c
    shader/arbprogram.c     shader/prog_debug.c        shader/program.c
    shader/atifragshader.c  shader/prog_execute.c      shader/programopt.c
    shader/nvfragparse.c    shader/prog_instruction.c  shader/prog_statevars.c
    shader/nvprogram.c      shader/prog_parameter.c    shader/shader_api.c

    # Main Mesa source
    main/accum.c         main/eval.c         main/queryobj.c
    main/api_arrayelt.c  main/execmem.c      main/rastpos.c
    main/api_loopback.c  main/extensions.c   main/rbadaptors.c
    main/api_noop.c      main/fbobject.c     main/renderbuffer.c
    main/api_validate.c  main/feedback.c     main/shaders.c
    main/arrayobj.c      main/fog.c          main/state.c
    main/attrib.c        main/framebuffer.c  main/stencil.c
    main/blend.c         main/get.c          main/texcompress.c
    main/bufferobj.c     main/getstring.c    main/texcompress_fxt1.c
    main/buffers.c       main/hash.c         main/texcompress_s3tc.c
    main/clip.c          main/hint.c         main/texenvprogram.c
    main/colortab.c      main/histogram.c    main/texformat.c
    main/context.c       main/image.c        main/teximage.c
    main/convolve.c      main/imports.c      main/texobj.c
    main/debug.c         main/light.c        main/texrender.c
    main/depth.c         main/lines.c        main/texstate.c
    main/depthstencil.c  main/matrix.c       main/texstore.c
    main/dispatch.c      main/mipmap.c       main/varray.c
    main/dlist.c         main/mm.c           main/vsnprintf.c
    main/drawpix.c       main/pixel.c        main/vtxfmt.c
    main/enable.c        main/points.c
    main/enums.c         main/polygon.c

    # GL API
    glapi/glapi.c glapi/glthread.c

    # Maths
    math/m_debug_clip.c   math/m_eval.c       math/m_vector.c
    math/m_debug_norm.c   math/m_matrix.c     math/m_xform.c
    math/m_debug_xform.c  math/m_translate.c

    # Software rasteriser
    swrast/s_aaline.c         swrast/s_buffers.c   swrast/s_imaging.c  swrast/s_texcombine.c
    swrast/s_aatriangle.c     swrast/s_context.c   swrast/s_lines.c    swrast/s_texfilter.c
    swrast/s_accum.c          swrast/s_copypix.c   swrast/s_logic.c    swrast/s_texstore.c
    swrast/s_alpha.c          swrast/s_depth.c     swrast/s_masking.c  swrast/s_triangle.c
    swrast/s_atifragshader.c  swrast/s_drawpix.c   swrast/s_points.c   swrast/s_zoom.c
    swrast/s_bitmap.c         swrast/s_feedback.c  swrast/s_readpix.c
    swrast/s_blend.c          swrast/s_fog.c       swrast/s_span.c
    swrast/s_blit.c           swrast/s_fragprog.c  swrast/s_stencil.c

    # Software rasteriser setup
    swrast_setup/ss_context.c  swrast_setup/ss_triangle.c

    # Transform and lighting
    tnl/t_context.c   tnl/t_vb_light.c    tnl/t_vb_texgen.c       tnl/t_vertex_sse.c
    tnl/t_draw.c      tnl/t_vb_normals.c  tnl/t_vb_texmat.c       tnl/t_vp_build.c
    tnl/t_pipeline.c  tnl/t_vb_points.c   tnl/t_vb_vertex.c
    tnl/t_vb_cull.c   tnl/t_vb_program.c  tnl/t_vertex.c
    tnl/t_vb_fog.c    tnl/t_vb_render.c   tnl/t_vertex_generic.c

    # Vertex buffer objects
    vbo/vbo_context.c     vbo/vbo_exec_draw.c  vbo/vbo_save.c          
    vbo/vbo_split_copy.c
    vbo/vbo_exec_api.c    vbo/vbo_exec_eval.c  vbo/vbo_save_draw.c  
    vbo/vbo_split_inplace.c
    vbo/vbo_exec_array.c  vbo/vbo_rebase.c     vbo/vbo_save_loopback.c
    vbo/vbo_exec.c        vbo/vbo_save_api.c   vbo/vbo_split.c

    # Minimal driver support
    drivers/common/driverfuncs.c
)

SET(MESA_INCLUDES
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/glapi
	${CMAKE_CURRENT_SOURCE_DIR}/main
	${CMAKE_CURRENT_SOURCE_DIR}/math
	${CMAKE_CURRENT_SOURCE_DIR}/shader
	${CMAKE_CURRENT_SOURCE_DIR}/shader/grammar
	${CMAKE_CURRENT_SOURCE_DIR}/shader/slang
)

INCLUDE_DIRECTORIES(${MESA_INCLUDES})
ADD_DEFINITIONS(-DFIRTREE)

ADD_LIBRARY(mesa STATIC ${MESA_SOURCES})
SET_TARGET_PROPERTIES(mesa PROPERTIES INCLUDE_DIRS "${MESA_INCLUDES}")

# Vim modeline
# vim:ts=4:sw=4:et
