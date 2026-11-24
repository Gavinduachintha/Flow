#include "ui/window.h"

#include <gtk/gtk.h>
#include <math.h>

#include "fan.h"
#include "system_info.h"
#include "thermal.h"

#define AUTO_TEMP_MIN 45.0
#define AUTO_TEMP_MAX 75.0
#define PWM_MAX 255
#define REFRESH_SECONDS 2

typedef struct {
    GtkWidget *window;
    GtkWidget *temperature_value;
    GtkWidget *temperature_bar;
    GtkWidget *fan_value;
    GtkWidget *frequency_value;
    GtkWidget *status_label;
    GtkWidget *auto_switch;
    GtkWidget *pwm_scale;
    GtkWidget *pwm_value_label;
    guint timer_id;
    gboolean auto_mode;
    gboolean updating_scale;
    gboolean startup_refresh;
    int pwm_value;
} PibreezeUI;

static GtkWidget *
make_card (void)
{
    GtkWidget *card;

    card = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_css_classes (card,
                                (const char *[]) {"card", NULL});
    gtk_widget_set_margin_start (card, 12);
    gtk_widget_set_margin_end (card, 12);
    gtk_widget_set_margin_top (card, 6);
    gtk_widget_set_margin_bottom (card, 6);

    return card;
}

static GtkWidget *
make_card_title (const char *text)
{
    GtkWidget *label;

    label = gtk_label_new (text);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_add_css_class (label, "card-title");

    return label;
}

static void
apply_css (void)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    const char *css =
        "window { background: #eef1f5; }\n"
        ".card { background: #ffffff; border-radius: 14px; padding: 16px; }\n"
        ".card-title { color: #55606c; font-size: 13px; font-weight: 600; }\n"
        ".big-value { color: #20252b; font-size: 34px; font-weight: 800; }\n"
        ".medium-value { color: #20252b; font-size: 26px; font-weight: 700; }\n"
        ".status-error { color: #c62828; font-weight: 700; }\n";

    provider = gtk_css_provider_new ();
#if GTK_CHECK_VERSION(4, 12, 0)
    gtk_css_provider_load_from_string (provider, css);
#else
    gtk_css_provider_load_from_data (provider, css, -1);
