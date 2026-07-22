/*
 * Copyright (C) 2026 NetzWirbel Contributors
 */

#pragma once
#include "NetzWirbel/App.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include "NetzWirbel/DOM/Window.hpp"
#include "NetzWirbel/DOM/Menu.hpp"
#include "NetzWirbel/DOM/Toolbar.hpp"
#include "NetzWirbel/DOM/StatusBar.hpp"
#include "NetzWirbel/DOM/TabContainer.hpp"
#include <iostream>
#include <memory>
#include <emscripten.h>

namespace NetzWirbel {
namespace Examples {

class DesktopApp : public App {
public:
    void on_init(Context* ctx) override {
        ctx_ = ctx;
        g_app = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_app);
        g_app->set_attribute(ctx->strings.style, "width: 100vw; height: 100vh; overflow: hidden; background: linear-gradient(135deg, #fce4ec 0%, #e8eaf6 50%, #e0f2f1 100%); color: #333; font-family: monospace; position: fixed; top: 0; left: 0; margin: 0;");
        Command cmd;
        cmd.type = CommandType::APPEND_CHILD;
        cmd.target_id = 0;
        cmd.arg1 = g_app->get_id();
        ctx->send_command(cmd);

        // Attach window manager
        WindowManager::attach(g_app, ctx);
        WindowManager::set_margin_top(32); // MenuBar
        WindowManager::set_margin_bottom(32); // StatusBar

        // Bind global document for keyboard shortcuts using JS to synchronously preventDefault
        emscripten_run_script(
            "document.addEventListener('keydown', function(e) {"
            "   var isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;"
            "   if (isMac && e.metaKey && (e.key === 'x' || e.key === 'X')) {"
            "       e.preventDefault();"
            "       window.history.back();"
            "   } else if (!isMac && e.altKey && (e.key === 'x' || e.key === 'X')) {"
            "       e.preventDefault();"
            "       window.history.back();"
            "   }"
            "});"
        );

        create_menu_bar();
        create_toolbar();
        create_status_bar();
    }

