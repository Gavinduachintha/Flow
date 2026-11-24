
#include <gtk/gtk.h>

#include "ui/window.h"

static void
activate (GtkApplication *app,
          gpointer         user_data)
{
    GtkWidget *window;

    (void) user_data;

    window = flow_window_new (app);
    gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new ("com.zenlab64.flow", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}