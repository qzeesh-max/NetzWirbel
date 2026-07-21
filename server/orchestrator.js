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

const express = require('express');
const path = require('path');
const WebSocket = require('ws');
const fs = require('fs');
const net = require('net');

const app = express();
const port = 3000;

// Add COOP and COEP headers for SharedArrayBuffer support
app.use((req, res, next) => {
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    // Ensure .wasm files have correct MIME type
    if (req.url.endsWith('.wasm')) {
        res.setHeader('Content-Type', 'application/wasm');
    }
    next();
});

// Serve the static files from the project root
app.use(express.static(path.join(__dirname, '..')));

// List of examples that are native-only backends (not WebAssembly)
const NATIVE_BACKENDS = ['OrderMatchBackend', 'OdysseyBackend'];

// Launcher UI
app.get('/', (req, res) => {
    const examplesDir = path.join(__dirname, '..', 'examples');
    let examples = [];
    if (fs.existsSync(examplesDir)) {
        examples = fs.readdirSync(examplesDir).filter(f => fs.statSync(path.join(examplesDir, f)).isDirectory());
    }
    
    const html = `
    <!DOCTYPE html>
    <html>
    <head>
        <title>NetzWirbel Examples Launcher</title>
        <style>
            body { font-family: sans-serif; padding: 2rem; background: #121212; color: #fff; }
            h1 { color: #00e5ff; }
            ul { list-style: none; padding: 0; }
            li { margin-bottom: 1rem; }
            a { display: block; padding: 1rem; background: #1e1e1e; border: 1px solid #333; color: #fff; text-decoration: none; border-radius: 8px; transition: 0.2s; }
            a:hover { background: #333; border-color: #00e5ff; }
            .native-badge { display: inline-block; padding: 2px 8px; background: #ff9800; color: #000; border-radius: 4px; font-size: 0.8em; margin-left: 8px; }
        </style>
    </head>
    <body>
        <div style="display: flex; align-items: center; gap: 1rem; margin-bottom: 1rem;">
            <img src="/logo.png" style="height: 48px;" alt="NetzWirbel Logo">
            <h1 style="margin: 0;">NetzWirbel Examples Launcher</h1>
        </div>
        <ul>
            ${examples.map(ex => {
                const isNative = NATIVE_BACKENDS.includes(ex);
                const hasHtml = fs.existsSync(path.join(__dirname, '..', 'build', 'examples', ex, 'index.html'));
                const hasWasm = fs.existsSync(path.join(__dirname, '..', 'build', 'examples', ex, ex + '.js'));
                if (isNative) {
                    return `<li><a href="/run/${ex}">${ex}<span class="native-badge">Native Backend</span></a></li>`;
                }
                return `<li><a href="${hasHtml ? `/build/examples/${ex}/index.html` : `/run/${ex}`}">${ex}</a></li>`;
            }).join('')}
            ${examples.length === 0 ? '<li>No examples built yet. Run cmake and make.</li>' : ''}
        </ul>
    </body>
    </html>
    `;
    res.send(html);
});

