# NetzWirbel::Event

The base event class that holds information propagated from the browser DOM to C++.

## `Event` (Base)

```cpp
Event(const std::string& type);
```
- **`const std::string& get_type() const`**: Returns the type of the event (e.g. "click", "input").

## `MouseEvent` (Derived)

Represents events like `click`, `mousemove`, `mousedown`, `mouseup`.

- **`double get_client_x() const`**: X coordinate relative to the client area.
- **`double get_client_y() const`**: Y coordinate relative to the client area.

> [!TIP]
> If you register a mouse event listener, you can use `static_cast<const NetzWirbel::MouseEvent&>(event)` inside your callback to safely access coordinates.

## `KeyboardEvent` (Derived)

Represents events like `keydown`, `keyup`, `keypress`.

- **`const std::string& get_key() const`**: Returns the key value of the key pressed (e.g., "Enter", "a", "Escape").
