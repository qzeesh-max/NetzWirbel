/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

class RingBufferConsumer {
    constructor(wasmMemory, headerOffset, capacity, itemSize) {
        this.memory = wasmMemory;
        this.headerOffset = headerOffset;
        this.capacity = capacity;
        this.itemSize = itemSize;
        
        this.headArray = new Uint32Array(wasmMemory.buffer, headerOffset, 1);
        this.tailArray = new Uint32Array(wasmMemory.buffer, headerOffset + 4, 1);
        
        // RingBufferHeader is alignas(8) and has 5 * uint32_t (20 bytes) => rounded up to 24 bytes
        this.bufferStart = headerOffset + 24; 
    }
    
    isEmpty() {
        const head = Atomics.load(this.headArray, 0);
        const tail = Atomics.load(this.tailArray, 0);
        return head === tail;
    }
    
    pop() {
        if (this.isEmpty()) return null;
        
        const tail = Atomics.load(this.tailArray, 0);
        const offset = this.bufferStart + tail * this.itemSize;
        
        // Copy item data
        const itemBuffer = new ArrayBuffer(this.itemSize);
        const itemArray = new Uint8Array(itemBuffer);
        const srcArray = new Uint8Array(this.memory.buffer, offset, this.itemSize);
        itemArray.set(srcArray);
        
        const nextTail = (tail + 1) & (this.capacity - 1);
        Atomics.store(this.tailArray, 0, nextTail);
        
        return new DataView(itemBuffer);
    }
}

class RingBufferProducer {
    constructor(wasmMemory, headerOffset, capacity, itemSize) {
        this.memory = wasmMemory;
        this.headerOffset = headerOffset;
        this.capacity = capacity;
        this.itemSize = itemSize;
        
        this.headArray = new Int32Array(wasmMemory.buffer, headerOffset, 1);
        this.tailArray = new Int32Array(wasmMemory.buffer, headerOffset + 4, 1);
        this.bufferStart = headerOffset + 24; 
    }
    
    isFull() {
        const head = Atomics.load(this.headArray, 0);
        const tail = Atomics.load(this.tailArray, 0);
        return ((head + 1) & (this.capacity - 1)) === tail;
    }
    
    push(dataView) {
        if (this.isFull()) return false;
        
        const head = Atomics.load(this.headArray, 0);
        const offset = this.bufferStart + head * this.itemSize;
        
        const destArray = new Uint8Array(this.memory.buffer, offset, this.itemSize);
        const srcArray = new Uint8Array(dataView.buffer);
        destArray.set(srcArray);
        
        const nextHead = (head + 1) & (this.capacity - 1);
        Atomics.store(this.headArray, 0, nextHead);
        
        // Wake up consumer
        Atomics.notify(this.headArray, 0, 1);
        return true;
    }
}

class NetzWirbelBridge {
    constructor(wasmMemory, cppToJsOffset, jsToCppOffset, capacity, mallocFn, freeFn) {
        this.memory = wasmMemory;
        this.malloc = mallocFn;
        this.free = freeFn;
        
        // Command (C++ -> JS): 32 bytes (size of Command struct)
        this.cppToJs = new RingBufferConsumer(wasmMemory, cppToJsOffset, capacity, 32);
        
        // EventMsg (JS -> C++): 64 bytes (size of EventMsg struct)
        this.jsToCpp = new RingBufferProducer(wasmMemory, jsToCppOffset, capacity, 64);
        
        this.elements = new Map();
        this.stringRegistry = new Map();
        this.eventBacklog = [];

        // Apply global styles for the UI framework
        if (!document.getElementById("netzwirbel-global-style")) {
            const style = document.createElement("style");
            style.id = "netzwirbel-global-style";
            style.innerHTML = `
                body {
                    user-select: none;
                    -webkit-user-select: none;
                    font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
                }
                input, textarea, [contenteditable] {
                    user-select: text;
                    -webkit-user-select: text;
                }
            `;
            document.head.appendChild(style);
        }
        
        this.CMD = {
            CREATE_ELEMENT: 1,
            SET_ATTRIBUTE: 2,
            SET_PROPERTY_STRING: 3,
            SET_PROPERTY_BOOL: 4,
            SET_PROPERTY_NUMBER: 5,
            ADD_EVENT_LISTENER: 6,
            APPEND_CHILD: 7,
            SET_TEXT_CONTENT: 8,
            PING: 9,
            BIND_ELEMENT: 10,
            REGISTER_STRING: 11,
            FOCUS: 12,
            SELECT: 13,
            SET_NUMERIC_ONLY: 14,
            SET_STYLES: 15
        };
        
        this.EVENT = {
            EVENT: 1,
            PROPERTY_CHANGED_STRING: 2,
            PROPERTY_CHANGED_BOOL: 3,
            PROPERTY_CHANGED_NUMBER: 4,
            PONG: 5
        };
    }

