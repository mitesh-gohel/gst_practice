//To play video, which is saved in file

/*
Working pipeline :
gst-launch-1.0  filesrc location=/home/mitesh/Downloads/Cristiano_Ronaldo.webm ! decodebin ! videoconvert ! autovideosink
*/

#include<gst/gst.h>

typedef struct _CustomData
{
	GstElement *pipeline;
	GstElement *source, *sink, *decode;
	GstElement *convert;		//real
	GMainLoop *main_loop;
}CustomData;

void 
pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data)
{
	GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	GstPadLinkReturn ret;
	
	if(gst_pad_is_linked(sink_pad)){
		goto exit;
	}

	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);
	
	if(! g_str_has_prefix(new_pad_type, "video/x-raw")){
		g_print("Ingoring - %s\n", new_pad_type);
		goto exit;
	}
	ret = gst_pad_link(new_pad, sink_pad);
	if(GST_PAD_LINK_FAILED(ret) ) {
		g_print("Linking failed\n");
	}
	else {
		g_print("Linking succeed\n");
	}
	
	exit :
		if(new_pad_caps != NULL)
			gst_caps_unref(new_pad_caps);
		gst_object_unref(sink_pad);
}

gboolean 
handle_message(GstBus *bus, GstMessage *msg, CustomData *data)
{
	GError *err;
	gchar *debug_info;

	switch(GST_MESSAGE_TYPE(msg))
	{
		case  GST_MESSAGE_ERROR :
			gst_message_parse_error(msg, &err, &debug_info);
			g_printerr("Error recieved from %s : %s\n", GST_OBJECT_NAME(msg->src), err->message);
			g_printerr("Debug info : %s\n", debug_info ? debug_info : "None");
			g_free(debug_info);
			g_clear_error(&err);
			gst_element_set_state(data->pipeline, GST_STATE_NULL);
			g_main_loop_quit(data->main_loop);
			break;
		case GST_MESSAGE_EOS :
			g_print("Emnd of stream\n");
			g_main_loop_quit(data->main_loop);
	}
	return TRUE;
}


int 
main(int argc, char **argv)
{
	CustomData data;
	GstBus *bus;
	GstStateChangeReturn ret;

	gst_init(&argc, &argv);
	
	data.pipeline = gst_pipeline_new("test-pipeline");

	//data.source = gst_element_factory_make("videotestsrc", "source");	//debug
	//data.source = gst_element_factory_make("filesrc", "my-source");		//real
	data.source = gst_element_factory_make("v4l2src", "my-webcam");
	g_print("Source name = %s\n", GST_OBJECT_NAME(data.source));
	data.decode = gst_element_factory_make("decodebin", "my-decodebin");
	data.convert = gst_element_factory_make("videoconvert", "convert");		//real
	data.sink = gst_element_factory_make("autovideosink", "sink");

	//if(!data.pipeline || !data.source || !data.sink) {				//debug
	if(!data.pipeline || !data.source || !data.decode || !data.convert || !data.sink) {		//real
		g_printerr("All elements are not created\n");
		return -1;
	}

	//g_object_set(data.source, "location", "/home/mitesh/Downloads/Cristiano_Ronaldo.webm", NULL);	//real

	g_signal_connect(data.decode, "pad-added", G_CALLBACK(pad_added_handler), &data);
	
	/*Add elements in bin*/
	//gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.sink, NULL);		//debug
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.decode, data.convert, data.sink, NULL);	//real

	/*link elements*/
	//if(gst_element_link_many(data.source, data.sink, NULL) != TRUE) {			//debug
	if(gst_element_link_many(data.source, data.decode, NULL) != TRUE) {		//real
		g_printerr("Unable to link elements\n");
		gst_object_unref(data.pipeline);
		return -1;
	}
	if(gst_element_link_many(data.convert, data.sink, NULL) != TRUE){
		g_printerr("Unable to link convert and sink element");
		gst_object_unref(data.pipeline);
	}
	g_print("Element linking succeed\n");

	bus = gst_element_get_bus(data.pipeline);
	gst_bus_add_watch(bus, (GstBusFunc)handle_message, &data);
	gst_object_unref(bus);

	ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Not able to play video\n");
		gst_object_unref(data.pipeline);
		return -1;
	}
	
	data.main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(data.main_loop);

	/*Free resource*/
	g_main_loop_unref(data.main_loop);
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);

	return 0;
}
