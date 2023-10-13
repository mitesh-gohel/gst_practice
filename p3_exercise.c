/* Basic tutorial 3  Exercise */
#include<gst/gst.h>

typedef struct _CustomData
{
	GstElement *pipeline;
	GstElement *source;
	GstElement *convert;
	//GstElement *resample;
	GstElement *sink;
}CustomData;

static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data);

int main(int argc, char ** argv)
{
	CustomData data;
	GstStateChangeReturn ret;
	GstBus *bus;
	GstMessage *msg;
	gboolean terminate = FALSE;

	gst_init(&argc, &argv);
	
	data.pipeline = gst_pipeline_new("test-pipeline");
	
	data.source = gst_element_factory_make("uridecodebin", "source");
	data.convert = gst_element_factory_make("videoconvert", "convert");
	//data.resample = gst_element_factory_make("videoresample", "resample");
	data.sink = gst_element_factory_make("autovideosink", "sink");

	if(!data.pipeline || !data.source || !data.convert || !data.sink)
	{
		g_printerr("All elements are not created\n");
		return -1;
	}
	
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.sink, NULL);
	/* Note : we will link  source letter */
	if(!gst_element_link_many(data.convert, data.sink, NULL))
	{
		g_printerr("Not able to link elements");
		gst_object_unref(data.pipeline);
		return -1;
	}

	/* Set URI to play */
	g_object_set(data.source, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm" , NULL);

	/* set hannder for "pad-added" signal */
	g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

	ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		g_printerr("State change failed\n");
		g_object_unref(data.pipeline);
		return -1;
	}

	bus = gst_element_get_bus(data.pipeline);
	do
	{
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);
		if(msg != NULL)
		{	
			GError *err;
			gchar *debug_info;
			switch(GST_MESSAGE_TYPE(msg))
			{
				case GST_MESSAGE_EOS :
					g_print("End of stream\n");
					terminate = TRUE;
					break;
				case GST_MESSAGE_ERROR :
					gst_message_parse_error(msg, &err, &debug_info);
					g_print("From %s element error received : %s\n", GST_OBJECT_NAME(msg->src), err->message);
					g_print("Debug infromaion : %s\n", debug_info ? debug_info : "none");
					terminate = TRUE;
					break;
				/* print state change information */
				case GST_MESSAGE_STATE_CHANGED :
					/* We are only instrested in state change message from the pipeline */
					if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline))
					{
						GstState old_state, new_state, pending_state;
						gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
						g_print("State changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
					}
					break;
				default :
					g_print("Unexpected message\n");
					
			}
			gst_message_unref(msg);
		}
	}while(!terminate);

	gst_object_unref(bus);
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);
	return 0;
}


static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data)
{
	GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	GstPadLinkReturn ret;

	g_print("Received new pad %s from element %s\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));
	
	if(gst_pad_is_linked(sink_pad))
	{
		g_print("Pad is already linked\n");
		goto exit;
	}

	/* check new pad's type */
	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);
	if(! g_str_has_prefix(new_pad_type, "video/x-raw"))
	{
		g_print("It has type %s, which is not raw video.  Ignore it.\n", new_pad_type);
		goto exit;
	}

	/*link pad*/
	g_print("New pad type : %s\n", new_pad_type);
	ret = gst_pad_link(new_pad, sink_pad);
	if(GST_PAD_LINK_FAILED(ret))
	{
		g_print("Pad linking failed\n");
	}
	else
	{
		g_print("Pad linking succeeded\n");
	}

	exit :
		if(new_pad_caps != NULL)
			gst_caps_unref(new_pad_caps);
		gst_object_unref(sink_pad);
}
