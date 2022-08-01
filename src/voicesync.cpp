#include <gtkmm.h>

#include <iostream>

class App : public Gtk::Window
{
public:
    App();
private:
    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &cr);
    void OnScreenChanged(const Glib::RefPtr<Gdk::Screen> &screen);
    bool OnButtonPressed(GdkEventButton *eventButton);
    bool OnButtonReleased(GdkEventButton *eventButton);
    bool OnMotionNotify(GdkEventMotion *eventMotion);
    void ShowExitConfirmationDialog();

    Gtk::EventBox mEventBox;
    Gtk::Image mImage;
    bool mMotioned = false;
};

App::App()
    :Gtk::Window()
{
    set_title("voice sync test");
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file("./res/00.png");
    constexpr int HEIGHT = 128;
    float scale = (float)HEIGHT / pixbuf->get_height();
    int width = pixbuf->get_width() * scale;
    pixbuf = pixbuf->scale_simple(width, HEIGHT, Gdk::INTERP_BILINEAR);
    mImage.set(pixbuf);

    mEventBox.set_events(Gdk::EventMask::BUTTON_PRESS_MASK | Gdk::EventMask::BUTTON_RELEASE_MASK | Gdk::EventMask::BUTTON_MOTION_MASK);
    mEventBox.signal_button_press_event().connect(sigc::mem_fun(*this, &App::OnButtonPressed));
    mEventBox.signal_button_release_event().connect(sigc::mem_fun(*this, &App::OnButtonReleased));
    mEventBox.signal_motion_notify_event().connect(sigc::mem_fun(*this, &App::OnMotionNotify));
    mEventBox.add(mImage);

    add(mEventBox);

    show_all_children();

    set_app_paintable(true);
    set_decorated(false);

    signal_draw().connect(sigc::mem_fun(*this, &App::OnDraw));
    signal_screen_changed().connect(sigc::mem_fun(*this, &App::OnScreenChanged));

    OnScreenChanged(get_screen());
}

int main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);
    App app;
    Gtk::Main::run(app);
    return 0;
}

gboolean supports_alpha = FALSE;

void App::OnScreenChanged(const Glib::RefPtr<Gdk::Screen> &screen)
{
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen->gobj());
    gtk_widget_set_visual(Gtk::Widget::gobj(), visual);

    std::cout << "screen_changed" << std::endl;
}

bool App::OnDraw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    cr->set_source_rgba(1.0, 1.0, 1.0, 0.0);
    cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
    cr->paint();
    cr->set_operator(Cairo::Operator::OPERATOR_OVER);

    return on_draw(cr);
}

bool App::OnButtonPressed(GdkEventButton *eventButton)
{
    mMotioned = false;
    std::cout << "OnButtonPressed" << std::endl;
    return true;
}

bool App::OnButtonReleased(GdkEventButton *eventButton)
{
    if(!mMotioned)
    {
        ShowExitConfirmationDialog();
    }
    mMotioned = false;
    std::cout << "OnButtonReleased" << std::endl;
    return true;
}

bool App::OnMotionNotify(GdkEventMotion *eventMotion)
{
    mMotioned = true;
    int width = get_width();
    int height = get_height();
    int x = eventMotion->x_root - (width / 2);
    int y = eventMotion->y_root - (height / 2);
    move(x, y);
    std::cout << "OnMotionNotify: " << (int)eventMotion->x_root << ", " << (int)eventMotion->y_root << std::endl;
    return true;
}

void App::ShowExitConfirmationDialog()
{
    Gtk::MessageDialog dialog(*this, "Exit App?", false, Gtk::MessageType::MESSAGE_QUESTION, Gtk::ButtonsType::BUTTONS_YES_NO);

    int result = dialog.run();

    if(result == Gtk::RESPONSE_YES)
    {
        gtk_main_quit();
    }
}
