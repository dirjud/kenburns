/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) <2010> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * This file was (probably) generated from gstvideoflip.c,
 * gstvideoflip.c,v 1.7 2003/11/08 02:48:59 dschleef Exp 
 */
/**
 * SECTION:element-kenburns
 *
 * kenburns is a keyword for the Ken Burns effect
 * (http://en.wikipedia.org/wiki/Ken_burns_effect) named after Ken
 * Burns. It zooms and pans in a slow effect that really pulls focus
 * into the image.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch videotestsrc ! kenburns ! autovideosink
 * ]| This pipeline does the default zoom/pan on the videotestpattern. The
sharp lines in this test pattern and the nearest neighbor interpolation make
it look jumpy. A more natural still image that is higher resolution than the
output will look more smooth.
 * <title<Example with still image</title>
 * |[
 * gst-launch filesrc location=test.jpg ! decodebin2 ! imagefreeze ! kenburns zoom1=0.75 xcenter1=0.75 ycenter1=0.25 zoom2=1.0 xcenter2=0.5 ycenter2=0.5 duration=10000000000 ! video/x-raw-yuv,width=640,height=480 ! autovideosink
 * ]| This will read in a still image called test.jpg, decode it, and pass it 
 * to the imagefreeze element to produce a video stream from the still image.
 * The kenburns element then starts in zoomed into the upper right region and
 * pans out to the entire image over the duration of 10 seconds. The capsfilter
 * at the end causes the kenburns element to resize to the desired output
 * resolution.
 * </refsect2>
 *
 * 
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstkenburns.h"

#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/controller/gstcontroller.h>
#include <math.h>

#define DEFAULT_XPOS 0.0
#define DEFAULT_YPOS 0.0
#define DEFAULT_ZPOS 1.0
#define DEFAULT_XROT 0.0
#define DEFAULT_YROT 0.0
#define DEFAULT_ZROT 0.0
#define DEFAULT_INTERP_METHOD GST_KENBURNS_INTERP_METHOD_NEAREST
#define DEFAULT_BORDER   0
#define DEFAULT_FOV 60
#define DEFAULT_BGCOLOR 0x00000000

enum {
  BG_ALPHA,
  BG_RED,
  BG_GREEN,
  BG_BLUE,
};

/* GstKenburns properties */

enum
{
  PROP_0,
  PROP_XPOS,
  PROP_YPOS,
  PROP_ZPOS,
  PROP_XROT,
  PROP_YROT,
  PROP_ZROT,
  PROP_INTERP_METHOD,
  PROP_BORDER,
  PROP_FOV,
  PROP_BGCOLOR,
  /* FILL ME */
};


#define COMP_Y(ret, r, g, b) \
{ \
   ret = (int) (((19595 * r) >> 16) + ((38470 * g) >> 16) + ((7471 * b) >> 16)); \
   ret = CLAMP (ret, 0, 255); \
}

#define COMP_U(ret, r, g, b) \
{ \
   ret = (int) (-((11059 * r) >> 16) - ((21709 * g) >> 16) + ((32768 * b) >> 16) + 128); \
   ret = CLAMP (ret, 0, 255); \
}

#define COMP_V(ret, r, g, b) \
{ \
   ret = (int) (((32768 * r) >> 16) - ((27439 * g) >> 16) - ((5329 * b) >> 16) + 128); \
   ret = CLAMP (ret, 0, 255); \
}

GST_DEBUG_CATEGORY_STATIC (gst_kenburns_debug);
#define GST_CAT_DEFAULT gst_kenburns_debug

static GstStaticPadTemplate gst_kenburns_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
		    GST_VIDEO_CAPS_YUV("{AYUV, I420}") ";"
    )
    );

static GstStaticPadTemplate gst_kenburns_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{AYUV, I420}") ";"
    )
    );

GST_BOILERPLATE (GstKenburns, gst_kenburns, GstVideoFilter,
    GST_TYPE_VIDEO_FILTER);

