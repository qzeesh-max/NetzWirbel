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

### `void set_style(...)`
```cpp
void set_style(const std::string& name, const std::string& value);
void remove_style(const std::string& name);
void set_styles(const std::vector<std::pair<std::string, std::string>>& styles);
```
Issues a highly efficient `SET_STYLES` command. Mutates only the specified CSS properties without rewriting or touching the entire `style` string attribute, preventing conflicts with other styles. Passing an empty string value via `set_style`, or using `remove_style`, will remove the CSS property from the element.

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

### `void append_child(std::shared_ptr<Element> child)`
Appends another Element instance as a child node in the DOM. Issues an `APPEND_CHILD` command.

## Event Handling

### `void add_event_listener(...)`
```cpp
void add_event_listener(const std::string& event_type, std::function<void(const Event&)> callback);
void add_event_listener(uint32_t event_type_id, std::function<void(const Event&)> callback);
```
Registers an event callback in C++ and issues an `ADD_EVENT_LISTENER` command to JS.

### `virtual void handle_event(const Event& event)`
Called by the Context when a JS event occurs.

### `virtual void handle_property_changed(...)`
Overloadable methods called when Javascript reports that an element property has changed (e.g., user input).
