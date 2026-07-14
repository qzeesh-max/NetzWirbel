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

#include "NetzWirbel/App.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace NetzWirbel;

struct Particle {
    double x, y;
    double vx, vy;
    double life;
    std::shared_ptr<HTMLDivElement> el;
};

class GalacticDisplayApp : public App {
public:
    void on_init(Context* ctx) override {
        this->ctx_ = ctx;
        srand(42);

        g_app = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_app);
        g_app->set_attribute(ctx_->strings.style, "position: relative; width: 100vw; height: 100vh; background-color: #000; overflow: hidden; margin: -8px;");
        
        Command cmd;
        cmd.type = CommandType::APPEND_CHILD;
        cmd.target_id = 0;
        cmd.arg1 = g_app->get_id();
        ctx->send_command(cmd);

        auto title = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(title);
        title->set_text_content("Galactic Particle System");
        title->set_attribute(ctx_->strings.style, "position: absolute; top: 20px; left: 20px; color: #fff; font-family: sans-serif; font-size: 2em; z-index: 100; text-shadow: 0 0 10px #fff;");
        g_app->append_child(title);

        g_fps_display = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_fps_display);
        g_fps_display->set_attribute(ctx_->strings.style, "position: absolute; top: 60px; left: 20px; color: #0f0; font-family: monospace; font-size: 1.2em; z-index: 100;");
        g_app->append_child(g_fps_display);

        // We use 300 particles
        for (int i = 0; i < 300; ++i) {
            Particle p;
            p.x = 400.0;
            p.y = 300.0;
            double angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
            double speed = ((double)rand() / RAND_MAX) * 3.0 + 1.0;
            p.vx = std::cos(angle) * speed;
            p.vy = std::sin(angle) * speed;
            p.life = ((double)rand() / RAND_MAX) * 1.0 + 0.5;

            p.el = std::make_shared<HTMLDivElement>(ctx);
            ctx->register_element(p.el);
            
            int r = rand() % 255;
            int g_c = rand() % 255;
            int b = 255;
            
            std::string bg = "rgb(" + std::to_string(r) + "," + std::to_string(g_c) + "," + std::to_string(b) + ")";
            std::string style = "position: absolute; width: 4px; height: 4px; border-radius: 50%; background-color: " + bg + "; box-shadow: 0 0 8px " + bg + ";";
            
            p.el->set_attribute(ctx_->strings.style, style);
            g_app->append_child(p.el);
            g_particles.push_back(p);
        }

        g_stats_pane = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_stats_pane);
        g_stats_pane->set_attribute(ctx_->strings.style, "position: absolute; bottom: 0; left: 0; right: 0; height: 40px; background-color: rgba(0,0,0,0.8); color: #fff; padding: 10px; display: flex; align-items: center; white-space: pre-wrap; font-size: 14px; font-family: monospace; z-index: 100; border-top: 1px solid #333;");
        g_stats_pane->set_text_content("Performance Stats: initializing...");
        g_app->append_child(g_stats_pane);
    }

    void on_tick(double time) override {
        ctx_->send_ping();

        double dt = (time - last_time) / 1000.0;
        last_time = time;
        if (dt <= 0.0) dt = 0.016;

        frames++;
        if (time - last_fps_time >= 1000.0) {
            if (g_fps_display) {
                g_fps_display->set_text_content("FPS: " + std::to_string(frames));
            }
            frames = 0;
            last_fps_time = time;
        }

        double cx = 400.0 + std::sin(time * 0.001) * 200.0;
        double cy = 300.0 + std::cos(time * 0.0013) * 150.0;

        for (auto& p : g_particles) {
            p.x += p.vx * dt * 60.0;
            p.y += p.vy * dt * 60.0;
            p.life -= dt;

            if (p.life <= 0) {
                p.x = cx;
                p.y = cy;
                double angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
                double speed = ((double)rand() / RAND_MAX) * 3.0 + 1.0;
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed;
                p.life = ((double)rand() / RAND_MAX) * 1.0 + 0.5;
            }

            std::string transform = "translate(" + std::to_string(p.x) + "px, " + std::to_string(p.y) + "px) scale(" + std::to_string(p.life) + ")";
            p.el->set_styles({
                {"transform", transform},
                {"opacity", std::to_string(p.life)}
            });
        }

        static double last_stats_time = 0;
        if (time - last_stats_time > 1000) {
            auto stats = ctx_->get_rtt_stats();
            std::stringstream ss;
            ss << "RTT (ms) | Low: " << format_number(stats.low)
               << " | High: " << format_number(stats.high)
               << " | Avg: " << format_number(stats.avg)
               << " | Median: " << format_number(stats.median)
               << " | Mode: " << format_number(stats.mode)
               << " | StdDev: " << format_number(stats.stddev)
               << " | p99: " << format_number(stats.p99);
            g_stats_pane->set_text_content(ss.str());
            last_stats_time = time;
        }
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_app;
    std::shared_ptr<HTMLDivElement> g_fps_display;
    std::shared_ptr<HTMLDivElement> g_stats_pane;
    std::vector<Particle> g_particles;
    
    double last_time = 0;
    double last_fps_time = 0;
    int frames = 0;

    std::string format_number(double p) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << p;
        return ss.str();
    }
};


