#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include "libavutil/avutil.h"
#include "video_splitter.h"

const int MAX_SIZE_IN_SECS = 4 * 60 * 60; // 4 hours
#define SPLIT_VIDEO_DURATION (4.0 * 3600.0)  // 4 hours in seconds
#define MAX_HOURS_VIDEO "06"
#define MAX_MINUTES_VIDEO "00"
#define MIN_HOURS_VIDEO "00"
#define MIN_MINUTES_VIDEO "30"
#define DEFAULT_SECONDS "00"

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

typedef struct {
    TimeInputData *max_duration;
    TimeInputData *min_duration;
    char* filename;
} SplitVideoInput; 

TimeInputData* create_time_input(GtkWidget *parent_box, const char* default_hours, const char* default_minutes, const char* default_seconds) {
    GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // inputs
    GtkWidget *hours_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *hours_label = gtk_label_new("HH");
    GtkWidget *hours_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(hours_entry), default_hours);
    gtk_entry_set_max_length(GTK_ENTRY(hours_entry), 2);
    const char *max_hours = gtk_entry_get_text(GTK_ENTRY(hours_entry));
    gtk_widget_set_size_request(hours_entry, 50, -1);
    gtk_box_pack_start(GTK_BOX(hours_column), hours_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hours_column), hours_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), hours_column, FALSE, FALSE, 0);

    // separator
    GtkWidget *separator_column1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *spacer1 = gtk_label_new("");  
    GtkWidget *colon1 = gtk_label_new(":");
    gtk_box_pack_start(GTK_BOX(separator_column1), spacer1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(separator_column1), colon1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), separator_column1, FALSE, FALSE, 0);

    // minutes
    GtkWidget *minutes_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *minutes_label = gtk_label_new("MM");
    GtkWidget *minutes_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(minutes_entry), default_minutes);
    const char *minutes = gtk_entry_get_text(GTK_ENTRY(minutes_entry));
    gtk_entry_set_max_length(GTK_ENTRY(minutes_entry), 2);
    gtk_widget_set_size_request(minutes_entry, 50, -1);
    gtk_box_pack_start(GTK_BOX(minutes_column), minutes_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(minutes_column), minutes_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), minutes_column, FALSE, FALSE, 0);

    // separator
    GtkWidget *separator_column2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *spacer2 = gtk_label_new("");  
    GtkWidget *colon2 = gtk_label_new(":");
    gtk_box_pack_start(GTK_BOX(separator_column2), spacer2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(separator_column2), colon2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), separator_column2, FALSE, FALSE, 0);

    // seconds
    GtkWidget *seconds_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *seconds_label = gtk_label_new("SS");
    GtkWidget *seconds_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(seconds_entry), default_seconds);
    const char *seconds = gtk_entry_get_text(GTK_ENTRY(seconds_entry));
    gtk_entry_set_max_length(GTK_ENTRY(seconds_entry), 2);
    gtk_widget_set_size_request(seconds_entry, 50, -1);
    gtk_box_pack_start(GTK_BOX(seconds_column), seconds_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(seconds_column), seconds_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), seconds_column, FALSE, FALSE, 0);

    // add to main box
    gtk_box_pack_start(GTK_BOX(parent_box), time_box, FALSE, FALSE, 0);

    gtk_widget_show_all(time_box);

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
    gtk_box_pack_start(GTK_BOX(row), video_name_label, FALSE, FALSE, 0);

    GtkWidget *part_label = gtk_label_new(part);
    gtk_box_pack_start(GTK_BOX(row), part_label, FALSE, FALSE, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", val);
    GtkWidget *label_val = gtk_label_new(buf);
    gtk_box_pack_start(GTK_BOX(row), label_val, FALSE, FALSE, 0);

    GtkWidget *check = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(row), check, FALSE, FALSE, 0);

    return row;
}
int get_total_seconds_from_time_input(TimeInputData *time_data) {
    const char *hours_text = gtk_entry_get_text(GTK_ENTRY(time_data->hours));
    const char *minutes_text = gtk_entry_get_text(GTK_ENTRY(time_data->minutes));
    const char *seconds_text = gtk_entry_get_text(GTK_ENTRY(time_data->seconds));
    
    int hours = (strlen(hours_text) > 0) ? atoi(hours_text) : 0;
    int minutes = (strlen(minutes_text) > 0) ? atoi(minutes_text) : 0;
    int seconds = (strlen(seconds_text) > 0) ? atoi(seconds_text) : 0;
    
    return hours * 3600 + minutes * 60 + seconds;
}

