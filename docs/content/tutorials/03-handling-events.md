# Tutorial 3: Handling Events

NetzWirbel allows you to listen for browser DOM events directly in C++. 

## Adding Event Listeners

Use the `add_event_listener` method on an `Element` instance.

```cpp
#include "NetzWirbel/DOM/Elements.hpp"
#include <iostream>

auto btn = std::make_shared<NetzWirbel::HTMLButtonElement>(g_ctx);
btn->set_text_content("Submit");

// Add a click listener
btn->add_event_listener("click", [](const NetzWirbel::Event& e) {
    std::cout << "Button was clicked! Event type: " << e.get_type() << std::endl;
});

// For mouse events, you can cast the event to MouseEvent
auto canvas = std::make_shared<NetzWirbel::HTMLCanvasElement>(g_ctx);
canvas->add_event_listener("mousemove", [](const NetzWirbel::Event& e) {
    if (e.get_type() == "mousemove") {
        const auto& mouse_e = static_cast<const NetzWirbel::MouseEvent&>(e);
        std::cout << "Mouse moved to: " << mouse_e.get_client_x() << ", " << mouse_e.get_client_y() << std::endl;
    }
});
```

## Processing Events

For the event callbacks to fire, your C++ application must regularly process the events coming from JavaScript. In a typical game loop or requestAnimationFrame loop, you should call `process_events()` on the Context.

```cpp
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void netzwirbel_tick(double time) {
        if (!g_ctx) return;
        
        // This reads from the JS-to-C++ ring buffer and triggers your callbacks
        g_ctx->process_events();
    }
}
```

## Two-Way Property Binding

Certain elements, like `HTMLInputElement`, automatically sync their state back to C++ when the user interacts with them. NetzWirbel's JS bridge listens for `input` events and sends a `PROPERTY_CHANGED_STRING` command back to C++.

```cpp
auto input = std::make_shared<NetzWirbel::HTMLInputElement>(g_ctx);

// When the user types in the input box, input->get_value() will automatically update in C++!
// You can also manually intercept this by overriding `handle_property_changed` in a custom subclass.
```
