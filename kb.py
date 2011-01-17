import gst, gobject, sys
argv = sys.argv
gobject.threads_init()

src = gst.element_factory_make("videotestsrc")
caps1 = gst.element_factory_make("capsfilter")
caps1.props.caps = gst.Caps("video/x-raw-yuv,width=1280,height=960,format=(fourcc)I420,framerate=(fraction)15/1")
kb = gst.element_factory_make("kenburns")
caps2 = gst.element_factory_make("capsfilter")
caps2.props.caps = gst.Caps("video/x-raw-yuv,width=640,height=480,format=(fourcc)I420,framerate=(fraction)15/1")
dur = 2*gst.SECOND

filename = "transform"

kb.props.zpos = 1.3
kb.props.xpos = -1.8
kb.props.zrot = 0
kb.props.yrot = 0
kb.props.xrot = 0
kb.props.fov  = 60
controller = gst.Controller(kb, "zpos", "xpos", "ypos", "zrot", "yrot", "xrot", "fov")
#controller.set_interpolation_mode("zpos", gst.INTERPOLATE_LINEAR)
freq = 0.25 

def LFO(which, amplitude, offset, timeshift=0, freq_factor=2.1):
    global freq, filename, controller
    c = gst.LFOControlSource()
    c.props.amplitude = amplitude
    c.props.offset    = offset
    c.props.frequency = freq
    c.props.timeshift = timeshift
    controller.set_control_source(which, c)
    freq = freq / freq_factor
    filename += which

#LFO("xpos", amplitude=0.5, offset=0.0, freq_factor=1.0)
#LFO("ypos", amplitude=0.5, offset=0.0, timeshift=int(1.0 / freq / 4.0 * gst.SECOND))
#LFO("zpos", amplitude=1.0, offset=1.25)
#LFO("zrot", amplitude=180, offset=0)
#LFO("yrot", amplitude=180, offset=0)
#LFO("xrot", amplitude=180, offset=0)
#LFO("fov",  amplitude=30,  offset=60)

queue = gst.element_factory_make("queue")
queue.props.max_size_time = 10 * gst.SECOND
queue.props.max_size_bytes   = 0
queue.props.max_size_buffers = 0

loop = gobject.MainLoop(is_running=True)

if 1:
    sink  = gst.element_factory_make("autovideosink")
    pipeline = gst.Pipeline()
    pipeline.add(src,caps1, kb, caps2, queue, sink)
    gst.element_link_many(src, caps1, kb, caps2, queue, sink)
else:
    gnlcomp = gst.element_factory_make("gnlcomposition")
    gnlsrc  = gst.element_factory_make("gnlsource")
    bin = gst.Bin()
    bin.add(src, caps1, kb, caps2)
    gst.element_link_many(src, caps1, kb, caps2)
    bin.add_pad(gst.GhostPad("src", caps.get_pad("src")))
    gnlsrc.add(bin)
    gnlcomp.add(gnlsrc)

    enc = gst.element_factory_make("ffenc_mpeg4")
    mux = gst.element_factory_make("mp4mux")
    snk = gst.element_factory_make("filesink")
    snk.props.location = filename + ".mp4"
    pipeline = gst.Pipeline()
    pipeline.add(gnlcomp, enc, mux, snk)
    gst.element_link_many(enc, mux, snk)

    def on_pad(comp, pad, element):
        capspad = element.get_compatible_pad(pad, pad.get_caps())
        pad.link(capspad)
    gnlcomp.connect("pad-added", on_pad, enc)

    gnlsrc.props.start = 0
    gnlsrc.props.duration = 20 * gst.SECOND
    gnlsrc.props.media_start = 0
    gnlsrc.props.media_duration = 20 * gst.SECOND
    

# now run the pipeline
bus = pipeline.get_bus()
bus.add_signal_watch()
def on_message(bus, message, loop):
    if message.type == gst.MESSAGE_EOS:
        loop.quit()
    elif message.type == gst.MESSAGE_ERROR:
        print message
        loop.quit()
bus.connect("message", on_message, loop)
pipeline.set_state(gst.STATE_PLAYING)
loop.run()
pipeline.set_state(gst.STATE_NULL)
