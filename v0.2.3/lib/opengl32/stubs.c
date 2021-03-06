/* $Id: stubs.c,v 1.3 2004/02/03 14:23:42 royce Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/stubs.c
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Royce Mitchell III (Royce3)
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/*#include <gl/gl.h>*/
#include "opengl32.h"

GLFUNCLIST*
OPENGL32_GetFuncList(); // see bottom of this file...

/*
 * @implemented
 */
void
WINAPI
glAccum (
	GLenum op,
	GLfloat value )
{
	GLFUNCLIST* list = OPENGL32_GetFuncList();
	/* FIXME - jump directly to target... */
	if ( list )
		(*list->glAccum)(op,value);
}

/*
 * @implemented
 */
static // not exported, except via wglGetProcAddress
void
WINAPI
glAddSwapHintRectWIN (
	GLint x,
	GLint y,
	GLsizei width,
	GLsizei height )
{
	(*OPENGL32_GetFuncList()->glAddSwapHintRectWIN)(x,y,width,height);
}

/*
 * @implemented
 */
void
WINAPI
glAlphaFunc (
	GLenum func,
	GLclampf ref )
{
	(*OPENGL32_GetFuncList()->glAlphaFunc)(func,ref);
}

/*
 * @unimplemented
 */
GLboolean
WINAPI
glAreTexturesResident (
	GLsizei n,
	GLuint * textures
	GLboolean * residences )
{
	UNIMPLEMENTED; /* FIXME */
}

/*
 * @unimplemented
 */
void
WINAPI
glArrayElement ( GLint index )
{
	UNIMPLEMENTED; /* FIXME */
}

/*
 * @unimplemented
 */
void
WINAPI
glBegin ( GLenum mode )
{
	UNIMPLEMENTED; /* FIXME */
}

/*
 * @unimplemented
 */
void
WINAPI
glBindTexture ( GLenum target, GLuint texture )
{
	UNIMPLEMENTED; /* FIXME */
}

/*
glBindTexture=glBindTexture@8
glBitmap=glBitmap@28
glBlendFunc=glBlendFunc@8
glCallList=glCallList@4
glCallLists=glCallLists@12
glClear=glClear@4
glClearAccum=glClearAccum@16
glClearColor=glClearColor@16
glClearDepth=glClearDepth@4
glClearIndex=glClearIndex@4
glClearStencil=glClearStencil@4
glClipPlane=glClipPlane@8

glColor3b=glColor3b@3
glColor3bv=glColor3b@4
glColor3d=glColor3d@24
glColor3dv=glColor3d@4
glColor3f=glColor3f@12
glColor3fv=glColor3fv@4
glColor3i=glColor3i@12
glColor3iv=glColor3iv@4
glColor3s=glColor3s@6
glColor3sv=glColor3sv@4
glColor3ub=glColor3ub@3
glColor3ubv=glColor3ubv@4
glColor3ui=glColor3ui@12
glColor3uiv=glColor3uiv@4
glColor3us=glColor3us@6
glColor3usv=glColor3usv@4

glColor4b=glColor4b@4
glColor4bv=glColor4bv@4
glColor4d=glColor4d@32
glColor4dv=glColor4dv@4
glColor4f=glColor4f@16
glColor4fv=glColor4fv@4
glColor4i=glColor4i@16
glColor4iv=glColor4iv@4
glColor4s=glColor4s@8
glColor4sv=glColor4sv@4
glColor4ub=glColor4ub@4
glColor4ubv=glColor4ubv@4
glColor4ui=glColor4ui@16
glColor4uiv=glColor4uiv@4
glColor4us=glColor4us@8
glColor4usv=glColor4usv@4

glColorMask=glColorMask@4
glColorMaterial=glColorMaterial@8
glColorPointer=glColorPointer@16
glCopyPixels=glCopyPixels@20
glCopyTexImage1D=glCopyTexImage1D@28
glCopyTexImage2D=glCopyTexImage2D@32
glCopyTexSubImage1D=glCopyTexSubImage1D@24
glCopyTexSubImage2D=glCopyTexSubImage2D@32
glCullFace=glCullFace@4
;glDebugEntry
glDeleteLists=glDeleteLists@8
glDeleteTextures=glDeleteTextues@8
glDepthFunc=glDepthFunc@4
glDepthMask=glDepthMask@4
glDepthRange=glDepthRange@8
glDisable=glDisable@4
glDisableClientState=glDisableClientState@4
glDrawArrays=glDrawArrays@12
glDrawBuffer=glDrawBuffer@4
glDrawElements=glDrawElements@16
glDrawPixels=glDrawPixels@20
glEdgeFlag=glEdgeFlag@1
glEdgeFlagPointer=glEdgeFlagPointer@8
glEdgeFlagv=glEdgeFlagv@4
glEnable=glEnable@4
glEnableClientState=glEnableClientState@4
*/

