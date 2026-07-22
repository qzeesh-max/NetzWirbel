const net = require('net');
const client = new net.Socket();

client.connect(6001, '127.0.0.1', () => {
    console.log('Connected to OdysseyBackend');
    // Wait for initial commands (CREATE_ELEMENT etc)
    setTimeout(() => {
        // Send a click event on an element. But we need an ID.
        // Let's just send mousedown on ID 1, 2, 3...
        for(let id=1; id<=20; id++) {
            const buf = Buffer.alloc(64);
            buf.writeUInt32LE(1, 0); // EventType::EVENT
            buf.writeUInt32LE(id, 4); // target_id
            
            // We need a valid pointer for "mousedown". This is hard because we don't know the pointer.
            // Wait, we can't easily construct a valid EventMsg from Node if it requires WASM memory pointers!
        }
    }, 1000);
});
