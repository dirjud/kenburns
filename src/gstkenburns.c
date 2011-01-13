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

#define DEFAULT_ZOOM1 1.0
#define DEFAULT_ZOOM2 0.5
#define DEFAULT_XCENTER1 0.5
#define DEFAULT_YCENTER1 0.5
#define DEFAULT_XCENTER2 0.5
#define DEFAULT_YCENTER2 0.5
#define DEFAULT_DURATION 5000000000ll
#define DEFAULT_INTERP_METHOD GST_KENBURNS_INTERP_METHOD_NEAREST
#define DEFAULT_PAN_METHOD    GST_KENBURNS_PAN_METHOD_VELOCITY_RAMP
#define DEFAULT_PAN_ACCEL     0.5
#define DEFAULT_BORDER   0

/* GstKenburns properties */
enum
{
  PROP_0,
  PROP_ZOOM_START,
  PROP_ZOOM_END,
  PROP_XCENTER_START,
  PROP_YCENTER_START,
  PROP_XCENTER_END,
  PROP_YCENTER_END,
  PROP_DURATION,
  PROP_INTERP_METHOD,
  PROP_PAN_METHOD,
  PROP_PAN_ACCEL,
  PROP_BORDER,
  /* FILL ME */
};

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

#define GST_TYPE_KENBURNS_PAN_METHOD (gst_kenburns_pan_method_get_type())
static GType
gst_kenburns_pan_method_get_type (void)
{
  static GType kenburns_pan_method_type = 0;
  static const GEnumValue kenburns_pan_method[] = {
    {GST_KENBURNS_PAN_METHOD_EXTERNAL, "external", "Motion is controlled externally, allowing GstControllers on 'zoom1', 'xcenter1', and 'ycenter1'."}, 
    {GST_KENBURNS_PAN_METHOD_LINEAR, "linear", "Linear panning (constant velocity)"},
    {GST_KENBURNS_PAN_METHOD_POWER,  "power",  "The pan distance is calculated by the distance raised to a specified power." },
    {GST_KENBURNS_PAN_METHOD_VELOCITY_RAMP,  "ramp",   "Ramp velocity of pan with a constant velocity both up and down." },
    {0, NULL, NULL},
  };

  if (!kenburns_pan_method_type) {
    kenburns_pan_method_type =
        g_enum_register_static ("GstKenburnsPanType", kenburns_pan_method);
  }
  return kenburns_pan_method_type;
}