/*
 * @unimplemented
 */
void
WINAPI
glEnd ( void )
{
	UNIMPLEMENTED; /* FIXME */
}

/*
glEndList=glEndList@0

glEvalCoord1d=glEvalCoord1d@8
glEvalCoord1dv=glEvalCoord1dv@4
glEvalCoord1f=glEvalCoord1f@4
glEvalCoord1fv=glEvalCoord1fv@4
glEvalCoord2d=glEvalCoord2d@16
glEvalCoord2dv=glEvalCoord2dv@4
glEvalCoord2f=glEvalCoord2f@8
glEvalCoord2fv=glEvalCoord2fv@4

glEvalMesh1=glEvalMesh1@12
glEvalMesh2=glEvalMesh2@12
glEvalPoint1=glEvalPoint1@4
glEvalPoint2=glEvalPoint2@8
glFeedbackBuffer=glFeedbackBuffer@12
glFinish=glFinish@0
glFlush=glFlush@0
glFogf=glFogf@8
glFogfv=glFogv@8
glFogi=glFogi@8
glFogiv=glFogiv@8
glFrontFace=glFrontFace@4
glFrustum=glFrustum@48
glGenLists=glGenLists@4
glGenTextures=glGenTextires@8
glGetBooleanv=glGetBooleanv@8
glGetClipPlane=glGetClipPlane@8
glGetDoublev=glGetDoublev@8
glGetError=glGetError@0
glGetFloatv=glGetFloatv@8
glGetIntegerv=glGetIntegerv@8
glGetLightfv=glGetLightfv@12
glGetLightiv=glGetLightiv@12
glGetMapdv=glGetMapdv@12
glGetMapfv=glGetMapfv@12
glGetMapiv=glGetMapiv@12
glGetMaterialfv=glGetMaterialfv@12
glGetMaterialiv=glGetMaterialiv@12
glGetPixelMapfv=glGetPixelMapfv@8
glGetPixelMapuiv=glGetPixelMapuiv@8
glGetPixelMapusv=glGetPixelMapusv@8
glGetPointerv=glGetPointerv@8
glGetPolygonStipple=glGetPolygonStipple@4
glGetString=glGetString@4
glGetTexEnvfv=glGetTexEnvfv@12
glGetTexEnviv=glGetTexEnviv@12
glGetTexGendv=glGetTexGendv@12
glGetTexGenfv=glGetTexGenfv@12
glGetTexGeniv=glGetTexGeniv@12
glGetTexImage=glGetTexImage@20
glGetTexLevelParameterfv=glGetTexLevelParameterfv@16
glGetTexLevelParameteriv=glGetTexLevelParameteriv@16
glGetTexParameterfv=glGetTexParameterfv@12
glGetTexParameteriv=glGetTexParameteriv@12
glHint=glHint@8
glIndexd=glIndexd@8
glIndexdv=glIndexdv@4
glIndexf=glIndexf@4
glIndexfv=glIndexfv@4
glIndexi=glIndexi@4
glIndexiv=glIndexiv@4
glIndexMask=glIndexMask@4
glIndexPointer=glIndexPointer@12
glIndexs=glIndexs@2
glIndexsv=glIndexsv@4
glIndexub=glIndexub@1
glIndexubv=glIndexubv@4
glInitNames=glInitNames@0
glInterleavedArrays=glInterleavedArrays@12
glIsEnabled=glIsEnabled@4
glIsList=glIsList@4
glIsTexture=glTexture@4

glLightf=glLightf@12
glLightfv=glLightfv@12
glLighti=glLighti@12
glLightiv=glLightiv@12
glLightModelf=glLightModelf@8
glLightModelfv=glLightModelfv@8
glLightModeli=glLightModeli@8
glLightModeliv=glLightModeliv@8

glLineStipple=glLineStipple@6
glLineWidth=glLineWidth@4
glListBase=glListBase@4
glLoadIdentity=glLoadIdentity@0
glLoadMatrixd=glLoadMatrixd@4
glLoadMatrixf=glLoadMatrixf@4
glLoadName=glLoadName@4
glLogicOp=glLogicOp@4

glMap1d=glMap1d@32
glMap1f=glMap1f@24
glMap2d=glMap2d@56
glMap2f=glMap2f@40
glMapGrid1d=glMapGrid1d@20
glMapGrid1f=glMapGrid1f@12
glMapGrid2d=glMapGrid2d@40
glMapGrid2f=glMapGrid2f@24

glMaterialf=glMaterialf@12
glMaterialfv=glMaterialfv@12
glMateriali=glMateriali@12
glMaterialiv=glMaterialiv@12

glMatrixMode=glMatrixMode@4
glMultMatrixd=glMultMatrixd@4
glMultMatrixf=glMultMatrixf@4
glNewList=glNewList@8

glNormal3b=glNormal3b@3
glNormal3bv=glNormal3bv@4
glNormal3d=glNormal3d@24
glNormal3dv=glNormal3dv@4
glNormal3f=glNormal3f@12
glNormal3fv=glNormal3fv@4
glNormal3i=glNormal3i@12
glNormal3iv=glNormal3iv@4
glNormal3s=glNormal3s@6
glNormal3sv=glNormal3sv@4
glNormalPointer=glNormalPointer@12

glOrtho=glOrtho@24
glPassThrough=glPassThrough@4
glPixelMapfv=glPixelMapfv@12
glPixelMapuiv=glPixelMapuiv@12
glPixelMapusv=glPixelMapusv@12
glPixelStoref=glPixelStoref@8
glPixelStorei=glPixelStorei@8
glPixelTransferf=glPixelTransferf@8
glPixelTransferi=glPixelTransferi@8
glPixelZoom=glPixelZoom@8
glPointSize=glPointSize@4
glPolygonMode=glPolygonMode@8
glPolygonOffset=glPolygonOffset@8
glPolygonStipple=glPolygonStipple@4
glPopAttrib=glPopAttrib@0
glPopClientAttrib=glPopClientAttrib@0
glPopMatrix=glPopMatrix@0
glPopName=glPopName@0
glPrioritizeTextures=glPrioritizeTextures@12
glPushAttrib=glPushAttrib@4
glPushClientAttrib=glPushClientAttrib@4
glPushMatrix=glPushMatrix@0
glPushName=glPushName@4
glRasterPos2d=glRasterPos2d@16
glRasterPos2dv=glRasterPos2dv@4
glRasterPos2f=glRasterPos2f@8
glRasterPos2fv=glRasterPos2fv@4
glRasterPos2i=glRasterPos2i@8
glRasterPos2iv=glRasterPos2iv@4
glRasterPos2s=glRasterPos2s@8
glRasterPos2sv=glRasterPos2sv@8
glRasterPos3d=glRasterPos3d@24
glRasterPos3dv=glRasterPos3dv@4
glRasterPos3f=glRasterPos3f@12
glRasterPos3fv=glRasterPos3fv@4
glRasterPos3i=glRasterPos3i@12
glRasterPos3iv=glRasterPos3iv@4
glRasterPos3s=glRasterPos3s@12
glRasterPos3sv=glRasterPos3sv@4
glRasterPos4d=glRasterPos4d@32
glRasterPos4dv=glRasterPos4dv@4
glRasterPos4f=glRasterPos4f@16
glRasterPos4fv=glRasterPos4fv@4
glRasterPos4i=glRasterPos4i@16
glRasterPos4iv=glRasterPos4iv@4
glRasterPos4s=glRasterPos4s@16
glRasterPos4sv=glRasterPos4sv@4
glReadBuffer=glReadBuffer@4
glReadPixels=glReadPixels@28
glRectd=glRectd@32
glRectdv=glRectdv@4
glRectf=glRectf@16
glRectfv=glRectfv@4
glRecti=glRecti@16
glRectiv=glRectiv@4
glRects=glRects@16
glRectsv=glRectsv@4
glRenderMode=glRenderMode@4
glRotated=glRotated@32
glRotatef=glRotatef@16
glScaled=glScaled@24
glScalef=glScalef@12
glScissor=glScissor@16
glSelectBuffer=glSelectBuffer@8
glShadeModel=glShadeModel@4
glStencilFunc=glStencilFunc@12
glStencilMask=glStencilMask@4
glStencilOp=glStencilOp@12

glTexCoord1d=glTexCoord1d@8
glTexCoord1dv=glTexCoord1dv@4
glTexCoord1f=glTexCoord1f@4
glTexCoord1fv=glTexCoord1fv@4
glTexCoord1i=glTexCoord1i@4
glTexCoord1iv=glTexCoord1iv@4
glTexCoord1s=glTexCoord1s@2
glTexCoord1sv=glTexCoord1sv@4

glTexCoord2d=glTexCoord2d@16
glTexCoord2dv=glTexCoord2dv@4
glTexCoord2f=glTexCoord2f@8
glTexCoord2fv=glTexCoord2fv@4
glTexCoord2i=glTexCoord2i@8
glTexCoord2iv=glTexCoord2iv@4
glTexCoord2s=glTexCoord2s@4
glTexCoord2sv=glTexCoord2sv@4

glTexCoord3d=glTexCoord3d@24
glTexCoord3dv=glTexCoord3dv@4
glTexCoord3f=glTexCoord3f@12
glTexCoord3fv=glTexCoord3fv@4
glTexCoord3i=glTexCoord3i@12
glTexCoord3iv=glTexCoord3iv@4
glTexCoord3s=glTexCoord3s@6
glTexCoord3sv=glTexCoord3sv@4

glTexCoord4d=glTexCoord4d@32
glTexCoord4dv=glTexCoord4dv@4
glTexCoord4f=glTexCoord4f@16
glTexCoord4fv=glTexCoord4fv@4
glTexCoord4i=glTexCoord4i@16
glTexCoord4iv=glTexCoord4iv@4
glTexCoord4s=glTexCoord4s@8
glTexCoord4sv=glTexCoord4sv@4

glTexCoordPointer=glTexCoordPointer@16
glTexEnvf=glTexEnvf@12
glTexEnvfv=glTexEnvfv@12
glTexEnvi=glTexEnvi@12
glTexEnviv=glTexEnviv@12
glTexGend=glTexGend@16
glTexGendv=glTexGendv@12
glTexGenf=glTexGenf@12
glTexGenfv=glTexGenfv@12
glTexGeni=glTexGeni@12
glTexGeniv=glTexGeniv@12
glTexImage1D=glTexImage1D@32
glTexImage2D=glTexImage2d@36
glTexParameterf=glTexParameterf@12
glTexParameterfv=glTexParameterfv@12
glTexParameteri=glTexParameteri@12
glTexParameteriv=glTexParameteriv@12
glTexSubImage1D=glTexSubImage1D@28
glTexSubImage2D=glTexSubImage2D@36

glTranslated=glTranslated@32
glTranslatef=glTranslated@16

glVertex2d=glVertex2d@16
glVertex2dv=glVertex2dv@4
glVertex2f=glVertex2f@8
glVertex2fv=glVertex2fv@4
glVertex2i=glVertex2i@8
glVertex2iv=glVertex2iv@4
glVertex2s=glVertex2s@4
glVertex2sv=glVertex2sv@4

glVertex3d=glVertex3d@24
glVertex3dv=glVertex3dv@4
glVertex3f=glVertex3f@12
glVertex3fv=glVertex3fv@4
glVertex3i=glVertex3i@12
glVertex3iv=glVertex3iv@4
glVertex3s=glVertex3s@6
glVertex3sv=glVertex3sv@4

glVertex4d=glVertex4d@32
glVertex4dv=glVertex4dv@4
glVertex4f=glVertex4f@16
glVertex4fv=glVertex4fv@4
glVertex4i=glVertex4i@16
glVertex4iv=glVertex4iv@4
glVertex4s=glVertex4s@8
glVertex4sv=glVertex4sv@4

glVertexPointer=glVertexPointer@16
glViewport=glViewport@16

wglChoosePixelFormat=GDI32.ChoosePixelFormat
wglCopyContext=wglCopyContext@12
wglCreateContext=wglCreateContext@4
wglCreateLayerContext=wglCreateLayerContext@8
wglDeleteContext=wglDeleteContext@4
wglDescribeLayerPlane=wglDescribeLayerPlane@20
wglDescribePixelFormat=GDI32.DescribePixelFormat
wglGetCurrentContext=wglGetCurrentContext@0
wglGetCurrentDC=wglGetCurrentDC@0
wglGetLayerPaletteEntries=wglGetLayerPaletteEntries@20
wglGetPixelFormat=GDI32.GetPixelFormat
*/