    void on_tick(double time) override {
        ctx_->send_ping();
        
        static int last_sec = 0;
        int current_sec = static_cast<int>(time / 1000);
        if (current_sec != last_sec && g_clock_panel) {
            time_t now = std::time(nullptr);
            tm* ltm = localtime(&now);
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", ltm);
            g_clock_panel->set_text_content_conflated(buf);
            last_sec = current_sec;
        }
    }

private:
    void create_menu_bar() {
        g_menu_bar = std::make_shared<MenuBar>(ctx_);
        ctx_->register_element(g_menu_bar);

        // File Menu
        auto file_menu = std::make_shared<Menu>(ctx_);
        ctx_->register_element(file_menu);
        
        bool isMac = emscripten_run_script_int("navigator.platform.toUpperCase().indexOf('MAC') >= 0") != 0;
        std::string exit_accel = isMac ? "⌘X" : "Alt+X";
        auto exit_item = std::make_shared<MenuItem>(ctx_, "Exit", "❌", exit_accel);
        ctx_->register_element(exit_item);
        exit_item->set_on_click([]() { emscripten_run_script("window.history.back();"); });
        file_menu->add_item(exit_item);
        
        g_menu_bar->add_menu("File", file_menu);

        // Examples Menu
        auto examples_menu = std::make_shared<Menu>(ctx_);
        ctx_->register_element(examples_menu);
        
        auto md_item = std::make_shared<MenuItem>(ctx_, "MarketData", "📈");
        ctx_->register_element(md_item);
        md_item->set_on_click([this]() { spawn_iframe_game("MarketData", "/run/MarketData"); });
        examples_menu->add_item(md_item);
        
        g_menu_bar->add_menu("Examples", examples_menu);

        // Games Menu
        auto games_menu = std::make_shared<Menu>(ctx_);
        ctx_->register_element(games_menu);
        
        auto ttt_item = std::make_shared<MenuItem>(ctx_, "Tic Tac Toe", "⭕");
        ctx_->register_element(ttt_item);
        ttt_item->set_on_click([this]() { spawn_iframe_game("Tic Tac Toe", "data:text/html;base64,PCFET0NUWVBFIGh0bWw+CjxodG1sPgo8aGVhZD4KPHN0eWxlPgpib2R5IHsgYmFja2dyb3VuZDogIzIyMjsgY29sb3I6ICNmZmY7IG1hcmdpbjogMDsgZGlzcGxheTogZmxleDsganVzdGlmeS1jb250ZW50OiBjZW50ZXI7IGFsaWduLWl0ZW1zOiBjZW50ZXI7IGhlaWdodDogMTAwdmg7IGZvbnQtZmFtaWx5OiBzYW5zLXNlcmlmOyBmbGV4LWRpcmVjdGlvbjogY29sdW1uOyB9Ci5ib2FyZCB7IGRpc3BsYXk6IGdyaWQ7IGdyaWQtdGVtcGxhdGUtY29sdW1uczogcmVwZWF0KDMsIDEwMHB4KTsgZ3JpZC1nYXA6IDVweDsgYmFja2dyb3VuZDogIzU1NTsgcGFkZGluZzogNXB4OyB9Ci5jZWxsIHsgd2lkdGg6IDEwMHB4OyBoZWlnaHQ6IDEwMHB4OyBiYWNrZ3JvdW5kOiAjMzMzOyBkaXNwbGF5OiBmbGV4OyBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjsgYWxpZ24taXRlbXM6IGNlbnRlcjsgZm9udC1zaXplOiA2NHB4OyBjdXJzb3I6IHBvaW50ZXI7IH0KLmNlbGw6aG92ZXIgeyBiYWNrZ3JvdW5kOiAjNDQ0OyB9CiNzdGF0dXMgeyBtYXJnaW4tYm90dG9tOiAyMHB4OyBmb250LXNpemU6IDI0cHg7IH0KPC9zdHlsZT4KPC9oZWFkPgo8Ym9keT4KPGRpdiBpZD0ic3RhdHVzIj5QbGF5ZXIgWCdzIHR1cm48L2Rpdj4KPGRpdiBjbGFzcz0iYm9hcmQiIGlkPSJib2FyZCI+PC9kaXY+CjxzY3JpcHQ+CmxldCBib2FyZCA9IFsnJywnJywnJywnJywnJywnJywnJywnJywnJ107CmxldCB0dXJuID0gJ1gnOwpsZXQgYWN0aXZlID0gdHJ1ZTsKY29uc3Qgc3RhdHVzID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ3N0YXR1cycpOwpjb25zdCBiRGl2ID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ2JvYXJkJyk7CmZ1bmN0aW9uIGNoZWNrV2luKCkgewogIGNvbnN0IGxpbmVzID0gW1swLDEsMl0sWzMsNCw1XSxbNiw3LDhdLFswLDMsNl0sWzEsNCw3XSxbMiw1LDhdLFswLDQsOF0sWzIsNCw2XV07CiAgZm9yKGxldCBsIG9mIGxpbmVzKSB7CiAgICBpZihib2FyZFtsWzBdXSAmJiBib2FyZFtsWzBdXSA9PT0gYm9hcmRbbFsxXV0gJiYgYm9hcmRbbFswXV0gPT09IGJvYXJkW2xbMl1dKSByZXR1cm4gYm9hcmRbbFswXV07CiAgfQogIHJldHVybiBib2FyZC5pbmNsdWRlcygnJykgPyBudWxsIDogJ0RyYXcnOwp9CmZ1bmN0aW9uIHJlbmRlcigpIHsKICBiRGl2LmlubmVySFRNTCA9ICcnOwogIGJvYXJkLmZvckVhY2goKGMsIGkpID0+IHsKICAgIGxldCBlbCA9IGRvY3VtZW50LmNyZWF0ZUVsZW1lbnQoJ2RpdicpOwogICAgZWwuY2xhc3NOYW1lID0gJ2NlbGwnOwogICAgZWwuaW5uZXJUZXh0ID0gYzsKICAgIGVsLm9uY2xpY2sgPSAoKSA9PiB7CiAgICAgIGlmKCFhY3RpdmUgfHwgYm9hcmRbaV0pIHJldHVybjsKICAgICAgYm9hcmRbaV0gPSB0dXJuOwogICAgICBsZXQgd2luID0gY2hlY2tXaW4oKTsKICAgICAgaWYod2luKSB7CiAgICAgICAgc3RhdHVzLmlubmVyVGV4dCA9IHdpbiA9PT0gJ0RyYXcnID8gJ0RyYXchJyA6IHdpbiArICcgd2lucyEnOwogICAgICAgIGFjdGl2ZSA9IGZhbHNlOwogICAgICB9IGVsc2UgewogICAgICAgIHR1cm4gPSB0dXJuID09PSAnWCcgPyAnTycgOiAnWCc7CiAgICAgICAgc3RhdHVzLmlubmVyVGV4dCA9IGBQbGF5ZXIgJHt0dXJufSdzIHR1cm5gOwogICAgICB9CiAgICAgIHJlbmRlcigpOwogICAgfTsKICAgIGJEaXYuYXBwZW5kQ2hpbGQoZWwpOwogIH0pOwp9CnJlbmRlcigpOwo8L3NjcmlwdD4KPC9ib2R5Pgo8L2h0bWw+Cg=="); });
        games_menu->add_item(ttt_item);
        
        auto space_item = std::make_shared<MenuItem>(ctx_, "Space Invaders", "👾");
        ctx_->register_element(space_item);
        space_item->set_on_click([this]() { spawn_iframe_game("Space Invaders", "data:text/html;base64,PCFET0NUWVBFIGh0bWw+CjxodG1sPgo8aGVhZD4KPHN0eWxlPgpib2R5IHsgYmFja2dyb3VuZDogYmxhY2s7IGNvbG9yOiB3aGl0ZTsgbWFyZ2luOiAwOyBvdmVyZmxvdzogaGlkZGVuOyBkaXNwbGF5OiBmbGV4OyBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjsgYWxpZ24taXRlbXM6IGNlbnRlcjsgaGVpZ2h0OiAxMDB2aDsgZm9udC1mYW1pbHk6IG1vbm9zcGFjZTsgfQpjYW52YXMgeyBiYWNrZ3JvdW5kOiAjMTExOyBib3JkZXI6IDJweCBzb2xpZCAjMzMzOyB9Cjwvc3R5bGU+CjwvaGVhZD4KPGJvZHk+CjxjYW52YXMgaWQ9ImdhbWUiIHdpZHRoPSI0MDAiIGhlaWdodD0iNDAwIj48L2NhbnZhcz4KPHNjcmlwdD4KY29uc3QgY2FudmFzID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ2dhbWUnKTsKY29uc3QgY3R4ID0gY2FudmFzLmdldENvbnRleHQoJzJkJyk7CmxldCBwbGF5ZXIgPSB7eDogMTgwLCB5OiAzNTAsIHc6IDQwLCBoOiAyMH07CmxldCBidWxsZXRzID0gW107CmxldCBlbmVtaWVzID0gW107CmZvcihsZXQgaT0wOyBpPDU7IGkrKykgewogIGZvcihsZXQgaj0wOyBqPDEwOyBqKyspIHsKICAgIGVuZW1pZXMucHVzaCh7eDogMzAgKyBqKjMwLCB5OiAzMCArIGkqMzAsIHc6IDIwLCBoOiAyMCwgYWxpdmU6IHRydWV9KTsKICB9Cn0KbGV0IGtleXMgPSB7fTsKd2luZG93LmFkZEV2ZW50TGlzdGVuZXIoJ2tleWRvd24nLCBlID0+IGtleXNbZS5jb2RlXSA9IHRydWUpOwp3aW5kb3cuYWRkRXZlbnRMaXN0ZW5lcigna2V5dXAnLCBlID0+IGtleXNbZS5jb2RlXSA9IGZhbHNlKTsKCmZ1bmN0aW9uIHVwZGF0ZSgpIHsKICBpZiAoa2V5c1snQXJyb3dMZWZ0J10gJiYgcGxheWVyLnggPiAwKSBwbGF5ZXIueCAtPSA0OwogIGlmIChrZXlzWydBcnJvd1JpZ2h0J10gJiYgcGxheWVyLnggPCAzNjApIHBsYXllci54ICs9IDQ7CiAgaWYgKGtleXNbJ1NwYWNlJ10pIHsKICAgIGlmIChidWxsZXRzLmxlbmd0aCA9PT0gMCB8fCBidWxsZXRzW2J1bGxldHMubGVuZ3RoLTFdLnkgPCAzMDApIHsKICAgICAgYnVsbGV0cy5wdXNoKHt4OiBwbGF5ZXIueCArIDE4LCB5OiBwbGF5ZXIueSwgdzogNCwgaDogMTB9KTsKICAgIH0KICB9CiAgCiAgYnVsbGV0cy5mb3JFYWNoKGIgPT4gYi55IC09IDcpOwogIGJ1bGxldHMgPSBidWxsZXRzLmZpbHRlcihiID0+IGIueSA+IDApOwogIAogIGJ1bGxldHMuZm9yRWFjaChiID0+IHsKICAgIGVuZW1pZXMuZm9yRWFjaChlID0+IHsKICAgICAgaWYgKGUuYWxpdmUgJiYgYi54IDwgZS54ICsgZS53ICYmIGIueCArIGIudyA+IGUueCAmJiBiLnkgPCBlLnkgKyBlLmggJiYgYi55ICsgYi5oID4gZS55KSB7CiAgICAgICAgZS5hbGl2ZSA9IGZhbHNlOwogICAgICAgIGIueSA9IC0xMDA7CiAgICAgIH0KICAgIH0pOwogIH0pOwogIAogIGN0eC5jbGVhclJlY3QoMCwwLDQwMCw0MDApOwogIGN0eC5maWxsU3R5bGUgPSAnIzBmMCc7CiAgY3R4LmZpbGxSZWN0KHBsYXllci54LCBwbGF5ZXIueSwgcGxheWVyLncsIHBsYXllci5oKTsKICAKICBjdHguZmlsbFN0eWxlID0gJyNmZmYnOwogIGJ1bGxldHMuZm9yRWFjaChiID0+IGN0eC5maWxsUmVjdChiLngsIGIueSwgYi53LCBiLmgpKTsKICAKICBjdHguZmlsbFN0eWxlID0gJyNmMDAnOwogIGxldCBtb3ZpbmdSaWdodCA9IHRydWU7CiAgZW5lbWllcy5mb3JFYWNoKGUgPT4gewogICAgaWYgKGUuYWxpdmUpIGN0eC5maWxsUmVjdChlLngsIGUueSwgZS53LCBlLmgpOwogIH0pOwogIAogIHJlcXVlc3RBbmltYXRpb25GcmFtZSh1cGRhdGUpOwp9CnVwZGF0ZSgpOwo8L3NjcmlwdD4KPC9ib2R5Pgo8L2h0bWw+Cg=="); });
        games_menu->add_item(space_item);
        
        g_menu_bar->add_menu("Games", games_menu);

        g_app->append_child(g_menu_bar);
    }

