# Tutorial 4: The Market Data Example

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

## 3. High-Frequency Updates

When a message like `MD|SYM10|100.50|100.40|100.60|500|500|10000` is received, the C++ code locates the relevant row and issues batch updates.

```cpp
tr.cells[5]->set_text_content(parts[1]); // Update Last Price
```

To provide visual feedback (flashing green for an uptick, red for a downtick), it dynamically modifies the `style` attribute of the price cell:
```cpp
std::string color = (tr.last_px >= old_last) ? "#00ff00" : "#ff4444";
std::string cell_style = "... background-color: " + row_bg + "; color: " + color + "; transition: color 0.2s;";
tr.cells[5]->set_attribute("style", cell_style);
```

Because NetzWirbel writes these updates sequentially into a lock-free ring buffer, thousands of cell updates per second can be queued in C++ and flushed efficiently to Javascript, avoiding the severe performance degradation typical of heavy JS interop.

## 4. Performance Statistics

At the bottom of the screen, the example appends a stats pane that displays the round-trip time (RTT) metrics collected by NetzWirbel's ping/pong mechanism, demonstrating the low latency of the shared memory bridge.
