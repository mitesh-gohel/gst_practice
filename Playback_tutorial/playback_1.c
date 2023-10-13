/* Playback tutorail -1 : Playbin usage */
#include<stdio.h>
#include<gst/gst.h>

typedef struct _CustomData
{
	GstElement *playbin; 	/*One and only element of pipeline*/
	gint n_video;		/*no. of video streams */
	gint n_audio;
	gint n_text;
	gint current_video;	/*Currently playing video */
	gint current_audio;
	gint current_text;
	GMainLoop *main_loop;
}CustomData;

/*playbin flags*/
typedef enum
{
	GST_PLAY_FLAG_VIDEO = (1 << 0), /*Video output*/
	GST_PLAY_FLAG_AUDIO = (1 << 1),	/*Audiio output*/
	GST_PLAY_FLAG_TEXT = (1 << 2),	/*Subtitle output*/
}GstPlayFlags;

static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data);
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data);

int main(int argc, char **argv)
{
	CustomData data;
	GstBus *bus;
	gint flags;
	GIOChannel *io_stdin;
	GstStateChangeReturn ret;
	
	gst_init(&argc, &argv);

	data.playbin = gst_element_factory_make("playbin", "playbin");
	if(!data.playbin)
	{
		g_error("All elements are not created\n");
		return -1;
	}
	g_object_set(data.playbin, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_cropped_multilingual.webm", NULL);
	
	/*Enable  Video and Audio.   Disable Subtitle*/
	g_object_get(data.playbin, "flags", &flags, NULL);
	flags = flags | GST_PLAY_FLAG_VIDEO;
	flags = flags | GST_PLAY_FLAG_AUDIO;
	flags = flags & (~ GST_PLAY_FLAG_TEXT);
	g_object_set(data.playbin, "flags", flags, NULL);

	/*Set connetion speed , This will affect internal decisions os playbin*/
	g_object_set(data.playbin, "connection-speed", 56, NULL);
	
	bus = gst_element_get_bus(data.playbin);
	gst_bus_add_watch(bus, (GstBusFunc)handle_message, &data);	/*When new message will arrive, it will run handle_message()*/

	/*Add keyboard watch so we get notified of keystorkes*/
	//For unix 
	io_stdin = g_io_channel_unix_new(fileno(stdin));
	g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

	ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		g_error("Unable to change state into playing\n");
		gst_object_unref(data.playbin);
		return -1;
	}

	/* Create Glib Main Loop and set it to run */
	data.main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(data.main_loop);

	/* Free resources */
	g_main_loop_unref(data.main_loop);
	g_io_channel_unref(io_stdin);
	gst_object_unref(bus);
	gst_element_set_state(data.playbin, GST_STATE_NULL);
	gst_object_unref(data.playbin);
	
	return 0;
}


static void analyze_streams(CustomData *data)
{
	gint i;
	GstTagList *tags;
	gchar *str;
	uint bitrate;

	g_object_get(data->playbin, "n-video", &data->n_video, NULL);
	g_object_get(data->playbin, "n-audio", &data->n_audio, NULL);
	g_object_get(data->playbin, "n-text", &data->n_text, NULL);
	g_print("Total video strems : %d, Total audio streams : %d, Total text streams : %d\n", data->n_video, data->n_audio, data->n_text);

	for(i=0; i<data->n_video; ++i)
	{
		tags = NULL;
		g_signal_emit_by_name(data->playbin, "get-video-tags", i, &tags);	/* Action signal */
		if(tags)
		{
			g_print("Video stream %d :\n", i);
			gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &str);
			g_print("\tcodec : %s\n", str ? str : "None");
			g_free(str);
			gst_tag_list_free(tags);
		}
	}

	for(i=0; i<data->n_audio; ++i)
	{
		tags = NULL;
		g_signal_emit_by_name(data->playbin, "get-audio-tags", i, &tags); 	/* Action siganal */
		if(tags)
		{
			g_print("Audio stream %d :\n", i);
			if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &str))
			{
				g_print("\tcodec : %s\n", str ? str : "None");
				g_free(str);
			}
			if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str))
			{
				g_print("\tAudio language : %s\n", str ? str : "None");
				g_free(str);
			}
			if(gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &bitrate))
			{
				g_print("\tbitrate : %d\n", bitrate);
			}
			gst_tag_list_free(tags);
		}
	}
	
	for(i=0; i<data->n_text; ++i)
	{
		tags = NULL;
		g_signal_emit_by_name(data->playbin, "get-text-tags", i, &tags);	/* Action signal */
		if(tags)
		{
			g_print("Subtitle stream %d :\n", i);
			if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str))
			{
				g_print("\tSubtitle language : %s\n", str ? str : "None");
				g_free(str);
			}
			gst_tag_list_free(tags);
		}
	}

	/*Retrive currently playing stream number*/
	g_object_get(data->playbin, "current-video", &data->current_video, NULL);
	g_object_get(data->playbin, "current-audio", &data->current_audio, NULL);
	g_object_get(data->playbin, "current-text", &data->current_text, NULL);
	g_print("\nCurrentl playing video stream %d, audio stream %d, subtitle stream %d\n", data->current_video, data->current_audio, data->current_text);
	g_print("Type any number and hit Enter to change audio stream.\n");
}

static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data)
{
	GError *err;
	gchar *debug_info;
	switch(GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS :
			g_print("End of stream\n");
			g_main_loop_quit(data->main_loop);
			break;
		case GST_MESSAGE_ERROR :
			gst_message_parse_error(msg, &err, &debug_info);
			g_printerr("Error recieved from %s : %s\n", GST_OBJECT_NAME(msg->src), err->message);
			g_printerr("Debug info : %s\n", debug_info ? debug_info : "None");
			g_clear_error(&err);
			g_free(debug_info);
			g_main_loop_quit(data->main_loop);
			break;
		case GST_MESSAGE_STATE_CHANGED :
			{
				GstState old_state, new_state, pending_state;
				gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
				if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin))
				{
					if(new_state == GST_STATE_PLAYING)
						analyze_streams(data);
				}
			}
			break;
	}
	return TRUE;
}

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data)
{
	gchar *str = NULL;

	if(g_io_channel_read_line(source , &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL)
	{
		int index = g_ascii_strtoull(str, NULL, 0);
		if(index <0 || index >= data->n_audio)
		{
			g_printerr("Index out of bounds\n");
		}
		else
		{
			g_print("Setting current audio stream to %d\n", index);
			g_object_set(data->playbin, "current-audio", index, NULL);
		}
	}
	g_free(str);

	return TRUE;
}
