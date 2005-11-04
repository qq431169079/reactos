/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_eval.h"
#include "context.h"
#include "macros.h"
#include "math/m_eval.h"
#include "t_vtx_api.h"


static void clear_active_eval1( TNLcontext *tnl, GLuint attr ) 
{
   tnl->vtx.eval.map1[attr].map = 0;
}

static void clear_active_eval2( TNLcontext *tnl, GLuint attr ) 
{
   tnl->vtx.eval.map2[attr].map = 0;
}

static void set_active_eval1( TNLcontext *tnl, GLuint attr, GLuint dim, 
			      struct gl_1d_map *map )
{
   if (!tnl->vtx.eval.map1[attr].map) {
      tnl->vtx.eval.map1[attr].map = map;
      tnl->vtx.eval.map1[attr].sz = dim;
   }
} 

static void set_active_eval2( TNLcontext *tnl, GLuint attr, GLuint dim, 
			      struct gl_2d_map *map )
{
   if (!tnl->vtx.eval.map2[attr].map) {
      tnl->vtx.eval.map2[attr].map = map;
      tnl->vtx.eval.map2[attr].sz = dim;
   }
} 

void _tnl_update_eval( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint attr;

   /* Vertex program maps have priority over conventional attribs */

   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      clear_active_eval1( tnl, attr );
      clear_active_eval2( tnl, attr );
   }

   if (ctx->VertexProgram._Enabled) {
      for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
	 if (ctx->Eval.Map1Attrib[attr]) 
	    set_active_eval1( tnl, attr, 4, &ctx->EvalMap.Map1Attrib[attr] );

	 if (ctx->Eval.Map2Attrib[attr]) 
	    set_active_eval2( tnl, attr, 4, &ctx->EvalMap.Map2Attrib[attr] );
      }
   }

   if (ctx->Eval.Map1Color4) 
      set_active_eval1( tnl, VERT_ATTRIB_COLOR0, 4, &ctx->EvalMap.Map1Color4 );
      
   if (ctx->Eval.Map2Color4) 
      set_active_eval2( tnl, VERT_ATTRIB_COLOR0, 4, &ctx->EvalMap.Map2Color4 );

   if (ctx->Eval.Map1TextureCoord4) 
      set_active_eval1( tnl, VERT_ATTRIB_TEX0, 4, &ctx->EvalMap.Map1Texture4 );
   else if (ctx->Eval.Map1TextureCoord3) 
      set_active_eval1( tnl, VERT_ATTRIB_TEX0, 3, &ctx->EvalMap.Map1Texture3 );
   else if (ctx->Eval.Map1TextureCoord2) 
      set_active_eval1( tnl, VERT_ATTRIB_TEX0, 2, &ctx->EvalMap.Map1Texture2 );
   else if (ctx->Eval.Map1TextureCoord1) 
      set_active_eval1( tnl, VERT_ATTRIB_TEX0, 1, &ctx->EvalMap.Map1Texture1 );

   if (ctx->Eval.Map2TextureCoord4) 
      set_active_eval2( tnl, VERT_ATTRIB_TEX0, 4, &ctx->EvalMap.Map2Texture4 );
   else if (ctx->Eval.Map2TextureCoord3) 
      set_active_eval2( tnl, VERT_ATTRIB_TEX0, 3, &ctx->EvalMap.Map2Texture3 );
   else if (ctx->Eval.Map2TextureCoord2) 
      set_active_eval2( tnl, VERT_ATTRIB_TEX0, 2, &ctx->EvalMap.Map2Texture2 );
   else if (ctx->Eval.Map2TextureCoord1) 
      set_active_eval2( tnl, VERT_ATTRIB_TEX0, 1, &ctx->EvalMap.Map2Texture1 );

   if (ctx->Eval.Map1Normal) 
      set_active_eval1( tnl, VERT_ATTRIB_NORMAL, 3, &ctx->EvalMap.Map1Normal );

   if (ctx->Eval.Map2Normal) 
      set_active_eval2( tnl, VERT_ATTRIB_NORMAL, 3, &ctx->EvalMap.Map2Normal );

   if (ctx->Eval.Map1Vertex4) 
      set_active_eval1( tnl, VERT_ATTRIB_POS, 4, &ctx->EvalMap.Map1Vertex4 );
   else if (ctx->Eval.Map1Vertex3) 
      set_active_eval1( tnl, VERT_ATTRIB_POS, 3, &ctx->EvalMap.Map1Vertex3 );

   if (ctx->Eval.Map2Vertex4) 
      set_active_eval2( tnl, VERT_ATTRIB_POS, 4, &ctx->EvalMap.Map2Vertex4 );
   else if (ctx->Eval.Map2Vertex3) 
      set_active_eval2( tnl, VERT_ATTRIB_POS, 3, &ctx->EvalMap.Map2Vertex3 );

   tnl->vtx.eval.new_state = 0;
}



