/*
 * TinyGL driver for WIN32 GDI
 *
 * @orignal Fabrice Bellard
 *
 * @modify 2017-Jul-17
 * @author Atmarkartworks t.m
 *
 *
 */

#include <stdio.h>
#include <windows.h>

#include <GL/gl.h>
#include "zgl.h"
#include <GL/gdi.h>



typedef struct {
    GLContext *gl_context;

    int xsize,ysize;
    int pixtype; /* pixel type in TinyGL */

    HWND drawable;
    LPBITMAPINFO dib_info;

} TinyGDIContext;



GDIContext gdiCreateContext(GDIContext shareList, int flags)
{
  TinyGDIContext *ctx;

  if (shareList != NULL) {
    gl_fatal_error("No sharing available in TinyGL");
  }
  ctx=gl_malloc(sizeof(TinyGDIContext));
  if (!ctx)
      return NULL;
  ctx->gl_context=NULL;
  ctx->dib_info = NULL;
  return (GDIContext) ctx;
}

void gdiDestroyContext( GDIContext ctx1 )
{
  TinyGDIContext *ctx = (TinyGDIContext *) ctx1;

  if (ctx->gl_context->zb != NULL) {
    ZB_close(ctx->gl_context->zb);
  }

  if (ctx->dib_info != NULL) {
      gl_free(ctx->dib_info);
  }

  if (ctx->gl_context != NULL) {
    glClose();
  }

  gl_free(ctx);
}


/* resize the GDI pain : we try to use the xsize and ysize
   given. We return the effective size which is guaranted to be smaller */

static int gdi_resize_viewport(GLContext *c,int *xsize_ptr,int *ysize_ptr)
{
  TinyGDIContext *ctx;
  int xsize,ysize;

  ctx=(TinyGDIContext *)c->opaque;

  xsize=*xsize_ptr;
  ysize=*ysize_ptr;



  /* we ensure that xsize and ysize are multiples of 2 for the zbuffer.
     TODO: find a better solution */
  xsize&=~3;
  ysize&=~3;

  ctx->dib_info->bmiHeader.biWidth=xsize;
  ctx->dib_info->bmiHeader.biHeight=ysize;
  ctx->xsize = xsize;
  ctx->ysize = ysize;

  if (xsize == 0 || ysize == 0) return -1;

  *xsize_ptr=xsize;
  *ysize_ptr=ysize;


  /* resize the Z buffer */
  ZB_resize(c->zb,NULL,xsize,ysize);
  return 0;
}




/* we assume here that drawable is a window */
int gdiMakeCurrent( HWND drawable, GDIContext ctx1, int width, int height)
{
  TinyGDIContext *ctx = (TinyGDIContext *) ctx1;
  int mode, xsize, ysize;
  ZBuffer *zb;

  if (ctx->gl_context == NULL) {
      /* create the TinyGL context */
      xsize = width;
      ysize = height;


      if (ctx->dib_info == NULL) {
          ctx->dib_info = gl_malloc(sizeof(BITMAPINFO));
          if (!ctx->dib_info) {
              fprintf(stderr, "Error while initializeing DIB info.\n");
              exit(1);

            }

          ctx->dib_info->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
          ctx->dib_info->bmiHeader.biWidth=width;
          ctx->dib_info->bmiHeader.biHeight=height;
          ctx->dib_info->bmiHeader.biPlanes=1;
          ctx->dib_info->bmiHeader.biBitCount=16;
          ctx->dib_info->bmiHeader.biCompression=BI_RGB;
          ctx->dib_info->bmiHeader.biSizeImage=0;
          ctx->dib_info->bmiHeader.biXPelsPerMeter=0;
          ctx->dib_info->bmiHeader.biYPelsPerMeter=0;
          ctx->dib_info->bmiHeader.biClrUsed=0;
          ctx->dib_info->bmiHeader.biClrImportant=0;

      }

      /* currently, we only support 16 bit rendering */
      mode = ZB_MODE_5R6G5B;
      //zb=ZB_open(xsize,ysize,mode,0,NULL,NULL,NULL);
      zb=ZB_open(xsize,ysize,mode,0,NULL,NULL,NULL);
      if (zb == NULL) {
          fprintf(stderr, "Error while initializing Z buffer\n");
          exit(1);
      }

      ctx->pixtype = MWPF_TRUECOLOR565;


      /* initialisation of the TinyGL interpreter */
      glInit(zb);
      ctx->gl_context=gl_get_context();
      ctx->gl_context->opaque=(void *) ctx;
      ctx->gl_context->gl_resize_viewport=gdi_resize_viewport;

      /* set the viewport : we force a call to gdi_resize_viewport */
      ctx->gl_context->viewport.xsize=-1;
      ctx->gl_context->viewport.ysize=-1;

      glViewport(0, 0, xsize, ysize);
  }

  return 1;
}

void gdiSwapBuffers(HWND drawable)
{
    GLContext *gl_context;
    TinyGDIContext *ctx;
    HDC hdc;

    /* retrieve the current GDIContext */
    gl_context=gl_get_context();
    ctx=(TinyGDIContext *)gl_context->opaque;

    hdc = GetDC(drawable);

    StretchDIBits(hdc,0,ctx->ysize,ctx->xsize,-ctx->ysize, 0,0,ctx->xsize,ctx->ysize, gl_context->zb->pbuf, ctx->dib_info, DIB_RGB_COLORS,SRCCOPY);

    ReleaseDC(drawable, hdc);

    ShowWindow (drawable, SW_SHOW);
    UpdateWindow (drawable);
}