    readString(ptrOrId, len) {
        if (len === 0xFFFFFFFF) {
            return this.stringRegistry.get(ptrOrId);
        }
        const bytes = new Uint8Array(this.memory.buffer, ptrOrId, len);
        const copy = new Uint8Array(bytes); // copy into non-shared buffer
        const str = new TextDecoder().decode(copy);
        if (this.free) {
            this.free(ptrOrId); // Fix memory leak
        }
        return str;
    }
    
    writeString(str) {
        const bytes = new TextEncoder().encode(str);
        const ptr = this.malloc(bytes.length);
        const mem = new Uint8Array(this.memory.buffer, ptr, bytes.length);
        mem.set(bytes);
        return { ptr, len: bytes.length };
    }

    processCommands() {
        this.flushEventBacklog();
        while (!this.cppToJs.isEmpty()) {
            const dataView = this.cppToJs.pop();
            const type = dataView.getUint32(0, true);
            const targetId = dataView.getUint32(4, true);
            const arg1 = dataView.getUint32(8, true);
            const arg2 = dataView.getUint32(12, true);
            const arg3 = dataView.getUint32(16, true);
            const arg4 = dataView.getUint32(20, true);
            const numVal = dataView.getFloat64(24, true);

            this.handleCommand(type, targetId, arg1, arg2, arg3, arg4, numVal);
        }
    }

    handleCommand(type, targetId, arg1, arg2, arg3, arg4, numVal) {
        switch (type) {
            case this.CMD.REGISTER_STRING: {
                const str = this.readString(arg1, arg2);
                this.stringRegistry.set(targetId, str);
                break;
            }
            case this.CMD.CREATE_ELEMENT: {
                const tagName = this.readString(arg1, arg2);
                const el = document.createElement(tagName);
                el.dataset.netzwirbelId = targetId;
                this.elements.set(targetId, el);
                
                if (tagName === "input" || tagName === "textarea" || tagName === "select") {
                    el.addEventListener("change", (e) => this.sendPropertyChangeString(targetId, "value", el.value));
                    if(tagName !== "select") el.addEventListener("input", (e) => this.sendPropertyChangeString(targetId, "value", el.value));
                }
                break;
            }
            case this.CMD.SET_ATTRIBUTE: {
                const el = this.elements.get(targetId);
                if (el) {
                    const key = this.readString(arg1, arg2);
                    const val = this.readString(arg3, arg4);
                    el.setAttribute(key, val);
                }
                break;
            }
            case this.CMD.SET_PROPERTY_STRING: {
                const el = this.elements.get(targetId);
                if (el) {
                    const key = this.readString(arg1, arg2);
                    const val = this.readString(arg3, arg4);
                    el[key] = val;
                }
                break;
            }
            case this.CMD.SET_PROPERTY_BOOL: {
                const el = this.elements.get(targetId);
                if (el) {
                    const key = this.readString(arg1, arg2);
                    el[key] = (arg3 !== 0);
                }
                break;
            }
            case this.CMD.SET_PROPERTY_NUMBER: {
                const el = this.elements.get(targetId);
                if (el) {
                    const key = this.readString(arg1, arg2);
                    el[key] = numVal;
                }
                break;
            }
            case this.CMD.SET_STYLES: {
                const el = this.elements.get(targetId);
                if (el) {
                    const cssStr = this.readString(arg1, arg2);
                    const pairs = cssStr.split(';');
                    for (let i = 0; i < pairs.length; ++i) {
                        const pair = pairs[i];
                        if (!pair) continue;
                        const idx = pair.indexOf(':');
                        if (idx > 0) {
                            const k = pair.substring(0, idx).trim();
                            const v = pair.substring(idx + 1).trim();
                            if (v) {
                                el.style.setProperty(k, v);
                            } else {
                                el.style.removeProperty(k);
                            }
                        } else {
                            const k = pair.trim();
                            if (k) el.style.removeProperty(k);
                        }
                    }
                }
                break;
            }
            case this.CMD.ADD_EVENT_LISTENER: {
                const el = this.elements.get(targetId);
                if (el) {
                    const eventType = this.readString(arg1, arg2);
                    el.addEventListener(eventType, (e) => this.sendEvent(targetId, e));
                }
                break;
            }
            case this.CMD.APPEND_CHILD: {
                const parent = this.elements.get(targetId);
                const child = this.elements.get(arg1);
                if (parent) {
                    if (child) parent.appendChild(child);
                } else if (targetId === 0) {
                    // targetId 0 could mean the document.body
                    if (child) document.body.appendChild(child);
                }
                break;
            }
            case this.CMD.SET_TEXT_CONTENT: {
                const el = this.elements.get(targetId);
                if (el) {
                    const text = this.readString(arg1, arg2);
                    el.textContent = text;
                }
                break;
            }
            case this.CMD.PING: {
                this.sendPong(numVal);
                break;
            }
            case this.CMD.BIND_ELEMENT: {
                const selector = this.readString(arg1, arg2);
                const el = document.querySelector(selector);
                if (el) {
                    el.dataset.netzwirbelId = targetId;
                    this.elements.set(targetId, el);
                    
                    const tagName = el.tagName.toLowerCase();
                    if (tagName === "input" || tagName === "textarea") {
                        el.addEventListener("input", (e) => this.sendPropertyChangeString(targetId, "value", el.value));
                    }
                } else {
                    console.error("NetzWirbel: BIND_ELEMENT failed to find selector:", selector);
                }
                break;
            }
            case this.CMD.FOCUS: {
                const el = this.elements.get(targetId);
                if (el) el.focus();
                break;
            }
            case this.CMD.SELECT: {
                const el = this.elements.get(targetId);
                if (el) el.select();
                break;
            }
            case this.CMD.SET_NUMERIC_ONLY: {
                const el = this.elements.get(targetId);
                if (el) {
                    const allowDecimal = (arg1 !== 0);
                    el.addEventListener("input", function(e) {
                        const original = this.value;
                        const filtered = allowDecimal ? original.replace(/[^0-9.]/g, '') : original.replace(/[^0-9]/g, '');
                        if (original !== filtered) {
                            this.value = filtered;
                        }
                    });
                }
                break;
            }
        }
    }

