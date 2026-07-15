# NetzWirbel::Elements (Derived)

NetzWirbel provides several strongly-typed wrappers around `NetzWirbel::Element` for common HTML elements.

## `HTMLDivElement`
A basic wrapper for the `div` tag.
- **Constructor**: `HTMLDivElement(Context* ctx)`

## `HTMLButtonElement`
A wrapper for the `button` tag.
- **`bool get_disabled() const`**
- **`void set_disabled(bool disabled)`**: Synchronizes the `disabled` property.

## `HTMLInputElement`
A wrapper for the `input` tag. This element automatically synchronizes its value from JS when the user types.
- **`std::string get_value() const`**
- **`void set_value(const std::string& value)`**: Synchronizes the `value` property.

## `HTMLCanvasElement`
A wrapper for the `canvas` tag.
- **`int get_width() const`**
- **`void set_width(int width)`**
- **`int get_height() const`**
- **`void set_height(int height)`**

## `HTMLImageElement`
A wrapper for the `img` tag.
- **`std::string get_src() const`**
- **`void set_src(const std::string& src)`**: Synchronizes the `src` attribute.

## `WindowElement`
A wrapper around the global DOM `window` object. This element will not create a new DOM node, but rather binds itself to the window if you call `ctx->bind_element(win, "window")`. Useful for handling global events such as `keydown`.
- **Constructor**: `WindowElement(Context* ctx)`

## `DocumentElement`
A wrapper around the global DOM `document` object. Similarly, use `ctx->bind_element(doc, "document")`.
- **Constructor**: `DocumentElement(Context* ctx)`
