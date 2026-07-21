# NetzWirbel::Context

The `Context` is the core orchestrator of NetzWirbel on the C++ side. It manages the lifecycle of DOM elements and facilitates communication via the ring buffers.

## Initialization

```cpp
Context(void* cpp_to_js_mem, void* js_to_cpp_mem, uint32_t capacity);
```
- **`cpp_to_js_mem`**: Pointer to the memory chunk dedicated to the Producer ring buffer (C++ sending to JS).
- **`js_to_cpp_mem`**: Pointer to the memory chunk dedicated to the Consumer ring buffer (C++ receiving from JS).
- **`capacity`**: Maximum number of commands/events the buffers can hold. Must be a power of 2.

## Methods

### `void register_element(std::shared_ptr<Element> el)`
Registers an element with the context. Generally, you do not need to call this manually; it is called within the `Element` constructor.

### `void unregister_element(uint32_t id)`
Removes an element from the context's internal tracking map by its ID.

### `std::shared_ptr<Element> get_element(uint32_t id)`
Retrieves a previously registered element by its ID.

### `void bind_element(std::shared_ptr<Element> el, const std::string& selector)`
Instructs the Javascript bridge to locate an existing DOM element using `document.querySelector(selector)` and bind the provided C++ element to it. This allows you to construct the UI via HTML/CSS and only attach logic using C++.

### `void send_command(const Command& cmd)`
Pushes a command into the `cpp_to_js_` ring buffer. Blocks briefly if the buffer is full (though typically it operates lock-free).

### `void process_events()`
Drains the `js_to_cpp_` ring buffer, processing any incoming events (e.g., `EVENT`, `PROPERTY_CHANGED_STRING`) and routing them to the appropriate `Element` instances.

### `void send_ping()`
Sends a ping command to Javascript to measure Round Trip Time (RTT).

### `RTTStats get_rtt_stats() const`
Returns the latency statistics measured by ping/pong interactions.

### `void increment_conflation_hits()`
Increments the internal lock-free counter for tracking the number of redundant DOM updates skipped by the conflation engine. Used internally by `Element`.

### `uint64_t get_conflation_hits() const`
Returns the total number of conflation hits (redundant commands skipped) since context initialization.

### `uint32_t register_string(const std::string& str)`
Registers a string with the Javascript bridge and returns a persistent unique ID. This prevents the framework from repeatedly transmitting the same string (e.g., heavily used attribute keys or values) over the ring buffer, saving memory and performance. 

## Pre-registered Strings
Upon context initialization, `Context::strings` is automatically populated with the IDs of commonly used HTML tags, attributes, CSS styles, events, and values.

```cpp
auto el = std::make_shared<Element>(ctx, ctx->strings.div);
el->set_attribute(ctx->strings.class_, "container");
el->set_property(ctx->strings.id, "my-div");
el->add_event_listener(ctx->strings.click, [](const Event& e) {
    // ...
});
```
This is the recommended way to create elements and assign standard attributes.

### List of Pre-registered Strings (`Context::strings`)

**Elements**:
`div`, `span`, `input`, `button`, `a`, `img`, `p`, `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `table`, `tr`, `td`, `th`, `thead`, `tbody`, `form`, `label`, `select`, `option`, `textarea`, `ul`, `li`

**Attributes**:
`id`, `class_` (maps to "class"), `style`, `src`, `href`, `type`, `value`, `placeholder`, `disabled`, `checked`, `name`, `data_val` (maps to "data-val")

**Styles**:
`color`, `background_color` (maps to "background-color"), `font_size` (maps to "font-size"), `font_weight` (maps to "font-weight"), `margin`, `padding`, `border`, `display`, `flex`, `grid`

**Events**:
`click`, `mouseup`, `mousedown`, `mousemove`, `input_event` (maps to "input"), `change`, `keydown`, `keyup`, `submit`

**Common Values**:
`true_` (maps to "true"), `false_` (maps to "false"), `text`, `zero` (maps to "0")