static void update_start_stop_params(GstKenburns *kb) {
  double src_ratio, dst_ratio;
  double width_start, height_start, width_end, height_end;
  src_ratio = kb->src_width / (double) kb->src_height;
  dst_ratio = kb->dst_width / (double) kb->dst_height;

  if(src_ratio > dst_ratio) {
    width_start  = kb->src_width * kb->zoom1;
    height_start = width_start / dst_ratio;
    width_end    = kb->src_width * kb->zoom2;
    height_end   = width_end   / dst_ratio;
  } else {
    height_start = kb->src_height * kb->zoom1;
    width_start  = height_start * dst_ratio;
    height_end   = kb->src_height * kb->zoom2;
    width_end    = height_end * dst_ratio;
  }
  kb->x0start = kb->xcenter1 * kb->src_width  - width_start/2;
  kb->x1start = kb->xcenter1 * kb->src_width  + width_start/2;
  kb->y0start = kb->ycenter1 * kb->src_height - height_start/2;
  kb->y1start = kb->ycenter1 * kb->src_height + height_start/2;
  kb->x0end   = kb->xcenter2 * kb->src_width  - width_end/2;
  kb->x1end   = kb->xcenter2 * kb->src_width  + width_end/2;
  kb->y0end   = kb->ycenter2 * kb->src_height - height_end/2;
  kb->y1end   = kb->ycenter2 * kb->src_height + height_end/2;
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

  update_start_stop_params(kb);

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

static void scale_and_crop_i420(GstKenburns *kb, 
				const guint8 *src, 
				guint8 *dst,
				double x0, double y0, double x1, double y1) {
  int xdst, ydst, xsrc, ysrc, Y, U, V;
  int dst_strideY, dst_strideUV, src_strideY, src_strideUV;
  int dst_offsetU, dst_offsetV,  src_offsetU, src_offsetV;
  int posYdst, posUdst, posVdst;
  int posYsrc, posUsrc, posVsrc;

  dst_strideY  = gst_video_format_get_row_stride(kb->dst_fmt, 0, kb->dst_width);
  dst_strideUV = gst_video_format_get_row_stride(kb->dst_fmt, 1, kb->dst_width);
  src_strideY  = gst_video_format_get_row_stride(kb->src_fmt, 0, kb->src_width);
  src_strideUV = gst_video_format_get_row_stride(kb->src_fmt, 1, kb->src_width);

  dst_offsetU = gst_video_format_get_component_offset(kb->dst_fmt, 1, kb->dst_width, kb->dst_height);
  dst_offsetV = gst_video_format_get_component_offset(kb->dst_fmt, 2, kb->dst_width, kb->dst_height);
  src_offsetU = gst_video_format_get_component_offset(kb->src_fmt, 1, kb->src_width, kb->src_height);
  src_offsetV = gst_video_format_get_component_offset(kb->src_fmt, 2, kb->src_width, kb->src_height);

  double zx = (x1-x0) / (kb->dst_width);
  double zy = (y1-y0) / (kb->dst_height);

  for(ydst=0; ydst < kb->dst_height; ydst++) {
    for(xdst=0; xdst < kb->dst_width; xdst++) {
      xsrc = x0 + (xdst + 0.5) * zx;
      ysrc = y0 + (ydst + 0.5) * zy;
      if(xsrc < 0 || xsrc >= kb->src_width || 
	 ysrc < 0 || ysrc >= kb->src_height ||
	 xdst < kb->border || xdst >= kb->dst_width  - kb->border ||
	 ydst < kb->border || ydst >= kb->dst_height - kb->border) {
	// use black if requesting a pixel that is out of bounds
	Y  = 0;
	U = 128;
	V = 128;
      } else {
	posYsrc = xsrc + ysrc*src_strideY;
	posUsrc = xsrc/2 + ysrc/2*src_strideUV + src_offsetU;
	posVsrc = xsrc/2 + ysrc/2*src_strideUV + src_offsetV;
	Y = src[posYsrc];
	U = src[posUsrc];
	V = src[posVsrc];
      }
      posYdst = xdst + ydst*dst_strideY;
      posUdst = xdst/2 + ydst/2*dst_strideUV + dst_offsetU;
      posVdst = xdst/2 + ydst/2*dst_strideUV + dst_offsetV;
      dst[posYdst] = Y;
      dst[posUdst] = U;
      dst[posVdst] = V;
    }
  }
}

static void scale_and_crop_ayuv(GstKenburns *kb, 
				const guint8 *src, 
				guint8 *dst,
				double x0, double y0, double x1, double y1) {
  int xdst, ydst, xsrc, ysrc;
  int dst_stride, src_stride;
  int pos_dst, pos_src;

  dst_stride = gst_video_format_get_row_stride(kb->dst_fmt, 0, kb->dst_width);
  src_stride = gst_video_format_get_row_stride(kb->src_fmt, 0, kb->src_width);

  double zx = (x1-x0) / (kb->dst_width );
  double zy = (y1-y0) / (kb->dst_height);

  for(ydst=0; ydst < kb->dst_height; ydst++) {
    for(xdst=0; xdst < kb->dst_width; xdst++) {
      xsrc = x0 + (xdst + 0.5) * zx;
      ysrc = y0 + (ydst + 0.5) * zy;

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

static GstFlowReturn
gst_kenburns_transform (GstBaseTransform * trans, GstBuffer * in,
    GstBuffer * out)
{
  GstKenburns *kb = GST_KENBURNS (trans);
  guint8 *dst;
  const guint8 *src;
  double x0, y0, x1, y1, dist,p;

  src = GST_BUFFER_DATA (in);
  dst = GST_BUFFER_DATA (out);

  if(kb->pan_method == GST_KENBURNS_PAN_METHOD_EXTERNAL) {
    gst_object_sync_values (G_OBJECT (kb), GST_BUFFER_TIMESTAMP (in));
    update_start_stop_params(kb);
  }

  GST_OBJECT_LOCK (kb);
  GstClockTime ts = GST_BUFFER_TIMESTAMP(in);
  if(ts >= kb->duration) ts = kb->duration;

  if(kb->duration == 0) {
    dist = 0;
  } else {
    dist = (double) ts / kb->duration; // linear pan is baseline and gets modified as requested
  }

  switch(kb->pan_method) {
  case GST_KENBURNS_PAN_METHOD_POWER:
    p = 1.0/(1.0-kb->pan_accel/2.0);
    if(dist < 0.5) {
      dist =     pow(2*dist,     p) / 2.0;
    } else {
      dist = 1 - pow(2*(1-dist), p) / 2.0;
    }
    break;
  case GST_KENBURNS_PAN_METHOD_VELOCITY_RAMP:
    p = kb->pan_accel/2.0;
    if(p>0) {// p==0 means linear, so we do nothing to avoid the divide by 0
      if(dist < p) {
	dist = dist*dist/2.0/p/(1-p);
      } else if(dist < (1 - p)) {
	dist = (dist - p/2) / (1-p);
      } else {
	dist = (dist - dist*dist/2 - p*p + p - 0.5)/p/(1-p);
      }
    }
    break;
  case GST_KENBURNS_PAN_METHOD_EXTERNAL:
    dist = 0.0;
    break;
  default:
    break;
  }

  // make sure distance is within bounds
  if(dist > 1.0) dist = 1.0;
  if(dist < 0.0) dist = 0.0;

  x0 = kb->x0start + (kb->x0end-kb->x0start) * dist;
  y0 = kb->y0start + (kb->y0end-kb->y0start) * dist;
  x1 = kb->x1start + (kb->x1end-kb->x1start) * dist;
  y1 = kb->y1start + (kb->y1end-kb->y1start) * dist;

  switch (kb->src_fmt) {
  case GST_VIDEO_FORMAT_I420:
    scale_and_crop_i420(kb, src, dst, x0, y0, x1, y1);
    break;
  case GST_VIDEO_FORMAT_AYUV:
    scale_and_crop_ayuv(kb, src, dst, x0, y0, x1, y1);
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
    case PROP_ZOOM_START:
      kb->zoom1 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_ZOOM_END:
      kb->zoom2 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_XCENTER_START:
      kb->xcenter1 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_YCENTER_START:
      kb->ycenter1 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_XCENTER_END:
      kb->xcenter2 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_YCENTER_END:
      kb->ycenter2 = g_value_get_double(value);
      update_start_stop_params(kb);
      break;
    case PROP_DURATION:
      kb->duration = g_value_get_uint64(value);
      update_start_stop_params(kb);
      break;
    case PROP_INTERP_METHOD:
      kb->interp_method = g_value_get_enum (value);
      break;
    case PROP_PAN_METHOD:
      kb->pan_method = g_value_get_enum (value);
      break;
    case PROP_PAN_ACCEL:
      kb->pan_accel = g_value_get_double(value);
      break;
    case PROP_BORDER:
      kb->border = g_value_get_int(value);
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
    case PROP_ZOOM_START:
      g_value_set_double(value, kb->zoom1);
      break;
    case PROP_ZOOM_END:
      g_value_set_double(value, kb->zoom2);
      break;
    case PROP_XCENTER_START:
      g_value_set_double(value, kb->xcenter1);
      break;
    case PROP_YCENTER_START:
      g_value_set_double(value, kb->ycenter1);
      break;
    case PROP_XCENTER_END:
      g_value_set_double(value, kb->xcenter2);
      break;
    case PROP_YCENTER_END:
      g_value_set_double(value, kb->ycenter2);
      break;
    case PROP_DURATION:
      g_value_set_uint64(value, kb->duration);
      break;
    case PROP_INTERP_METHOD:
      g_value_set_enum(value, kb->interp_method);
      break;
    case PROP_PAN_METHOD:
      g_value_set_enum(value, kb->pan_method);
      break;
    case PROP_PAN_ACCEL:
      g_value_set_double(value, kb->pan_accel);
      break;
    case PROP_BORDER:
      g_value_set_int(value, kb->border);
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

  gst_element_class_set_details_simple (element_class, "kenburns",
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

  GST_DEBUG_CATEGORY_INIT (gst_kenburns_debug, "kenburns", 0, "kenburns");

  gobject_class->set_property = gst_kenburns_set_property;
  gobject_class->get_property = gst_kenburns_get_property;

  g_object_class_install_property (gobject_class, PROP_ZOOM_START,
      g_param_spec_double ("zoom1", "Starting Zoom", "Zoom of the initial image. 1.0 means output will be the same size as original. 0.5 means the output image will have half the zoom of the original image.",
			   0.01, 100.0, 0.9,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_ZOOM_END,
      g_param_spec_double ("zoom2", "Ending Zoom", "Ending Zoom",
			   0.01, 100.0, 0.8,
			   G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_XCENTER_START,
      g_param_spec_double ("xcenter1", "Starting X position", "Starting X position as proportion of image where 0.5 is the center.",
			   -100.0, 100.0, 0.5,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_YCENTER_START,
      g_param_spec_double ("ycenter1", "Starting Y position", "Starting Y position as proportion of image where 0.5 is the center.",
			   -100.0, 100.0, 0.5,
			   G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_XCENTER_END,
      g_param_spec_double ("xcenter2", "Ending X position", "Ending X position as proportion of image where 0.5 is the center.",
			   -100.0, 100.0, 0.5,
			   G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_YCENTER_END,
      g_param_spec_double ("ycenter2", "Ending Y position", "Ending Y position as proportion of image where 0.5 is the center.",
			   -100.0, 100.0, 0.5,
			   G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_DURATION,
      g_param_spec_uint64 ("duration", "Duration of Effect in nanoseconds", "Duration of effect in nanoseconds",
			   0, G_MAXUINT64, 5000000000ull,
			   G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_INTERP_METHOD,
      g_param_spec_enum ("interp-method", "Interpolation method",
			 "Method for interpolating the output image", 
			 GST_TYPE_KENBURNS_INTERP_METHOD,
			 DEFAULT_INTERP_METHOD,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PAN_METHOD,
      g_param_spec_enum ("pan-method", "Panning method",
			 "Method for panning the image. The linear method causes the pan to move uniformly with a constant velocity. The other seek a more gradual pan by giving a non-constant pan. The 'pan-accel' parameter controls this other methods.",
			 GST_TYPE_KENBURNS_PAN_METHOD,
			 DEFAULT_PAN_METHOD,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PAN_ACCEL,
      g_param_spec_double ("pan-accel", "Panning Acceleration", "0.0 means constant velocity and 1.0 is the max acceleration. Only valid when not in linear panning mode.",
			   0.0, 1.0, DEFAULT_PAN_ACCEL,
			   G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_BORDER,
      g_param_spec_int ("border", "Frame border on output image", "Number of pixels to use as a border around the output image.",
			   0, G_MAXINT32, DEFAULT_BORDER,
			   G_PARAM_READWRITE));

  trans_class->set_caps       = GST_DEBUG_FUNCPTR (gst_kenburns_set_caps);
  trans_class->transform      = GST_DEBUG_FUNCPTR (gst_kenburns_transform);
  trans_class->transform_caps = GST_DEBUG_FUNCPTR (gst_kenburns_transform_caps);
}

static void
gst_kenburns_init (GstKenburns * kb, GstKenburnsClass * klass)
{
  kb->zoom1         = DEFAULT_ZOOM1;
  kb->zoom2         = DEFAULT_ZOOM2;
  kb->xcenter1      = DEFAULT_XCENTER1;
  kb->ycenter1      = DEFAULT_YCENTER1;
  kb->xcenter2      = DEFAULT_XCENTER2;
  kb->ycenter2      = DEFAULT_YCENTER2;;
  kb->duration      = DEFAULT_DURATION;
  kb->interp_method = DEFAULT_INTERP_METHOD;
  kb->pan_method    = DEFAULT_PAN_METHOD;
  kb->pan_accel     = DEFAULT_PAN_ACCEL;

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

  return gst_element_register (kenburns, "kenburns", GST_RANK_NONE,
      GST_TYPE_KENBURNS);
}


/* gstreamer looks for this structure to register kenburnss
 *
 * exchange the string 'Template kenburns' with your kenburns description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "kenburns",
    "Ken Burns still zoom/crop/pan",
    kenburns_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
