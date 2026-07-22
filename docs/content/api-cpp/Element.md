# NetzWirbel::Element

The base class for all C++ representation of DOM elements. 

## Constructor

```cpp
Element(Context* ctx, const std::string& tag_name);
Element(Context* ctx, uint32_t tag_name_id);
```
Constructs a new element and automatically issues a `CREATE_ELEMENT` command to Javascript. Using `tag_name_id` with `Context::strings` is heavily recommended to save memory and network overhead.

## Properties

### `uint32_t get_id() const`
Returns the unique auto-generated ID of the element, which is used to sync with the Javascript proxy object.

### `const std::string& get_tag_name() const`
Returns the HTML tag name (e.g., "div", "button").

## DOM Manipulation

### `void set_attribute(...)`
```cpp
void set_attribute(const std::string& key, const std::string& value);
void set_attribute(uint32_t key_id, const std::string& value);
void set_attribute(uint32_t key_id, uint32_t value_id);
```
Issues a `SET_ATTRIBUTE` command. Overloads taking `uint32_t` IDs map to registered strings in the JS bridge.

### `void set_class_conflated(const std::string& class_name)`
Issues a lock-free, O(1) amortized `SET_CLASS_CONFLATED` command. It bypasses the standard ring buffer queue, ensuring that if multiple rapid updates to `className` occur before Javascript can render them, only the very latest class is transmitted. Highly recommended for rapidly changing visual states (like market data up/down ticks).

### `void set_style(...)`
```cpp
void set_style(const std::string& name, const std::string& value);
void remove_style(const std::string& name);
void set_styles(std::initializer_list<std::pair<std::string, std::string>> styles);
```
Issues a highly efficient `SET_STYLES` command. Mutates only the specified CSS properties without rewriting or touching the entire `style` string attribute, preventing conflicts with other styles. Passing an empty string value via `set_style`, or using `remove_style`, will remove the CSS property from the element.

### `void set_style_conflated(const std::string& name, const std::string& value)`
Issues a lock-free, O(1) amortized `SET_STYLE_CONFLATED` command for an individual CSS property. Uses a dedicated atomic pointer per CSS property to safely conflate redundant style updates (e.g. rapidly changing `color` or `width`) without filling the standard queue.

### `void set_property(...)`
```cpp
void set_property(const std::string& key, const std::string& value);
void set_property(uint32_t key_id, const std::string& value);
void set_property(uint32_t key_id, uint32_t value_id);

void set_property(const std::string& key, bool value);
void set_property(uint32_t key_id, bool value);

void set_property(const std::string& key, double value);
void set_property(uint32_t key_id, double value);
```
Issues a `SET_PROPERTY` command of the appropriate type.

### `void set_text_content(...)`
```cpp
void set_text_content(const std::string& text);
void set_text_content(uint32_t text_id);
```
Issues a `SET_TEXT_CONTENT` command.

### `void set_text_content_conflated(const std::string& text)`
Updates the text content of the element using a lock-free, O(1) amortized approach (`SET_TEXT_CONTENT_CONFLATED`). If multiple text updates are issued before Javascript processes them, intermediate updates are safely discarded in C++, leaving only the final text string for JS to read. This is crucial for high-frequency data applications (like 60Hz order books).

### `void append_child(std::shared_ptr<Element> child)`
Appends another Element instance as a child node in the DOM. Issues an `APPEND_CHILD` command.

### `virtual void remove_child(std::shared_ptr<Element> child)`
Removes the specified child element from the DOM and issues a `REMOVE_CHILD` command. Note: This does not automatically destroy the child element in C++ memory; it merely detaches it.

### `virtual void destroy()`
Recursively destroys this element and all its children. Removes it from its parent (if any), unregisters it from the `Context`, and issues a `DESTROY_ELEMENT` command to Javascript to release the DOM node and break memory links. Useful for cleaning up UI components when they are permanently closed.

## Event Handling

### `void add_event_listener(...)`
```cpp
void add_event_listener(const std::string& event_type, std::function<void(const Event&)> callback, bool prevent_default = false);
void add_event_listener(uint32_t event_type_id, std::function<void(const Event&)> callback, bool prevent_default = false);
```
Registers an event callback in C++ and issues an `ADD_EVENT_LISTENER` command to JS.

### `virtual void handle_event(const Event& event)`
Called by the Context when a JS event occurs.

### `virtual void handle_property_changed(...)`
Overloadable methods called when Javascript reports that an element property has changed (e.g., user input).