static void on_split_video_selected(GtkButton *button, gpointer user_data) {
    const SplitVideoInput *split_video_input= (const SplitVideoInput *)user_data; 
    printf("Splitting video: %s\n", split_video_input->filename);

    double max_duration = get_total_seconds_from_time_input(split_video_input->max_duration);
    double min_duration = get_total_seconds_from_time_input(split_video_input->min_duration);
    printf("max duration %d", max_duration);
    printf("min duration %d", min_duration);
    
    split_video(split_video_input->filename, max_duration, min_duration);    
}

static void
on_video_selected(const char *filename)  
{
    if (!filename) return;

    g_print("Selected video: %s\n", filename);

    // get video information
    VideoInfo *video_info = get_video_info(filename);
    printf("video_info->filename: %s\n", video_info->filename);
    printf("video_info->duration: %.2f\n", video_info->duration);
    printf("video_info->file_size: %ld\n", video_info->filesize);

    char size_str[32];
    size_into_readable(video_info, size_str, sizeof(size_str));
    printf("File size: %s\n", size_str);

    if (video_info) {
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(win), "Video Info");
        gtk_window_set_default_size(GTK_WINDOW(win), 400, 200);

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(win), box);

        // Filename label
        char buf[256];
        snprintf(buf, sizeof(buf), "Filename: %s", video_info->filename);
        GtkWidget *label_name = gtk_label_new(buf);
        gtk_box_pack_start(GTK_BOX(box), label_name, FALSE, FALSE, 0);

        // Duration label
        if (video_info->duration > 0) {
            int duration_secs = (int)video_info->duration;
            int hours = duration_secs / 3600;
            int minutes = (duration_secs / 60) - hours * 60;
            int seconds = duration_secs - minutes * 60 - hours * 3600;
            snprintf(buf, sizeof(buf), "Duration: %02d:%02d:%02d", hours, minutes, seconds);
        } else {
            snprintf(buf, sizeof(buf), "Duration: unknown");
        }
        GtkWidget *label_dur = gtk_label_new(buf);
        gtk_box_pack_start(GTK_BOX(box), label_dur, FALSE, FALSE, 0);

        // File size label
        snprintf(buf, sizeof(buf), "File size: %s", size_str);
        GtkWidget *label_size = gtk_label_new(buf);
        gtk_box_pack_start(GTK_BOX(box), label_size, FALSE, FALSE, 0);

        // Max time
        GtkWidget *max_size_video_label = gtk_label_new("Max tiempo por video");
        gtk_box_pack_start(GTK_BOX(box), max_size_video_label, FALSE, FALSE, 0);
        TimeInputData *max_time_data =
            create_time_input(box, MAX_HOURS_VIDEO, MAX_MINUTES_VIDEO, DEFAULT_SECONDS);

        // Min time
        GtkWidget *min_size_video_label = gtk_label_new("Min tiempo por video");
        gtk_box_pack_start(GTK_BOX(box), min_size_video_label, FALSE, FALSE, 0);
        TimeInputData *min_time_data =
            create_time_input(box, MIN_HOURS_VIDEO, MIN_MINUTES_VIDEO, DEFAULT_SECONDS);

        GtkWidget *btn = gtk_button_new_with_label("Split!");
        gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

        SplitVideoInput *split_video_input = g_malloc(sizeof(SplitVideoInput));
        split_video_input->filename = g_strdup(video_info->filename);
        split_video_input->max_duration = max_time_data;
        split_video_input->min_duration = min_time_data;
        g_signal_connect(btn, "clicked", G_CALLBACK(on_split_video_selected), (gpointer)split_video_input);
        gtk_widget_show_all(win);
    }
}

static void on_pick_video_clicked(GtkButton *button, gpointer user_data) {
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select a Video File",
        parent_window,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    // Create a filter for video files
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
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), video_filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        on_video_selected(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_activate(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Video Picker");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);

    GtkWidget *button = gtk_button_new_with_label("Pick file");
    g_signal_connect(button, "clicked", G_CALLBACK(on_pick_video_clicked), window);
    gtk_container_add(GTK_CONTAINER(window), button);

    gtk_widget_show_all(window);
}

int main (int argc, char *argv[]) {
  GtkApplication *app = gtk_application_new ("com.example.GtkApplication",
                                             G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