// Dynamic Runner for examples
app.get('/run/:example', (req, res) => {
    const ex = req.params.example;
    
    // Handle native-only backends with a info page
    if (NATIVE_BACKENDS.includes(ex)) {
        const html = `
        <!DOCTYPE html>
        <html>
        <head>
            <title>NetzWirbel - ${ex}</title>
            <style>
                body { font-family: sans-serif; padding: 2rem; background: #121212; color: #fff; }
                h1 { color: #00e5ff; }
                .info { background: #1e1e1e; border: 1px solid #333; padding: 1.5rem; border-radius: 8px; margin: 1rem 0; }
                code { background: #2a2a2a; padding: 0.2rem 0.5rem; border-radius: 4px; color: #4fc3f7; }
                pre { background: #1a1a1a; padding: 1rem; border-radius: 4px; overflow-x: auto; color: #ccc; }
                a { color: #00e5ff; }
            </style>
        </head>
        <body>
            <div style="display: flex; align-items: center; gap: 1rem; margin-bottom: 1rem;">
                <img src="/logo.png" style="height: 48px;" alt="NetzWirbel Logo">
                <h1 style="margin: 0;">${ex} <span style="font-size: 0.6em; color: #ff9800;">(Native Backend)</span></h1>
            </div>
            <div class="info">
                <p>This is a native backend that runs as a system process, not in the browser.</p>
                <h3>To start (Linux/Mac):</h3>
                <pre>./examples/${ex}/build-native/${ex} examples/${ex}/ordermatch.cfg</pre>
                <h3>To build (Linux/Mac):</h3>
                <pre>cd examples/${ex}/build-native && cmake .. && make</pre>
                <h3>To start (Windows):</h3>
                <pre>.\\build_${ex.toLowerCase().replace('backend', '')}\\${ex}.exe examples\\${ex}\\ordermatch.cfg</pre>
                <h3>To build (Windows):</h3>
                <pre>.\\build_${ex.toLowerCase().replace('backend', '')}.cmd</pre>
                <p>The BanzaiExchange frontend connects to this backend through the orchestrator's proxy on WebSocket port 3001.</p>
                <p><a href="/">← Back to Examples</a></p>
            </div>
        </body>
        </html>
        `;
        return res.send(html);
    }
    
    const htmlPath = path.join(__dirname, '..', 'build', 'examples', ex, 'index.html');
    if (fs.existsSync(htmlPath)) {
        return res.redirect(`/build/examples/${ex}/index.html`);
    }

    const html = `
    <!DOCTYPE html>
    <html>
    <head>
        <title>NetzWirbel - ${ex}</title>
    </head>
    <body>
        <h1>${ex} Example</h1>
        <script src="/js/netzwirbel_bridge.js"></script>
        <script>
            var Module = {
                onRuntimeInitialized: function() {
                    const capacity = 8192; // Increased to 8192 to allow large initialization bursts
                    
                    const cmdSize = _netzwirbel_get_command_size ? _netzwirbel_get_command_size() : 32;
                    const evSize = _netzwirbel_get_event_msg_size ? _netzwirbel_get_event_msg_size() : 64;
                    const headerSize = _netzwirbel_get_ring_buffer_header_size ? _netzwirbel_get_ring_buffer_header_size() : 384;
                    
                    const cppToJsPtr = _malloc(capacity * cmdSize + headerSize);
                    const jsToCppPtr = _malloc(capacity * evSize + headerSize);
                    
                    _netzwirbel_init(cppToJsPtr, jsToCppPtr, capacity);
                    
                    const layout = {
                        cmdSize: cmdSize,
                        evSize: evSize,
                        headerSize: headerSize,
                        headOffset: (typeof _netzwirbel_get_ring_buffer_head_offset === "function") ? _netzwirbel_get_ring_buffer_head_offset() : 0,
                        tailOffset: (typeof _netzwirbel_get_ring_buffer_tail_offset === "function") ? _netzwirbel_get_ring_buffer_tail_offset() : 128
                    };
                    
                    const bridge = new NetzWirbelBridge(Module.wasmMemory, cppToJsPtr, jsToCppPtr, capacity, _malloc, _free, layout);
                    
                    function loop(time) {
                        bridge.processCommands();
                        
                        // Give Emscripten SDL/GL the canvas if it was just dynamically created
                        if (!Module.canvas) {
                            var canvasEl = document.getElementById('canvas');
                            if (canvasEl) {
                                Module.canvas = canvasEl;
                            }
                        }
                        
                        _netzwirbel_tick(time);
                        requestAnimationFrame(loop);
                    }
                    requestAnimationFrame(loop);
                }
            };
        </script>
        <script src="/build/examples/${ex}/${ex}.js"></script>
    </body>
    </html>
    `;
    res.send(html);
});

// Proxy for cross-origin images to bypass COEP restrictions
const https = require('https');
app.get('/proxy-image', (req, res) => {
    const imageUrl = req.query.url;
    if (!imageUrl) return res.status(400).send('Missing url parameter');
    
    try {
        const parsedUrl = new URL(imageUrl);
        const allowedDomains = ['deckofcardsapi.com'];
        if (!allowedDomains.includes(parsedUrl.hostname)) {
            return res.status(403).send('Forbidden: Domain not allowed');
        }
    } catch (e) {
        return res.status(400).send('Invalid URL');
    }
    
    https.get(imageUrl, (response) => {
        res.setHeader('Cross-Origin-Resource-Policy', 'cross-origin');
        res.setHeader('Access-Control-Allow-Origin', '*');
        res.setHeader('Content-Type', response.headers['content-type'] || 'image/png');
        response.pipe(res);
    }).on('error', (err) => {
        res.status(500).send('Proxy error: ' + err.message);
    });
});

// Start HTTP server
const server = app.listen(port, () => {
    console.log(`NetzWirbel Orchestrator running at http://localhost:${port}`);
    console.log(`Ensure you access via localhost or HTTPS for SharedArrayBuffer to work!`);
});

// Start WebSocket server on the same port
const wss = new WebSocket.Server({ server });

let marketDataInterval = null;
let tickers = [];

// Initialize tickers state
for (let i = 0; i < 65; ++i) {
    let last_px = 100.0 + (Math.random() * 1000) / 10.0;
    tickers.push({
        symbol: "SYM" + i,
        last_px: last_px,
        bid_px: last_px - 0.05,
        ask_px: last_px + 0.05,
        bid_size: Math.floor(Math.random() * 10 + 1) * 100,
        ask_size: Math.floor(Math.random() * 10 + 1) * 100,
        total_vol: 0
    });
}

