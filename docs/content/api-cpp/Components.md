# NetzWirbel UI Components

NetzWirbel provides several high-level UI components built on top of the base DOM elements. These components provide enhanced functionality, styling, and event handling out of the box.

## `Button`
A styled button component inheriting from `HTMLButtonElement`.
- **Constructor**: `Button(Context* ctx)`
- **`void set_label(const std::string& label)`**: Updates the button text.
- **`void set_icon(const std::string& icon)`**: Sets an icon on the button.
- **`void set_primary(bool primary)`**: Toggles primary styling.
- **`void set_on_click(std::function<void()> cb)`**: Sets the click event handler.

## `TextBox`
A specialized text input component inheriting from `HTMLInputElement`.
- **Constructor**: `TextBox(Context* ctx)`
- **`void set_text(const std::string& text)`**: Sets the current text value.
- **`std::string get_text() const`**: Retrieves the current text.
- **`void set_placeholder(const std::string& placeholder)`**: Sets the placeholder text.
- **`void set_password(bool is_password)`**: Configures the input to act as a password field.
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
- **`std::string get_text() const`**: Retrieves the current rich text content.
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
- **Constructor**: `Window(Context* ctx, const std::string& id, const std::string& title, int x, int y, int width, int height)`
- **`void set_title(const std::string& title)`**: Sets the window header title.
- **`std::string get_window_id() const`**: Retrieves the window ID.
- **`std::shared_ptr<Element> get_content_container() const`**: Returns the container element where window contents should be appended.
- **`std::shared_ptr<Element> get_header() const`**: Returns the header container element.
- **`void set_bounds(int x, int y, int width, int height)`**: Sets the dimensions and position of the window.
- **`void set_visible(bool visible)`**: Shows or hides the window.
- **`bool is_visible() const`**: Returns whether the window is currently visible.
- **`void set_minimized(bool minimized)`**: Minimizes or restores the window.
- **`bool is_minimized() const`**: Returns whether the window is currently minimized.
- **`void set_maximized(bool maximized)`**: Maximizes or restores the window.
- **`bool is_maximized() const`**: Returns whether the window is currently maximized.
- **`void set_modal(bool modal)`**: Configures the window to act as a modal dialog, elevating its z-index significantly.
- **`void bring_to_front()`**: Brings the window to the very front of the screen.
- **Control Buttons**:
  - `set_minimize_button_enabled(bool)` / `set_minimize_button_hidden(bool)`
  - `set_maximize_button_enabled(bool)` / `set_maximize_button_hidden(bool)`
  - `set_close_button_enabled(bool)` / `set_close_button_hidden(bool)`
- **Callbacks**:
  - `void set_on_move(std::function<void(int, int)> cb)`
  - `void set_on_resize(std::function<void(int, int)> cb)`
  - `void set_on_minimize(std::function<void(bool)> cb)`
  - `void set_on_maximize(std::function<void(bool)> cb)`
  - `void set_on_close(std::function<void()> cb)`
- **Getters**: `get_x()`, `get_y()`, `get_width()`, `get_height()`, `get_z_index()`

### `WindowManager`
A global utility class for managing floating windows and global event listeners.
- **`static void initialize_global_listeners(Context* ctx)`**: Hooks into global `mousemove` and `mouseup` events. This is automatically called when the first `Window` is created, but **must be called manually** during initialization if you are using draggable components (like `Grid` column reordering) in an application without `Window` containers.

## `Grid<T, ColEnum>`
A highly functional, sortable, and resizable data grid component. `ColEnum` defaults to `int`, but can be set to an `enum class` to strictly type your grid columns.
- **Constructor**: `Grid(Context* ctx)`
- **`void add_column(const std::string& name, int default_width)`**: Defines a new column.
- **`void set_on_render_row(std::function<void(std::shared_ptr<GridRow<T, ColEnum>>, const T&)> cb)`**: Sets the callback to render row cells based on data of type `T`.
- **`std::shared_ptr<GridRow<T, ColEnum>> add_row(const T& data)`**: Adds a new row with the specified data.
- **`void clear_rows()`**: Removes all rows from the grid.
- **`void set_on_cell_double_click(std::function<void(std::shared_ptr<GridRow<T, ColEnum>>, ColEnum)> cb)`**: Triggered when a cell is double-clicked. The `ColEnum` is passed to uniquely identify which column was clicked.
- **`int get_col_width(size_t idx) const`**: Returns the current width of a specific column.
- **Column Reordering & Freezing**:
  - **`void set_frozen_columns(int count)`**: Locks the first `count` columns to the left side. Frozen columns cannot be dragged, and other columns cannot be dragged into the frozen area.
  - **`std::vector<int> get_column_orders() const`**: Retrieves the current visual order of columns (useful for layout persistence).
  - **`void set_column_orders(const std::vector<int>& orders)`**: Programmatically restores the visual order of columns.
  - **`void set_on_column_order_changed(std::function<void()> cb)`**: Triggered when the user drag-and-drops a column header to reorder it. Note: Requires `WindowManager::initialize_global_listeners()` to be active.
