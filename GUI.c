#include <gtk/gtk.h>
#include <glib/gstdio.h>

// --- CSS Data ---
// Brighter, more vibrant theme using teals, blues, and stronger contrasts.
const gchar *css_data =
    "/* General Window Styling */\n"
    "#main_window {\n"
    "    /* Brighter teal/aqua gradient background */\n"
    "    background-image: linear-gradient(to bottom, #e0f7fa, #b2ebf2); /* Light cyan -> Lighter cyan */\n"
    "    color: #1a237e; /* Dark blue text for contrast */\n"
    "}\n"
    "\n"
    "/* Frame Styling */\n"
    "frame {\n"
    "    background-color: rgba(255, 255, 255, 0.6); /* Semi-transparent white */\n"
    "    border: 1px solid #26c6da; /* Medium Cyan border */\n"
    "    border-radius: 8px; /* Slightly more rounded */\n"
    "    padding: 8px;\n"
    "    margin-bottom: 8px;\n"
    "    box-shadow: 0 1px 3px rgba(0, 110, 130, 0.1);\n"
    "}\n"
    "\n"
    "frame > border { \n"
    "    border: none;\n"
    "    border-radius: 8px;\n"
    "}\n"
    "\n"
    "frame > label { /* Style frame titles */\n"
    "    font-weight: bold;\n"
    "    color: #00796b; /* Dark Teal title */\n"
    "    margin-bottom: 6px;\n"
    "    margin-left: 5px;\n"
    "}\n"
    "\n"
    "/* Button Styling */\n"
    "button {\n"
    "    background-image: linear-gradient(to bottom, #26c6da, #00acc1); /* Cyan gradient */\n"
    "    color: white; /* White text */\n"
    "    font-weight: bold;\n"
    "    text-shadow: 1px 1px 1px rgba(0, 0, 0, 0.2);\n"
    "    border: 1px solid #00838f; /* Darker Cyan border */\n"
    "    border-radius: 6px;\n"
    "    padding: 8px 15px; /* Slightly larger padding */\n"
    "    margin: 3px;\n"
    "    transition: all 0.2s ease-in-out;\n"
    "    box-shadow: 0 3px 5px rgba(0, 96, 100, 0.2); /* Slightly stronger shadow */\n"
    "}\n"
    "\n"
    "button:hover {\n"
    "    background-image: linear-gradient(to bottom, #4dd0e1, #26c6da); /* Lighter Cyan on hover */\n"
    "    box-shadow: 0 5px 8px rgba(0, 96, 100, 0.3);\n"
    "    transform: translateY(-1px); /* Subtle lift */\n"
    "}\n"
    "\n"
    "button:active {\n"
    "    background-image: linear-gradient(to bottom, #00acc1, #0097a7); /* Darker Cyan when pressed */\n"
    "    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);\n"
    "    transform: translateY(1px);\n"
    "}\n"
    "\n"
    "/* Combo Box Styling */\n"
    "combobox > box > cellview { \n"
    "    padding: 6px;\n"
    "    border-radius: 5px; \n"
    "    background-color: #e0f7fa; /* Match window background */\n"
    "    color: #006064; /* Dark cyan text */\n"
    "}\n"
    "\n"
    "combobox button.combo { \n"
    "    padding: 0px 6px;\n"
    "    border-radius: 0 5px 5px 0;\n"
    "    border-left: none; \n"
    "    background-image: linear-gradient(to bottom, #4dd0e1, #26c6da); /* Match button hover gradient */\n"
    "    border: 1px solid #00838f;\n"
    "    border-left: 1px solid #00838f;\n"
    "}\n"
    "combobox button.combo:hover {\n"
    "     background-image: linear-gradient(to bottom, #80deea, #4dd0e1);\n"
    "}\n"
    "combobox button.combo:active {\n"
    "     background-image: linear-gradient(to bottom, #26c6da, #00acc1);\n"
    "}\n"
    "\n"
    "/* Spin Button Styling */\n"
    "spinbutton {\n"
    "    border-radius: 5px;\n"
    "    border: 1px solid #26c6da; /* Medium Cyan border */\n"
    "}\n"
    "spinbutton entry {\n"
    "    padding: 6px;\n"
    "    border: none; \n"
    "    background-color: #e0f7fa; /* Light cyan background */\n"
    "    border-radius: 5px 0 0 5px;\n"
    "    color: #006064; /* Dark cyan text */\n"
    "}\n"
    "spinbutton button.up {\n"
    "    border-radius: 0 5px 0 0;\n"
    "    background-image: linear-gradient(to bottom, #80deea, #4dd0e1); /* Lighter cyan for arrows */\n"
    "    border: 1px solid #00acc1; border-left: none;\n"
    "    padding: 2px 5px;\n"
    "}\n"
    "spinbutton button.down {\n"
    "    border-radius: 0 0 5px 0;\n"
    "    background-image: linear-gradient(to bottom, #80deea, #4dd0e1); /* Lighter cyan for arrows */\n"
    "    border: 1px solid #00acc1; border-left: none; border-top: none;\n"
    "    padding: 2px 5px;\n"
    "}\n"
    "spinbutton button:hover {\n"
    "    background-image: linear-gradient(to bottom, #b2ebf2, #80deea);\n"
    "}\n"
    "spinbutton button:active {\n"
    "    background-image: linear-gradient(to bottom, #4dd0e1, #26c6da);\n"
    "}\n"
    "\n"
    "/* Text View Styling (Consoles/Logs/Memory) */\n"
    "#log_view textview, #memory_view textview {\n"
    "    background-image: linear-gradient(to bottom, #f5f5f5, #e0e0e0); /* Slightly brighter Gray gradient */\n"
    "    color: #212121; /* Dark gray text for readability */\n"
    "    font-family: monospace;\n"
    "    padding: 8px;\n"
    "    border-radius: 6px;\n"
    "    border: 1px solid #bdbdbd;\n"
    "    box-shadow: inset 0 1px 3px rgba(0,0,0,0.1);\n"
    "}\n"
    "\n"
    "/* Style the ScrolledWindow containing the TextViews */\n"
    "#log_view, #memory_view {\n"
    "    border: none;\n"
    "    border-radius: 6px;\n"
    "}\n"
    "\n"
    "/* TreeView Styling (Placeholders, adjust if using real TreeViews) */\n"
    "treeview {\n"
    "    border: 1px solid #4dd0e1; /* Medium Cyan border */\n"
    "    border-radius: 6px;\n"
    "    background-color: #e0f7fa; /* Light cyan background */\n"
    "}\n"
    "treeview header button { /* Style column headers */\n"
    "    background-image: linear-gradient(to bottom, #80deea, #4dd0e1); /* Lighter cyan header */\n"
    "    color: #004d40; /* Very dark teal text */\n"
    "    border: 1px solid #26c6da;\n"
    "    border-radius: 0;\n"
    "    padding: 6px;\n"
    "    font-weight: bold;\n"
    "    box-shadow: none;\n"
    "    text-shadow: none;\n"
    "}\n"
    "treeview header button:hover {\n"
    "    background-image: linear-gradient(to bottom, #b2ebf2, #80deea);\n"
    "}\n"
    "treeview:selected { /* Style selected rows */\n"
    "    background-color: #00bcd4; /* Vibrant Cyan selection */\n"
    "    color: white;\n"
    "}\n"
    "\n"
    "/* Label Styling */\n"
    "label {\n"
    "    color: #004d40; /* Dark Teal default text color */\n"
    "    /* #overview_label { font-size: 1.1em; color: #00796b; } */\n"
    "}\n"
    "\n"
    ; // End of the string literal definition