#define GST_TYPE_KENBURNS_INTERP_METHOD (gst_kenburns_interp_method_get_type())
static GType
gst_kenburns_interp_method_get_type (void)
{
  static GType kenburns_interp_method_type = 0;
  static const GEnumValue kenburns_interp_method[] = {
    {GST_KENBURNS_INTERP_METHOD_NEAREST, "nearest", "nearest"},
    {0, NULL, NULL},
  };

  if (!kenburns_interp_method_type) {
    kenburns_interp_method_type =
        g_enum_register_static ("GstKenburnsInterpType", kenburns_interp_method);
  }
  return kenburns_interp_method_type;
}

static gboolean gst_kenburns_set_caps (GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps) {
  GstKenburns *kb = GST_KENBURNS (trans);
  gboolean ret;
  ret = gst_video_format_parse_caps (incaps, &kb->src_fmt, &kb->src_width, &kb->src_height);
  if (!ret) {
    GST_ERROR_OBJECT (trans, "Invalid caps: %" GST_PTR_FORMAT, incaps);
    return FALSE;
  }

  ret = gst_video_format_parse_caps (outcaps, &kb->dst_fmt, &kb->dst_width, &kb->dst_height);
  if (!ret) {
    GST_ERROR_OBJECT (trans, "Invalid caps: %" GST_PTR_FORMAT, outcaps);
    return FALSE;
  }

  return TRUE;
  // nocap:
  //  return FALSE;
}

static GstCaps *
gst_kenburns_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * from)
{
  GstKenburns *kb = GST_KENBURNS (trans);
  GstCaps *to, *ret;
  const GstCaps *templ;
  GstStructure *structure;
  GstPad *other;

  to = gst_caps_copy (from);
  /* Just to be sure... */
  gst_caps_truncate (to);
  structure = gst_caps_get_structure (to, 0);

  // let the width and height transform
  gst_structure_remove_field (structure, "width");
  gst_structure_remove_field (structure, "height");

  // everything else has to stay identical

  /* filter against set allowed caps on the pad */
  other = (direction == GST_PAD_SINK) ? trans->srcpad : trans->sinkpad;

  templ = gst_pad_get_pad_template_caps (other);
  ret = gst_caps_intersect (to, templ);
  gst_caps_unref (to);

  GST_DEBUG_OBJECT (kb, "direction %d, transformed %" GST_PTR_FORMAT
      " to %" GST_PTR_FORMAT, direction, from, ret);

  return ret;
}


