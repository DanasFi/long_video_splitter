#include "glib.h"
#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include "libavutil/avutil.h"
#include "video_splitter.h"

const int MAX_SIZE_IN_SECS = 4 * 60 * 60; // 4 hours
#define SPLIT_VIDEO_DURATION (4.0 * 3600.0)  // 4 hours in seconds

typedef struct {
    char* filename;
    double duration;
    long filesize;
} VideoInfo;
    

typedef struct {
    GtkWidget *hours;
    GtkWidget *minutes;
    GtkWidget *seconds;
} TimeInputData;

TimeInputData* create_time_input(GtkWidget *parent_box, const char *filename) {
    GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // inputs
    GtkWidget *hours_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *hours_label = gtk_label_new("HH");
    GtkWidget *hours_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(hours_entry), "06");  // Default to 6 hours
    gtk_entry_set_max_length(GTK_ENTRY(hours_entry), 2);
    gtk_widget_set_size_request(hours_entry, 50, -1);
    gtk_box_append(GTK_BOX(hours_column), hours_label);
    gtk_box_append(GTK_BOX(hours_column), hours_entry);
    gtk_box_append(GTK_BOX(time_box), hours_column);

    // separator
    GtkWidget *separator_column1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *spacer1 = gtk_label_new("");  
    GtkWidget *colon1 = gtk_label_new(":");
    gtk_box_append(GTK_BOX(separator_column1), spacer1);
    gtk_box_append(GTK_BOX(separator_column1), colon1);
    gtk_box_append(GTK_BOX(time_box), separator_column1);

    // minutes
    GtkWidget *minutes_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *minutes_label = gtk_label_new("MM");
    GtkWidget *minutes_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(minutes_entry), "00");  
    gtk_entry_set_max_length(GTK_ENTRY(minutes_entry), 2);
    gtk_widget_set_size_request(minutes_entry, 50, -1);
    gtk_box_append(GTK_BOX(minutes_column), minutes_label);
    gtk_box_append(GTK_BOX(minutes_column), minutes_entry);
    gtk_box_append(GTK_BOX(time_box), minutes_column);

    // separator
    GtkWidget *separator_column2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *spacer2 = gtk_label_new("");  
    GtkWidget *colon2 = gtk_label_new(":");
    gtk_box_append(GTK_BOX(separator_column2), spacer2);
    gtk_box_append(GTK_BOX(separator_column2), colon2);
    gtk_box_append(GTK_BOX(time_box), separator_column2);

    // seconds
    GtkWidget *seconds_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *seconds_label = gtk_label_new("SS");
    GtkWidget *seconds_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(seconds_entry), "00");  // Default to 0 seconds
    gtk_entry_set_max_length(GTK_ENTRY(seconds_entry), 2);
    gtk_widget_set_size_request(seconds_entry, 50, -1);
    gtk_box_append(GTK_BOX(seconds_column), seconds_label);
    gtk_box_append(GTK_BOX(seconds_column), seconds_entry);
    gtk_box_append(GTK_BOX(time_box), seconds_column);

    // add to main box
    gtk_box_append(GTK_BOX(parent_box), time_box);

    TimeInputData *time_data = g_malloc(sizeof(TimeInputData));
    time_data->hours = hours_entry;
    time_data->minutes = minutes_entry;
    time_data->seconds = seconds_entry;

    return time_data;
}

void size_into_readable(const VideoInfo *info, char *buf, size_t buf_size) {
    if (!info || info->filesize < 0) {
        snprintf(buf, buf_size, "unknown");
        return;
    }

    double size = (double)info->filesize;

    if (size < (1024.0 * 1024.0 * 1024.0)) {
        // show as mb
        double size_mb = size / (1024.0 * 1024.0);
        snprintf(buf, buf_size, "%.2f MB", size_mb);
    } else {
        // show as gb
        double size_gb = size / (1024.0 * 1024.0 * 1024.0);
        snprintf(buf, buf_size, "%.2f GB", size_gb);
    }
}

VideoInfo *get_video_info(const char* video_path) {
    AVFormatContext *input_ctx = NULL;
    int ret = avformat_open_input(&input_ctx, video_path, NULL, NULL);
    if (ret != 0) {
        fprintf(stderr, "Error opening input file %s\n", video_path);
        return NULL;
    }

    ret = avformat_find_stream_info(input_ctx, NULL);
    if (ret != 0) {
        fprintf(stderr, "Error obtaining stream info %s\n", video_path);
        return NULL;
    }
    av_dump_format(input_ctx,0 , video_path, 0);
    
    VideoInfo *video_info = malloc(sizeof(VideoInfo));

    video_info->filename = g_strdup(video_path);
    if (input_ctx->duration != AV_NOPTS_VALUE) {
        video_info->duration = (double)input_ctx->duration / AV_TIME_BASE;
    } else {
        video_info->duration = -1.0;
    }

    FILE *fp = fopen(video_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error obtaining file size");
        video_info->filesize = -1;
    } else {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fclose(fp);
        video_info->filesize = file_size;
    }
    return video_info;
    
}

GtkWidget *make_split_row(const char *video_name, const char *part, double val) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);  

    GtkWidget *video_name_label = gtk_label_new(video_name);
    gtk_box_append(GTK_BOX(row), video_name_label);

    GtkWidget *part_label = gtk_label_new(part);
    gtk_box_append(GTK_BOX(row), part_label);

    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", val);
    GtkWidget *label_val = gtk_label_new(buf);
    gtk_box_append(GTK_BOX(row), label_val);

    GtkWidget *check = gtk_check_button_new();
    gtk_box_append(GTK_BOX(row), check);

    return row;
}