- **Sorting Configuration**:
  - `int sort_col_idx_`: The index of the column currently being sorted.
  - `SortDirection sort_dir_`: Direction of the sort (`SORT_NONE`, `SORT_ASC`, `SORT_DESC`).
  - `std::function<bool(const T& a, const T& b, ColEnum col_idx)> sort_cmp_`: Callback that performs the comparison between two rows for sorting.
- **`void apply_sort()`**: Re-sorts the rows visually based on the current sort configuration. Call this periodically if your underlying data updates rapidly.

### `GridRow<T, ColEnum>`
A row within a `Grid<T, ColEnum>`.
- **`void add_cell(const std::string& text, int width_px, std::optional<ColEnum> col_name = std::nullopt)`**: Adds a text cell with a defined width to the row. You should pass your strongly-typed `ColEnum` to identify the cell.
- **`std::shared_ptr<Element> get_cell(size_t index)`**: Retrieves the DOM element for a cell.
- **`void update_data(const T& data)`**: Updates the underlying data object associated with the row. **Must be called** when the data changes for sorting to correctly reflect the new values.
- **`const T& get_data() const`**: Retrieves the row's data object.
- **`void set_on_double_click(std::function<void(GridRow<T, ColEnum>*, ColEnum)> cb)`**: Sets the internal double-click handler.

## Menus
A powerful suite of components for creating traditional desktop-like menu systems.

### `MenuBar`
The top-level container for application menus.
- **Constructor**: `MenuBar(Context* ctx)`
- **`void add_menu(const std::string& title, std::shared_ptr<Menu> menu)`**: Attaches a dropdown menu to the menu bar.

### `Menu`
A dropdown menu or submenu.
- **Constructor**: `Menu(Context* ctx)`
- **`void add_item(std::shared_ptr<MenuItem> item)`**: Appends an item to the menu.
- **`void add_submenu(std::shared_ptr<Menu> submenu)`**: Registers a nested submenu.
- **`void show_at(int x, int y)`**: Manually positions and displays the menu (often used for context menus).

### `ContextMenu`
A specialized menu designed to appear at the mouse cursor location when a user right-clicks.
- **Constructor**: `ContextMenu(Context* ctx)`
- **`void attach_to(std::shared_ptr<Element> target)`**: Attaches the context menu to the specified DOM element to automatically handle right-click events.

### `MenuItem`
An individual action item inside a `Menu`.
- **Constructor**: `MenuItem(Context* ctx, const std::string& text, const std::string& icon = "", const std::string& accelerator = "", bool is_breaker = false)`
- **`void set_on_click(std::function<void()> cb)`**: Sets the click event handler.
- **`void set_submenu(std::shared_ptr<Menu> submenu)`**: Attaches a submenu to this item, which opens on hover.

## `Toolbar`
A horizontal or vertical container for quick-access action buttons.
- **Constructor**: `Toolbar(Context* ctx)`
- **`static std::shared_ptr<Toolbar> createFromTemplate(Context* ctx, const ToolbarConfig& config)`**: Factory method to generate a populated toolbar quickly.
- **`void add_button(std::shared_ptr<ToolbarButton> button)`**: Appends a button.

### `ToolbarButton`
A compact button inside a `Toolbar`.
- **Constructor**: `ToolbarButton(Context* ctx, const std::string& icon, const std::string& tooltip)`

## `StatusBar`
A bottom-aligned container for displaying application state and metrics.
- **Constructor**: `StatusBar(Context* ctx)`
- **`void add_panel(std::shared_ptr<StatusBarPanel> panel)`**: Appends a panel.

### `StatusBarPanel`
An individual section inside the `StatusBar`.
- **Constructor**: `StatusBarPanel(Context* ctx, BevelStyle style = BevelStyle::None)`
- **`void set_bevel_style(BevelStyle style)`**: Alters the visual inset/outset appearance.

## `TabContainer`
A container for managing multiple tabbed views.
- **Constructor**: `TabContainer(Context* ctx)`
- **`void add_tab(const std::string& title, std::shared_ptr<Element> content)`**: Creates a new tab linked to the provided content element.
- **`void select_tab(size_t index)`**: Programmatically switches to a tab.
- **`void set_on_state_change(std::function<void(size_t)> cb)`**: Handler triggered when the active tab changes.
- **`bool handle_accelerator(const std::string& key, uint8_t modifiers)`**: Intercepts `Alt+Number` globally to switch tabs.