/*
static void scale_and_crop_ayuv(GstKenburns *kb,const guint8 *src,guint8 *dst) {
  double src_ratio, dst_ratio;
  double wsrc, hsrc, wdst, hdst;
  double xzoom, yzoom, xrotxy, yrotxy;
  double txdst, tydst, txsrc, tysrc;
  double zx, zy, x0, y0, cos_theta, sin_theta;
  int xdst, ydst, xsrc, ysrc;
  int dst_stride, src_stride;
  int pos_dst, pos_src;

  dst_stride = gst_video_format_get_row_stride(kb->dst_fmt, 0, kb->dst_width);
  src_stride = gst_video_format_get_row_stride(kb->src_fmt, 0, kb->src_width);

  // Calculate the width and height being cropped out of the input image.
  // This only depends on the zpos (zoom) and the aspect ratio of the output
  // view port as the unit zpos needs calculated from the letterboxed input
  // image.
  src_ratio = kb->src_width / (double) kb->src_height;
  dst_ratio = kb->dst_width / (double) kb->dst_height;
  if(src_ratio > dst_ratio) {
    wsrc = kb->src_width * kb->zpos;
    hsrc = wsrc / dst_ratio;
  } else {
    hsrc = kb->src_height * kb->zpos;
    wsrc = hsrc * dst_ratio;
  }
  wdst = kb->dst_width;
  hdst = kb->dst_height;
  zx = wsrc / wdst;
  zy = hsrc / hdst;
  x0 = kb->xpos * kb->src_width;
  y0 = kb->ypos * kb->src_height;
  
  cos_theta = cos(kb->zrot * M_PI / 180);
  sin_theta = sin(kb->zrot * M_PI / 180);

  txdst = kb->dst_width /2.0 - 0.5;
  tydst = kb->dst_height/2.0 - 0.5;
  txsrc = kb->src_width /2.0;
  tysrc = kb->src_height/2.0;

  for(ydst=0; ydst < kb->dst_height; ydst++) {
    for(xdst=0; xdst < kb->dst_width; xdst++) {
      //translate to dest img center coord, zoom, and tranlate by xpos/ypos.
      xzoom = (xdst - txdst) * zx + x0;
      yzoom = (ydst - tydst) * zy + y0;

      // rotate and translate back from center coordinates
      xrotxy = xzoom * cos_theta - yzoom * sin_theta + txsrc;
      yrotxy = xzoom * sin_theta + yzoom * cos_theta + tysrc;

      xsrc = (int) (xrotxy + 0.4999);
      ysrc = (int) (yrotxy + 0.4999);

      pos_dst = xdst*4 + ydst * dst_stride;
      pos_src = xsrc*4 + ysrc * src_stride;

      if(xsrc < 0 || xsrc >= kb->src_width || 
	 ysrc < 0 || ysrc >= kb->src_height ||
	 xdst < kb->border || xdst >= kb->dst_width  - kb->border ||
	 ydst < kb->border || ydst >= kb->dst_height - kb->border) {
	// use transparent black if requesting a pixel that is out of bounds
	memset(dst + pos_dst, 0, 4);
      } else {
	// otherwise copy the nearest neighbor pixel
	memcpy(dst + pos_dst, src + pos_src, 4);
      }
    }
  }
}
*/

#define TRANSFORM_SETUP(kb) \
  double src_aspect_ratio, dst_aspect_ratio; \
  double wsrc, hsrc, zoomx, zoomy; \
  double thetax, thetay, thetaz, yd0, xd0, z0;	\
  double cos_thetax, sin_thetax, tan_thetax; \
  double cos_thetay, sin_thetay, tan_thetay; \
  double cos_thetaz, sin_thetaz; \
  \
  src_aspect_ratio  = kb->src_width / (double) kb->src_height; \
  dst_aspect_ratio  = kb->dst_width / (double) kb->dst_height; \
  /* calculate letterbox width and height based on output aspect ratio */\
  if(src_aspect_ratio > dst_aspect_ratio) { \
    wsrc = kb->src_width;    \
    hsrc = wsrc / dst_aspect_ratio; \
  } else { \
    hsrc = kb->src_height; \
    wsrc = hsrc * dst_aspect_ratio; \
  } \
  zoomx = wsrc / kb->dst_width; \
  zoomy = hsrc / kb->dst_height;\
  \
  thetax = kb->xrot*M_PI/180; \
  thetay = kb->yrot*M_PI/180; \
  thetaz = kb->zrot*M_PI/180; \
  cos_thetax = cos(thetax); sin_thetax = sin(thetax); tan_thetax = tan(thetax);\
  cos_thetay = cos(thetay); sin_thetay = sin(thetay); tan_thetay = tan(thetay);\
  cos_thetaz = cos(thetaz); sin_thetaz = sin(thetaz); \
  \
  xd0 = 0.5 - kb->dst_width/2  - (wsrc-kb->src_width) / 2 / zoomx / kb->zpos; \
  yd0 = 0.5 - kb->dst_height/2 - (hsrc-kb->src_height)/ 2 / zoomy / kb->zpos; \
  z0 = wsrc / 2 / tan(kb->fov / 2 / 180 * M_PI); \