    void create_toolbar() {
        ToolbarConfig config;
        config.is_vertical = false;
        config.buttons.push_back({"⭕", "Tic Tac Toe", [this]() { spawn_iframe_game("Tic Tac Toe", "data:text/html;base64,PCFET0NUWVBFIGh0bWw+CjxodG1sPgo8aGVhZD4KPHN0eWxlPgpib2R5IHsgYmFja2dyb3VuZDogIzIyMjsgY29sb3I6ICNmZmY7IG1hcmdpbjogMDsgZGlzcGxheTogZmxleDsganVzdGlmeS1jb250ZW50OiBjZW50ZXI7IGFsaWduLWl0ZW1zOiBjZW50ZXI7IGhlaWdodDogMTAwdmg7IGZvbnQtZmFtaWx5OiBzYW5zLXNlcmlmOyBmbGV4LWRpcmVjdGlvbjogY29sdW1uOyB9Ci5ib2FyZCB7IGRpc3BsYXk6IGdyaWQ7IGdyaWQtdGVtcGxhdGUtY29sdW1uczogcmVwZWF0KDMsIDEwMHB4KTsgZ3JpZC1nYXA6IDVweDsgYmFja2dyb3VuZDogIzU1NTsgcGFkZGluZzogNXB4OyB9Ci5jZWxsIHsgd2lkdGg6IDEwMHB4OyBoZWlnaHQ6IDEwMHB4OyBiYWNrZ3JvdW5kOiAjMzMzOyBkaXNwbGF5OiBmbGV4OyBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjsgYWxpZ24taXRlbXM6IGNlbnRlcjsgZm9udC1zaXplOiA2NHB4OyBjdXJzb3I6IHBvaW50ZXI7IH0KLmNlbGw6aG92ZXIgeyBiYWNrZ3JvdW5kOiAjNDQ0OyB9CiNzdGF0dXMgeyBtYXJnaW4tYm90dG9tOiAyMHB4OyBmb250LXNpemU6IDI0cHg7IH0KPC9zdHlsZT4KPC9oZWFkPgo8Ym9keT4KPGRpdiBpZD0ic3RhdHVzIj5QbGF5ZXIgWCdzIHR1cm48L2Rpdj4KPGRpdiBjbGFzcz0iYm9hcmQiIGlkPSJib2FyZCI+PC9kaXY+CjxzY3JpcHQ+CmxldCBib2FyZCA9IFsnJywnJywnJywnJywnJywnJywnJywnJywnJ107CmxldCB0dXJuID0gJ1gnOwpsZXQgYWN0aXZlID0gdHJ1ZTsKY29uc3Qgc3RhdHVzID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ3N0YXR1cycpOwpjb25zdCBiRGl2ID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ2JvYXJkJyk7CmZ1bmN0aW9uIGNoZWNrV2luKCkgewogIGNvbnN0IGxpbmVzID0gW1swLDEsMl0sWzMsNCw1XSxbNiw3LDhdLFswLDMsNl0sWzEsNCw3XSxbMiw1LDhdLFswLDQsOF0sWzIsNCw2XV07CiAgZm9yKGxldCBsIG9mIGxpbmVzKSB7CiAgICBpZihib2FyZFtsWzBdXSAmJiBib2FyZFtsWzBdXSA9PT0gYm9hcmRbbFsxXV0gJiYgYm9hcmRbbFswXV0gPT09IGJvYXJkW2xbMl1dKSByZXR1cm4gYm9hcmRbbFswXV07CiAgfQogIHJldHVybiBib2FyZC5pbmNsdWRlcygnJykgPyBudWxsIDogJ0RyYXcnOwp9CmZ1bmN0aW9uIHJlbmRlcigpIHsKICBiRGl2LmlubmVySFRNTCA9ICcnOwogIGJvYXJkLmZvckVhY2goKGMsIGkpID0+IHsKICAgIGxldCBlbCA9IGRvY3VtZW50LmNyZWF0ZUVsZW1lbnQoJ2RpdicpOwogICAgZWwuY2xhc3NOYW1lID0gJ2NlbGwnOwogICAgZWwuaW5uZXJUZXh0ID0gYzsKICAgIGVsLm9uY2xpY2sgPSAoKSA9PiB7CiAgICAgIGlmKCFhY3RpdmUgfHwgYm9hcmRbaV0pIHJldHVybjsKICAgICAgYm9hcmRbaV0gPSB0dXJuOwogICAgICBsZXQgd2luID0gY2hlY2tXaW4oKTsKICAgICAgaWYod2luKSB7CiAgICAgICAgc3RhdHVzLmlubmVyVGV4dCA9IHdpbiA9PT0gJ0RyYXcnID8gJ0RyYXchJyA6IHdpbiArICcgd2lucyEnOwogICAgICAgIGFjdGl2ZSA9IGZhbHNlOwogICAgICB9IGVsc2UgewogICAgICAgIHR1cm4gPSB0dXJuID09PSAnWCcgPyAnTycgOiAnWCc7CiAgICAgICAgc3RhdHVzLmlubmVyVGV4dCA9IGBQbGF5ZXIgJHt0dXJufSdzIHR1cm5gOwogICAgICB9CiAgICAgIHJlbmRlcigpOwogICAgfTsKICAgIGJEaXYuYXBwZW5kQ2hpbGQoZWwpOwogIH0pOwp9CnJlbmRlcigpOwo8L3NjcmlwdD4KPC9ib2R5Pgo8L2h0bWw+Cg=="); }});
        config.buttons.push_back({"👾", "Space Invaders", [this]() { spawn_iframe_game("Space Invaders", "data:text/html;base64,PCFET0NUWVBFIGh0bWw+CjxodG1sPgo8aGVhZD4KPHN0eWxlPgpib2R5IHsgYmFja2dyb3VuZDogYmxhY2s7IGNvbG9yOiB3aGl0ZTsgbWFyZ2luOiAwOyBvdmVyZmxvdzogaGlkZGVuOyBkaXNwbGF5OiBmbGV4OyBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjsgYWxpZ24taXRlbXM6IGNlbnRlcjsgaGVpZ2h0OiAxMDB2aDsgZm9udC1mYW1pbHk6IG1vbm9zcGFjZTsgfQpjYW52YXMgeyBiYWNrZ3JvdW5kOiAjMTExOyBib3JkZXI6IDJweCBzb2xpZCAjMzMzOyB9Cjwvc3R5bGU+CjwvaGVhZD4KPGJvZHk+CjxjYW52YXMgaWQ9ImdhbWUiIHdpZHRoPSI0MDAiIGhlaWdodD0iNDAwIj48L2NhbnZhcz4KPHNjcmlwdD4KY29uc3QgY2FudmFzID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ2dhbWUnKTsKY29uc3QgY3R4ID0gY2FudmFzLmdldENvbnRleHQoJzJkJyk7CmxldCBwbGF5ZXIgPSB7eDogMTgwLCB5OiAzNTAsIHc6IDQwLCBoOiAyMH07CmxldCBidWxsZXRzID0gW107CmxldCBlbmVtaWVzID0gW107CmZvcihsZXQgaT0wOyBpPDU7IGkrKykgewogIGZvcihsZXQgaj0wOyBqPDEwOyBqKyspIHsKICAgIGVuZW1pZXMucHVzaCh7eDogMzAgKyBqKjMwLCB5OiAzMCArIGkqMzAsIHc6IDIwLCBoOiAyMCwgYWxpdmU6IHRydWV9KTsKICB9Cn0KbGV0IGtleXMgPSB7fTsKd2luZG93LmFkZEV2ZW50TGlzdGVuZXIoJ2tleWRvd24nLCBlID0+IGtleXNbZS5jb2RlXSA9IHRydWUpOwp3aW5kb3cuYWRkRXZlbnRMaXN0ZW5lcigna2V5dXAnLCBlID0+IGtleXNbZS5jb2RlXSA9IGZhbHNlKTsKCmZ1bmN0aW9uIHVwZGF0ZSgpIHsKICBpZiAoa2V5c1snQXJyb3dMZWZ0J10gJiYgcGxheWVyLnggPiAwKSBwbGF5ZXIueCAtPSA0OwogIGlmIChrZXlzWydBcnJvd1JpZ2h0J10gJiYgcGxheWVyLnggPCAzNjApIHBsYXllci54ICs9IDQ7CiAgaWYgKGtleXNbJ1NwYWNlJ10pIHsKICAgIGlmIChidWxsZXRzLmxlbmd0aCA9PT0gMCB8fCBidWxsZXRzW2J1bGxldHMubGVuZ3RoLTFdLnkgPCAzMDApIHsKICAgICAgYnVsbGV0cy5wdXNoKHt4OiBwbGF5ZXIueCArIDE4LCB5OiBwbGF5ZXIueSwgdzogNCwgaDogMTB9KTsKICAgIH0KICB9CiAgCiAgYnVsbGV0cy5mb3JFYWNoKGIgPT4gYi55IC09IDcpOwogIGJ1bGxldHMgPSBidWxsZXRzLmZpbHRlcihiID0+IGIueSA+IDApOwogIAogIGJ1bGxldHMuZm9yRWFjaChiID0+IHsKICAgIGVuZW1pZXMuZm9yRWFjaChlID0+IHsKICAgICAgaWYgKGUuYWxpdmUgJiYgYi54IDwgZS54ICsgZS53ICYmIGIueCArIGIudyA+IGUueCAmJiBiLnkgPCBlLnkgKyBlLmggJiYgYi55ICsgYi5oID4gZS55KSB7CiAgICAgICAgZS5hbGl2ZSA9IGZhbHNlOwogICAgICAgIGIueSA9IC0xMDA7CiAgICAgIH0KICAgIH0pOwogIH0pOwogIAogIGN0eC5jbGVhclJlY3QoMCwwLDQwMCw0MDApOwogIGN0eC5maWxsU3R5bGUgPSAnIzBmMCc7CiAgY3R4LmZpbGxSZWN0KHBsYXllci54LCBwbGF5ZXIueSwgcGxheWVyLncsIHBsYXllci5oKTsKICAKICBjdHguZmlsbFN0eWxlID0gJyNmZmYnOwogIGJ1bGxldHMuZm9yRWFjaChiID0+IGN0eC5maWxsUmVjdChiLngsIGIueSwgYi53LCBiLmgpKTsKICAKICBjdHguZmlsbFN0eWxlID0gJyNmMDAnOwogIGxldCBtb3ZpbmdSaWdodCA9IHRydWU7CiAgZW5lbWllcy5mb3JFYWNoKGUgPT4gewogICAgaWYgKGUuYWxpdmUpIGN0eC5maWxsUmVjdChlLngsIGUueSwgZS53LCBlLmgpOwogIH0pOwogIAogIHJlcXVlc3RBbmltYXRpb25GcmFtZSh1cGRhdGUpOwp9CnVwZGF0ZSgpOwo8L3NjcmlwdD4KPC9ib2R5Pgo8L2h0bWw+Cg=="); }});
        
        g_toolbar = Toolbar::createFromTemplate(ctx_, config);
        g_app->append_child(g_toolbar);
        WindowManager::set_margin_left(0);
        WindowManager::set_margin_top(72);
    }

