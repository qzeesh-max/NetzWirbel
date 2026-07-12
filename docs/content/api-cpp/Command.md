# NetzWirbel::Command & EventMsg

These structures define the memory layout of the structs passed over the Wasm ring buffers. 

## `CommandType` Enum
Defines the operation C++ is requesting Javascript to perform.
```cpp
enum class CommandType : uint32_t {
    CREATE_ELEMENT = 1,
    SET_ATTRIBUTE = 2,
    SET_PROPERTY_STRING = 3,
    SET_PROPERTY_BOOL = 4,
    SET_PROPERTY_NUMBER = 5,
    ADD_EVENT_LISTENER = 6,
    APPEND_CHILD = 7,
    SET_TEXT_CONTENT = 8,
    PING = 9
};
```

## `Command` Struct (C++ to JS)
Size: 32 bytes.
```cpp
struct Command {
    CommandType type;
    uint32_t target_id;
    uint32_t arg1; 
    uint32_t arg2; 
    uint32_t arg3; 
    uint32_t arg4; 
    double num_val;
};
```
Depending on the `type`, the `arg` fields encode C-string pointers (`uint32_t`), string lengths, or boolean values.

## `EventMsg` Struct (JS to C++)
Size: 64 bytes.
```cpp
struct EventMsg {
    EventType type;
    uint32_t target_id;
    uint32_t event_type_ptr; 
    uint32_t event_type_len;
    uint32_t prop_name_ptr;
    uint32_t prop_name_len;
    uint32_t str_val_ptr;
    uint32_t str_val_len;
    double num_val;
    bool bool_val;
    double client_x;
    double client_y;
};
```
This encapsulates a DOM event or property update from Javascript. The `ptr` fields point to strings allocated in the WebAssembly memory by JS.
