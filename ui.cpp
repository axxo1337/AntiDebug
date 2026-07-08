#include <mutex>
#include <atomic>

#include "ui.h"

//
// [SECTION] Variables
//

static std::atomic<ftxui::ScreenInteractive*> p_screen{ nullptr };

//
// [SECTION] Functions
//

void UI::routine()
{
    using namespace ftxui;

    auto screen{ ftxui::ScreenInteractive::Fullscreen() };
    std::vector<Component> checkboxes;
    auto& options{ AntiDebug::getOptions() };

    p_screen = &screen;

    /* Lock guard scope */
    {
        std::lock_guard<std::mutex> guard(AntiDebug::options_mutex);
        for (auto& option : options)
            checkboxes.push_back(Checkbox(std::string(option.name), &option.enabled));
    }

    auto checkbox_container{ Container::Vertical(checkboxes) };
    auto component = Renderer(checkbox_container, [&] {
        std::lock_guard<std::mutex> guard(AntiDebug::options_mutex);
        int detections_count{};
        Elements checkbox_elements;

        for (int i{}; i < options.size(); i++)
        {
            if (options[i].detected)
                detections_count++;

            auto status{ options[i].detected ? text(" [DETECTED]") | color(Color::Red) : text("") };
            checkbox_elements.push_back(
                hbox({
                    checkboxes[i]->Render(),
                    status
                    })
            );
        }

        bool is_detected{ detections_count > 0 };
        std::string detection_text("Debugging is currently ");

        if (is_detected)
            detection_text += "detected " + std::to_string(detections_count) + (detections_count > 1 ? " times." : " time.");
        else
            detection_text += "not detected.";

        return vbox({
            text("AntiDebug - axxo1337") | bold | center,
            text(detection_text) | (is_detected ? color(Color::Red) : color(Color::Green)) | center,
            separator(),
            vbox(checkbox_elements) | flex | vscroll_indicator | frame,
            separator(),
            text("Use arrow keys to navigate, Space to toggle, Q to quit") | dim
        }) | border;
    });

    component |= CatchEvent([&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q'))
        {
            running = false;
            screen.Exit();
            return true;
        }

        return false;
    });

    screen.Loop(component);
    p_screen = nullptr;
}

void UI::triggerUpdate()
{
    auto screen_ptr = p_screen.load();
    if (screen_ptr != nullptr)
        screen_ptr->PostEvent(ftxui::Event::Custom);
}