#define TRANSFORM(xsrc, ysrc, xdst, ydst) \
  { \
  double x0, y0, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4;	\
      \
      /* translate dest image coordinates to input image coordinates	\
         (0,0) at the center of the input image */			\
      x0 = ( xdst + xd0 ) * zoomx; \
      y0 = ( ydst + yd0 ) * zoomy; \
      \
      /* do the z-axis rotation first because it is easiest */	\
      x1 =  x0 * cos_thetaz + y0 * sin_thetaz; \
      y1 = -x0 * sin_thetaz + y0 * cos_thetaz; \
      z1 =  z0; \
      \
      /* find y-axis rotated image crosses the FOV */ \
      z2 = z1 * z1 / (z1 + y1 * tan_thetay); \
      y2 = y1 * z1 / (z1 + y1 * tan_thetay); \
      x2 = x1 * z2 / z1; \
      \
      /* find x-axis rotated image crosses the FOV */ \
      z3 = z2 * z2 / (z2 + x2 * tan_thetax); \
      x3 = x2 * z2 / (z2 + x2 * tan_thetax); \
      y3 = y2 * z3 / z2; \
      \
      /* rotate back to source image plane */		\
      x4 =  x3*cos_thetax                            - (z3-z1)*sin_thetax; \
      y4 =  x3*sin_thetax*sin_thetay + y3*cos_thetay - (z3-z1)*sin_thetay*cos_thetax; \
      \
      /* perform zoom and translation and then translate back to coordinates \
         with (0,0) in the upper left corner */ 	\
      xsrc = (x4 + kb->xpos*wsrc/2) * kb->zpos + wsrc/2; \
      ysrc = (y4 + kb->ypos*hsrc/2) * kb->zpos + hsrc/2; \
  }

#define OUT_OF_BOUNDS(kb, xsrc, ysrc, xdst, ydst)	\
  (xsrc < 0 || xsrc >= kb->src_width  || \
   ysrc < 0 || ysrc >= kb->src_height || \
   xdst < kb->border || xdst >= kb->dst_width  - kb->border || \
   ydst < kb->border || ydst >= kb->dst_height - kb->border)


static void transform_ayuv(GstKenburns *kb,const guint8 *src,guint8 *dst) {
  double xsrc0, ysrc0;
  int xdst, ydst, xsrc, ysrc;
  int pos_dst, pos_src;
  int dst_stride, src_stride; 
  guint8 bgcolor[4]; // background color

  dst_stride = gst_video_format_get_row_stride(kb->dst_fmt, 0, kb->dst_width);
  src_stride = gst_video_format_get_row_stride(kb->src_fmt, 0, kb->src_width);

  TRANSFORM_SETUP(kb);

  bgcolor[0] = kb->bgcolor[BG_ALPHA];
  COMP_Y (bgcolor[1], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);
  COMP_U (bgcolor[2], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);
  COMP_V (bgcolor[3], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);

  for(ydst=0; ydst < kb->dst_height; ydst++) {
    for(xdst=0; xdst < kb->dst_width; xdst++) {
      TRANSFORM (xsrc0, ysrc0, xdst, ydst);

      // We have the position of source image as a float, so convert it
      // to the nearest neighbor.
      xsrc = (int) floor(xsrc0);
      ysrc = (int) floor(ysrc0);

      pos_dst = xdst*4 + ydst * dst_stride;
      pos_src = xsrc*4 + ysrc * src_stride;

      // check if this pixel is in bounds and if not, then pass the specified
      // background color.
      if( OUT_OF_BOUNDS (kb, xsrc, ysrc, xdst, ydst) ) {
	// use transparent black if requesting a pixel that is out of bounds
	memcpy(dst + pos_dst, bgcolor, 4);
      } else {
	// otherwise copy the nearest neighbor pixel
	memcpy(dst + pos_dst, src + pos_src, 4);
      }
    }
  }
}

