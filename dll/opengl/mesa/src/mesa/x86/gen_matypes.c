/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes
 */

/*
 * This generates an asm version of mtypes.h (called matypes.h), so that
 * Mesa's x86 assembly code can access the internal structures easily.
 * This will be particularly useful when developing new x86 asm code for
 * Mesa, including lighting, clipping, texture image conversion etc.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include "main/glheader.h"
#include "main/mtypes.h"
#include "tnl/t_context.h"


#undef offsetof
#define offsetof( type, member ) ((size_t) &((type *)0)->member)


#define OFFSET_HEADER( x )						\
do {									\
   printf( "\n" );							\
   printf( "\n" );							\
   printf( "/* ====================================================="	\
	   "========\n" );						\
   printf( " * Offsets for %s\n", x );					\
   printf( " */\n" );							\
   printf( "\n" );							\
} while (0)

#define DEFINE_HEADER( x )						\
do {									\
   printf( "\n" );							\
   printf( "/*\n" );							\
   printf( " * Flags for %s\n", x );					\
   printf( " */\n" );							\
   printf( "\n" );							\
} while (0)

#define OFFSET( s, t, m )						\
   printf( "#define %s\t%lu\n", s, (unsigned long) offsetof( t, m ) );

#define SIZEOF( s, t )							\
   printf( "#define %s\t%lu\n", s, (unsigned long) sizeof(t) );

#define DEFINE( s, d )							\
   printf( "#define %s\t0x%" PRIx64 "\n", s, (uint64_t) d );