#endif

    display = gdk_display_get_default ();
    if (display != NULL) {
        gtk_style_context_add_provider_for_display (
            display,
            GTK_STYLE_PROVIDER (provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref (provider);
}

static void
set_status_error (PibreezeUI *ui,
                  const char *message)
{
    gtk_label_set_text (GTK_LABEL (ui->status_label), message);
    gtk_widget_add_css_class (ui->status_label, "status-error");
}

static void
set_status_ok (PibreezeUI *ui)
{
    gtk_label_set_text (GTK_LABEL (ui->status_label), "Fan control ready");
    gtk_widget_remove_css_class (ui->status_label, "status-error");
}

static void
write_fan_pwm (PibreezeUI *ui,
               int         pwm)
{
    if (set_fan_speed (pwm) != 0) {
        set_status_error (
            ui,
            "Fan write failed - check PWM permissions");
        return;
    }

    ui->pwm_value = pwm;
    set_status_ok (ui);
}

static int
temperature_to_pwm (double temperature)
{
    double range;
    int pwm;

    if (temperature <= AUTO_TEMP_MIN) {
        return 0;
    }
    if (temperature >= AUTO_TEMP_MAX) {
        return PWM_MAX;
    }

    range = AUTO_TEMP_MAX - AUTO_TEMP_MIN;
    pwm = (int) lround (
        ((temperature - AUTO_TEMP_MIN) / range) * (double) PWM_MAX);

    return pwm;
}

static void
update_pwm_controls (PibreezeUI *ui,
                     int         pwm)
{
    int percentage;
    char text[32];

    percentage = (int) lround (((double) pwm / (double) PWM_MAX) * 100.0);

    ui->updating_scale = TRUE;
    gtk_range_set_value (GTK_RANGE (ui->pwm_scale), (double) pwm);
    ui->updating_scale = FALSE;

    g_snprintf (text, sizeof (text), "%d PWM  (%d%%)", pwm, percentage);
    gtk_label_set_text (GTK_LABEL (ui->pwm_value_label), text);
}

static gboolean
refresh_readings (gpointer user_data)
{
    PibreezeUI *ui;
    double temperature;
    double frequency;
    int fan_rpm;
    char text[64];

    ui = (PibreezeUI *) user_data;

    if (get_cpu_temperature (&temperature) == 0) {
        g_snprintf (text, sizeof (text), "%.1f °C", temperature);
        gtk_label_set_text (GTK_LABEL (ui->temperature_value), text);

        if (temperature < 0.0) {
            temperature = 0.0;
        } else if (temperature > 100.0) {
            temperature = 100.0;
        }
        gtk_level_bar_set_value (
            GTK_LEVEL_BAR (ui->temperature_bar),
            temperature);

        if (ui->auto_mode && ui->startup_refresh) {
            update_pwm_controls (ui, temperature_to_pwm (temperature));
            ui->pwm_value = temperature_to_pwm (temperature);
            ui->startup_refresh = FALSE;
        } else if (ui->auto_mode) {
            int pwm;

            pwm = temperature_to_pwm (temperature);
            update_pwm_controls (ui, pwm);
            if (pwm != ui->pwm_value) {
                write_fan_pwm (ui, pwm);
            }
        }
    } else {
        gtk_label_set_text (
            GTK_LABEL (ui->temperature_value),
            "-- °C");
    }

    if (get_fan_speed (&fan_rpm) == 0) {
        g_snprintf (text, sizeof (text), "%d RPM", fan_rpm);
        gtk_label_set_text (GTK_LABEL (ui->fan_value), text);
    } else {
        gtk_label_set_text (GTK_LABEL (ui->fan_value), "-- RPM");
    }

    if (get_cpu_frequency (&frequency) == 0.0) {
        g_snprintf (text, sizeof (text), "%.2f GHz", frequency);
        gtk_label_set_text (GTK_LABEL (ui->frequency_value), text);
    } else {
        gtk_label_set_text (GTK_LABEL (ui->frequency_value), "-- GHz");
    }

    return G_SOURCE_CONTINUE;
}

static void
on_pwm_changed (GtkRange    *range,
                gpointer     user_data)
{
    PibreezeUI *ui;
    int pwm;

    ui = (PibreezeUI *) user_data;
    if (ui->updating_scale) {
        return;
    }

    pwm = (int) lround (gtk_range_get_value (range));
    update_pwm_controls (ui, pwm);

    if (!ui->auto_mode) {
        write_fan_pwm (ui, pwm);
    }
}

static gboolean
on_auto_mode_changed (GtkSwitch *sw,
                      gboolean   state,
                      gpointer   user_data)
{
    PibreezeUI *ui;

    (void) sw;

    ui = (PibreezeUI *) user_data;
    ui->auto_mode = state;
    gtk_widget_set_sensitive (ui->pwm_scale, !state);

    if (state) {
        ui->startup_refresh = FALSE;
        gtk_label_set_text (
            GTK_LABEL (ui->status_label),
            "Automatic fan control");
        gtk_widget_remove_css_class (ui->status_label, "status-error");
    } else {
        gtk_label_set_text (
            GTK_LABEL (ui->status_label),
            "Manual fan control");
        gtk_widget_remove_css_class (ui->status_label, "status-error");
    }

    return FALSE;
}

static void
on_window_destroy (GtkWidget *window,
                   gpointer   user_data)
{
    PibreezeUI *ui;

    (void) window;

    ui = (PibreezeUI *) user_data;
    if (ui->timer_id != 0) {
        g_source_remove (ui->timer_id);
        ui->timer_id = 0;
    }
    g_free (ui);
}

GtkWidget *
pibreeze_window_new (GtkApplication *app)
{
    PibreezeUI *ui;
    GtkWidget *window;
    GtkWidget *scroller;
    GtkWidget *main_box;
    GtkWidget *temperature_card;
    GtkWidget *temperature_title;
    GtkWidget *fan_card;
    GtkWidget *fan_title;
    GtkWidget *frequency_card;
    GtkWidget *frequency_title;
    GtkWidget *control_card;
    GtkWidget *control_title;
    GtkWidget *control_row;
    GtkWidget *control_label;

    ui = g_new0 (PibreezeUI, 1);
    ui->auto_mode = TRUE;
    ui->startup_refresh = TRUE;
    ui->pwm_value = -1;

    apply_css ();

    window = gtk_application_window_new (app);
    ui->window = window;
    gtk_window_set_title (GTK_WINDOW (window), "PiBreeze");
    gtk_window_set_default_size (GTK_WINDOW (window), 380, 520);

    scroller = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (scroller),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC);

    main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top (main_box, 8);
    gtk_widget_set_margin_bottom (main_box, 8);
    gtk_scrolled_window_set_child (
        GTK_SCROLLED_WINDOW (scroller),
        main_box);
    gtk_window_set_child (GTK_WINDOW (window), scroller);

    temperature_card = make_card ();
    temperature_title = make_card_title ("CPU temperature");
    ui->temperature_value = gtk_label_new ("-- °C");
    gtk_widget_set_halign (ui->temperature_value, GTK_ALIGN_START);
    gtk_widget_add_css_class (ui->temperature_value, "big-value");

    ui->temperature_bar = gtk_level_bar_new ();
    gtk_level_bar_set_min_value (
        GTK_LEVEL_BAR (ui->temperature_bar),
        0.0);
    gtk_level_bar_set_max_value (
        GTK_LEVEL_BAR (ui->temperature_bar),
        100.0);
    gtk_level_bar_set_value (
        GTK_LEVEL_BAR (ui->temperature_bar),
        0.0);

    gtk_box_append (GTK_BOX (temperature_card), temperature_title);
    gtk_box_append (GTK_BOX (temperature_card), ui->temperature_value);
    gtk_box_append (GTK_BOX (temperature_card), ui->temperature_bar);
    gtk_box_append (GTK_BOX (main_box), temperature_card);

    fan_card = make_card ();
    fan_title = make_card_title ("Fan speed");
    ui->fan_value = gtk_label_new ("-- RPM");
    gtk_widget_set_halign (ui->fan_value, GTK_ALIGN_START);
    gtk_widget_add_css_class (ui->fan_value, "medium-value");
    gtk_box_append (GTK_BOX (fan_card), fan_title);
    gtk_box_append (GTK_BOX (fan_card), ui->fan_value);
    gtk_box_append (GTK_BOX (main_box), fan_card);

    frequency_card = make_card ();
    frequency_title = make_card_title ("CPU frequency");
    ui->frequency_value = gtk_label_new ("-- GHz");
    gtk_widget_set_halign (ui->frequency_value, GTK_ALIGN_START);
    gtk_widget_add_css_class (ui->frequency_value, "medium-value");
    gtk_box_append (GTK_BOX (frequency_card), frequency_title);
    gtk_box_append (GTK_BOX (frequency_card), ui->frequency_value);
    gtk_box_append (GTK_BOX (main_box), frequency_card);

    control_card = make_card ();
    control_title = make_card_title ("Fan control");
    gtk_box_append (GTK_BOX (control_card), control_title);

    control_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    control_label = gtk_label_new ("Auto mode");
    gtk_widget_set_hexpand (control_label, TRUE);
    gtk_widget_set_halign (control_label, GTK_ALIGN_START);

    ui->auto_switch = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (ui->auto_switch), TRUE);
    gtk_box_append (GTK_BOX (control_row), control_label);
    gtk_box_append (GTK_BOX (control_row), ui->auto_switch);
    gtk_box_append (GTK_BOX (control_card), control_row);

    ui->pwm_scale = gtk_scale_new_with_range (
        GTK_ORIENTATION_HORIZONTAL,
        0.0,
        (double) PWM_MAX,
        1.0);
    gtk_scale_set_digits (GTK_SCALE (ui->pwm_scale), 0);
    gtk_widget_set_sensitive (ui->pwm_scale, FALSE);
    gtk_range_set_value (GTK_RANGE (ui->pwm_scale), 128.0);
    ui->pwm_value_label = gtk_label_new ("128 PWM  (50%)");
    gtk_widget_set_halign (ui->pwm_value_label, GTK_ALIGN_END);

    gtk_box_append (GTK_BOX (control_card), ui->pwm_scale);
    gtk_box_append (GTK_BOX (control_card), ui->pwm_value_label);

    ui->status_label = gtk_label_new ("Automatic fan control");
    gtk_widget_set_halign (ui->status_label, GTK_ALIGN_START);
    gtk_box_append (GTK_BOX (control_card), ui->status_label);
    gtk_box_append (GTK_BOX (main_box), control_card);

    g_signal_connect (
        ui->pwm_scale,
        "value-changed",
        G_CALLBACK (on_pwm_changed),
        ui);
    g_signal_connect (
        ui->auto_switch,
        "state-set",
        G_CALLBACK (on_auto_mode_changed),
        ui);
    g_signal_connect (
        window,
        "destroy",
        G_CALLBACK (on_window_destroy),
        ui);

    ui->timer_id = g_timeout_add_seconds (
        REFRESH_SECONDS,
        refresh_readings,
        ui);

    return window;
}
