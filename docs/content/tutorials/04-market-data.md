# Tutorial 4: The Market Data Example

> [!WARNING]
> **High Bandwidth Usage:** This example streams simulated high-frequency market data from the server to demonstrate the extreme performance and efficiency of NetzWirbel's lock-free conflation mechanism. It may consume significant network bandwidth while running.

NetzWirbel is well-suited for high-frequency updates, such as rendering live financial market data. The `MarketData` example demonstrates this by rendering a grid of tickers and updating them dynamically via a WebSocket stream.

## 1. Initializing the UI Grid

The example first sets up a `HTMLDivElement` container with CSS Grid to act as the table structure.

```cpp
g_grid_container = std::make_shared<HTMLDivElement>(g_ctx);
g_ctx->register_element(g_grid_container);
g_grid_container->set_attribute("style", "display: grid; grid-template-columns: 1fr 1fr 1fr 1fr 1fr 1fr 1.5fr; ...");
g_app->append_child(g_grid_container);
```

It then creates 65 simulated `TickerRow` objects, each containing an array of 7 cell `HTMLDivElement` instances corresponding to Symbol, Bid Size, Bid Price, Ask Price, Ask Size, Last Price, and Total Volume. All of these cells are appended to the grid container.

## 2. Setting up the Data Stream

The example registers a WebSocket listener to process incoming messages from a local server running on port 3000.

```cpp
NetzWirbel::Network::set_websocket_on_message([](const std::string& msg) {
    // 1. Parse string
    // 2. Locate corresponding TickerRow
    // 3. Update element text
});

NetzWirbel::Network::connect_websocket("ws://localhost:3000");
```

## 3. High-Frequency Updates & Lock-Free Conflation

When a message like `MD|SYM10|100.50|100.40|100.60|500|500|10000` is received, the C++ code locates the relevant row and updates its cells. Because market data can arrive faster than the browser can render frames (60Hz), NetzWirbel provides lock-free, O(1) amortized **conflation** methods. 

Instead of queueing thousands of redundant price updates into the ring buffer, the app uses `set_text_content_conflated`:

```cpp
row_ptr->get_cell(5)->set_text_content_conflated(parts[1]); // Update Last Price Lock-Free
```

To provide visual feedback (flashing green for an uptick, red for a downtick), it dynamically modifies the `className` using `set_class_conflated`:
```cpp
std::string color_class = (tr.last_px >= old_last) ? "md-cell md-up" : "md-cell md-down";
row_ptr->get_cell(5)->set_class_conflated(color_class);
```

If multiple updates arrive before the JS event loop can process them, the C++ bridge safely skips the old commands and simply overwrites an atomic pointer. When Javascript finally renders the frame, it reads the absolute freshest data directly from the shared memory.

## 4. Performance Statistics

At the bottom of the screen, the example appends a stats pane that displays the round-trip time (RTT) metrics collected by NetzWirbel's ping/pong mechanism, as well as the **Conflated Hits** counter (`ctx_->get_conflation_hits()`). The hit counter demonstrates how many thousands of redundant DOM updates were safely dropped by the lock-free conflation engine during data bursts.