void _tnl_do_EvalCoord1f(GLcontext* ctx, GLfloat u)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint attr;

   for (attr = 1; attr <= _TNL_ATTRIB_INDEX; attr++) {
      struct gl_1d_map *map = tnl->vtx.eval.map1[attr].map;
      if (map) {
	 GLfloat uu = (u - map->u1) * map->du;
	 GLfloat data[4];

	 ASSIGN_4V(data, 0, 0, 0, 1);

	 _math_horner_bezier_curve(map->Points, data, uu, 
				   tnl->vtx.eval.map1[attr].sz, 
				   map->Order);

	 COPY_SZ_4V( tnl->vtx.attrptr[attr],
		     tnl->vtx.attrsz[attr],
		     data );
      }
   }

   /** Vertex -- EvalCoord1f is a noop if this map not enabled:
    **/
   if (tnl->vtx.eval.map1[0].map) {
      struct gl_1d_map *map = tnl->vtx.eval.map1[0].map;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vertex[4];

      ASSIGN_4V(vertex, 0, 0, 0, 1);

      _math_horner_bezier_curve(map->Points, vertex, uu, 
				tnl->vtx.eval.map1[0].sz, 
				map->Order);

      if (tnl->vtx.eval.map1[0].sz == 4) 
	 glVertex4fv( vertex );
      else
	 glVertex3fv( vertex ); 
   }
}



void _tnl_do_EvalCoord2f( GLcontext* ctx, GLfloat u, GLfloat v )
{   
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint attr;

   for (attr = 1; attr <= _TNL_ATTRIB_INDEX; attr++) {
      struct gl_2d_map *map = tnl->vtx.eval.map2[attr].map;
      if (map) {
	 GLfloat uu = (u - map->u1) * map->du;
	 GLfloat vv = (v - map->v1) * map->dv;
	 GLfloat data[4];

	 ASSIGN_4V(data, 0, 0, 0, 1);

	 _math_horner_bezier_surf(map->Points, 
				  data, 
				  uu, vv, 
				  tnl->vtx.eval.map2[attr].sz, 
				  map->Uorder, map->Vorder);

	 COPY_SZ_4V( tnl->vtx.attrptr[attr],
		     tnl->vtx.attrsz[attr],
		     data );
      }
   }

   /** Vertex -- EvalCoord2f is a noop if this map not enabled:
    **/
   if (tnl->vtx.eval.map2[0].map) {
      struct gl_2d_map *map = tnl->vtx.eval.map2[0].map;
      GLfloat uu = (u - map->u1) * map->du;
      GLfloat vv = (v - map->v1) * map->dv;
      GLfloat vertex[4];

      ASSIGN_4V(vertex, 0, 0, 0, 1);

      if (ctx->Eval.AutoNormal) {
	 GLfloat normal[4];
         GLfloat du[4], dv[4];

         _math_de_casteljau_surf(map->Points, vertex, du, dv, uu, vv, 
				 tnl->vtx.eval.map2[0].sz,
				 map->Uorder, map->Vorder);

	 if (tnl->vtx.eval.map2[0].sz == 4) {
	    du[0] = du[0]*vertex[3] - du[3]*vertex[0];
	    du[1] = du[1]*vertex[3] - du[3]*vertex[1];
	    du[2] = du[2]*vertex[3] - du[3]*vertex[2];
	 
	    dv[0] = dv[0]*vertex[3] - dv[3]*vertex[0];
	    dv[1] = dv[1]*vertex[3] - dv[3]*vertex[1];
	    dv[2] = dv[2]*vertex[3] - dv[3]*vertex[2];
	 }


         CROSS3(normal, du, dv);
         NORMALIZE_3FV(normal);
	 normal[3] = 1.0;

	 COPY_SZ_4V( tnl->vtx.attrptr[_TNL_ATTRIB_NORMAL],
		     tnl->vtx.attrsz[_TNL_ATTRIB_NORMAL],
		     normal );

      }
      else {
         _math_horner_bezier_surf(map->Points, vertex, uu, vv, 
				  tnl->vtx.eval.map2[0].sz,
				  map->Uorder, map->Vorder);
      }

      if (tnl->vtx.attrsz[0] == 4) 
	 glVertex4fv( vertex );
      else
	 glVertex3fv( vertex ); 
   }
}


