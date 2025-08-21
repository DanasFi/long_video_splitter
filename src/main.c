#include <gtk/gtk.h>

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
        
        // Here you can use the video file path
        // For example: load it into a video player, process it, etc.
        
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
