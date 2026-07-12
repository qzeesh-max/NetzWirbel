# NetzWirbel UI Components

NetzWirbel provides several high-level UI components built on top of the base DOM elements. These components provide enhanced functionality, styling, and event handling out of the box.

## `Button`
A styled button component inheriting from `HTMLButtonElement`.
- **Constructor**: `Button(Context* ctx, const std::string& label = "")`
- **`void set_label(const std::string& label)`**: Updates the button text.
- **`void set_primary(bool primary)`**: Toggles primary styling.
- **`void set_on_click(std::function<void()> cb)`**: Sets the click event handler.

## `TextBox`
A specialized text input component inheriting from `HTMLInputElement`.
- **Constructor**: `TextBox(Context* ctx)`
- **`void set_text(const std::string& text)`**: Sets the current text value.
- **`std::string get_text() const`**: Retrieves the current text.
- **`void set_placeholder(const std::string& placeholder)`**: Sets the placeholder text.
- **`void set_on_change(std::function<void(const std::string&)> cb)`**: Sets the change event handler.

## `TextArea`
A multi-line text input component.
- **Constructor**: `TextArea(Context* ctx)`
- **`void set_text(const std::string& text)`**: Sets the text content.
- **`std::string get_text() const`**: Retrieves the text content.
- **`void set_placeholder(const std::string& placeholder)`**: Sets the placeholder text.
- **`void set_on_change(std::function<void(const std::string&)> cb)`**: Sets the change event handler.

## `RichTextArea`
A rich text editor component.
- **Constructor**: `RichTextArea(Context* ctx)`
- **`void set_text(const std::string& text)`**: Sets the initial rich text content.
- **`void set_on_change(std::function<void(const std::string&)> cb)`**: Event handler triggered when content changes.

## `Spinner`
A numeric spinner component with increment and decrement controls.
- **Constructor**: `Spinner(Context* ctx)`
- **`void set_value(double value)`**: Sets the current numeric value.
- **`double get_value() const`**: Gets the current numeric value.
- **`void set_min(double min)`**: Sets the minimum allowed value.
- **`void set_max(double max)`**: Sets the maximum allowed value.
- **`void set_step(double step)`**: Sets the step increment size.
- **`void set_decimals(int dec)`**: Sets the number of decimal places to display.
- **`void set_on_change(std::function<void(double)> cb)`**: Triggered when the value changes.

## `Window`
A draggable, resizable window container component.
- **Constructor**: `Window(Context* ctx, const std::string& title = "Window")`
- **`void set_title(const std::string& title)`**: Sets the window header title.
- **`std::shared_ptr<Element> get_content_container()`**: Returns the container element where window contents should be appended.
- **`void set_size(int w, int h)`**: Sets the initial dimensions.
- **`void set_position(int x, int y)`**: Sets the initial position on the screen.
- **`void show()`**: Displays the window.
- **`void hide()`**: Hides the window.

## `Grid<T>`
A highly functional, sortable, and resizable data grid component.
- **Constructor**: `Grid(Context* ctx)`
- **`void add_column(const std::string& name, int min_w, int initial_w)`**: Defines a new column.
- **`void set_on_render_row(std::function<void(std::shared_ptr<GridRow<T>>, const T&)> cb)`**: Sets the callback to render row cells based on data of type `T`.
- **`std::shared_ptr<GridRow<T>> add_row(const T& data)`**: Adds a new row with the specified data.
- **`void clear_rows()`**: Removes all rows from the grid.
- **`void set_on_cell_double_click(std::function<void(std::shared_ptr<GridRow<T>>, const std::string&)> cb)`**: Triggered when a cell is double-clicked.
- **`int get_col_width(size_t idx) const`**: Returns the current width of a specific column.
- **`void apply_sort()`**: Re-sorts the rows based on the current sort configuration.

### `GridRow<T>`
A row within a `Grid<T>`.
- **`void set_cell(size_t idx, std::shared_ptr<Element> el)`**: Sets the DOM element for a specific column index.
- **`std::shared_ptr<Element> get_cell(size_t idx)`**: Retrieves the DOM element for a cell.
- **`void update_data(const T& data)`**: Updates the underlying data object associated with the row.
- **`const T& get_data() const`**: Retrieves the row's data object.
- **`void set_on_double_click(std::function<void(GridRow<T>*, const std::string&)> cb)`**: Sets the internal double-click handler.
