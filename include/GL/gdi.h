#ifndef _tinygl_GDI_H
#define _tinygl_GDI_H


#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *GDIContext;

extern GDIContext gdiCreateContext( GDIContext shareList, int flags );

extern void gdiDestroyContext( GDIContext ctx );

extern int gdiMakeCurrent( HWND drawable, GDIContext ctx, int width, int height );

extern void gdiSwapBuffers( HWND drawable );


#ifdef __cplusplus
}
#endif


#define MWPF_TRUECOLOR565 99

#endif //_tinygl_GDI_H
