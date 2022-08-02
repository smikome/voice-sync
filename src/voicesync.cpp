#include <gtkmm.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <iostream>


#define VOICE_SYNC_DOUBLE_CLICK_INTERVAL 300

class App : public Gtk::Window
{
public:
    App();
    bool StartAudioCapture();
    void EndAudioCapture();
    void SetLoudness(const float &value);

private:
    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &cr);
    void OnScreenChanged(const Glib::RefPtr<Gdk::Screen> &screen);
    bool OnButtonPressed(GdkEventButton *eventButton);
    bool OnButtonReleased(GdkEventButton *eventButton);
    bool OnMotionNotify(GdkEventMotion *eventMotion);
    void ShowExitConfirmationDialog();
    void SetImage(const std::string &fileName);
    bool UpdateImage();
    void DisableDoubleClick();

    Gtk::EventBox mEventBox;
    Gtk::Image mImage;
    bool mCanDoubleClick = false;

    ma_device mDevice;
    bool mIsCapturing = false;
    float mLoudness = 0.0f;
};

App::App()
    :Gtk::Window()
{
    set_title("voice sync test");

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

    SetImage("./res/00.png");

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &App::UpdateImage), 1000 / 12);
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
    std::cout << "OnButtonPressed" << std::endl;
    return true;
}

bool App::OnButtonReleased(GdkEventButton *eventButton)
{
    if(mCanDoubleClick)
    {
        ShowExitConfirmationDialog();
        mCanDoubleClick = false;
    }
    else
    {
        mCanDoubleClick = true;
        Glib::signal_timeout().connect_once(sigc::mem_fun(*this, &App::DisableDoubleClick), VOICE_SYNC_DOUBLE_CLICK_INTERVAL);
    }
    std::cout << "OnButtonReleased" << std::endl;
    return true;
}

bool App::OnMotionNotify(GdkEventMotion *eventMotion)
{
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

void App::SetImage(const std::string &fileName)
{
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(fileName);
    constexpr int HEIGHT = 128;
    float scale = (float)HEIGHT / pixbuf->get_height();
    int width = pixbuf->get_width() * scale;
    pixbuf = pixbuf->scale_simple(width, HEIGHT, Gdk::INTERP_BILINEAR);
    mImage.set(pixbuf);
    mImage.show();
}

bool App::UpdateImage()
{
    static int mode = 0;
    mode++;
    if(mLoudness > 2.0f)
    {
        mode %= 4;
    }
    else if(mLoudness > 1.2f)
    {
        mode %= 3;
    }
    else if(mLoudness > 0.8f)
    {
        mode %= 2;
    }
    else
    {
        mode = 0;
    }
    if(mode == 0)
    {
        SetImage("./res/00.png");
    }
    else if(mode == 1)
    {
        SetImage("./res/01.png");
    }
    else if(mode == 2)
    {
        SetImage("./res/02.png");
    }
    else
    {
        SetImage("./res/03.png");
    }
    return true;
}

void App::DisableDoubleClick()
{
    mCanDoubleClick = false;
}

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    App *app = (App *)pDevice->pUserData;
    MA_ASSERT(app != NULL);

    int channels = 1;
    float* pInputF32 = (float*)pInput;
    float loudness = 0.0f;

    for (int iFrame = 0; iFrame < frameCount; iFrame += 1) {
        for (int iChannel = 0; iChannel < channels; iChannel += 1) {
            float abs = pInputF32[iFrame * channels + iChannel];
            if(abs < 0)
            {
                abs *= -1;
            }
            loudness += abs;
        }
    }

    loudness = (loudness / frameCount) * 100;

    std::cout << "loudness: " << loudness << std::endl;
    app->SetLoudness(loudness);

    (void)pOutput;
}

bool App::StartAudioCapture()
{
    ma_result result;
    ma_device_config deviceConfig;

    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format   = ma_format_f32;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate       = 5000;
    deviceConfig.dataCallback     = data_callback;
    deviceConfig.pUserData = (void *)this;

    result = ma_device_init(NULL, &deviceConfig, &mDevice);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize capture device.\n");
        return false;
    }

    result = ma_device_start(&mDevice);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&mDevice);
        printf("Failed to start device.\n");
        return false;
    }

    return true;
}

void App::EndAudioCapture()
{
    if(!mIsCapturing)
    {
        return;
    }
    mIsCapturing = false;
    ma_device_uninit(&mDevice);
}

void App::SetLoudness(const float &value)
{
    mLoudness = value;
}

int main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);
    App app;
    app.StartAudioCapture();
    Gtk::Main::run(app);
    app.EndAudioCapture();
    return 0;
}