// --- Rest of your C code remains the same ---

// Placeholder function for button clicks - replace with actual logic
static void button_clicked(GtkWidget *widget, gpointer data) {
    const gchar *button_label = gtk_button_get_label(GTK_BUTTON(widget));
    g_print("%s clicked\n", button_label);
    // Add simulation logic here based on the button clicked
}

// Placeholder function for scheduler selection - replace with actual logic
static void scheduler_selected(GtkComboBox *widget, gpointer data) {
    gchar *selected_scheduler = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
    if (selected_scheduler) {
        g_print("Scheduler selected: %s\n", selected_scheduler);
        // Add logic to switch scheduler here
        g_free(selected_scheduler);
    }
}

// Placeholder function for quantum adjustment - replace with actual logic
static void quantum_adjusted(GtkSpinButton *spin_button, gpointer data) {
    gint quantum = gtk_spin_button_get_value_as_int(spin_button);
    g_print("Quantum adjusted to: %d\n", quantum);
    // Add logic to update quantum here
}


// Main application activation function
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *main_vbox, *top_hbox, *bottom_hbox;
    GtkWidget *dashboard_frame, *scheduler_frame, *resource_frame, *memory_frame, *log_frame, *control_frame;
    GtkWidget *dashboard_vbox, *scheduler_vbox, *resource_vbox, *log_vbox, *control_vbox;
    GtkWidget *label;
    GtkWidget *process_list_view, *ready_queue_view, *blocked_queue_view; // Placeholder TreeViews
    GtkWidget *memory_text_view; // Placeholder for Memory View
    GtkWidget *log_text_view; // Placeholder for Log/Console
    GtkWidget *scheduler_combo;
    GtkWidget *start_button, *stop_button, *reset_button, *add_process_button, *step_button, *run_button;
    GtkWidget *quantum_label, *quantum_spin_button;
    GtkAdjustment *quantum_adjustment;
    GtkCssProvider *css_provider; // <<--- CSS Provider
    GdkDisplay *display;
    GdkScreen *screen;


    // --- Load CSS ---  (Do this early)
    css_provider = gtk_css_provider_new();
    // Use NULL for GError argument for simplicity here, check error in real app
    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);

    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);

    // Apply the CSS provider to the whole screen (affects all windows of the app)
    gtk_style_context_add_provider_for_screen(screen,
                                              GTK_STYLE_PROVIDER(css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    // Decrement reference count as the screen now holds a reference
    g_object_unref(css_provider);


    // --- Main Window ---
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "OS Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 750); // Slightly taller
    gtk_container_set_border_width(GTK_CONTAINER(window), 15); // More padding
    gtk_widget_set_name(window, "main_window"); // <<--- Assign name for CSS targeting


    // --- Main Layout ---
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15); // Increased spacing
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    top_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15); // Increased spacing
    gtk_box_pack_start(GTK_BOX(main_vbox), top_hbox, TRUE, TRUE, 0);

    bottom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15); // Increased spacing
    gtk_box_pack_start(GTK_BOX(main_vbox), bottom_hbox, TRUE, TRUE, 0);


    // --- Main Dashboard Frame ---
    dashboard_frame = gtk_frame_new("Main Dashboard");
    gtk_box_pack_start(GTK_BOX(top_hbox), dashboard_frame, TRUE, TRUE, 0);
    dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8); // Spacing inside frame
    gtk_container_set_border_width(GTK_CONTAINER(dashboard_frame), 8);
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_vbox);

    // Overview Section (Placeholders)
    label = gtk_label_new("Overview: Processes: -, Clock: 0, Scheduler: -");
    // gtk_widget_set_name(label, "overview_label"); // Example if specific styling needed
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);

    // Process List (Placeholder TreeView)
    label = gtk_label_new("\nProcess List:"); // Consider using frame label property instead if using real TreeView
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);
    // TODO: Replace with actual GtkTreeView and GtkListStore for processes
    process_list_view = gtk_text_view_new(); // Simple placeholder
    // If replacing with TreeView, style 'treeview' in CSS
    gtk_text_view_set_editable(GTK_TEXT_VIEW(process_list_view), FALSE);
    GtkWidget* scrolled_window_processes = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window_processes), process_list_view);
    gtk_widget_set_vexpand(scrolled_window_processes, TRUE);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), scrolled_window_processes, TRUE, TRUE, 0);


    // Queue Section (Placeholders)
    label = gtk_label_new("\nQueues:");
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);
    // TODO: Add sections/labels/Treeviews for Ready Queue, Blocked Queue, Running Process
    label = gtk_label_new("Ready Queue: [Placeholder]");
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new("Blocked Queue: [Placeholder]");
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new("Running Process: [Placeholder]");
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), label, FALSE, FALSE, 0);


    // --- Scheduler Control Panel Frame ---
    scheduler_frame = gtk_frame_new("Scheduler Control");
    gtk_box_pack_start(GTK_BOX(top_hbox), scheduler_frame, FALSE, FALSE, 0); // Use less space
    scheduler_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(scheduler_frame), 8);
    gtk_container_add(GTK_CONTAINER(scheduler_frame), scheduler_vbox);

    // Scheduler Selection Dropdown
    label = gtk_label_new("Algorithm:");
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), label, FALSE, FALSE, 0);
    scheduler_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scheduler_combo), "Multilevel Feedback Queue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(scheduler_combo), 0); // Default selection
    g_signal_connect(scheduler_combo, "changed", G_CALLBACK(scheduler_selected), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), scheduler_combo, FALSE, FALSE, 0);

    // Quantum Adjustment
    quantum_label = gtk_label_new("\nRound Robin Quantum:");
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), quantum_label, FALSE, FALSE, 5); // Add padding top
    // Adjustment: value, lower, upper, step_increment, page_increment, page_size
    quantum_adjustment = gtk_adjustment_new(2.0, 1.0, 100.0, 1.0, 5.0, 0.0); // Example quantum 2
    quantum_spin_button = gtk_spin_button_new(quantum_adjustment, 1.0, 0); // 0 decimal places
    g_signal_connect(quantum_spin_button, "value-changed", G_CALLBACK(quantum_adjusted), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), quantum_spin_button, FALSE, FALSE, 0);

    // Start/Stop/Reset Buttons
    start_button = gtk_button_new_with_label("Start");
    g_signal_connect(start_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), start_button, FALSE, FALSE, 10); // Add padding top

    stop_button = gtk_button_new_with_label("Stop");
    g_signal_connect(stop_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), stop_button, FALSE, FALSE, 0);

    reset_button = gtk_button_new_with_label("Reset");
    g_signal_connect(reset_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), reset_button, FALSE, FALSE, 0);


    // --- Resource Management Panel Frame ---
    resource_frame = gtk_frame_new("Resource Management");
    gtk_box_pack_start(GTK_BOX(top_hbox), resource_frame, FALSE, FALSE, 0); // Use less space
    resource_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(resource_frame), 8);
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_vbox);

    // Mutex Status (Placeholders)
    label = gtk_label_new("Mutex Status:");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new("userInput: Free");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new("userOutput: Free");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new("file: Free");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);

    // Blocked Queue for Resources (Placeholder)
    label = gtk_label_new("\nResource Blocked Queue:");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);
    // TODO: Add a TreeView or ListBox here
    label = gtk_label_new("[Placeholder List]");
    gtk_box_pack_start(GTK_BOX(resource_vbox), label, FALSE, FALSE, 0);


    // --- Memory Viewer Frame ---
    memory_frame = gtk_frame_new("Memory View (60 words)"); //
    gtk_box_pack_start(GTK_BOX(bottom_hbox), memory_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(memory_frame), 8);
    // TODO: Use GtkGrid for a visual grid or GtkTextView for text representation
    memory_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(memory_text_view), FALSE);
    GtkWidget* scrolled_window_memory = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_name(scrolled_window_memory, "memory_view"); // <<--- Assign name for CSS
    gtk_container_add(GTK_CONTAINER(scrolled_window_memory), memory_text_view);
    gtk_widget_set_vexpand(scrolled_window_memory, TRUE);
    gtk_widget_set_hexpand(scrolled_window_memory, TRUE);
    gtk_container_add(GTK_CONTAINER(memory_frame), scrolled_window_memory);
    // Populate with initial empty state or placeholder text
    GtkTextBuffer *mem_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(memory_text_view));
    gtk_text_buffer_set_text(mem_buffer, "Memory visualization placeholder...\n(Styled with CSS)", -1);

    // --- Log & Console Panel Frame ---
    log_frame = gtk_frame_new("Log & Console");
    gtk_box_pack_start(GTK_BOX(bottom_hbox), log_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(log_frame), 8);
    log_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(log_frame), log_vbox);

    // Execution Log / Event Messages (Placeholder TextView)
    log_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_text_view), GTK_WRAP_WORD_CHAR);
    GtkWidget* scrolled_window_log = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_name(scrolled_window_log, "log_view"); // <<--- Assign name for CSS
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_log), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window_log), log_text_view);
    gtk_widget_set_vexpand(scrolled_window_log, TRUE);
    gtk_box_pack_start(GTK_BOX(log_vbox), scrolled_window_log, TRUE, TRUE, 0);
    // Populate with initial empty state or placeholder text
    GtkTextBuffer *log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text_view));
    gtk_text_buffer_set_text(log_buffer, "Execution Log and System Events will appear here...\n(Styled with CSS)\n", -1);


    // --- Process & Execution Control Frame ---
    control_frame = gtk_frame_new("Process & Execution Control");
    gtk_box_pack_start(GTK_BOX(bottom_hbox), control_frame, FALSE, FALSE, 0); // Use less space
    control_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(control_frame), 8);
    gtk_container_add(GTK_CONTAINER(control_frame), control_vbox);

    // Add Process Button
    add_process_button = gtk_button_new_with_label("Add Process");
    // TODO: Connect to a function that opens a file chooser
    //       and handles process creation/configuration
    g_signal_connect(add_process_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_vbox), add_process_button, FALSE, FALSE, 0);

    // TODO: Add widgets for setting arrival time if needed
    label = gtk_label_new("(Set arrival time after adding)"); // Placeholder text
    gtk_box_pack_start(GTK_BOX(control_vbox), label, FALSE, FALSE, 0);


    // Step-by-Step Execution Button
    step_button = gtk_button_new_with_label("Step (1 Clock Cycle)");
    g_signal_connect(step_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_vbox), step_button, FALSE, FALSE, 10); // Padding top

    // Auto Execution Button
    run_button = gtk_button_new_with_label("Run Continuously");
    g_signal_connect(run_button, "clicked", G_CALLBACK(button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_vbox), run_button, FALSE, FALSE, 0);


    // --- Show Window ---
    gtk_widget_show_all(window);
}

// Main function
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example.os_scheduler", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}