    void create_status_bar() {
        g_status_bar = std::make_shared<StatusBar>(ctx_);
        ctx_->register_element(g_status_bar);

        auto info_panel = std::make_shared<StatusBarPanel>(ctx_, BevelStyle::Depressed);
        ctx_->register_element(info_panel);
        info_panel->set_text_content("Ready");
        g_status_bar->add_panel(info_panel);
        
        g_clock_panel = std::make_shared<StatusBarPanel>(ctx_, BevelStyle::Embossed);
        ctx_->register_element(g_clock_panel);
        g_clock_panel->set_attribute(ctx_->strings.style, "margin-left: auto; width: 100px; text-align: right;");
        g_status_bar->add_panel(g_clock_panel);

        g_app->append_child(g_status_bar);
        WindowManager::set_margin_bottom(32);
    }

    void spawn_tic_tac_toe() {
        auto win = std::make_shared<Window>(ctx_, "win_ttt_" + std::to_string(++win_count_), "Tic Tac Toe", 100, 100, 300, 300);
        ctx_->register_element(win);
        win->get_content_container()->set_text_content("Tic Tac Toe placeholder.");
        g_app->append_child(win);
    }

    void spawn_iframe_game(const std::string& title, const std::string& url) {
        auto win = std::make_shared<Window>(ctx_, "win_iframe_" + std::to_string(++win_count_), title, 150, 150, 600, 480);
        ctx_->register_element(win);
        
        auto iframe = std::make_shared<HTMLIFrameElement>(ctx_);
        ctx_->register_element(iframe);
        iframe->set_attribute(ctx_->register_string("src"), url);
        iframe->set_attribute(ctx_->strings.style, "width: 100%; height: 100%; border: none;");
        
        win->get_content_container()->append_child(iframe);
        g_app->append_child(win);
    }

    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_app;
    std::shared_ptr<MenuBar> g_menu_bar;
    std::shared_ptr<Toolbar> g_toolbar;
    std::shared_ptr<StatusBar> g_status_bar;
    std::shared_ptr<StatusBarPanel> g_clock_panel;
    int win_count_ = 0;
};

} // namespace Examples
} // namespace NetzWirbel
