# Tutorial 2: Creating Elements

Once your NetzWirbel `Context` is initialized, you can create and manipulate elements from C++.

## Creating a basic element

You can use the built-in derived classes in `Elements.hpp` or the base `Element` class. 

```cpp
#include "NetzWirbel/DOM/Elements.hpp"
#include <memory>

// Inside your initialization or update logic...

// Create a div
auto container = std::make_shared<NetzWirbel::HTMLDivElement>(g_ctx);

// Create a canvas
auto canvas = std::make_shared<NetzWirbel::HTMLCanvasElement>(g_ctx);
canvas->set_width(800);
canvas->set_height(600);

// Append the canvas to the container
container->append_child(canvas);

// To append to the document body, use targetId 0 (handled internally by Context/Bridge)
// Note: Currently, append_child expects an Element. You can set up a root element
// or modify the bridge to handle a special "body" append. 
```

## Modifying Properties and Attributes

NetzWirbel provides generic methods to set attributes and properties, as well as strongly-typed methods in derived classes.

```cpp
auto input = std::make_shared<NetzWirbel::HTMLInputElement>(g_ctx);
input->set_attribute("placeholder", "Enter text here...");
input->set_value("Initial Value"); // Uses the derived class method

auto btn = std::make_shared<NetzWirbel::HTMLButtonElement>(g_ctx);
btn->set_text_content("Click Me!");
btn->set_disabled(true);
```

> [!NOTE]
> Changes made via C++ are queued in the `cpp_to_js_` ring buffer and are asynchronously applied to the actual browser DOM on the next animation frame.
