/* Basic tutorial 2  Extercixe*/
#include<gst/gst.h>

int main(int argc, char **argv)
{
	GstElement *pipeline;
	GstElement *source, *sink;
	GstStateChangeReturn ret;
	GstBus *bus;
	GstMessage *msg;
	GstElement *filter, *convert;

	gst_init(&argc, &argv);

	/* create elements */
	source = gst_element_factory_make("videotestsrc", "source");
	sink = gst_element_factory_make("autovideosink", "sink");
	filter = gst_element_factory_make("vertigotv", "my_filter");
	convert = gst_element_factory_make("videoconvert", "converter");

	/*create empty pipeline */
	pipeline = gst_pipeline_new("test-pipeline");

	if(!pipeline || !source || !filter || !convert || ! sink)
	{
		g_printerr("All elements are not created\n");
		return -1;
	}

	/* Add elements in pipline */
	//gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
	gst_bin_add_many(GST_BIN(pipeline), source, filter, convert, sink, NULL);
	/* Link elements */
	//if(gst_element_link_filtered(source, sink, filter)!= TRUE)
	if(gst_element_link_many(source, filter, convert, sink, NULL) != TRUE)
	{
		g_printerr("Elements could not be link\n");
		gst_object_unref(pipeline);	
		return -1;
	}

	/*  Modify source patten */
	g_object_set(source, "pattern", 0, NULL);  /* instead of 0, you can try 1, 2, 3, ... etc number */

	ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		g_printerr("Not able to change state of video");
		gst_object_unref(pipeline);
		return -1;
	}
	
	bus = gst_element_get_bus(pipeline);
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	if(msg != NULL)
	{
		GError *err;
		gchar *debug_info;
		switch(GST_MESSAGE_TYPE(msg))
		{
			case GST_MESSAGE_ERROR :
				gst_message_parse_error(msg, &err, &debug_info);
				g_printerr("Error received from element %s : %s\n", GST_OBJECT_NAME(msg->src), err->message);
				g_printerr("Debug info in message : %s\n", debug_info ? debug_info : "none");
				g_clear_error(&err);
				g_free(debug_info);
				break;
			case GST_MESSAGE_EOS :
				g_print("EOS\n");
				break;
			default :
				g_printerr("Unexpected message\n");
		}
		gst_message_unref(msg);
	}

	/* clean up */
	gst_object_unref(bus);	
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);

	return 0;
}