int main( int argc, char **argv )
{
   printf( "/*\n" );
   printf( " * This file is automatically generated from the Mesa internal type\n" );
   printf( " * definitions.  Do not edit directly.\n" );
   printf( " */\n" );
   printf( "\n" );
   printf( "#ifndef __ASM_TYPES_H__\n" );
   printf( "#define __ASM_TYPES_H__\n" );
   printf( "\n" );


   /* struct gl_context offsets:
    */
   OFFSET_HEADER( "struct gl_context" );

   OFFSET( "CTX_DRIVER_CTX              ", struct gl_context, DriverCtx );
   printf( "\n" );
   OFFSET( "CTX_LIGHT_ENABLED           ", struct gl_context, Light.Enabled );
   OFFSET( "CTX_LIGHT_SHADE_MODEL       ", struct gl_context, Light.ShadeModel );
   OFFSET( "CTX_LIGHT_COLOR_MAT_FACE    ", struct gl_context, Light.ColorMaterialFace );
   OFFSET( "CTX_LIGHT_COLOR_MAT_MODE    ", struct gl_context, Light.ColorMaterialMode );
   OFFSET( "CTX_LIGHT_COLOR_MAT_MASK    ", struct gl_context, Light.ColorMaterialBitmask );
   OFFSET( "CTX_LIGHT_COLOR_MAT_ENABLED ", struct gl_context, Light.ColorMaterialEnabled );
   OFFSET( "CTX_LIGHT_ENABLED_LIST      ", struct gl_context, Light.EnabledList );
   OFFSET( "CTX_LIGHT_NEED_VERTS        ", struct gl_context, Light._NeedVertices );
   OFFSET( "CTX_LIGHT_FLAGS             ", struct gl_context, Light._Flags );
   OFFSET( "CTX_LIGHT_BASE_COLOR        ", struct gl_context, Light._BaseColor );


   /* struct vertex_buffer offsets:
    */
   OFFSET_HEADER( "struct vertex_buffer" );

   OFFSET( "VB_SIZE                ", struct vertex_buffer, Size );
   OFFSET( "VB_COUNT               ", struct vertex_buffer, Count );
   printf( "\n" );
   OFFSET( "VB_ELTS                ", struct vertex_buffer, Elts );
   OFFSET( "VB_OBJ_PTR             ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_POS] );
   OFFSET( "VB_EYE_PTR             ", struct vertex_buffer, EyePtr );
   OFFSET( "VB_CLIP_PTR            ", struct vertex_buffer, ClipPtr );
   OFFSET( "VB_PROJ_CLIP_PTR       ", struct vertex_buffer, NdcPtr );
   OFFSET( "VB_CLIP_OR_MASK        ", struct vertex_buffer, ClipOrMask );
   OFFSET( "VB_CLIP_MASK           ", struct vertex_buffer, ClipMask );
   OFFSET( "VB_NORMAL_PTR          ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_NORMAL] );
   OFFSET( "VB_EDGE_FLAG           ", struct vertex_buffer, EdgeFlag );
   OFFSET( "VB_TEX_COORD_PTR       ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_TEX] );
   OFFSET( "VB_INDEX_PTR           ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_COLOR_INDEX] );
   OFFSET( "VB_COLOR_PTR           ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_COLOR0] );
   OFFSET( "VB_SECONDARY_COLOR_PTR ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_COLOR1] );
   OFFSET( "VB_FOG_COORD_PTR       ", struct vertex_buffer, AttribPtr[_TNL_ATTRIB_FOG] );
   OFFSET( "VB_PRIMITIVE           ", struct vertex_buffer, Primitive );
   printf( "\n" );

   DEFINE_HEADER( "struct vertex_buffer" );

   /* XXX use new labels here someday after vertex proram is done */
   DEFINE( "VERT_BIT_OBJ           ", VERT_BIT_POS );
   DEFINE( "VERT_BIT_NORM          ", VERT_BIT_NORMAL );
   DEFINE( "VERT_BIT_RGBA          ", VERT_BIT_COLOR0 );
   DEFINE( "VERT_BIT_SPEC_RGB      ", VERT_BIT_COLOR1 );
   DEFINE( "VERT_BIT_FOG_COORD     ", VERT_BIT_FOG );
   DEFINE( "VERT_BIT_TEX0          ", VERT_BIT_TEX0 );
   DEFINE( "VERT_BIT_TEX1          ", VERT_BIT_TEX1 );
   DEFINE( "VERT_BIT_TEX2          ", VERT_BIT_TEX2 );
   DEFINE( "VERT_BIT_TEX3          ", VERT_BIT_TEX3 );


   /* GLvector4f offsets:
    */
   OFFSET_HEADER( "GLvector4f" );

   OFFSET( "V4F_DATA          ", GLvector4f, data );
   OFFSET( "V4F_START         ", GLvector4f, start );
   OFFSET( "V4F_COUNT         ", GLvector4f, count );
   OFFSET( "V4F_STRIDE        ", GLvector4f, stride );
   OFFSET( "V4F_SIZE          ", GLvector4f, size );
   OFFSET( "V4F_FLAGS         ", GLvector4f, flags );

   DEFINE_HEADER( "GLvector4f" );

   DEFINE( "VEC_MALLOC        ", VEC_MALLOC );
   DEFINE( "VEC_NOT_WRITEABLE ", VEC_NOT_WRITEABLE );
   DEFINE( "VEC_BAD_STRIDE    ", VEC_BAD_STRIDE );
   printf( "\n" );
   DEFINE( "VEC_SIZE_1        ", VEC_SIZE_1 );
   DEFINE( "VEC_SIZE_2        ", VEC_SIZE_2 );
   DEFINE( "VEC_SIZE_3        ", VEC_SIZE_3 );
   DEFINE( "VEC_SIZE_4        ", VEC_SIZE_4 );


   /* GLmatrix offsets:
    */
   OFFSET_HEADER( "GLmatrix" );

   OFFSET( "MATRIX_DATA   ", GLmatrix, m );
   OFFSET( "MATRIX_INV    ", GLmatrix, inv );
   OFFSET( "MATRIX_FLAGS  ", GLmatrix, flags );
   OFFSET( "MATRIX_TYPE   ", GLmatrix, type );


   /* struct gl_light offsets:
    */
   OFFSET_HEADER( "struct gl_light" );

   OFFSET( "LIGHT_NEXT              ", struct gl_light, next );
   OFFSET( "LIGHT_PREV              ", struct gl_light, prev );
   printf( "\n" );
   OFFSET( "LIGHT_AMBIENT           ", struct gl_light, Ambient );
   OFFSET( "LIGHT_DIFFUSE           ", struct gl_light, Diffuse );
   OFFSET( "LIGHT_SPECULAR          ", struct gl_light, Specular );
   OFFSET( "LIGHT_EYE_POSITION      ", struct gl_light, EyePosition );
   OFFSET( "LIGHT_SPOT_DIRECTION    ", struct gl_light, SpotDirection );
   OFFSET( "LIGHT_SPOT_EXPONENT     ", struct gl_light, SpotExponent );
   OFFSET( "LIGHT_SPOT_CUTOFF       ", struct gl_light, SpotCutoff );
   OFFSET( "LIGHT_COS_CUTOFF        ", struct gl_light, _CosCutoff );
   OFFSET( "LIGHT_CONST_ATTEN       ", struct gl_light, ConstantAttenuation );
   OFFSET( "LIGHT_LINEAR_ATTEN      ", struct gl_light, LinearAttenuation );
   OFFSET( "LIGHT_QUADRATIC_ATTEN   ", struct gl_light, QuadraticAttenuation );
   OFFSET( "LIGHT_ENABLED           ", struct gl_light, Enabled );
   printf( "\n" );
   OFFSET( "LIGHT_FLAGS             ", struct gl_light, _Flags );
   printf( "\n" );
   OFFSET( "LIGHT_POSITION          ", struct gl_light, _Position );
   OFFSET( "LIGHT_VP_INF_NORM       ", struct gl_light, _VP_inf_norm );
   OFFSET( "LIGHT_H_INF_NORM        ", struct gl_light, _h_inf_norm );
   OFFSET( "LIGHT_NORM_DIRECTION    ", struct gl_light, _NormSpotDirection );
   OFFSET( "LIGHT_VP_INF_SPOT_ATTEN ", struct gl_light, _VP_inf_spot_attenuation );
   printf( "\n" );
   OFFSET( "LIGHT_SPOT_EXP_TABLE    ", struct gl_light, _SpotExpTable );
   OFFSET( "LIGHT_MAT_AMBIENT       ", struct gl_light, _MatAmbient );
   OFFSET( "LIGHT_MAT_DIFFUSE       ", struct gl_light, _MatDiffuse );
   OFFSET( "LIGHT_MAT_SPECULAR      ", struct gl_light, _MatSpecular );
   printf( "\n" );
   SIZEOF( "SIZEOF_GL_LIGHT         ", struct gl_light );

   DEFINE_HEADER( "struct gl_light" );

   DEFINE( "LIGHT_SPOT              ", LIGHT_SPOT );
   DEFINE( "LIGHT_LOCAL_VIEWER      ", LIGHT_LOCAL_VIEWER );
   DEFINE( "LIGHT_POSITIONAL        ", LIGHT_POSITIONAL );
   printf( "\n" );
   DEFINE( "LIGHT_NEED_VERTICES     ", LIGHT_NEED_VERTICES );


   /* struct gl_lightmodel offsets:
    */
   OFFSET_HEADER( "struct gl_lightmodel" );

   OFFSET( "LIGHT_MODEL_AMBIENT       ", struct gl_lightmodel, Ambient );
   OFFSET( "LIGHT_MODEL_LOCAL_VIEWER  ", struct gl_lightmodel, LocalViewer );
   OFFSET( "LIGHT_MODEL_TWO_SIDE      ", struct gl_lightmodel, TwoSide );
   OFFSET( "LIGHT_MODEL_COLOR_CONTROL ", struct gl_lightmodel, ColorControl );


   printf( "\n" );
   printf( "\n" );
   printf( "#endif /* __ASM_TYPES_H__ */\n" );

   return 0;
}
