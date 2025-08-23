#include "glib.h"
#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include "libavutil/avutil.h"

typedef struct {
    char* filename;
    double duration;
} VideoInfo;
    
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
    VideoInfo *video_info = malloc(sizeof(VideoInfo));
    video_info->filename = g_strdup(video_path);
    if (input_ctx->duration != AV_NOPTS_VALUE) {
        video_info->duration = (double)input_ctx->duration / AV_TIME_BASE;
    } else {
        video_info->duration = -1.0;
    }
    return video_info;
    
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

            GtkWidget *max_split_time = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(max_split_time), "Enter time");
            gtk_box_append(GTK_BOX(box), max_split_time);

            GtkWidget *btn = gtk_button_new_with_label("Split!");
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
    
    // Cleanup
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
  // Create a new application
  GtkApplication *app = gtk_application_new ("com.example.GtkApplication",
                                             G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