static void on_split_video_selected(GtkButton *button, gpointer user_data) {
    const char *file_path = (const char *)user_data; 
    printf("Splitting video: %s\n", file_path);

    split_video(file_path, SPLIT_VIDEO_DURATION);    
}

static void
on_video_selected(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file;
    GError *error = NULL;
    
    file = gtk_file_dialog_open_finish(dialog, res, &error);
    
    if (error != NULL) {
        if (error->code != GTK_DIALOG_ERROR_CANCELLED) {
            g_print("Error selecting video: %s\n", error->message);
        }
        g_error_free(error);
        return;
    }
    
    if (file != NULL) {
        char *path = g_file_get_path(file);
        g_print("Selected video: %s\n", path);
        
        // get video information
        VideoInfo *video_info = get_video_info(path);
        printf("video_info->filename: %s\n", video_info->filename);
        printf("video_info->duration: %.2f\n", video_info->duration);
        printf("video_info->file_size: %ld\n", video_info->filesize);
        char size_str[32];
        size_into_readable(video_info, size_str, sizeof(size_str));
        printf("File size: %s\n", size_str);

        if (video_info) {
            GtkWidget *win = gtk_window_new();
            gtk_window_set_title(GTK_WINDOW(win), "Video Info");
            gtk_window_set_default_size(GTK_WINDOW(win), 400, 200);

            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            gtk_window_set_child(GTK_WINDOW(win), box);

            char buf[256];
            snprintf(buf, sizeof(buf), "Filename: %s", video_info->filename);
            GtkWidget *label_name = gtk_label_new(buf);
            gtk_box_append(GTK_BOX(box), label_name);

            if (video_info->duration > 0) {
                int duration_secs = (int)video_info->duration;
                int hours = duration_secs / 3600;
                int minutes = (duration_secs / 60 - hours * 60);
                int seconds = (duration_secs - minutes * 60 - hours * 3600);
                snprintf(buf, sizeof(buf), "Duration: %02d:%02d:%02d", hours, minutes, seconds);
            } else {
                snprintf(buf, sizeof(buf), "Duration: unknown");
            }
            GtkWidget *label_dur = gtk_label_new(buf);
            gtk_box_append(GTK_BOX(box), label_dur);

            snprintf(buf, sizeof(buf), "File size: %s", size_str);
            GtkWidget *label_size = gtk_label_new(buf);
            gtk_box_append(GTK_BOX(box), label_size);

            snprintf(buf, sizeof(buf), "Max tiempo por video");
            GtkWidget *max_size_video_label= gtk_label_new(buf);
            gtk_box_append(GTK_BOX(box), max_size_video_label);
            TimeInputData *max_time_data = create_time_input(box, video_info->filename);
            
            snprintf(buf, sizeof(buf), "Max tiempo por video");
            GtkWidget *min_size_video_label= gtk_label_new(buf);
            gtk_box_append(GTK_BOX(box), min_size_video_label);
            TimeInputData *min_time_data = create_time_input(box, video_info->filename);

            // make split predictions
            int number_of_splits = video_info->duration / MAX_SIZE_IN_SECS;
            GtkWidget *btn = gtk_button_new_with_label("Split!");
            gtk_box_append(GTK_BOX(box), btn);

            g_signal_connect(btn, "clicked", G_CALLBACK(on_split_video_selected), (gpointer)video_info->filename);
            gtk_box_append(GTK_BOX(box), btn);
            

            gtk_window_present(GTK_WINDOW(win));
        }

        g_free(path);
        g_object_unref(file);
    }
}


static void on_pick_video_clicked(GtkButton *button, gpointer user_data) {
     GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkFileDialog *dialog;
    
    dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select a Video File");
    
    GtkFileFilter *video_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(video_filter, "Videos");
    
    gtk_file_filter_add_pattern(video_filter, "*.mp4");
    gtk_file_filter_add_pattern(video_filter, "*.avi");
    gtk_file_filter_add_pattern(video_filter, "*.mkv");
    gtk_file_filter_add_pattern(video_filter, "*.mov");
    gtk_file_filter_add_pattern(video_filter, "*.wmv");
    gtk_file_filter_add_pattern(video_filter, "*.flv");
    gtk_file_filter_add_pattern(video_filter, "*.webm");
    gtk_file_filter_add_pattern(video_filter, "*.m4v");
    
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files");
    gtk_file_filter_add_pattern(all_filter, "*");
    
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, video_filter);
    g_list_store_append(filters, all_filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    
    gtk_file_dialog_open(dialog, parent_window, NULL, on_video_selected, NULL);
    
    // cleanup
    g_object_unref(video_filter);
    g_object_unref(all_filter);
    g_object_unref(filters);
    g_object_unref(dialog);
}

static void on_activate (GtkApplication *app) {
  // Create a main window
  GtkWidget *window = gtk_application_window_new (app);

  // button to pick file
  GtkWidget *button = gtk_button_new_with_label ("Pick file");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (on_pick_video_clicked), window);
  gtk_window_set_child (GTK_WINDOW (window), button);
  gtk_window_present (GTK_WINDOW (window));
}

int main (int argc, char *argv[]) {
  GtkApplication *app = gtk_application_new ("com.example.GtkApplication",
                                             G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