static void transform_i420(GstKenburns *kb, const guint8 *src, guint8 *dst) {
  double xsrc0, ysrc0;
  int xdst, ydst, xsrc, ysrc;
  int dst_strideY, dst_strideUV, src_strideY, src_strideUV;
  int dst_offsetU, dst_offsetV,  src_offsetU, src_offsetV;
  int posYdst, posUdst, posVdst;
  int posYsrc, posUsrc, posVsrc;
  int Y, U, V;
  guint8 bgcolor[3]; // background color

  dst_strideY  = gst_video_format_get_row_stride(kb->dst_fmt, 0, kb->dst_width);
  dst_strideUV = gst_video_format_get_row_stride(kb->dst_fmt, 1, kb->dst_width);
  src_strideY  = gst_video_format_get_row_stride(kb->src_fmt, 0, kb->src_width);
  src_strideUV = gst_video_format_get_row_stride(kb->src_fmt, 1, kb->src_width);

  dst_offsetU = gst_video_format_get_component_offset(kb->dst_fmt, 1, kb->dst_width, kb->dst_height);
  dst_offsetV = gst_video_format_get_component_offset(kb->dst_fmt, 2, kb->dst_width, kb->dst_height);
  src_offsetU = gst_video_format_get_component_offset(kb->src_fmt, 1, kb->src_width, kb->src_height);
  src_offsetV = gst_video_format_get_component_offset(kb->src_fmt, 2, kb->src_width, kb->src_height);

  TRANSFORM_SETUP(kb);

  COMP_Y (bgcolor[0], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);
  COMP_U (bgcolor[1], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);
  COMP_V (bgcolor[2], kb->bgcolor[BG_RED], kb->bgcolor[BG_GREEN], kb->bgcolor[BG_BLUE]);

  for(ydst=0; ydst < kb->dst_height; ydst++) {
    for(xdst=0; xdst < kb->dst_width; xdst++) {

      TRANSFORM (xsrc0, ysrc0, xdst, ydst);

      // We have the position of source image as a float, so convert it
      // to the nearest neighbor.
      xsrc = (int) floor(xsrc0);
      ysrc = (int) floor(ysrc0);

      // check if this pixel is in bounds and if not, then pass the specified
      // background color.
      if( OUT_OF_BOUNDS (kb, xsrc, ysrc, xdst, ydst) ) {
	// use transparent black if requesting a pixel that is out of bounds
	Y = bgcolor[0];
	U = bgcolor[1];
	V = bgcolor[2];
      } else {
	// otherwise copy the nearest neighbor pixel
	posYsrc = xsrc   + ysrc   * src_strideY;
	posUsrc = xsrc/2 + ysrc/2 * src_strideUV + src_offsetU;
	posVsrc = xsrc/2 + ysrc/2 * src_strideUV + src_offsetV;
	Y = src[posYsrc];
	U = src[posUsrc];
	V = src[posVsrc];
      }
      posYdst = xdst   + ydst   * dst_strideY;
      posUdst = xdst/2 + ydst/2 * dst_strideUV + dst_offsetU;
      posVdst = xdst/2 + ydst/2 * dst_strideUV + dst_offsetV;
      dst[posYdst] = Y;
      dst[posUdst] = U;
      dst[posVdst] = V;
    }
  }
}

static GstFlowReturn
gst_kenburns_transform (GstBaseTransform * trans, GstBuffer * in,
    GstBuffer * out)
{
  GstKenburns *kb = GST_KENBURNS (trans);
  guint8 *dst;
  const guint8 *src;

  src = GST_BUFFER_DATA (in);
  dst = GST_BUFFER_DATA (out);
  gst_object_sync_values (G_OBJECT (kb), GST_BUFFER_TIMESTAMP (in));

  GST_OBJECT_LOCK (kb);

  switch (kb->src_fmt) {
  case GST_VIDEO_FORMAT_I420:
    transform_i420(kb, src, dst);
    break;
  case GST_VIDEO_FORMAT_AYUV:
    transform_ayuv(kb, src, dst);
    break;
  default:
    break;
  }

  GST_OBJECT_UNLOCK (kb);

  return GST_FLOW_OK;
}

