/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifndef __GST_KENBURNS_H__
#define __GST_KENBURNS_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_KENBURNS \
  (gst_kenburns_get_type())
#define GST_KENBURNS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KENBURNS,GstKenburns))
#define GST_KENBURNS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KENBURNS,GstKenburnsClass))
#define GST_IS_KENBURNS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KENBURNS))
#define GST_IS_KENBURNS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KENBURNS))

typedef struct _GstKenburns GstKenburns;
typedef struct _GstKenburnsClass GstKenburnsClass;

/**
 * GstKenburnsInterpMethod:
 * @GST_KENBURNS_INTERP_METHOD_NEAREST: uses nearest neighbor interpolation. This is the fastest method but can have aliasing artifacts.
 *
 * Interpolation Method.
 */
typedef enum {
  GST_KENBURNS_INTERP_METHOD_NEAREST,
} GstKenburnsInterpMethod;

/**
 * GstKenburnsPanMethod:
 * @GST_KENBURNS_PAN_METHOD_EXTERNAL: motion is controlled externally using the 'zoom1', 'xcenter1', and 'ycenter1' favorables. Allows use of GstController to control zoom/panning.
 * @GST_KENBURNS_PAN_METHOD_LINEAR: pans the image linearly (constant velocity) from the start position to the ending position.
 * @GST_KENBURNS_PAN_METHOD_POWER: pans the image such that the distance of the pan, as a function of time, is dist = (2 * dist^param)/2. This gives a smoother start and stop.
 * @GST_KENBURNS_PAN_VELOCITY_RAMP: pans the image such that the velocity ramps up linearly, then plateaus, then ramps back down linearly.
 *
 * Panning Method.
 */
typedef enum {
  GST_KENBURNS_PAN_METHOD_EXTERNAL,
  GST_KENBURNS_PAN_METHOD_LINEAR,
  GST_KENBURNS_PAN_METHOD_POWER,
  GST_KENBURNS_PAN_METHOD_VELOCITY_RAMP,
} GstKenburnsPanMethod;

/**
 * GstKenburns:
 *
 * Opaque datastructure.
 */
struct _GstKenburns {
  GstVideoFilter videofilter;
  
  /* < private > */
  gint32 src_width, src_height;
  gint32 dst_width, dst_height;
  gint32 border;
  GstVideoFormat src_fmt, dst_fmt;

  // The starting and ending zoom factors are expressed as a fraction of the
  // original size of the image where 1.0 is unity and 0.5 means each
  // dimension is scaled down by two.
  gdouble zoom1, zoom2;

  // The starting and ending position are expressed as a fraction of
  // the original image where 0.5 is the center of the image.
  gdouble xcenter1, ycenter1, xcenter2, ycenter2; 

  // the duration in seconds over which the transition is to occur.
  GstClockTime duration;
  
  gdouble x0start, y0start, x1start, y1start;
  gdouble x0end,   y0end,   x1end,   y1end;

  GstKenburnsInterpMethod interp_method;
  GstKenburnsPanMethod pan_method;
  gdouble pan_accel;
};

struct _GstKenburnsClass {
  GstVideoFilterClass parent_class;
};

GType gst_kenburns_get_type (void);

G_END_DECLS

#endif /* __GST_KENBURNS_H__ */