/*
 * @unimplemented
 */
PROC wglGetProcAddress ( LPCSTR lpszProc )
{
	/* FIXME - what is the right strcmp() variant to use here? */
	/* NOTE - lpszProc comparisons *ARE* case-sensitive */
	GLFUNCLIST* funclist = OPENGL32_GetFuncList();
	if ( !strcmp ( lpszProc, "glAddSwapHintRectWIN" ) )
		return (PROC)(funclist->glAddSwapHintRectWIN);
	/* FIXME - fill in the rest of the functions returned by wglGetProcAddress */
	return NULL;
}

/*
wglMakeCurrent=wglMakeCurrent@8
wglRealizeLayerPalette=wglRealizeLayerPalette@12
wglSetLayerPaletteEntries=wglSetLayerPaletteEntries@20
wglSetPixelFormat=GDI32.SetPixelFormat
wglShareLists=wglShareLists@8
wglSwapBuffers=GDI32.SwapBuffers
wglSwapLayerBuffers=wglSwapLayerBuffers@8
wglUseFontBitmapsA=wglUseFontBitmapsA@16
wglUseFontBitmapsW=wglUseFontBitmapsW@16
wglUseFontOutlinesA=wglUseFontOutlinesA@32
wglUseFontOutlinesW=wglUseFontOutlinesW@32
*/

GLFUNCLIST*
OPENGL32_GetFuncList()
{
	threaddata = (OPENGL32_ThreadData*)TlsGetValue ( OPENGL32_tls );
	ASSERT(threaddata);
	return threaddata->list;
}