function startMarketDataSimulation() {
    if (marketDataInterval) return;
    marketDataInterval = setInterval(() => {
        if (wss.clients.size === 0) return;

        let updates = [];
        for (let i = 0; i < tickers.length; ++i) {
            if (Math.random() < 0.1) { // 10% chance to update each tick
                let t = tickers[i];
                t.last_px += (Math.random() * 100 - 50) / 100.0;
                t.bid_px = t.last_px - 0.05;
                t.ask_px = t.last_px + 0.05;
                t.bid_size = Math.floor(Math.random() * 10 + 1) * 100;
                t.ask_size = Math.floor(Math.random() * 10 + 1) * 100;
                
                let trade_sz = Math.floor(Math.random() * 100 + 1) * 100;
                t.total_vol += trade_sz;

                updates.push(`MD|${t.symbol}|${t.last_px.toFixed(2)}|${t.bid_px.toFixed(2)}|${t.ask_px.toFixed(2)}|${t.bid_size}|${t.ask_size}|${t.total_vol}`);
            }
        }

        if (updates.length > 0) {
            const payload = updates.join('\n');
            wss.clients.forEach(client => {
                if (client.readyState === WebSocket.OPEN) {
                    client.send(payload);
                }
            });
        }
    }, 50); // 50ms interval
}

startMarketDataSimulation();

wss.on('connection', (ws, req) => {
    if (req.url === '/fix') {
        console.log('Client connected to FIX proxy WebSocket.');
        const tcpSocket = new net.Socket();
        tcpSocket.connect(5001, '127.0.0.1', () => {
            console.log('Connected to OrderMatch TCP port 5001');
        });

        tcpSocket.on('data', (data) => {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(data);
            }
        });

        tcpSocket.on('close', () => {
            console.log('OrderMatch TCP connection closed');
            ws.close();
        });

        tcpSocket.on('error', (err) => {
            console.error('OrderMatch TCP error: ', err);
            ws.close();
        });

        ws.on('message', (message) => {
            if (tcpSocket.readyState === 'open' || tcpSocket.readyState === 'writeOnly') {
                tcpSocket.write(message);
            }
        });

        ws.on('close', () => {
            console.log('FIX Proxy WebSocket closed');
            tcpSocket.destroy();
        });
        return;
    }

    if (req.url === '/odyssey') {
        console.log('Client connected to Odyssey proxy WebSocket.');
        const tcpSocket = new net.Socket();
        
        tcpSocket.connect(6001, '127.0.0.1', () => {
            console.log('Connected to OdysseyBackend TCP port 6001');
        });

        tcpSocket.on('data', (data) => {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(data);
            }
        });

        tcpSocket.on('close', () => {
            console.log('OdysseyBackend TCP connection closed');
            ws.close();
        });

        tcpSocket.on('error', (err) => {
            console.error('OdysseyBackend TCP error: ', err);
            ws.close();
        });

        ws.on('message', (message, isBinary) => {
            if (!isBinary) {
                const msgStr = message.toString();
                const parts = msgStr.split('|');
                const cmd = parts[0];
                const username = parts[1];
                const layoutsFile = path.join(__dirname, 'layouts.json');
                let layouts = {};
                if (fs.existsSync(layoutsFile)) {
                    try {
                        layouts = JSON.parse(fs.readFileSync(layoutsFile, 'utf8'));
                    } catch (e) {
                        layouts = {};
                    }
                }

                if (cmd === 'GET_LAYOUT') {
                    const userLayout = layouts[username] || {};
                    ws.send(`LAYOUT|${username}|${JSON.stringify(userLayout)}`);
                } else if (cmd === 'SAVE_LAYOUT') {
                    const layoutJson = parts[2];
                    try {
                        layouts[username] = JSON.parse(layoutJson);
                        fs.writeFileSync(layoutsFile, JSON.stringify(layouts, null, 2), 'utf8');
                    } catch (e) {
                        console.error('Failed to parse/save layout json:', e);
                    }
                }
            } else {
                if (tcpSocket.readyState === 'open' || tcpSocket.readyState === 'writeOnly') {
                    tcpSocket.write(message);
                }
            }
        });

        ws.on('close', () => {
            console.log('Odyssey Proxy WebSocket closed');
            tcpSocket.destroy();
        });
        return;
    }

    console.log('Client connected to WebSocket.');
    
    // Send initial snapshot
    let snapshot = tickers.map(t => `MD|${t.symbol}|${t.last_px.toFixed(2)}|${t.bid_px.toFixed(2)}|${t.ask_px.toFixed(2)}|${t.bid_size}|${t.ask_size}|${t.total_vol}`);
    ws.send(snapshot.join('\n'));

    ws.on('message', (message) => {
        console.log(`Received over WS: ${message}`);
        // Echo back for testing
        ws.send(`Echo from server: ${message}`);
    });
});