static void
gst_kenburns_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstKenburns *kb = GST_KENBURNS (object);

  switch (prop_id) {
    case PROP_XPOS:
      kb->xpos = g_value_get_double(value);
      break;
    case PROP_YPOS:
      kb->ypos = g_value_get_double(value);
      break;
    case PROP_ZPOS:
      kb->zpos = g_value_get_double(value);
      break;
    case PROP_XROT:
      kb->xrot = g_value_get_double(value);
      break;
    case PROP_YROT:
      kb->yrot = g_value_get_double(value);
      break;
    case PROP_ZROT:
      kb->zrot = g_value_get_double(value);
      break;
    case PROP_FOV:
      kb->fov = g_value_get_double(value);
      break;
    case PROP_INTERP_METHOD:
      kb->interp_method = g_value_get_enum (value);
      break;
    case PROP_BORDER:
      kb->border = g_value_get_int(value);
      break;
    case PROP_BGCOLOR:
      { uint tmp = g_value_get_uint(value);
	kb->bgcolor[BG_ALPHA] = (tmp >> 24) & 0xFF;
	kb->bgcolor[BG_RED]   = (tmp >> 16) & 0xFF;
	kb->bgcolor[BG_GREEN] = (tmp >>  8) & 0xFF;
	kb->bgcolor[BG_BLUE]  = (tmp >>  0) & 0xFF;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_kenburns_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstKenburns *kb = GST_KENBURNS (object);

  switch (prop_id) {
  case PROP_XPOS:
    g_value_set_double(value, kb->xpos);
    break;
  case PROP_YPOS:
    g_value_set_double(value, kb->ypos);
    break;
  case PROP_ZPOS:
    g_value_set_double(value, kb->zpos);
    break;
  case PROP_XROT:
    g_value_set_double(value, kb->xrot);
    break;
  case PROP_YROT:
    g_value_set_double(value, kb->yrot);
    break;
  case PROP_ZROT:
    g_value_set_double(value, kb->zrot);
    break;
  case PROP_FOV:
    g_value_set_double(value, kb->fov);
    break;
  case PROP_INTERP_METHOD:
    g_value_set_enum(value, kb->interp_method);
    break;
  case PROP_BORDER:
    g_value_set_int(value, kb->border);
    break;
  case PROP_BGCOLOR:
    g_value_set_uint(value, ((kb->bgcolor[BG_ALPHA] << 24) |
			     (kb->bgcolor[BG_RED]   << 16) |
			     (kb->bgcolor[BG_GREEN] <<  8) |
			     (kb->bgcolor[BG_BLUE]  <<  0) ));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_kenburns_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "percieve",
      "Filter/Effect/Video",
      "Kenburnsors video", "David Schleef <ds@schleef.org>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_kenburns_sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_kenburns_src_template));
}

static void
gst_kenburns_class_init (GstKenburnsClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_kenburns_debug, "percieve", 0, "percieve");

  gobject_class->set_property = gst_kenburns_set_property;
  gobject_class->get_property = gst_kenburns_get_property;

  g_object_class_install_property (gobject_class, PROP_XPOS,
      g_param_spec_double ("xpos", "x viewing position", "x=0.0 corresonds to no x translation. 1.0 corresponds to an x translation of the input image width.",
			   -G_MAXDOUBLE, G_MAXDOUBLE, DEFAULT_XPOS,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_YPOS,
      g_param_spec_double ("ypos", "y viewing position", "y=0.0 corresponds to no y translation. 1.0 corresponds to a y translation of the input image height.",
			   -G_MAXDOUBLE, G_MAXDOUBLE, DEFAULT_YPOS,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_ZPOS,
      g_param_spec_double ("zpos", "z viewing position", "z=1.0 corresponds to the viewing distance to see a letterbox image at the output.",
			   0.001, G_MAXDOUBLE, DEFAULT_ZPOS,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_XROT,
      g_param_spec_double ("xrot", "roation about x axis", "Rotation of input image about the x-axis in degrees about its center.",
			   -G_MAXDOUBLE, G_MAXDOUBLE, DEFAULT_XROT,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_YROT,
      g_param_spec_double ("yrot", "roation about y axis", "Rotation of input image about the y-axis in degrees about its center.",
			   -G_MAXDOUBLE, G_MAXDOUBLE, DEFAULT_YROT,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_ZROT,
      g_param_spec_double ("zrot", "roation about z axis", "Rotation of input image about the z-axis in degrees about its center.",
			   -G_MAXDOUBLE, G_MAXDOUBLE, DEFAULT_ZROT,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_FOV,
      g_param_spec_double ("fov", "Field of View Angle", "Total angle in field of view.",
			   0.001, 180, DEFAULT_FOV,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_INTERP_METHOD,
      g_param_spec_enum ("interp-method", "Interpolation method",
			 "Method for interpolating the output image", 
			 GST_TYPE_KENBURNS_INTERP_METHOD,
			 DEFAULT_INTERP_METHOD,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BORDER,
      g_param_spec_int ("border", "Frame border on output image", "Number of pixels to use as a border around the output image.",
			   0, G_MAXINT32, DEFAULT_BORDER,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_BGCOLOR,
      g_param_spec_uint ("background-color", "Background Color", "Color to use for background. Should be of the form '(a<<24) | (r<<16) | (g<<8) | (b<<0)' where a is alpha, r is red, g is green, and b is blue. Alpha will be ignored for formats that do not support alpha.",
			   0, G_MAXUINT32, DEFAULT_BGCOLOR,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  trans_class->set_caps       = GST_DEBUG_FUNCPTR (gst_kenburns_set_caps);
  trans_class->transform      = GST_DEBUG_FUNCPTR (gst_kenburns_transform);
  trans_class->transform_caps = GST_DEBUG_FUNCPTR (gst_kenburns_transform_caps);
}

static void
gst_kenburns_init (GstKenburns * kb, GstKenburnsClass * klass)
{
  kb->xpos      = DEFAULT_XPOS;
  kb->ypos      = DEFAULT_YPOS;
  kb->zpos      = DEFAULT_ZPOS;
  kb->xrot      = DEFAULT_XROT;
  kb->yrot      = DEFAULT_YROT;
  kb->zrot      = DEFAULT_ZROT;
  kb->interp_method = DEFAULT_INTERP_METHOD;
  kb->border  = DEFAULT_BORDER;
  kb->fov     = DEFAULT_FOV;
  kb->bgcolor[BG_ALPHA] = (DEFAULT_BGCOLOR >> 24) & 0xFF;
  kb->bgcolor[BG_RED]   = (DEFAULT_BGCOLOR >> 16) & 0xFF;
  kb->bgcolor[BG_GREEN] = (DEFAULT_BGCOLOR >> 8)  & 0xFF;
  kb->bgcolor[BG_BLUE]  = (DEFAULT_BGCOLOR >> 0)  & 0xFF;
  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (kb), FALSE);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
kenburns_init (GstPlugin * kenburns)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template motiondetect' with your description
   */
  //GST_DEBUG_CATEGORY_INIT (gst_kenburns_debug, "kenburns",
  //    0, "Overlay icons on a video stream and optionally have them blink");
  gst_controller_init (NULL, NULL);

  return gst_element_register (kenburns, "percieve", GST_RANK_NONE,
      GST_TYPE_KENBURNS);
}


/* gstreamer looks for this structure to register kenburnss
 *
 * exchange the string 'Template kenburns' with your kenburns description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "precieve",
    "Ken Burns still zoom/crop/pan",
    kenburns_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