    pushEvent(view) {
        if (this.eventBacklog.length > 0) {
            this.eventBacklog.push(view);
            this.flushEventBacklog();
            return;
        }
        if (!this.jsToCpp.push(view)) {
            this.eventBacklog.push(view);
        }
    }

    flushEventBacklog() {
        while (this.eventBacklog.length > 0) {
            const view = this.eventBacklog[0];
            if (this.jsToCpp.push(view)) {
                this.eventBacklog.shift();
            } else {
                break;
            }
        }
    }

    sendEvent(targetId, e) {
        const typeStr = this.writeString(e.type);
        
        const data = new ArrayBuffer(64);
        const view = new DataView(data);
        view.setUint32(0, this.EVENT.EVENT, true); // type
        view.setUint32(4, targetId, true); // target_id
        view.setUint32(8, typeStr.ptr, true); // event_type_ptr
        view.setUint32(12, typeStr.len, true); // event_type_len
        
        if (e.clientX !== undefined) {
            view.setFloat64(48, e.clientX, true); // client_x
            view.setFloat64(56, e.clientY, true); // client_y
        }
        
        if (e.key !== undefined) {
            const keyStr = this.writeString(e.key);
            view.setUint32(24, keyStr.ptr, true); // reuse str_val_ptr for key
            view.setUint32(28, keyStr.len, true); // reuse str_val_len for key
        }
        
        this.pushEvent(view);
    }
    
    sendPropertyChangeString(targetId, propName, value) {
        const propStr = this.writeString(propName);
        const valStr = this.writeString(value);
        
        const data = new ArrayBuffer(64);
        const view = new DataView(data);
        view.setUint32(0, this.EVENT.PROPERTY_CHANGED_STRING, true);
        view.setUint32(4, targetId, true);
        view.setUint32(16, propStr.ptr, true);
        view.setUint32(20, propStr.len, true);
        view.setUint32(24, valStr.ptr, true);
        view.setUint32(28, valStr.len, true);
        
        this.pushEvent(view);
    }
    
    sendPong(timestamp) {
        const data = new ArrayBuffer(64);
        const view = new DataView(data);
        view.setUint32(0, this.EVENT.PONG, true);
        view.setFloat64(32, timestamp, true);
        this.pushEvent(view);
    }
}

// Global polling loop
function startNetzWirbelLoop(bridge) {
    function loop() {
        bridge.processCommands();
        requestAnimationFrame(loop);
    }
    loop();
}
