#pragma once

#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

class Raindrop {
private:
    int h;
    
public:
    float x, y, v, tv, wind;
    
    Raindrop(int x_pos = 0, int height = 32) : h(height), x(x_pos), wind(0) {
        y = -(rand() % (h * 10));
        v = 1;
        tv = 5; // target velocity
    }
    
    void update() {
        if (v < tv) v += 0.2f + (0.01 * y);
        if (v > tv) v -= 0.2f + (0.01 * y);
        
        y += (int)(v + wind * 0.8f);
        x += (int)(wind * 0.3f);
        
        if (y > h) {
            y = -(rand() % (h * 10));
            v = 1;
        }
        if (y < -4) {
            y = -(rand() % (h * 10));
            v = 1;
        }
        if (x > h) {
            x = -(rand() % (h * 10));
            y = rand() % h;
            v = 1;
        }
        if (x < -4) {
            x = rand() % (h * 10);
            y = rand() % h;
            v = 1;
        }
    }
    
    void draw(PicoGraphics& gfx, Pen& pen, float w = 0) {
        wind = w;
        gfx.set_pen(pen);
        gfx.pixel(Point((int)x, (int)y));
        update();
    }
};

class Rain {
private:
    std::vector<Raindrop> raindrops;
    int w;
    
public:
    float wind;
    
    Rain(int width = 32) : w(width), wind(0) {
        generateRainDrops();
    }
    
    void generateRainDrops() {
        raindrops.clear();
        raindrops.reserve(w);
        for (int x = 0; x < w; x++) {
            raindrops.emplace_back(x);
        }
    }
    
    void draw(PicoGraphics& gfx, Pen& pen) {
        for (auto& drop : raindrops) {
            drop.draw(gfx, pen, wind);
        }
    }
};

class Mountain {
private:
    PicoGraphics& gfx;
    float pCurve, waveMod, currentHillHeight;
    int w, h, yoffset;
    std::vector<Point> pointCloud;
    Point lastPoint;
    
public:
    std::vector<Pen> greens;
    
    Mountain(PicoGraphics& graphics, float waveM = 4, int width = 32, int height = 32) 
        : gfx(graphics), pCurve(-1), waveMod(waveM), currentHillHeight(0),
          w(width), h(height), yoffset(12) {
        lastPoint = Point(0, 16);
        createPalette();
        generatePointCloud();
    }
    
    void createPalette() {
        greens.clear();
        greens.push_back(gfx.create_pen(42, 170, 138));
        greens.push_back(gfx.create_pen(26, 187, 43));
        greens.push_back(gfx.create_pen(50, 205, 50));
        greens.push_back(gfx.create_pen(1, 50, 32));
        greens.push_back(gfx.create_pen(150, 255, 150));
        greens.push_back(gfx.create_pen(71, 135, 120));
    }
    
    void updatePalette(const std::vector<std::tuple<int, int, int>>& colours) {
        // Recreate all mountain pens with new colors
        for (size_t i = 0; i < greens.size() && i < colours.size(); i++) {
            auto& [r, g, b] = colours[i];
            greens[i] = gfx.create_pen(r, g, b);
        }
    }
    
    void generatePointCloud(float pCurv = 0, float waveM = 4, int yoff = 12) {
        if (pCurv == pCurve && waveM == waveMod) return;
        
        waveMod = waveM;
        
        if (waveMod > currentHillHeight) currentHillHeight += 0.01f;
        if (waveMod < currentHillHeight) currentHillHeight -= 0.01f;
        
        yoffset = yoff;
        pointCloud.clear();
        pointCloud.push_back(Point(-1, yoffset));
        pCurve = pCurv;
        
        for (int j = 0; j < w * 2; j++) {
            float s = cos(pCurve * 0.001f + j * 0.1f) * currentHillHeight;
            if (s <= 0) {
                lastPoint = Point(j, (int)s + yoffset);
            } else {
                lastPoint = Point(j, (int)(-s * 0.8f) + yoffset);
            }
            pointCloud.push_back(lastPoint);
        }
        pointCloud.push_back(Point(w, yoffset));
    }
    
    void drawMountains(Pen& pen) {
        gfx.set_pen(pen);
        
        // Draw filled polygon
        std::vector<Point> points = pointCloud;
        points.push_back(Point(w, h));
        points.push_back(Point(0, h));
        gfx.polygon(points);
        
        // Draw outline and shading
        int minY = 100, maxY = -100;
        for (const auto& point : pointCloud) {
            gfx.set_pen(greens[0]);
            gfx.pixel(point);
            if (point.y < minY) minY = point.y;
            if (point.y > maxY) maxY = point.y;
        }
        
        // Add shading
        for (const auto& point : pointCloud) {
            if (point.y > minY + 1) {
                gfx.set_pen(greens[3]);
                gfx.pixel(point);
            }
            if (point.y == minY && point.y < 12) {
                gfx.set_pen(greens[1]);
                gfx.pixel(Point(point.x, point.y + 1));
            }
        }
    }
};

// Forward declaration
class Road;

class SceneryObject {
private:
    Pen tree1, tree2, bushCol, lamppost, streetlamp, cactus_green, palm_trunk, palm_leaves, metal_grey, tower_red, billboard_white, pyramid_sand, pyramid_shadow, volcano_dark, lava_red, lava_orange;
    bool pens_created = false;
    
public:
    enum Type { TREE, BUSH, STREETLIGHT, SKYSCRAPER, BUILDING, OFFICE_TOWER, TUNNEL_INTRO, TUNNEL_OUTRO, CACTUS, PALM_TREE, WIND_TURBINE, RADIO_TOWER, BILLBOARD, MONUMENT, WATER_TOWER, FACTORY, CLOCK_TOWER, CHURCH, BARN, WINDMILL, PYRAMID, VOLCANO };
    Type type;
    float trackPosition;    // -1.0 to 1.0, left to right relative to track
    float roadY;           // Y position in road coordinates (0 = far away, h/2 = at player)
    bool active;           // Whether this object is currently active
    
    SceneryObject(Type obj_type = TREE) : type(obj_type), trackPosition(0), roadY(0), active(false) {}
    
    void createPens(PicoGraphics& gfx) {
        if (!pens_created) {
            tree1 = gfx.create_pen(34, 139, 34);     // Forest green
            tree2 = gfx.create_pen(0, 100, 0);       // Dark green
            bushCol = gfx.create_pen(85, 107, 47);   // Dark olive green
            lamppost = gfx.create_pen(169, 169, 169); // Dark grey
            streetlamp = gfx.create_pen(255, 255, 224); // Light yellow
            cactus_green = gfx.create_pen(0, 128, 0); // Cactus green
            palm_trunk = gfx.create_pen(139, 69, 19); // Brown trunk
            palm_leaves = gfx.create_pen(34, 139, 34); // Green leaves
            metal_grey = gfx.create_pen(128, 128, 128); // Metal grey
            tower_red = gfx.create_pen(255, 0, 0);    // Red tower
            billboard_white = gfx.create_pen(255, 255, 255); // White billboard
            pyramid_sand = gfx.create_pen(238, 203, 173); // Sandy beige
            pyramid_shadow = gfx.create_pen(205, 170, 125); // Darker sand
            volcano_dark = gfx.create_pen(64, 64, 64);  // Dark volcanic rock
            lava_red = gfx.create_pen(255, 69, 0);      // Bright lava red
            lava_orange = gfx.create_pen(255, 140, 0);  // Lava orange
            pens_created = true;
        }
    }
    
    void spawn(Type obj_type, float track_pos, float distance) {
        type = obj_type;
        trackPosition = track_pos;
        roadY = distance;  // Start at specified distance
        active = true;
    }
    
    void update(float road_speed, float road_curve, float road_hill, int h, std::function<void()> enterTunnel = nullptr, std::function<void()> exitTunnel = nullptr) {
        if (!active) return;
        
        // Move toward player based on road speed (same rate as original)
        roadY += road_speed * 0.008f;
        
        // Apply road curve effects (objects drift with the curves)
        trackPosition += road_curve * 0.002f;
        
        // Check for tunnel transitions when objects go off-screen
        if (roadY >= h / 2) {
            // Object is moving off-screen (past the player)
            if (type == TUNNEL_INTRO && enterTunnel) {
                enterTunnel();  // Trigger tunnel entrance
            } else if (type == TUNNEL_OUTRO && exitTunnel) {
                exitTunnel();   // Trigger tunnel exit
            }
            active = false;
        }
    }
    
    void draw(PicoGraphics& gfx, int w, int h, float road_curve, float road_hill) {
        if (!active) return;
        if (roadY >= h / 2) return;  // Don't draw if object has reached player
        if (roadY <= 1) return;  // Don't draw if object is too far in distance
        
        createPens(gfx);
        
        // Calculate perspective and screen position
        float perspective = roadY / (h/2);
        if (perspective > 1.0f) perspective = 1.0f;
        
        // Screen coordinates with perspective scaling
        float middlepoint = 0.5f + (road_curve/10.0f) * pow(1-perspective, 3);
        float hillpoint = (road_hill/4.0f) * pow(1-perspective, 2);
        
        int screen_x = (int)(w * (middlepoint + trackPosition * 0.7f * perspective));
        int screen_y = (int)(h - h/2 + roadY + hillpoint);
        
        // Scale based on perspective (closer = larger)
        float scale = 0.2f + 0.8f * perspective;
        
        // Skip if off screen
        if (screen_x < -10 || screen_x > w + 10 || screen_y < 0 || screen_y > h) return;
        
        switch (type) {
            case TREE:
                drawTree(gfx, screen_x, screen_y, scale);
                break;
            case BUSH:
                drawBush(gfx, screen_x, screen_y, scale);
                break;
            case STREETLIGHT:
                drawStreetLight(gfx, screen_x, screen_y, scale);
                break;
            case SKYSCRAPER:
                drawSkyscraper(gfx, screen_x, screen_y, scale, gfx.create_pen(80, 80, 90), gfx.create_pen(120, 140, 160));
                break;
            case BUILDING:
                drawBuilding(gfx, screen_x, screen_y, scale, gfx.create_pen(100, 100, 110), gfx.create_pen(140, 160, 180));
                break;
            case OFFICE_TOWER:
                drawOfficeTower(gfx, screen_x, screen_y, scale, gfx.create_pen(60, 60, 70), gfx.create_pen(255, 255, 200));
                break;
            case CACTUS:
                drawCactus(gfx, screen_x, screen_y, scale);
                break;
            case PALM_TREE:
                drawPalmTree(gfx, screen_x, screen_y, scale);
                break;
            case WIND_TURBINE:
                drawWindTurbine(gfx, screen_x, screen_y, scale);
                break;
            case RADIO_TOWER:
                drawRadioTower(gfx, screen_x, screen_y, scale);
                break;
            case BILLBOARD:
                drawBillboard(gfx, screen_x, screen_y, scale);
                break;
            case MONUMENT:
                drawMonument(gfx, screen_x, screen_y, scale);
                break;
            case WATER_TOWER:
                drawWaterTower(gfx, screen_x, screen_y, scale);
                break;
            case FACTORY:
                drawFactory(gfx, screen_x, screen_y, scale);
                break;
            case CLOCK_TOWER:
                drawClockTower(gfx, screen_x, screen_y, scale);
                break;
            case CHURCH:
                drawChurch(gfx, screen_x, screen_y, scale);
                break;
            case BARN:
                drawBarn(gfx, screen_x, screen_y, scale);
                break;
            case WINDMILL:
                drawWindmill(gfx, screen_x, screen_y, scale);
                break;
            case PYRAMID:
                drawPyramid(gfx, screen_x, screen_y, scale);
                break;
            case VOLCANO:
                drawVolcano(gfx, screen_x, screen_y, scale);
                break;
            case TUNNEL_INTRO:
            case TUNNEL_OUTRO:
                // Tunnel objects don't need individual rendering as they are handled by the tunnel overlay system
                break;
        }
    }
    
private:
    void drawTree(PicoGraphics& gfx, int x, int y, float scale) {
        // Tree trunk
        gfx.set_pen(gfx.create_pen(139, 69, 19)); // Saddle brown
        int trunk_height = std::max(1, (int)(4 * scale));
        gfx.rectangle(Rect(x, y - trunk_height, 1, trunk_height));
        
        // Tree foliage (green circle/blob)
        gfx.set_pen(tree1);
        int foliage_size = std::max(1, (int)(3 * scale));
        for (int dx = -foliage_size; dx <= foliage_size; dx++) {
            for (int dy = -foliage_size; dy <= foliage_size; dy++) {
                if (dx*dx + dy*dy <= foliage_size*foliage_size) {
                    gfx.pixel(Point(x + dx, y - trunk_height - foliage_size + dy));
                }
            }
        }
    }
    
    void drawBush(PicoGraphics& gfx, int x, int y, float scale) {
        gfx.set_pen(bushCol);
        int bush_size = std::max(1, (int)(2 * scale));
        for (int dx = -bush_size; dx <= bush_size; dx++) {
            for (int dy = -bush_size/2; dy <= bush_size/2; dy++) {
                if (dx*dx + dy*dy <= bush_size*bush_size) {
                    gfx.pixel(Point(x + dx, y + dy));
                }
            }
        }
    }
    
    void drawStreetLight(PicoGraphics& gfx, int x, int y, float scale) {
        // Light pole
        gfx.set_pen(lamppost);
        int pole_height = std::max(2, (int)(6 * scale));
        gfx.line(Point(x, y), Point(x, y - pole_height));
        
        // Light fixture
        if (scale > 0.3f) {
            gfx.set_pen(streetlamp);
            int lamp_size = std::max(1, (int)(2 * scale));
            
            // Determine which side of the road we're on
            if (x < 16) {
                // Left side: lamp points right (toward center)
                gfx.line(Point(x, y - pole_height), Point(x + lamp_size, y - pole_height));
            } else {
                // Right side: lamp points left (toward center)
                gfx.line(Point(x, y - pole_height), Point(x - lamp_size, y - pole_height));
            }
        }
    }
    
    void drawSkyscraper(PicoGraphics& gfx, int x, int y, float scale, Pen concrete_pen, Pen glass_pen) {
        // Very tall rectangular building with glass windows
        int building_width = std::max(2, (int)(4 * scale));
        int building_height = std::max(5, (int)(20 * scale));  // Much taller
        
        // Main building structure (concrete grey)
        gfx.set_pen(concrete_pen);
        gfx.rectangle(Rect(x - building_width/2, y - building_height, building_width, building_height));
        
        // Glass windows (lighter blue-grey)
        gfx.set_pen(glass_pen);
        if (scale > 0.3f) {
            // Draw window rows
            for (int floor = 1; floor < building_height - 2; floor += 3) {
                for (int window = 1; window < building_width - 1; window += 2) {
                    gfx.pixel(Point(x - building_width/2 + window, y - building_height + floor));
                }
            }
        }
        
        // Rooftop details (antenna/spire)
        if (scale > 0.4f) {
            gfx.set_pen(gfx.create_pen(60, 60, 70));
            gfx.line(Point(x, y - building_height), Point(x, y - building_height - (int)(3 * scale)));
        }
    }
    
    void drawBuilding(PicoGraphics& gfx, int x, int y, float scale, Pen concrete_pen, Pen glass_pen) {
        // Medium height building
        int building_width = std::max(2, (int)(5 * scale));
        int building_height = std::max(3, (int)(12 * scale));
        
        // Main building (concrete)
        gfx.set_pen(concrete_pen);
        gfx.rectangle(Rect(x - building_width/2, y - building_height, building_width, building_height));
        
        // Windows
        gfx.set_pen(glass_pen);
        if (scale > 0.2f) {
            for (int floor = 2; floor < building_height - 1; floor += 3) {
                for (int window = 1; window < building_width - 1; window += 2) {
                    gfx.pixel(Point(x - building_width/2 + window, y - building_height + floor));
                }
            }
        }
    }
    
    void drawOfficeTower(PicoGraphics& gfx, int x, int y, float scale, Pen dark_pen, Pen light_pen) {
        // Medium-tall building with lit windows (office lights)
        int building_width = std::max(3, (int)(6 * scale));
        int building_height = std::max(4, (int)(15 * scale));
        
        // Main building structure
        gfx.set_pen(dark_pen);
        gfx.rectangle(Rect(x - building_width/2, y - building_height, building_width, building_height));
        
        // Office lights (random lit windows)
        gfx.set_pen(light_pen);
        if (scale > 0.25f) {
            for (int floor = 1; floor < building_height - 1; floor += 2) {
                for (int window = 1; window < building_width - 1; window += 2) {
                    // Random chance for lit windows (simulating office workers)
                    if (rand() % 3 == 0) {  // 33% chance of lit window
                        gfx.pixel(Point(x - building_width/2 + window, y - building_height + floor));
                    }
                }
            }
        }
    }
    
    void drawCactus(PicoGraphics& gfx, int x, int y, float scale) {
        gfx.set_pen(cactus_green);
        int cactus_height = std::max(3, (int)(8 * scale));
        int cactus_width = std::max(1, (int)(2 * scale));
        
        // Main stem
        gfx.rectangle(Rect(x - cactus_width/2, y - cactus_height, cactus_width, cactus_height));
        
        // Side arms if large enough
        if (scale > 0.4f) {
            int arm_size = std::max(1, (int)(3 * scale));
            // Left arm
            gfx.rectangle(Rect(x - cactus_width - arm_size, y - cactus_height/2, arm_size, 1));
            gfx.rectangle(Rect(x - cactus_width - arm_size, y - cactus_height/2 - arm_size, 1, arm_size));
            // Right arm
            gfx.rectangle(Rect(x + cactus_width, y - cactus_height/2, arm_size, 1));
            gfx.rectangle(Rect(x + cactus_width + arm_size - 1, y - cactus_height/2 - arm_size, 1, arm_size));
        }
    }
    
    void drawPalmTree(PicoGraphics& gfx, int x, int y, float scale) {
        // Trunk
        gfx.set_pen(palm_trunk);
        int trunk_height = std::max(4, (int)(10 * scale));
        gfx.rectangle(Rect(x, y - trunk_height, 1, trunk_height));
        
        // Palm fronds
        gfx.set_pen(palm_leaves);
        int frond_size = std::max(2, (int)(4 * scale));
        
        // Draw 4 fronds in cardinal directions
        for (int i = -frond_size; i <= frond_size; i++) {
            gfx.pixel(Point(x + i, y - trunk_height)); // Horizontal frond
            gfx.pixel(Point(x, y - trunk_height + i)); // Vertical frond
        }
        
        // Diagonal fronds
        if (scale > 0.3f) {
            for (int i = 1; i <= frond_size/2; i++) {
                gfx.pixel(Point(x + i, y - trunk_height - i)); // Top-right
                gfx.pixel(Point(x - i, y - trunk_height - i)); // Top-left
            }
        }
    }
    
    void drawWindTurbine(PicoGraphics& gfx, int x, int y, float scale) {
        gfx.set_pen(gfx.create_pen(240, 240, 240)); // Light grey
        int tower_height = std::max(6, (int)(15 * scale));
        
        // Tower pole
        gfx.line(Point(x, y), Point(x, y - tower_height));
        
        // Turbine hub and blades
        if (scale > 0.3f) {
            gfx.pixel(Point(x, y - tower_height)); // Hub
            int blade_length = std::max(2, (int)(3 * scale));
            
            // Three wind turbine blades
            gfx.line(Point(x, y - tower_height), Point(x - blade_length, y - tower_height - 1));
            gfx.line(Point(x, y - tower_height), Point(x + blade_length, y - tower_height - 1));
            gfx.line(Point(x, y - tower_height), Point(x, y - tower_height - blade_length));
        }
    }
    
    void drawRadioTower(PicoGraphics& gfx, int x, int y, float scale) {
        gfx.set_pen(tower_red);
        int tower_height = std::max(8, (int)(20 * scale));
        
        // Main tower structure (tapered)
        for (int i = 0; i < tower_height; i++) {
            int width = std::max(1, (int)((tower_height - i) * scale * 0.3f));
            if (width > 1) {
                gfx.line(Point(x - width/2, y - i), Point(x + width/2, y - i));
            } else {
                gfx.pixel(Point(x, y - i));
            }
        }
        
        // Antenna segments
        if (scale > 0.4f) {
            gfx.set_pen(metal_grey);
            for (int i = tower_height/4; i < tower_height; i += tower_height/4) {
                int seg_width = std::max(1, (int)((tower_height - i) * scale * 0.2f));
                gfx.line(Point(x - seg_width, y - i), Point(x + seg_width, y - i));
            }
        }
    }
    
    void drawBillboard(PicoGraphics& gfx, int x, int y, float scale) {
        int board_width = std::max(3, (int)(8 * scale));
        int board_height = std::max(2, (int)(4 * scale));
        int pole_height = std::max(3, (int)(6 * scale));
        
        // Support poles
        gfx.set_pen(metal_grey);
        gfx.line(Point(x - board_width/3, y), Point(x - board_width/3, y - pole_height));
        gfx.line(Point(x + board_width/3, y), Point(x + board_width/3, y - pole_height));
        
        // Billboard surface
        gfx.set_pen(billboard_white);
        gfx.rectangle(Rect(x - board_width/2, y - pole_height - board_height, board_width, board_height));
        
        // Simple advertisement pattern
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(255, 0, 0)); // Red
            gfx.rectangle(Rect(x - board_width/2 + 1, y - pole_height - board_height + 1, board_width - 2, 1));
        }
    }
    
    void drawMonument(PicoGraphics& gfx, int x, int y, float scale) {
        gfx.set_pen(gfx.create_pen(105, 105, 105)); // Dark grey stone
        int monument_height = std::max(5, (int)(12 * scale));
        int base_width = std::max(3, (int)(6 * scale));
        
        // Base
        gfx.rectangle(Rect(x - base_width/2, y - 2, base_width, 2));
        
        // Tapering column
        for (int i = 0; i < monument_height; i++) {
            float taper = 1.0f - (float)i / monument_height * 0.5f;
            int width = std::max(1, (int)(base_width * taper * 0.6f));
            gfx.rectangle(Rect(x - width/2, y - 2 - i, width, 1));
        }
        
        // Top ornament
        if (scale > 0.4f) {
            gfx.set_pen(gfx.create_pen(255, 215, 0)); // Gold
            gfx.pixel(Point(x, y - 2 - monument_height));
        }
    }
    
    void drawWaterTower(PicoGraphics& gfx, int x, int y, float scale) {
        int tank_width = std::max(3, (int)(7 * scale));
        int tank_height = std::max(2, (int)(4 * scale));
        int leg_height = std::max(4, (int)(8 * scale));
        
        // Support legs
        gfx.set_pen(metal_grey);
        gfx.line(Point(x - tank_width/3, y), Point(x - tank_width/3, y - leg_height));
        gfx.line(Point(x + tank_width/3, y), Point(x + tank_width/3, y - leg_height));
        gfx.line(Point(x, y), Point(x, y - leg_height));
        
        // Water tank
        gfx.set_pen(gfx.create_pen(135, 206, 235)); // Sky blue
        gfx.rectangle(Rect(x - tank_width/2, y - leg_height - tank_height, tank_width, tank_height));
        
        // Tank rim
        gfx.set_pen(metal_grey);
        gfx.rectangle(Rect(x - tank_width/2, y - leg_height - tank_height, tank_width, 1));
    }
    
    void drawFactory(PicoGraphics& gfx, int x, int y, float scale) {
        int building_width = std::max(4, (int)(10 * scale));
        int building_height = std::max(3, (int)(8 * scale));
        
        // Main factory building
        gfx.set_pen(gfx.create_pen(70, 70, 80)); // Industrial grey
        gfx.rectangle(Rect(x - building_width/2, y - building_height, building_width, building_height));
        
        // Smokestacks
        gfx.set_pen(gfx.create_pen(60, 60, 60)); // Darker grey
        int stack_height = std::max(4, (int)(12 * scale));
        gfx.rectangle(Rect(x - building_width/3, y - building_height - stack_height, 1, stack_height));
        gfx.rectangle(Rect(x + building_width/4, y - building_height - stack_height, 1, stack_height));
        
        // Smoke (if large enough)
        if (scale > 0.4f) {
            gfx.set_pen(gfx.create_pen(180, 180, 180)); // Light grey smoke
            gfx.pixel(Point(x - building_width/3 - 1, y - building_height - stack_height - 1));
            gfx.pixel(Point(x + building_width/4 + 1, y - building_height - stack_height - 1));
        }
        
        // Windows
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(255, 255, 0)); // Yellow light
            for (int i = 2; i < building_height - 1; i += 2) {
                for (int j = 2; j < building_width - 1; j += 3) {
                    if (rand() % 3 == 0) { // Random lit windows
                        gfx.pixel(Point(x - building_width/2 + j, y - building_height + i));
                    }
                }
            }
        }
    }
    
    void drawClockTower(PicoGraphics& gfx, int x, int y, float scale) {
        int tower_width = std::max(2, (int)(4 * scale));
        int tower_height = std::max(6, (int)(18 * scale));
        
        // Tower body
        gfx.set_pen(gfx.create_pen(139, 69, 19)); // Brown brick
        gfx.rectangle(Rect(x - tower_width/2, y - tower_height, tower_width, tower_height));
        
        // Clock face
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(255, 255, 255)); // White clock face
            int clock_size = std::max(1, (int)(2 * scale));
            gfx.rectangle(Rect(x - clock_size/2, y - tower_height/2 - clock_size/2, clock_size, clock_size));
            
            // Clock hands
            gfx.set_pen(gfx.create_pen(0, 0, 0)); // Black hands
            gfx.pixel(Point(x, y - tower_height/2)); // Center
            gfx.pixel(Point(x, y - tower_height/2 - 1)); // Hour hand
            gfx.pixel(Point(x + 1, y - tower_height/2)); // Minute hand
        }
        
        // Spire
        if (scale > 0.4f) {
            gfx.set_pen(gfx.create_pen(50, 50, 50)); // Dark grey
            int spire_height = std::max(2, (int)(4 * scale));
            for (int i = 0; i < spire_height; i++) {
                int spire_width = std::max(1, spire_height - i);
                gfx.rectangle(Rect(x - spire_width/2, y - tower_height - i, spire_width, 1));
            }
        }
    }
    
    void drawChurch(PicoGraphics& gfx, int x, int y, float scale) {
        int church_width = std::max(3, (int)(7 * scale));
        int church_height = std::max(4, (int)(10 * scale));
        
        // Main church building
        gfx.set_pen(gfx.create_pen(139, 69, 19)); // Brown
        gfx.rectangle(Rect(x - church_width/2, y - church_height, church_width, church_height));
        
        // Steeple
        gfx.set_pen(gfx.create_pen(105, 105, 105)); // Grey
        int steeple_height = std::max(3, (int)(8 * scale));
        gfx.rectangle(Rect(x - 1, y - church_height - steeple_height, 2, steeple_height));
        
        // Cross on top
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(255, 255, 255)); // White cross
            gfx.pixel(Point(x, y - church_height - steeple_height - 1)); // Vertical
            gfx.pixel(Point(x, y - church_height - steeple_height - 2));
            gfx.pixel(Point(x - 1, y - church_height - steeple_height - 1)); // Horizontal
            gfx.pixel(Point(x + 1, y - church_height - steeple_height - 1));
        }
        
        // Windows
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(100, 100, 255)); // Blue stained glass
            for (int i = 2; i < church_height - 2; i += 3) {
                gfx.pixel(Point(x - 1, y - church_height + i));
                gfx.pixel(Point(x + 1, y - church_height + i));
            }
        }
    }
    
    void drawBarn(PicoGraphics& gfx, int x, int y, float scale) {
        int barn_width = std::max(4, (int)(9 * scale));
        int barn_height = std::max(3, (int)(7 * scale));
        
        // Main barn structure
        gfx.set_pen(gfx.create_pen(139, 0, 0)); // Dark red
        gfx.rectangle(Rect(x - barn_width/2, y - barn_height, barn_width, barn_height));
        
        // Roof
        gfx.set_pen(gfx.create_pen(105, 105, 105)); // Grey roof
        int roof_height = std::max(2, (int)(3 * scale));
        for (int i = 0; i < roof_height; i++) {
            int roof_width = barn_width - i;
            gfx.rectangle(Rect(x - roof_width/2, y - barn_height - i, roof_width, 1));
        }
        
        // Barn doors
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(101, 67, 33)); // Saddle brown
            gfx.rectangle(Rect(x - 1, y - barn_height/2, 2, barn_height/2));
        }
        
        // Silo (if large enough)
        if (scale > 0.4f) {
            gfx.set_pen(gfx.create_pen(192, 192, 192)); // Silver
            int silo_height = std::max(4, (int)(8 * scale));
            gfx.rectangle(Rect(x + barn_width/2 + 1, y - silo_height, 2, silo_height));
            
            // Silo top
            gfx.set_pen(gfx.create_pen(105, 105, 105)); // Grey
            gfx.pixel(Point(x + barn_width/2 + 1, y - silo_height - 1));
            gfx.pixel(Point(x + barn_width/2 + 2, y - silo_height - 1));
        }
    }
    
    void drawWindmill(PicoGraphics& gfx, int x, int y, float scale) {
        int mill_width = std::max(2, (int)(4 * scale));
        int mill_height = std::max(4, (int)(10 * scale));
        
        // Windmill body
        gfx.set_pen(gfx.create_pen(245, 245, 220)); // Beige
        gfx.rectangle(Rect(x - mill_width/2, y - mill_height, mill_width, mill_height));
        
        // Windmill blades
        gfx.set_pen(gfx.create_pen(139, 69, 19)); // Brown wood
        if (scale > 0.3f) {
            int blade_length = std::max(3, (int)(6 * scale));
            int blade_center_x = x;
            int blade_center_y = y - mill_height + mill_height/4;
            
            // Four blades in X pattern
            gfx.line(Point(blade_center_x - blade_length, blade_center_y - blade_length),
                    Point(blade_center_x + blade_length, blade_center_y + blade_length));
            gfx.line(Point(blade_center_x + blade_length, blade_center_y - blade_length),
                    Point(blade_center_x - blade_length, blade_center_y + blade_length));
        }
        
        // Central hub
        gfx.set_pen(gfx.create_pen(50, 50, 50)); // Dark grey
        gfx.pixel(Point(x, y - mill_height + mill_height/4));
        
        // Door
        if (scale > 0.3f) {
            gfx.set_pen(gfx.create_pen(101, 67, 33)); // Brown door
            gfx.rectangle(Rect(x - 1, y - mill_height/3, 1, mill_height/3));
        }
    }
    
    void drawPyramid(PicoGraphics& gfx, int x, int y, float scale) {
        int pyramid_width = std::max(3, (int)(8 * scale));
        int pyramid_height = std::max(2, (int)(6 * scale));
        
        // Draw pyramid from bottom up, getting narrower each row
        for (int row = 0; row < pyramid_height; row++) {
            int row_y = y - row;
            int row_width = pyramid_width - (row * 2 * pyramid_width / pyramid_height);
            if (row_width < 1) row_width = 1;
            
            // Alternate between light and shadow sides for 3D effect
            if (row < pyramid_height / 2) {
                gfx.set_pen(pyramid_sand); // Light side
            } else {
                gfx.set_pen(pyramid_shadow); // Shadow side
            }
            
            // Draw the row
            gfx.rectangle(Rect(x - row_width/2, row_y, row_width, 1));
        }
        
        // Add some detail lines for larger pyramids
        if (scale > 0.4f) {
            gfx.set_pen(pyramid_shadow);
            // Left edge shadow
            for (int i = 0; i < pyramid_height; i++) {
                int edge_width = pyramid_width - (i * 2 * pyramid_width / pyramid_height);
                if (edge_width > 0) {
                    gfx.pixel(Point(x - edge_width/2, y - i));
                }
            }
        }
    }
    
    void drawVolcano(PicoGraphics& gfx, int x, int y, float scale) {
        int volcano_width = std::max(4, (int)(10 * scale));
        int volcano_height = std::max(3, (int)(8 * scale));
        
        // Draw volcano base (cone shape)
        for (int row = 0; row < volcano_height; row++) {
            int row_y = y - row;
            int row_width = volcano_width - (row * volcano_width / volcano_height);
            if (row_width < 1) row_width = 1;
            
            gfx.set_pen(volcano_dark);
            gfx.rectangle(Rect(x - row_width/2, row_y, row_width, 1));
        }
        
        // Crater at the top
        if (scale > 0.3f) {
            int crater_width = std::max(1, volcano_width / 3);
            gfx.set_pen(lava_red);
            gfx.rectangle(Rect(x - crater_width/2, y - volcano_height, crater_width, 1));
        }
        
        // Lava flow effects for larger volcanoes
        if (scale > 0.5f) {
            // Random lava streams down the sides
            gfx.set_pen(lava_orange);
            for (int i = 0; i < 2; i++) {
                int stream_x = x + (i == 0 ? -volcano_width/3 : volcano_width/3);
                int stream_length = volcano_height / 2;
                for (int j = 0; j < stream_length; j++) {
                    if (rand() % 3 == 0) { // Intermittent lava pixels
                        gfx.pixel(Point(stream_x, y - volcano_height + j + 1));
                    }
                }
            }
            
            // Lava glow at crater
            gfx.set_pen(lava_red);
            if (volcano_width >= 6) {
                gfx.pixel(Point(x - 1, y - volcano_height));
                gfx.pixel(Point(x + 1, y - volcano_height));
            }
        }
    }
};

class OncomingCar {
public:
    float trackPosition;    // -1.0 to 1.0, where 0 is center of track
    float roadY;           // Y position in road coordinates (same as SceneryObject)
    bool active;           // Whether this car is currently active
    int color_index;       // Car color variant

private:
    Pen car_color, black, red, white;
    bool pens_created = false;
    
public:
    OncomingCar() : trackPosition(0), roadY(0), active(false), color_index(0) {}
    
    void spawn(float track_pos) {
        trackPosition = track_pos;
        roadY = 0.1f + (rand() % 5) * 0.1f;  // Start at far distance like scenery (0.1-0.5)
        active = true;
        color_index = rand() % 4;  // 4 car colors
    }
    
    void createPens(PicoGraphics& gfx) {
        if (!pens_created) {
            black = gfx.create_pen(0, 0, 0);
            red = gfx.create_pen(255, 0, 0);
            white = gfx.create_pen(255, 255, 255);
            pens_created = true;
        }
        
        // Create car color based on index
        switch (color_index) {
            case 0: car_color = gfx.create_pen(255, 255, 0); break; // Yellow
            case 1: car_color = gfx.create_pen(0, 255, 255); break; // Cyan  
            case 2: car_color = gfx.create_pen(255, 0, 255); break; // Magenta
            case 3: car_color = gfx.create_pen(0, 255, 0); break;   // Green
            default: car_color = gfx.create_pen(255, 255, 0); break;
        }
    }

    void update(float road_speed, float road_curve, float road_hill, int h) {
        if (!active) return;
        
        // Move toward player at same rate as scenery for consistency
        roadY += road_speed * 0.008f;
        
        // Apply road curve effects (same as scenery)
        trackPosition += road_curve * 0.002f;
        
        // Keep cars roughly on track
        if (trackPosition > 0.8f) trackPosition = 0.8f;
        if (trackPosition < -0.8f) trackPosition = -0.8f;
        
        // Deactivate when past the player (same as scenery)
        if (roadY >= h / 2) {
            active = false;
        }
    }
    
    void draw(PicoGraphics& gfx, int w, int h, float road_curve, float road_hill) {
        if (!active) return;
        if (roadY >= h / 2) return;  // Don't draw if car has reached player
        if (roadY <= 1) return;  // Don't draw if car is too far (same as scenery)
        
        createPens(gfx);
        
        // Use exact same positioning logic as SceneryObject for consistency
        float perspective = roadY / (h/2);
        if (perspective > 1.0f) perspective = 1.0f;
        
        // Screen coordinates with perspective scaling (same as scenery)
        float middlepoint = 0.5f + (road_curve/10.0f) * pow(1-perspective, 3);
        float hillpoint = (road_hill/4.0f) * pow(1-perspective, 2);
        
        int screen_x = (int)(w * (middlepoint + trackPosition * 0.7f * perspective));
        int screen_y = (int)(h - h/2 + roadY + hillpoint);
        
        // Scale based on perspective (same as scenery)
        float scale = 0.2f + 0.8f * perspective;
        
        // Skip if off screen
        if (screen_x < -10 || screen_x > w + 10 || screen_y < 0 || screen_y > h) return;
        
        // Draw car - simpler and more visible, scaled to match 8-pixel player car
        int car_width = std::max(3, (int)(8 * scale));
        int car_height = std::max(2, (int)(4 * scale));
        
        // Car body (main color)
        gfx.set_pen(car_color);
        gfx.rectangle(Rect(screen_x - car_width/2, screen_y - car_height, car_width, car_height));
        
        // Add contrast outline for visibility
        if (scale > 0.3f) {
            gfx.set_pen(black);
            // Top edge
            gfx.line(Point(screen_x - car_width/2, screen_y - car_height), 
                    Point(screen_x + car_width/2 - 1, screen_y - car_height));
            // Side edges
            gfx.pixel(Point(screen_x - car_width/2, screen_y - 1));
            gfx.pixel(Point(screen_x + car_width/2 - 1, screen_y - 1));
        }
        
        // Headlights when close enough
        if (scale > 0.5f) {
            gfx.set_pen(white);
            gfx.pixel(Point(screen_x - car_width/2 + 1, screen_y - car_height));
            gfx.pixel(Point(screen_x + car_width/2 - 2, screen_y - car_height));
        }
        
        // Red tail lights - always visible
        gfx.set_pen(red);
        if (car_width >= 4) {
            // Two tail lights when car is wide enough
            gfx.pixel(Point(screen_x - car_width/2 + 1, screen_y - 1));
            gfx.pixel(Point(screen_x + car_width/2 - 2, screen_y - 1));
        } else {
            // Single central tail light for smallest cars
            gfx.pixel(Point(screen_x, screen_y - 1));
        }
    }
    
    // Simple collision detection - check if car is near player position and close enough
    bool checkCollisionWithPlayer(float player_track_pos) {
        if (!active) return false;
        
        // Check if car is close to player (in the collision zone)
        bool close_enough = (roadY >= 10.0f && roadY <= 18.0f);  // Collision zone near player
        bool positions_overlap = std::abs(trackPosition - player_track_pos) < 0.4f;  // Track position overlap
        
        return close_enough && positions_overlap;
    }
    
    // Car-to-car collision detection
    bool checkCollisionWithCar(const OncomingCar& other) {
        if (!active || !other.active || this == &other) return false;
        
        // Check if cars are at similar distances and positions
        bool same_distance = std::abs(roadY - other.roadY) < 2.0f;
        bool same_position = std::abs(trackPosition - other.trackPosition) < 0.3f;
        
        return same_distance && same_position;
    }
    
    // Apply collision effects
    void applyCollisionBounce(float bounce_direction) {
        trackPosition += bounce_direction * 0.2f;  // Bounce sideways
        
        // Keep on track
        if (trackPosition > 0.8f) trackPosition = 0.8f;
        if (trackPosition < -0.8f) trackPosition = -0.8f;
    }
};

class Car {
public:
    float velocity = 0.0f;    // Current steering velocity (-1 left, +1 right)
    float position = 0.0f;    // Track position (-1 left edge, +1 right edge)
    float speed = 20.0f;      // Forward speed
    bool autoAccelEnabled = true;
    
private:
    Pen carCol, black, grey, red, white;
    bool pens_created = false;
    
    // Physics constants
    const float STEER_POWER = 0.05f;     // How quickly steering input affects velocity
    const float FRICTION = 0.85f;        // How quickly velocity decays (0-1, lower = more friction)
    const float MAX_VELOCITY = 0.1f;     // Maximum steering velocity
    const float AUTO_ACCEL_RATE = 0.3f;  // How fast auto-acceleration works
    const float AUTO_ACCEL_TARGET = 60.0f; // Target speed for auto-acceleration
    
public:
    Car() {}
    
    void createPens(PicoGraphics& gfx) {
        if (!pens_created) {
            carCol = gfx.create_pen(255, 0, 0);    // Red car
            black = gfx.create_pen(0, 0, 0);       // Black windows
            grey = gfx.create_pen(128, 128, 128);  // Grey details
            red = gfx.create_pen(255, 0, 0);       // Red details
            white = gfx.create_pen(255, 255, 255); // White details
            pens_created = true;
        }
    }
    
    void update(float leftInput, float rightInput) {
        // Update steering physics
        float steerInput = rightInput - leftInput;  // -1 left, +1 right
        
        // Apply steering input to velocity
        velocity += steerInput * STEER_POWER;
        
        // Apply friction to velocity
        velocity *= FRICTION;
        
        // Clamp velocity to maximum
        if (velocity > MAX_VELOCITY) velocity = MAX_VELOCITY;
        if (velocity < -MAX_VELOCITY) velocity = -MAX_VELOCITY;
        
        // Update position based on velocity
        position += velocity;
        
        // Keep car on track (with some tolerance for off-road feeling)
        if (position > 1.2f) {
            position = 1.2f;
            velocity *= 0.5f;  // Reduce velocity when hitting boundary
        }
        if (position < -1.2f) {
            position = -1.2f;
            velocity *= 0.5f;
        }
        
        // Auto-acceleration when enabled
        if (autoAccelEnabled && speed < AUTO_ACCEL_TARGET) {
            speed += AUTO_ACCEL_RATE;
            if (speed > AUTO_ACCEL_TARGET) {
                speed = AUTO_ACCEL_TARGET;
            }
        }
    }
    
    void draw(PicoGraphics& gfx) {
        createPens(gfx);
        
        int w = gfx.bounds.w;
        int h = gfx.bounds.h;
        
        // Car position calculation like original
        int carpos = w/2 + (int)(position * w * 0.3f) - 4;  // Center 8-pixel wide car
        
        // Keep car at bottom of screen like original
        int cary = h - 3;
        
        // Main car body (red, 8 pixels wide, 3 pixels tall)
        Pen red1 = gfx.create_pen(255, 0, 0);   // Bright red
        Pen red2 = gfx.create_pen(200, 0, 0);   // Darker red
        gfx.set_pen(red1);
        gfx.rectangle(Rect(carpos, cary, 8, 1));
        gfx.set_pen(red2);
        gfx.rectangle(Rect(carpos, cary + 1, 8, 1));
        gfx.rectangle(Rect(carpos, cary + 2, 8, 1));
        
        // White center stripe
        gfx.set_pen(white);
        gfx.rectangle(Rect(carpos + 3, cary + 1, 2, 1));
        
        // Windscreen (blue)
        Pen blue = gfx.create_pen(0, 0, 255);
        gfx.set_pen(blue);
        gfx.rectangle(Rect(carpos + 1, cary - 1, 6, 1));
        
        // Hair/driver details
        Pen yellow = gfx.create_pen(255, 255, 0);
        Pen brown = gfx.create_pen(139, 69, 19);
        gfx.set_pen(yellow);
        gfx.rectangle(Rect(carpos + 1, cary - 1, 2, 2));
        gfx.set_pen(brown);
        gfx.rectangle(Rect(carpos + 5, cary - 1, 2, 2));
        
        // Tyres (grey)
        gfx.set_pen(grey);
        gfx.rectangle(Rect(carpos, cary + 2, 2, 1));
        gfx.rectangle(Rect(carpos + 6, cary + 2, 2, 1));
        
        // Tail lights (red)
        gfx.set_pen(red1);
        gfx.rectangle(Rect(carpos, cary, 2, 1));
        gfx.rectangle(Rect(carpos + 6, cary, 2, 1));
    }
    
    float getTrackPosition() const {
        return position;
    }
};

class Road {
private:
    PicoGraphics& gfx;
    int w, h;
    int frameCount = 0;
    
    // Road geometry - following original pattern
    float distance = 0;
    float roadcurve = 0;
    float curve = 0;
    float tCurvature = 0;
    float pCurvature = 0;
    float roadhill = 0;
    float hill = 0;
    float tHillCurvature = 0;
    float pHillCurvature = 0;
    float sectionDistance = 0;
    float sectionLength = 4000;
    float elapsedTime = 0.016f; // ~60fps
    
    // Theme and visual state
    enum Theme { CITYSCAPE, NIGHT, VICE, DESERT, STARRYNIGHT, DAYTOO, SNOW, F32, RED, CYBER, SUNSET, OCEAN, NEON, DAY };
    Theme currentTheme = CITYSCAPE;
    
    // Road surface patterns
    std::vector<float> roadPattern;
    
    // Theme properties
    int hillHeight = 8;
    bool bTrees = true;
    bool bBushes = true;
    bool shwStrLgts = false;
    bool bSigns = false;
    bool bClouds = true;
    bool displayTime = true;
    bool rain = false;
    uint32_t rainTimer = 0;
    
    // Color themes
    Pen grassColour1, grassColour2, edgeCol, edgeCol2, sunCol1,sunCol2;
    std::vector<Pen> SCL; // Sky Color List
    std::vector<std::tuple<int, int, int>> hillColours;
    bool pens_created = false;
    
    // Sun/Moon features
    bool bSun = true, bMoon = false, bStars = false;
    int sunSizeMod = 0;
    Pen moonColour, white;
    
    // Scenery and objects
    std::unique_ptr<Mountain> mountain;
    std::unique_ptr<Rain> rainSystem;
    std::vector<SceneryObject> sceneryObjects;
    std::vector<OncomingCar> oncomingCars;
    
    // Tunnel system
    bool inTunnel = false;
    float tunnelProgress = 0.0f;  // 0.0 = not in tunnel, 1.0 = full tunnel
    uint32_t tunnelStartTime = 0;
    uint32_t tunnelDuration = 8000000;  // 8 seconds in microseconds
    
    // Spawning timers
    uint32_t lastScenerySpawn = 0;
    uint32_t lastCarSpawn = 0;
    
    // Auto theme changing
    uint32_t lastThemeChange = 0;
    float distanceSinceThemeChange = 0.0f;
    const float AUTO_THEME_DISTANCE = 1500.0f;  
    
public:
    float speed = 20.0f;  // Current road speed (synchronized with car speed)
    
    Road(PicoGraphics& graphics, int width, int height) 
        : gfx(graphics), w(width), h(height) {
        
        // Initialize road pattern
        roadPattern.resize(w);
        for (int i = 0; i < w; i++) {
            roadPattern[i] = 0.0f;
        }
        
        // Initialize mountain
        mountain = std::make_unique<Mountain>(gfx, 4, w, h);
        
        // Initialize rain
        rainSystem = std::make_unique<Rain>(w);
        
        // Initialize scenery and car pools
        sceneryObjects.resize(20);  // Pool of 20 scenery objects
        oncomingCars.resize(5);     // Pool of 5 oncoming cars
        
        // Set initial theme
        initPalette();
        setTheme(currentTheme);
        
        // Initialize auto theme timer
        lastThemeChange = time_us_64();
    }
    
    void initPalette() {
        // Initialize default pens if not created
        if (!pens_created) {
            grassColour1 = gfx.create_pen(0, 255, 0);
            grassColour2 = gfx.create_pen(0, 200, 0);
            edgeCol = gfx.create_pen(255, 255, 255);
            edgeCol2 = gfx.create_pen(200, 200, 200);
            moonColour = gfx.create_pen(255, 255, 255);
            white = gfx.create_pen(255, 255, 255);
            pens_created = true;
        }
    }
    
    Pen createDarkenedPen(uint8_t r, uint8_t g, uint8_t b, float brightness) {
        return gfx.create_pen((int)(r * brightness), (int)(g * brightness), (int)(b * brightness));
    }
    
    void setTheme(Theme theme) {
        currentTheme = theme;
        
        // Clear rain when switching themes (except where rain is intended)
        if (theme != NIGHT && theme != STARRYNIGHT) {
            rain = false;
            rainTimer = 0;
        }
        
        SCL.clear();
        
        switch (theme) {
            case DAY:
                hillHeight = 8;
                bTrees = true;
                bBushes = true;
                shwStrLgts = false;
                bSigns = false;
                bClouds = true;
                displayTime = true;
                rain = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 3;
                
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                grassColour1 = gfx.create_pen(0, 255, 0);      // Bright green
                grassColour2 = gfx.create_pen(0, 200, 0);      // Medium green
                edgeCol = gfx.create_pen(255, 255, 255);       // White
                edgeCol2 = gfx.create_pen(200, 200, 200);      // Light grey
                
                // Default green hill colors
                hillColours = {
                    {42, 170, 138}, {26, 187, 43}, {50, 205, 50}, 
                    {1, 50, 32}, {150, 255, 150}, {71, 135, 120}
                };
                
                // Sky gradient: light blue to white
                SCL.push_back(gfx.create_pen(135, 206, 235));  // Sky blue
                SCL.push_back(gfx.create_pen(176, 224, 230));  // Powder blue
                SCL.push_back(gfx.create_pen(220, 220, 220));  // Light grey
                SCL.push_back(gfx.create_pen(255, 255, 255));  // White
                break;
                
            case NIGHT:
                hillHeight = 8;
                bTrees = true;
                bBushes = true;
                shwStrLgts = true;
                bSigns = true;
                bClouds = false;
                displayTime = true;
                bSun = false;
                bMoon = true;
                bStars = true;
                sunSizeMod = 0;
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                
                // Start rain with 60 second timer
                if (rainTimer == 0) {
                    rain = true;
                    rainTimer = time_us_64();
                }
                
                grassColour1 = gfx.create_pen(0, 100, 0);      // Dark green
                grassColour2 = gfx.create_pen(0, 80, 0);       // Darker green
                edgeCol = gfx.create_pen(150, 150, 150);       // Grey
                edgeCol2 = gfx.create_pen(100, 100, 100);      // Dark grey
                
                hillColours = {
                    {132, 77, 163}, {102, 59, 148}, {67, 28, 118},
                    {34, 28, 105}, {8, 9, 66}
                };
                
                // Sky gradient: dark blue to black
                SCL.push_back(gfx.create_pen(25, 25, 112));    // Midnight blue
                SCL.push_back(gfx.create_pen(72, 61, 139));    // Dark slate blue
                SCL.push_back(gfx.create_pen(47, 79, 79));     // Dark slate grey
                SCL.push_back(gfx.create_pen(0, 0, 0));        // Black
                break;
                
            case STARRYNIGHT:
                hillHeight = 8;
                bTrees = true;
                bBushes = true;
                shwStrLgts = true;
                bSigns = true;
                bClouds = false;
                displayTime = true;
                bSun = false;
                bMoon = true;
                bStars = true;
                sunSizeMod = 0;
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                
                // Start rain with 60 second timer
                if (rainTimer == 0) {
                    rain = true;
                    rainTimer = time_us_64();
                }
                
                grassColour1 = gfx.create_pen(0, 60, 0);       // Very dark green
                grassColour2 = gfx.create_pen(0, 40, 0);       // Even darker green
                edgeCol = gfx.create_pen(120, 120, 120);       // Medium grey
                edgeCol2 = gfx.create_pen(80, 80, 80);         // Dark grey
                
                hillColours = {
                    {132, 77, 163}, {102, 59, 148}, {67, 28, 118},
                    {34, 28, 105}, {8, 9, 66}
                };
                
                // Sky gradient: deep purple to black with stars
                SCL.push_back(gfx.create_pen(75, 0, 130));     // Indigo
                SCL.push_back(gfx.create_pen(72, 61, 139));    // Dark slate blue
                SCL.push_back(gfx.create_pen(25, 25, 112));    // Midnight blue
                SCL.push_back(gfx.create_pen(0, 0, 0));        // Black
                break;
                
            case VICE:
                hillHeight = 3;
                bTrees = false;
                bBushes = false;
                shwStrLgts = true;
                bSigns = true;
                bClouds = false;
                displayTime = true;
                rain = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 0;
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                
                grassColour1 = gfx.create_pen(255, 20, 147);   // Deep pink
                grassColour2 = gfx.create_pen(255, 0, 255);    // Magenta
                edgeCol = gfx.create_pen(0, 255, 255);         // Cyan
                edgeCol2 = gfx.create_pen(255, 255, 0);        // Yellow
                
                // Keep default green hill colors for Vice theme
                hillColours = {
                    {42, 170, 138}, {26, 187, 43}, {50, 205, 50}, 
                    {1, 50, 32}, {150, 255, 150}, {71, 135, 120}
                };
                
                // Sky gradient: pink to purple
                SCL.push_back(gfx.create_pen(255, 20, 147));   // Deep pink
                SCL.push_back(gfx.create_pen(199, 21, 133));   // Medium violet red
                SCL.push_back(gfx.create_pen(128, 0, 128));    // Purple
                SCL.push_back(gfx.create_pen(75, 0, 130));     // Indigo
                break;
                
            case DESERT:
                hillHeight = 0;  // Flat desert
                bTrees = false;
                bBushes = false;
                shwStrLgts = false;
                bSigns = false;
                bClouds = true;
                displayTime = true;
                rain = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 4;
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                
                grassColour1 = gfx.create_pen(238, 203, 173);  // Navajo white (sand)
                grassColour2 = gfx.create_pen(222, 184, 135);  // Burlywood (sand)
                edgeCol = gfx.create_pen(160, 82, 45);         // Saddle brown
                edgeCol2 = gfx.create_pen(210, 180, 140);      // Tan
                
                hillColours = {
                    {243, 112, 49}, {247, 167, 65}, {239, 222, 99},
                    {197, 153, 96}, {145, 44, 12}
                };
                
                // Sky gradient: orange to yellow (sunset)
                SCL.push_back(gfx.create_pen(255, 165, 0));    // Orange
                SCL.push_back(gfx.create_pen(255, 140, 0));    // Dark orange
                SCL.push_back(gfx.create_pen(255, 215, 0));    // Gold
                SCL.push_back(gfx.create_pen(255, 255, 224));  // Light yellow
                break;
                
            case DAYTOO:
                hillHeight = 8;
                bTrees = true;
                bBushes = true;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 0;
                shwStrLgts = false;
                bSigns = false;
                bClouds = true;
                displayTime = true;
                rain = false;
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                
                grassColour1 = gfx.create_pen(50, 205, 50);    // Lime green
                grassColour2 = gfx.create_pen(34, 139, 34);    // Forest green
                edgeCol = gfx.create_pen(255, 255, 255);       // White
                edgeCol2 = gfx.create_pen(220, 220, 220);      // Light grey
                
                hillColours = {
                    {42, 170, 138}, {26, 187, 43}, {50, 205, 50}, 
                    {1, 50, 32}, {150, 255, 150}, {71, 135, 120}
                };
                
                // Sky gradient: bright blue to white
                SCL.push_back(gfx.create_pen(0, 191, 255));    // Deep sky blue
                SCL.push_back(gfx.create_pen(135, 206, 250));  // Light sky blue
                SCL.push_back(gfx.create_pen(176, 224, 230));  // Powder blue
                SCL.push_back(gfx.create_pen(240, 248, 255));  // Alice blue
                break;
                
            case SNOW:
                hillHeight = 8;
                bTrees = true;
                bBushes = false;
                shwStrLgts = false;
                bSigns = false;
                bClouds = true;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 6;
                displayTime = true;
                rain = false;
                
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                grassColour1 = gfx.create_pen(255, 250, 250);  // Snow white
                grassColour2 = gfx.create_pen(220, 220, 220);  // Gainsboro
                edgeCol = gfx.create_pen(169, 169, 169);       // Dark grey
                edgeCol2 = gfx.create_pen(192, 192, 192);      // Silver
                
                hillColours = {
                    {106, 112, 114}, {92, 103, 106}, {46, 70, 78},
                    {46, 74, 82}, {255, 255, 255}
                };
                
                // Sky gradient: grey to white (overcast)
                SCL.push_back(gfx.create_pen(128, 128, 128));  // Grey
                SCL.push_back(gfx.create_pen(169, 169, 169));  // Dark grey
                SCL.push_back(gfx.create_pen(211, 211, 211));  // Light grey
                SCL.push_back(gfx.create_pen(248, 248, 255));  // Ghost white
                break;
                
            case F32:
                hillHeight = 8;
                bTrees = true;
                bBushes = true;
                shwStrLgts = true;
                bSigns = true;
                bClouds = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 6;
                displayTime = true;
                rain = false;
                
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                grassColour1 = gfx.create_pen(85, 107, 47);    // Dark olive green
                grassColour2 = gfx.create_pen(107, 142, 35);   // Olive drab
                edgeCol = gfx.create_pen(105, 105, 105);       // Dim grey
                edgeCol2 = gfx.create_pen(128, 128, 128);      // Grey
                
                hillColours = {
                    {50, 50, 55}, {60, 60, 105}, {100, 100, 120}, {115, 115, 145}, {115, 120, 155}
                };
                
                // Sky gradient: military green to grey
                SCL.push_back(gfx.create_pen(85, 107, 47));    // Dark olive green
                SCL.push_back(gfx.create_pen(107, 142, 35));   // Olive drab
                SCL.push_back(gfx.create_pen(128, 128, 128));  // Grey
                SCL.push_back(gfx.create_pen(169, 169, 169));  // Dark grey
                break;
                
            case RED:
                hillHeight = 2;
                bTrees = false;
                bBushes = true;
                shwStrLgts = false;
                bSigns = false;
                bClouds = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                sunSizeMod = 0;
                displayTime = true;
                rain = false;
                
                sunCol1 = gfx.create_pen(255, 200, 0);
                sunCol2 = gfx.create_pen(250, 150, 0);
                grassColour1 = gfx.create_pen(139, 0, 0);      // Dark red
                grassColour2 = gfx.create_pen(178, 34, 34);    // Fire brick
                edgeCol = gfx.create_pen(255, 99, 71);         // Tomato
                edgeCol2 = gfx.create_pen(255, 69, 0);         // Orange red
                
                hillColours = {
                    {156, 0, 1}, {126, 24, 7}, {94, 18, 3}, {74, 15, 0}, {55, 0, 0}
                };
                
                // Sky gradient: red to black
                SCL.push_back(gfx.create_pen(220, 20, 60));    // Crimson
                SCL.push_back(gfx.create_pen(178, 34, 34));    // Fire brick
                SCL.push_back(gfx.create_pen(139, 0, 0));      // Dark red
                SCL.push_back(gfx.create_pen(0, 0, 0));        // Black
                break;
                
            case CYBER:
                hillHeight = 6;
                bTrees = false;
                bBushes = true;
                shwStrLgts = true;
                bSigns = true;
                bSun = false;
                bMoon = false;
                bStars = true;
                bClouds = false;
                sunSizeMod = 2;
                
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                grassColour1 = gfx.create_pen(0, 50, 100);
                grassColour2 = gfx.create_pen(0, 100, 150);
                edgeCol = gfx.create_pen(0, 255, 255);         // Cyan
                edgeCol2 = gfx.create_pen(255, 0, 255);        // Magenta
                
                hillColours = {
                    {0, 100, 150}, {0, 150, 200}, {0, 200, 255}, {50, 150, 255}, {100, 200, 255}
                };
                
                // Dark blue to electric blue sky gradient
                SCL.push_back(gfx.create_pen(0, 0, 50));       // Very dark blue
                SCL.push_back(gfx.create_pen(0, 50, 100));     // Dark blue
                SCL.push_back(gfx.create_pen(0, 100, 200));    // Electric blue
                SCL.push_back(gfx.create_pen(0, 150, 255));    // Bright cyan
                break;
                
            case SUNSET:
                hillHeight = 7;
                bTrees = true;
                bBushes = true;
                shwStrLgts = false;
                bSigns = true;
                bSun = true;
                bMoon = false;
                bStars = false;
                bClouds = true;
                
                sunCol1 = gfx.create_pen(255, 255, 0);
                sunCol2 = gfx.create_pen(255, 200, 0);
                grassColour1 = gfx.create_pen(100, 80, 0);
                grassColour2 = gfx.create_pen(150, 120, 50);
                edgeCol = gfx.create_pen(255, 100, 0);         // Orange
                edgeCol2 = gfx.create_pen(255, 200, 100);      // Light orange
                
                hillColours = {
                    {200, 100, 50}, {255, 150, 100}, {255, 200, 150}, {255, 180, 120}, {200, 120, 80}
                };
                
                // Sunset sky gradient from orange to purple
                SCL.push_back(gfx.create_pen(255, 150, 50));   // Orange
                SCL.push_back(gfx.create_pen(255, 100, 100));  // Orange-red
                SCL.push_back(gfx.create_pen(200, 50, 150));   // Purple-red
                SCL.push_back(gfx.create_pen(100, 0, 100));    // Dark purple
                break;
                
            case OCEAN:
                hillHeight = 5;
                bTrees = false;
                bBushes = true;
                shwStrLgts = false;
                bSigns = false;
                bSun = true;
                bMoon = false;
                bStars = false;
                bClouds = true;
                
                sunSizeMod = 4;
                sunCol1 = gfx.create_pen(0, 155, 255);
                sunCol2 = gfx.create_pen(0, 200, 255);
                grassColour1 = gfx.create_pen(0, 100, 100);
                grassColour2 = gfx.create_pen(0, 150, 150);
                edgeCol = gfx.create_pen(0, 200, 200);         // Aqua
                edgeCol2 = gfx.create_pen(100, 255, 255);      // Light aqua
                
                hillColours = {
                    {0, 50, 100}, {0, 80, 150}, {0, 120, 200}, {50, 150, 255}, {100, 200, 255}
                };
                
                // Ocean sky gradient from light blue to deep blue
                SCL.push_back(gfx.create_pen(135, 206, 250));  // Sky blue
                SCL.push_back(gfx.create_pen(70, 130, 180));   // Steel blue
                SCL.push_back(gfx.create_pen(25, 25, 112));    // Midnight blue
                SCL.push_back(gfx.create_pen(0, 0, 139));      // Dark blue
                break;
                
            case NEON:
                hillHeight = 4;
                bTrees = false;
                bBushes = true;
                shwStrLgts = true;
                bSigns = true;
                bSun = false;
                bMoon = false;
                bStars = true;
                bClouds = false;
                sunSizeMod = 3;
                
                sunCol1 = gfx.create_pen(255, 205, 0);
                sunCol2 = gfx.create_pen(205, 200, 0);
                grassColour1 = gfx.create_pen(100, 0, 100);
                grassColour2 = gfx.create_pen(200, 0, 200);
                edgeCol = gfx.create_pen( 0, 255, 0);         
                edgeCol2 = gfx.create_pen(0, 255, 0);          // Neon green
                
                hillColours = {
                    {255, 0, 150}, {150, 255, 0}, {255, 255, 0}, {255, 100, 200}, {200, 255, 100}
                };
                
                // Dark to neon sky gradient
                SCL.push_back(gfx.create_pen(0, 0, 0));        // Black
                SCL.push_back(gfx.create_pen(50, 0, 50));      // Dark purple
                SCL.push_back(gfx.create_pen(100, 0, 100));    // Purple
                SCL.push_back(gfx.create_pen(255, 0, 255));    // Bright magenta
                break;
                
            case CITYSCAPE:
                hillHeight = 0;    // No hills, just buildings
                bTrees = false;    // No trees in the city
                bBushes = false;   // No bushes, we'll use buildings instead
                shwStrLgts = true; // Plenty of street lights
                bSigns = true;     // Lots of city signage
                bSun = true;
                bMoon = false;
                bStars = false;
                bClouds = false;
                
                sunCol1 = gfx.create_pen(255, 155, 0);
                sunCol2 = gfx.create_pen(255, 100, 0);
                grassColour1 = gfx.create_pen(40, 40, 40);     // Dark asphalt
                grassColour2 = gfx.create_pen(60, 60, 60);     // Concrete sidewalk
                edgeCol = gfx.create_pen(255, 255, 0);         // Yellow road lines
                edgeCol2 = gfx.create_pen(255, 255, 0);      // White road markings
                
                // Urban building colors - concrete greys and glass blues
                hillColours = {
                    {80, 80, 90}, {100, 100, 110}, {60, 60, 70}, {120, 120, 130}, {90, 90, 100}, {140, 140, 150}
                };
                
                // Twilight cityscape sky - sunset behind buildings
                SCL.push_back(gfx.create_pen(255, 165, 0));    // Orange
                SCL.push_back(gfx.create_pen(255, 69, 0));     // Orange red
                SCL.push_back(gfx.create_pen(139, 0, 139));    // Dark magenta
                SCL.push_back(gfx.create_pen(25, 25, 112));    // Midnight blue
                break;
        }
        
        // Update mountain palette with new theme colors
        if (mountain) {
            mountain->updatePalette(hillColours);
        }
    }
    
    std::vector<Theme> getThemes() {
        return { DAY, NIGHT, STARRYNIGHT, VICE, DESERT, DAYTOO, SNOW, F32, RED, CYBER, SUNSET, OCEAN, NEON, CITYSCAPE };
    }
    
    void nextTheme() {
        int current = (int)currentTheme;
        current = (current + 1) % 14;  // 14 total themes
        setTheme((Theme)current);
    }
    
    void updateAutoThemeChange() {
        // Track distance traveled since last theme change
        distanceSinceThemeChange += speed * elapsedTime;
        
        // Check if it's time to change theme (based on distance traveled)
        if (distanceSinceThemeChange >= AUTO_THEME_DISTANCE) {
            nextTheme();
            distanceSinceThemeChange = 0.0f;
            lastThemeChange = time_us_64();
        }
    }
    
    void triggerTunnel() {
        if (!inTunnel && currentTheme == DAY) {  // Only in DAY theme for now
            inTunnel = true;
            tunnelProgress = 0.0f;
            tunnelStartTime = time_us_64();
        }
    }
    
    void updateTunnel() {
        if (inTunnel) {
            uint32_t elapsed = time_us_64() - tunnelStartTime;
            float progress = (float)elapsed / tunnelDuration;
            
            if (progress >= 1.0f) {
                // Tunnel complete
                inTunnel = false;
                tunnelProgress = 0.0f;
            } else {
                // Update tunnel progress (fade in/out)
                if (progress < 0.2f) {
                    // Fade in (first 20%)
                    tunnelProgress = progress / 0.2f;
                } else if (progress > 0.8f) {
                    // Fade out (last 20%)
                    tunnelProgress = (1.0f - progress) / 0.2f;
                } else {
                    // Full tunnel (middle 60%)
                    tunnelProgress = 1.0f;
                }
            }
        }
    }
    
    void updateRain() {
        // Check rain timer (60 seconds)
        if (rain && rainTimer > 0) {
            uint32_t elapsed = time_us_64() - rainTimer;
            if (elapsed > 60000000) {  // 60 seconds in microseconds
                rain = false;
                rainTimer = 0;
            }
        }
        
        // Random rain events for appropriate themes
        if (!rain && (currentTheme == NIGHT || currentTheme == STARRYNIGHT)) {
            int j = rand() % 1000;
            if (j == 1) {  // Very rare random chance
                rain = true;
                rainTimer = time_us_64();
            }
        }
    }
    
    void spawnScenery() {
        uint32_t current_time = time_us_64();
        
        // Spawn scenery every 0.5-2 seconds randomly (more frequent for testing)
        if (current_time - lastScenerySpawn > (uint32_t)(500000 + rand() % 1500000)) {
            
            // Find inactive scenery object
            for (auto& obj : sceneryObjects) {
                if (!obj.active) {
                    SceneryObject::Type type = SceneryObject::TREE;
                    
                    // Choose type based on theme with much more variety
                    if (currentTheme == VICE) {
                        int cityChoice = rand() % 5;
                        switch (cityChoice) {
                            case 0: type = SceneryObject::SKYSCRAPER; break;
                            case 1: type = SceneryObject::OFFICE_TOWER; break;
                            case 2: type = SceneryObject::BUILDING; break;
                            case 3: type = SceneryObject::BILLBOARD; break;
                            case 4: type = SceneryObject::RADIO_TOWER; break;
                            case 5: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::CACTUS : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 6: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;

                        }
                    } else if (currentTheme == DESERT) {
                        int desertChoice = rand() % 10;
                        switch (desertChoice) {
                            case 0: type = SceneryObject::CACTUS; break;
                            case 1: type = SceneryObject::WIND_TURBINE; break;
                            case 2: type = SceneryObject::RADIO_TOWER; break;
                            case 3: type = SceneryObject::MONUMENT; break;
                            case 4: type = SceneryObject::WATER_TOWER; break;
                            case 5: type = SceneryObject::BILLBOARD; break;
                            case 6: type = SceneryObject::PYRAMID; break;
                            case 7: type = SceneryObject::PYRAMID; break; // Higher chance for pyramids
                            case 8: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::CACTUS : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 9: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;
                        }
                    } else if (currentTheme == DAY || currentTheme == DAYTOO) {
                        int ruralChoice = rand() % 10;
                        switch (ruralChoice) {
                            case 0: type = SceneryObject::TREE; break;
                            case 1: type = SceneryObject::BUSH; break;
                            case 2: type = SceneryObject::STREETLIGHT; break;
                            case 3: type = SceneryObject::BUILDING; break;
                            case 4: type = SceneryObject::BARN; break;
                            case 5: type = SceneryObject::WINDMILL; break;
                            case 6: type = SceneryObject::WATER_TOWER; break;
                            case 7: type = SceneryObject::CHURCH; break;
                            case 8: type = SceneryObject::BILLBOARD; break;
                            case 9: type = SceneryObject::PALM_TREE; break;
                            case 10: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::CACTUS : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 11: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;

                        }
                    } else if (currentTheme == F32) {
                        int militaryChoice = rand() % 7;
                        switch (militaryChoice) {
                            case 0: type = SceneryObject::RADIO_TOWER; break;
                            case 1: type = SceneryObject::FACTORY; break;
                            case 2: type = SceneryObject::WATER_TOWER; break;
                            case 3: type = SceneryObject::BUSH; break;
                            case 4: type = SceneryObject::WIND_TURBINE; break;
                            case 5: type = SceneryObject::BILLBOARD; break;
                            case 6: type = SceneryObject::BUILDING; break;
                            case 7: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::CACTUS : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 8: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;

                        }
                    } else if (currentTheme == RED) {
                        int redChoice = rand() % 8;
                        switch (redChoice) {
                            case 0: type = SceneryObject::VOLCANO; break;
                            case 1: type = SceneryObject::VOLCANO; break; // Higher chance for volcanoes
                            case 2: type = SceneryObject::RADIO_TOWER; break;
                            case 3: type = SceneryObject::FACTORY; break;
                            case 4: type = SceneryObject::BUILDING; break;
                            case 5: type = SceneryObject::BILLBOARD; break;
                            case 6: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::VOLCANO : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 7: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;
                        }
                    } else if (currentTheme == SNOW) {
                        int winterChoice = rand() % 8;
                        switch (winterChoice) {
                            case 0: type = SceneryObject::TREE; break;
                            case 1: type = SceneryObject::CHURCH; break;
                            case 2: type = SceneryObject::BARN; break;
                            case 3: type = SceneryObject::WINDMILL; break;
                            case 4: type = SceneryObject::CLOCK_TOWER; break;
                            case 5: type = SceneryObject::WATER_TOWER; break;
                            case 6: type = SceneryObject::FACTORY; break;
                            case 7: type = SceneryObject::WIND_TURBINE; break;
                        }
                    } else {
                        // Enhanced variety for other themes
                        int generalChoice = rand() % 15;
                        switch (generalChoice) {
                            case 0: type = SceneryObject::TREE; break;
                            case 1: type = SceneryObject::BUSH; break;
                            case 2: type = SceneryObject::STREETLIGHT; break;
                            case 3: type = SceneryObject::BUILDING; break;
                            case 4: type = SceneryObject::WATER_TOWER; break;
                            case 5: type = SceneryObject::RADIO_TOWER; break;
                            case 6: type = SceneryObject::BILLBOARD; break;
                            case 7: type = SceneryObject::FACTORY; break;
                            case 8: type = SceneryObject::CHURCH; break;
                            case 9: type = SceneryObject::BARN; break;
                            case 10: type = SceneryObject::WINDMILL; break;
                            case 11: type = SceneryObject::CLOCK_TOWER; break;
                            case 12: type = SceneryObject::PALM_TREE; break;
                            case 13: type = SceneryObject::WIND_TURBINE; break;
                            case 14: type = SceneryObject::MONUMENT; break;
                            case 15: 
                                // Only spawn TUNNEL_INTRO if not already in tunnel
                                type = inTunnel ? SceneryObject::CACTUS : SceneryObject::TUNNEL_INTRO; 
                                break; 
                            case 16: 
                                // TUNNEL_OUTRO can spawn regardless of tunnel state (to allow exiting)
                                type = SceneryObject::TUNNEL_OUTRO; 
                                break;


                        }
                    }
                    
                    // Much wider spread positioning - objects can spawn much further from road
                    float side = (rand() % 2 == 0) ? -1.0f : 1.0f; // Left or right side
                    float base_distance = 0.6f + (rand() % 80) * 0.01f; // 0.6 to 1.4 from road center
                    float track_pos = side * base_distance;
                    
                    // Add some random variation for more natural placement
                    track_pos += (rand() % 40 - 20) * 0.005f; // ±0.1 variation
                    
                    // Start objects at varying distances for depth (spawn on horizon)
                    float start_distance = 0.1f + (rand() % 10) * 0.1f;  // 0.1-1.0 distance units (far horizon)
                    
                    obj.spawn(type, track_pos, start_distance);
                    lastScenerySpawn = current_time;
                    break;
                }
            }
        }
    }
    
    void spawnOncomingCar() {
        uint32_t current_time = time_us_64();
        
        // Spawn cars every 1-4 seconds randomly (more frequent for testing)
        if (current_time - lastCarSpawn > (uint32_t)(1000000 + rand() % 3000000)) {
            
            // Find inactive car
            for (auto& car : oncomingCars) {
                if (!car.active) {
                    // Random track position
                    float track_pos = -0.6f + (float)(rand() % 120) / 100.0f;  // -0.6 to 0.6
                    
                    car.spawn(track_pos);
                    lastCarSpawn = current_time;
                    break;
                }
            }
        }
    }
    
    void update() {
        frameCount++;
        
        // Update road geometry using original pattern
        distance += speed;
        sectionDistance += speed * elapsedTime;
        
        // Generate curves using sine waves like original
        roadcurve = sin(frameCount * 0.02f) * 2.0f;
        roadhill = sin(frameCount * 0.015f) * 1.5f;
        
        // Update curvature tracking like original
        float ftrackcurvediff = (roadcurve - pCurvature) * elapsedTime;
        pCurvature += ftrackcurvediff;
        tCurvature += round(((roadcurve) * sectionDistance) * 0.001f) * (speed * 0.01f);
        
        float ftrackhill = (roadhill - pHillCurvature) * elapsedTime;
        pHillCurvature += ftrackhill;
        tHillCurvature += round(((roadhill) * sectionDistance) * 0.001f) * (speed * 0.01f);
        
        // Update tunnel system
        updateTunnel();
        
        // Update rain system
        updateRain();
        
        // Update automatic theme changing
        updateAutoThemeChange();
        
        // Update mountain/hills using original pattern
        mountain->generatePointCloud(-tCurvature, hillHeight, 15);
        
        // Spawn and update scenery
        spawnScenery();
        for (auto& obj : sceneryObjects) {
            obj.update(speed, roadcurve, roadhill, h,
                [this]() { // enterTunnel callback
                    if (!inTunnel) {
                        inTunnel = true;
                        tunnelProgress = 0.0f;
                        tunnelStartTime = time_us_64();
                    }
                },
                [this]() { // exitTunnel callback
                    inTunnel = false;
                    tunnelProgress = 0.0f;
                }
            );
        }
        
        // Spawn and update oncoming cars
        spawnOncomingCar();
        for (auto& car : oncomingCars) {
            car.update(speed, roadcurve, roadhill, h);
        }
    }
    
    bool checkCollisions(const Car& player_car) {
        float player_pos = player_car.getTrackPosition();
        
        // Check player-car collisions
        for (auto& car : oncomingCars) {
            if (car.checkCollisionWithPlayer(player_pos)) {
                return true;
            }
        }
        
        // Check car-to-car collisions and apply bounce effects
        for (size_t i = 0; i < oncomingCars.size(); i++) {
            for (size_t j = i + 1; j < oncomingCars.size(); j++) {
                if (oncomingCars[i].checkCollisionWithCar(oncomingCars[j])) {
                    // Apply bounce effects to both cars
                    float bounce_direction = (oncomingCars[i].trackPosition < oncomingCars[j].trackPosition) ? -1.0f : 1.0f;
                    oncomingCars[i].applyCollisionBounce(bounce_direction);
                    oncomingCars[j].applyCollisionBounce(-bounce_direction);
                }
            }
        }
        
        return false;
    }
    
    void draw(const Car& player_car) {
        // Clear screen
        gfx.set_pen(0, 0, 0);
        gfx.clear();
        
        // Draw sky gradient
        drawSky();
        
        // Draw mountains/hills (behind road)
        if (!mountain->greens.empty()) {
            mountain->drawMountains(mountain->greens[2]);
        }
        
        // Draw road surface
        drawRoad();
        
        // Draw scenery objects
        for (auto& obj : sceneryObjects) {
            obj.draw(gfx, w, h, roadcurve, roadhill);
        }
        
        // Draw oncoming cars
        for (auto& car : oncomingCars) {
            car.draw(gfx, w, h, roadcurve, roadhill);
        }
        
        // Draw rain if enabled
        if (rain) {
            Pen rainPen = gfx.create_pen(100, 100, 255);
            rainSystem->draw(gfx, rainPen);
        }
        
        // Apply tunnel effect if in tunnel
        if (inTunnel && tunnelProgress > 0) {
            drawTunnel();
        }
    }
    
private:
    void drawSky() {
        if (SCL.empty()) return;
        
        // Draw sky gradient from top to middle of screen
        int skyHeight = h / 2;
        int bandsPerColor = std::max(1, skyHeight / (int)SCL.size());
        
        for (int y = 0; y < skyHeight; y++) {
            int colorIndex = std::min((int)SCL.size() - 1, y / bandsPerColor);
            gfx.set_pen(SCL[colorIndex]);
            
            for (int x = 0; x < w; x++) {
                gfx.pixel(Point(x, y));
            }
        }
        
        // Draw sun/moon
        int sunpos = abs(w / 3 - (int)(round(-tCurvature * 0.01f) + w));
        sunpos = (sunpos % 48) - 8;
        
        if (bSun) {
            int suny = h / 3;
            int suny2 = h / 2;
            int sunyMod = 6;
            
            // Adjust sun position for desert theme
            if (currentTheme == DESERT) {
                suny -= sunyMod;
                suny2 -= sunyMod;
            }
            
            gfx.set_pen(sunCol1);
            gfx.circle(Point(sunpos, suny), 8 - sunSizeMod);
            if (SCL.size() > 1 && currentTheme != DESERT) {
                gfx.set_pen(sunCol2);
                gfx.circle(Point(sunpos, suny2), 6 - sunSizeMod);
            }
            
            // Draw lines over the sun like in original version - these are the stylized stripes!
            gfx.set_pen(SCL[std::min(5, (int)SCL.size() - 1)]);
            for (int p = h/4 - sunyMod/2; p < w/2; p++) {
                if (p % 2 != 0) {
                    gfx.line(Point(0, p), Point(w, p));
                }
            }
        }
        
        if (bMoon) {
            gfx.set_pen(white);
            gfx.circle(Point(sunpos, 5), 4);
        }
        
        // Draw stars for night themes
        if (bStars && (currentTheme == STARRYNIGHT || currentTheme == NIGHT)) {
            drawStars();
        }
    }
    
    void drawStars() {
        gfx.set_pen(255, 255, 255);  // White stars
        
        // Draw some random-looking but consistent stars
        for (int i = 0; i < 20; i++) {
            int x = (i * 7 + 13) % w;
            int y = (i * 11 + 7) % (h / 3);  // Only in upper third
            if ((x + y) % 5 == 0) {  // Make it less dense
                gfx.pixel(Point(x, y));
            }
        }
    }
    
    void drawRoad() {
        // Draw road from middle of screen to bottom
        int roadStartY = h / 2;
        
        for (int y = roadStartY; y < h; y++) {
            float perspective = (float)(y - roadStartY) / (h/2);
            
            // Road curvature and hills - using original calculation
            float middlepoint = 0.5f + (roadcurve/10.0f) * pow(1-perspective, 3);
            float hillpoint = (roadhill/4.0f) * pow(1-perspective, 2);
            
            // Road width varies with perspective
            float roadwidth = 0.1f + perspective * 0.8f;
            int left_x = (int)(w * (middlepoint - roadwidth/2));
            int right_x = (int)(w * (middlepoint + roadwidth/2));
            
            // Adjust y position based on hills
            int adjusted_y = y + (int)hillpoint;
            if (adjusted_y >= h) adjusted_y = h - 1;
            if (adjusted_y < roadStartY) adjusted_y = roadStartY;
            
            // Draw grass on both sides with depth lighting
            float brightness = 0.2f + 0.8f * perspective;
            
            // Extract RGB values based on theme
            uint8_t grass1_r = 0, grass1_g = 255, grass1_b = 0;  // Default green
            uint8_t grass2_r = 0, grass2_g = 200, grass2_b = 0;  // Default darker green
            
            // Set grass colors based on current theme
            switch (currentTheme) {
                case DAY:
                case DAYTOO:
                    grass1_r = 0; grass1_g = 255; grass1_b = 0;   // Bright green
                    grass2_r = 0; grass2_g = 200; grass2_b = 0;   // Medium green
                    break;
                case NIGHT:
                    grass1_r = 0; grass1_g = 100; grass1_b = 0;   // Dark green
                    grass2_r = 0; grass2_g = 80; grass2_b = 0;    // Darker green
                    break;
                case STARRYNIGHT:
                    grass1_r = 0; grass1_g = 60; grass1_b = 0;    // Very dark green
                    grass2_r = 0; grass2_g = 40; grass2_b = 0;    // Even darker green
                    break;
                case VICE:
                    grass1_r = 255; grass1_g = 20; grass1_b = 147; // Deep pink
                    grass2_r = 255; grass2_g = 0; grass2_b = 255;  // Magenta
                    break;
                case DESERT:
                    grass1_r = 238; grass1_g = 203; grass1_b = 173; // Navajo white (sand)
                    grass2_r = 222; grass2_g = 184; grass2_b = 135; // Burlywood (sand)
                    break;
                case SNOW:
                    grass1_r = 255; grass1_g = 250; grass1_b = 250; // Snow white
                    grass2_r = 220; grass2_g = 220; grass2_b = 220; // Gainsboro
                    break;
                case F32:
                    grass1_r = 85; grass1_g = 107; grass1_b = 47;   // Dark olive green
                    grass2_r = 107; grass2_g = 142; grass2_b = 35;  // Olive drab
                    break;
                case RED:
                    grass1_r = 139; grass1_g = 0; grass1_b = 0;     // Dark red
                    grass2_r = 178; grass2_g = 34; grass2_b = 34;   // Fire brick
                    break;
                case CYBER:
                    grass1_r = 0; grass1_g = 255; grass1_b = 127;   // Spring green
                    grass2_r = 0; grass2_g = 128; grass2_b = 128;   // Teal
                    break;
                case SUNSET:
                    grass1_r = 100; grass1_g = 80; grass1_b = 0;    // Dark yellow-brown
                    grass2_r = 150; grass2_g = 120; grass2_b = 50;  // Light brown
                    break;
                case OCEAN:
                    grass1_r = 0; grass1_g = 100; grass1_b = 100;   // Dark teal
                    grass2_r = 0; grass2_g = 150; grass2_b = 150;   // Medium teal
                    break;
                case NEON:
                    grass1_r = 255; grass1_g = 0; grass1_b = 255;   // Magenta
                    grass2_r = 75; grass2_g = 0; grass2_b = 130;    // Indigo
                    break;
                case CITYSCAPE:
                    grass1_r = 40; grass1_g = 40; grass1_b = 40;    // Dark asphalt
                    grass2_r = 60; grass2_g = 60; grass2_b = 60;    // Concrete sidewalk
                    break;
            }
            
            Pen darkGrass1 = createDarkenedPen(grass1_r, grass1_g, grass1_b, brightness);
            Pen darkGrass2 = createDarkenedPen(grass2_r, grass2_g, grass2_b, brightness);
            
            // Alternating grass pattern with speed-responsive movement like original
            float grass_frequency = 20.0f * pow(1.0f - perspective, 3);
            float grass_movement = distance * 0.01f * (1.0f + speed * 0.02f);
            bool useGrass1 = sin(grass_frequency + grass_movement) > 0;
            Pen grassPen = useGrass1 ? darkGrass1 : darkGrass2;
            
            gfx.set_pen(grassPen);
            
            // Left grass
            for (int x = 0; x < left_x; x++) {
                gfx.pixel(Point(x, adjusted_y));
            }
            
            // Right grass  
            for (int x = right_x; x < w; x++) {
                gfx.pixel(Point(x, adjusted_y));
            }
            
            // Draw road surface (grey)
            Pen roadPen = gfx.create_pen((int)(50 * brightness), (int)(50 * brightness), (int)(50 * brightness));
            gfx.set_pen(roadPen);
            for (int x = left_x; x < right_x; x++) {
                gfx.pixel(Point(x, adjusted_y));
            }
            
            // Draw road edges with depth lighting
            uint8_t edge1_r = 255, edge1_g = 255, edge1_b = 255; // Default white
            uint8_t edge2_r = 200, edge2_g = 200, edge2_b = 200; // Default light grey
            
            // Set edge colors based on current theme
            switch (currentTheme) {
                case DAY:
                case DAYTOO:
                    edge1_r = 255; edge1_g = 255; edge1_b = 255; // White
                    edge2_r = 200; edge2_g = 200; edge2_b = 200; // Light grey
                    break;
                case NIGHT:
                    edge1_r = 150; edge1_g = 150; edge1_b = 150; // Grey
                    edge2_r = 100; edge2_g = 100; edge2_b = 100; // Dark grey
                    break;
                case STARRYNIGHT:
                    edge1_r = 120; edge1_g = 120; edge1_b = 120; // Medium grey
                    edge2_r = 80; edge2_g = 80; edge2_b = 80;    // Dark grey
                    break;
                case VICE:
                    edge1_r = 0; edge1_g = 255; edge1_b = 255;   // Cyan
                    edge2_r = 255; edge2_g = 255; edge2_b = 0;   // Yellow
                    break;
                case DESERT:
                    edge1_r = 160; edge1_g = 82; edge1_b = 45;   // Saddle brown
                    edge2_r = 210; edge2_g = 180; edge2_b = 140; // Tan
                    break;
                case SNOW:
                    edge1_r = 169; edge1_g = 169; edge1_b = 169; // Dark grey
                    edge2_r = 192; edge2_g = 192; edge2_b = 192; // Silver
                    break;
                case F32:
                    edge1_r = 105; edge1_g = 105; edge1_b = 105; // Dim grey
                    edge2_r = 128; edge2_g = 128; edge2_b = 128; // Grey
                    break;
                case RED:
                    edge1_r = 255; edge1_g = 99; edge1_b = 71;   // Tomato
                    edge2_r = 255; edge2_g = 69; edge2_b = 0;    // Orange red
                    break;
                case CYBER:
                    edge1_r = 0; edge1_g = 255; edge1_b = 255;   // Cyan
                    edge2_r = 255; edge2_g = 0; edge2_b = 255;   // Magenta
                    break;
                case SUNSET:
                    edge1_r = 255; edge1_g = 100; edge1_b = 0;   // Orange
                    edge2_r = 255; edge2_g = 200; edge2_b = 100; // Light orange
                    break;
                case OCEAN:
                    edge1_r = 0; edge1_g = 200; edge1_b = 200;   // Aqua
                    edge2_r = 100; edge2_g = 255; edge2_b = 255; // Light aqua
                    break;
                case NEON:
                    edge1_r = 255; edge1_g = 0; edge1_b = 255;   // Magenta
                    edge2_r = 0; edge2_g = 255; edge2_b = 0;     // Neon green
                    break;
                case CITYSCAPE:
                    edge1_r = 255; edge1_g = 255; edge1_b = 0;   // Yellow
                    edge2_r = 255; edge2_g = 255; edge2_b = 255; // White
                    break;
            }
            
            Pen darkEdge1 = createDarkenedPen(edge1_r, edge1_g, edge1_b, brightness);
            Pen darkEdge2 = createDarkenedPen(edge2_r, edge2_g, edge2_b, brightness);
            
            if (left_x > 0 && left_x < w) {
                gfx.set_pen(darkEdge1);
                gfx.pixel(Point(left_x, adjusted_y));
            }
            if (right_x > 0 && right_x < w) {
                gfx.set_pen(darkEdge2);
                gfx.pixel(Point(right_x, adjusted_y));
            }
            
            // Draw checkered flag finish line when approaching theme change
            float finishLineDistance = AUTO_THEME_DISTANCE - 200.0f; // Show flag 200 units before finish
            if (distanceSinceThemeChange >= finishLineDistance) {
                // Calculate flag position - appears in distance and moves toward player
                float flagProgress = (distanceSinceThemeChange - finishLineDistance) / 200.0f;
                float flagY = roadStartY + (y - roadStartY) * (1.0f - flagProgress);
                
                // Only draw if flag is at this scanline (with some tolerance)
                if (abs(y - (int)flagY) <= 1) {
                    // Draw checkered pattern across the road width
                    int checkerSize = std::max(1, (int)(4 * perspective)); // Smaller squares at distance
                    bool isBlack = ((left_x / checkerSize + y / checkerSize) % 2) == 0;
                    
                    for (int x = left_x; x < right_x; x++) {
                        // Alternate between black and white every checkerSize pixels
                        if ((x / checkerSize) % 2 != (y / checkerSize) % 2) {
                            isBlack = !isBlack;
                        }
                        
                        Pen flagPen;
                        if (isBlack) {
                            flagPen = gfx.create_pen((int)(0 * brightness), (int)(0 * brightness), (int)(0 * brightness));
                        } else {
                            flagPen = gfx.create_pen((int)(255 * brightness), (int)(255 * brightness), (int)(255 * brightness));
                        }
                        gfx.set_pen(flagPen);
                        gfx.pixel(Point(x, adjusted_y));
                        
                        // Reset for next pixel
                        isBlack = ((x / checkerSize + y / checkerSize) % 2) == 0;
                    }
                }
            }
            
            // Draw center line stripes with depth lighting and speed animation
            int center_x = (int)(w * middlepoint);
            if (center_x > left_x && center_x < right_x) {
                
                // Speed-responsive stripe animation based on distance traveled
                float stripeOffset = distance * 0.1f;
                int stripePattern = ((int)(y + stripeOffset) / 4) % 2;
                
                if (stripePattern == 0) {
                    Pen whitePen = gfx.create_pen((int)(255 * brightness), (int)(255 * brightness), (int)(255 * brightness));
                    gfx.set_pen(whitePen);
                } else {
                    Pen blackPen = gfx.create_pen((int)(20 * brightness), (int)(20 * brightness), (int)(20 * brightness));
                    gfx.set_pen(blackPen);
                }
                gfx.pixel(Point(center_x, adjusted_y));
            }
        }
    }
    
    void drawTunnel() {
        // Draw tunnel: black road, grey walls, yellow center lines
        // First draw road surface, then add tunnel walls
        
        //top half 
        for (int y = 0; y < h / 2; y++) {
            float perspective = (float)y / (h / 2);
            float hillpoint = (roadhill / 4.0f) * pow(1 - perspective, 2);
            
            int nrow = h / 2 + y + (int)hillpoint;
            if (nrow < 0 || nrow >= h) continue;
            
            // Road markers - improved speed-responsive calculation for realistic movement
            // Apply tunnel lighting effect to top half markers
            Pen darkened_yellow = createTunnelDarkenedPen(100, 100, 0, perspective);
            Pen darkened_black = createTunnelDarkenedPen(0, 0, 0, perspective);
            
            // Calculate stripe movement based on speed and distance - more realistic
            float stripe_frequency = 2.0f * pow(1.0f - perspective, 3);
            float speed_multiplier = speed * 0.02f; // Speed affects how fast stripes move
            float stripe_position = stripe_frequency + distance * speed_multiplier;
            
            // Different stripe patterns for different speeds
            Pen roadmarker = (sin(stripe_position * 0.3f) > 0.9f) ? darkened_yellow : darkened_black;
            
            gfx.set_pen(roadmarker);
        }

        //bottom half 
        for (int y = 0; y < h / 2; y++) {
            float perspective = (float)y / (h / 2);
            float middlepoint = 0.5f + (roadcurve / 10.0f) * pow(1 - perspective, 3);
            float hillpoint = (roadhill / 4.0f) * pow(1 - perspective, 2);
            float roadwidth = 0.1f + perspective * 0.80f;
            float clipwidth = roadwidth * 0.3f;
            roadwidth *= 0.6f;

            int leftgrass = (int)((middlepoint - roadwidth - clipwidth) * w);
            int leftclip = (int)((middlepoint - roadwidth) * w);
            int rightclip = (int)((middlepoint + roadwidth) * w);
            int rightgrass = (int)((middlepoint + roadwidth + clipwidth) * w);

            bool bBush = sin(20 * pow(1.0f - perspective, 3) + distance * 0.01f) > 0;
            
            // Apply tunnel lighting effect - darken colors based on perspective
            Pen grassCol, edgeC;
            if (bBush) {
                // Use theme-aware darkening for grassColour1
                grassCol = createDarkenedPenFromTheme(grassColour1, perspective, true, false);
            } else {
                // Use theme-aware darkening for grassColour2
                grassCol = createDarkenedPenFromTheme(grassColour2, perspective, false, false);
            }
            
            // Apply proper edge pattern like main road rendering
            float edgeClipMod = (float)w;
            bool edge_pattern = sin(edgeClipMod * pow(1.0f - perspective, 3) + distance * 0.1f) > 0;
            if (edge_pattern) {
                edgeC = createDarkenedPenFromTheme(edgeCol, perspective, false, true);
            } else {
                edgeC = createDarkenedPenFromTheme(edgeCol2, perspective, false, false);
            }

            int nrow = h / 2 - y - (int)hillpoint;
            int jrow = h / 2 + y;

            // Skip drawing if the road row is outside screen bounds
            if (nrow <= 0 || nrow >= h) continue;

            // Draw road surface
            gfx.set_pen(grassCol);
            gfx.line(Point(0, nrow), Point(w, nrow));

            gfx.set_pen(grassCol);
            gfx.line(Point(rightgrass, nrow), Point(w, nrow));
 
            int edgeDiff = leftclip - leftgrass;
            for (int e = 0; e <= edgeDiff; e++) {
                gfx.line(Point(leftclip - e, nrow-1), Point(leftclip - e, jrow));
                gfx.line(Point(leftgrass - e, nrow-1), Point(leftgrass - e, jrow));
                gfx.line(Point(rightclip +e, nrow-1), Point(rightclip +e, jrow));
                gfx.line(Point(rightgrass +e, nrow-1), Point(rightgrass +e, jrow));
            }
            
            gfx.set_pen(grassCol);
            gfx.line(Point(leftgrass, nrow-1), Point(leftclip, nrow-1));
            gfx.line(Point(rightclip, nrow-1), Point(rightgrass, nrow-1));
            gfx.line(Point(rightclip, nrow-1), Point(rightclip, jrow));
            gfx.line(Point(rightgrass, nrow-1), Point(rightgrass, jrow));
            gfx.line(Point(leftclip, nrow-1), Point(leftclip, jrow));
            gfx.line(Point(leftgrass, nrow-1), Point(leftgrass, jrow));

            // Road markers - improved speed-responsive calculation for realistic movement
            // Apply tunnel lighting effect to yellow markers
            Pen darkened_yellow = createTunnelDarkenedPen(255, 255, 0, perspective);

            // Calculate stripe movement based on speed and distance - more realistic
            float stripe_frequency = 100.0f * pow(1.0f - perspective, 3);
            float speed_multiplier = speed * 0.002f; // Speed affects how fast stripes move
            float stripe_position = stripe_frequency + distance * speed_multiplier;

            Pen roadmarker = (sin(stripe_position) > 0.8f) ? darkened_yellow : edgeC;

            gfx.set_pen(roadmarker);
            int m = (rightgrass - leftgrass) / 4;
            gfx.pixel(Point(leftgrass + m, nrow));
            gfx.pixel(Point(leftgrass + m*3, nrow));
        }
    }
    
    // Helper function for tunnel lighting - always darkens toward distance
    Pen createTunnelDarkenedPen(int r, int g, int b, float perspective) {
        // Traditional tunnel darkening - always darken toward horizon
        // At horizon (perspective = 0): 20% brightness
        // At bottom of screen (perspective = 1): 100% brightness
        float brightness = 0.2f + 0.8f * perspective;
        
        int dark_r = (int)(r * brightness);
        int dark_g = (int)(g * brightness);
        int dark_b = (int)(b * brightness);
        
        return gfx.create_pen(dark_r, dark_g, dark_b);
    }
    
    // Helper function to create darkened pen from existing pen using theme colors
    Pen createDarkenedPenFromTheme(Pen original_pen, float perspective, bool is_grass1, bool is_edge1) {
        // Get RGB values based on current theme and pen type
        int r, g, b;
        
        if (is_grass1) {
            // Use grassColour1 RGB values based on theme
            switch(currentTheme) {
                case DESERT: r = 245; g = 191; b = 66; break;  // Sand color
                case SNOW: r = 240; g = 248; b = 255; break;   // Snow white
                case RED: r = 139; g = 69; b = 19; break;      // Saddle brown
                case VICE: r = 255; g = 20; b = 147; break;    // Deep pink
                case F32: r = 85; g = 107; b = 47; break;      // Dark olive green
                case STARRYNIGHT: r = 0; g = 60; b = 0; break; // Very dark green
                case CYBER: r = 0; g = 255; b = 127; break;    // Spring green
                case SUNSET: r = 100; g = 80; b = 0; break;    // Dark yellow-brown
                case OCEAN: r = 0; g = 100; b = 100; break;    // Dark teal
                case NEON: r = 255; g = 0; b = 255; break;     // Magenta
                case CITYSCAPE: r = 40; g = 40; b = 40; break; // Dark asphalt
                default: r = 42; g = 170; b = 138; break;      // Default green
            }
        } else if (!is_grass1 && !is_edge1) {
            // Use grassColour2 RGB values based on theme
            switch(currentTheme) {
                case DESERT: r = 160; g = 82; b = 45; break;   // Saddle brown
                case SNOW: r = 176; g = 196; b = 222; break;   // Light steel blue
                case RED: r = 205; g = 92; b = 92; break;      // Indian red
                case VICE: r = 75; g = 0; b = 130; break;      // Indigo
                case F32: r = 107; g = 142; b = 35; break;     // Olive drab
                case STARRYNIGHT: r = 0; g = 40; b = 0; break; // Even darker green
                case CYBER: r = 0; g = 128; b = 128; break;    // Teal
                case SUNSET: r = 150; g = 120; b = 50; break;  // Light brown
                case OCEAN: r = 0; g = 150; b = 150; break;    // Medium teal
                case NEON: r = 75; g = 0; b = 130; break;      // Indigo
                case CITYSCAPE: r = 60; g = 60; b = 60; break; // Concrete sidewalk
                default: r = 26; g = 187; b = 43; break;       // Default green
            }
        } else if (is_edge1) {
            // Use edgeCol RGB values based on current theme
            switch(currentTheme) {
                case DAY: r = 0; g = 0; b = 0; break;           // Black
                case NIGHT: r = 235; g = 123; b = 120; break;   // Light pink
                case STARRYNIGHT: r = 51; g = 51; b = 68; break; // Dark blue
                case VICE: r = 51; g = 51; b = 68; break;       // Dark blue
                case DESERT: r = 255; g = 255; b = 255; break;  // White
                case DAYTOO: r = 0; g = 0; b = 0; break;        // Black
                case SNOW: r = 155; g = 51; b = 0; break;       // Brown
                case F32: r = 51; g = 51; b = 68; break;        // Dark blue
                case RED: r = 255; g = 0; b = 0; break;         // Red
                case CYBER: r = 0; g = 255; b = 255; break;     // Cyan
                case SUNSET: r = 255; g = 100; b = 0; break;    // Orange
                case OCEAN: r = 0; g = 200; b = 200; break;     // Aqua
                case NEON: r = 255; g = 0; b = 255; break;      // Magenta
                case CITYSCAPE: r = 255; g = 255; b = 0; break; // Yellow
                default: r = 0; g = 0; b = 0; break;            // Default black
            }
        } else {
            // Use edgeCol2 RGB values based on current theme
            switch(currentTheme) {
                case DAY: r = 255; g = 255; b = 255; break;     // White
                case NIGHT: r = 237; g = 191; b = 118; break;   // Light orange
                case STARRYNIGHT: r = 255; g = 255; b = 0; break; // Yellow
                case VICE: r = 255; g = 255; b = 0; break;      // Yellow
                case DESERT: r = 0; g = 0; b = 0; break;        // Black
                case DAYTOO: r = 255; g = 255; b = 255; break;  // White
                case SNOW: r = 255; g = 255; b = 0; break;      // Yellow
                case F32: r = 255; g = 255; b = 0; break;       // Yellow
                case RED: r = 255; g = 255; b = 255; break;     // White
                case CYBER: r = 255; g = 0; b = 255; break;     // Magenta
                case SUNSET: r = 255; g = 200; b = 100; break;  // Light orange
                case OCEAN: r = 100; g = 255; b = 255; break;   // Light aqua
                case NEON: r = 0; g = 255; b = 0; break;        // Neon green
                case CITYSCAPE: r = 255; g = 255; b = 255; break; // White
                default: r = 255; g = 255; b = 255; break;      // Default white
            }
        }
        
        return createDarkenedPen(r, g, b, perspective);
    }
    
    // Helper function to create theme-aware lighting for main road scene
    Pen createDarkenedPen(int r, int g, int b, float perspective) {
        // Get theme-specific brightness color for blending
        int brightness_r, brightness_g, brightness_b;
        switch(currentTheme) {
            case DESERT:
                // Desert uses bright yellow for lighting effect
                brightness_r = 100; brightness_g = 100; brightness_b = 10;
                break;
            default:
                // All other themes use dark for lighting effect
                brightness_r = 0; brightness_g = 0; brightness_b = 0;
                break;
        }
        
        // Blend between original color and brightness color based on perspective
        // At horizon (perspective = 0): more brightness color (20% original, 80% brightness)
        // At bottom of screen (perspective = 1): original color (100% original, 0% brightness)
        float original_factor = 0.2f + 0.8f * perspective;
        float brightness_factor = 0.1f - original_factor;
        
        int final_r = (int)(r * original_factor + brightness_r * brightness_factor);
        int final_g = (int)(g * original_factor + brightness_g * brightness_factor);
        int final_b = (int)(b * original_factor + brightness_b * brightness_factor);
        
        // Clamp values to 0-255 range
        final_r = std::max(0, std::min(255, final_r));
        final_g = std::max(0, std::min(255, final_g));
        final_b = std::max(0, std::min(255, final_b));
        
        return gfx.create_pen(final_r, final_g, final_b);
    }
};

class ArcadeRacerGame : public GameBase {
private:
    Car car;
    std::unique_ptr<Road> road;
    uint32_t last_button_time = 0;
    const uint32_t DEBOUNCE_MS = 200;
    bool collision_detected = false;
    uint32_t collision_time = 0;
    const uint32_t COLLISION_FLASH_DURATION = 500000; // 0.5 seconds in microseconds
    
public:
    ArcadeRacerGame() {}
    
    virtual ~ArcadeRacerGame() = default;
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        cosmic->set_brightness(0.8f);
        
        road = std::make_unique<Road>(graphics, CosmicUnicorn::WIDTH, CosmicUnicorn::HEIGHT);
        
        srand(time_us_64());
    }
    
    bool debounce(uint32_t current_time) {
        if (current_time - last_button_time > DEBOUNCE_MS * 1000) {
            last_button_time = current_time;
            return true;
        }
        return false;
    }
    
    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down, 
                    bool button_bright_up, bool button_bright_down) override {
        uint32_t current_time = time_us_64();
        
        if (button_bright_up) {
            cosmic->adjust_brightness(0.1f);
        }
        
        if (button_bright_down) {
            cosmic->adjust_brightness(-0.1f);
        }
        
        // Simple steering input - pass button states to car
        float leftInput = button_a ? 1.0f : 0.0f;
        float rightInput = button_vol_up ? 1.0f : 0.0f;
        
        // Update car physics
        car.update(leftInput, rightInput);
        
        // Gradual speed control - much smaller increments per frame
        // Button B: Brake
        if (button_b) {
            if (car.speed > 0) {
                car.speed -= 0.8f;  // Gradual braking
                if (car.speed < 0) {
                    car.speed = 0;
                }
                car.autoAccelEnabled = false;  // Disable auto-acceleration when braking
            }
        }
        
        // Button C: Accelerate (more intuitive than Volume Down)
        if (button_c) {
            if (car.speed < 100) {
                car.speed += 0.5f;  // Gradual acceleration
                if (car.speed > 100) {
                    car.speed = 100;
                }
                car.autoAccelEnabled = true;  // Re-enable auto-acceleration when manually accelerating
            }
        }
        
        // Volume Down: Alternative acceleration (keep for backward compatibility)
        if (button_vol_down) {
            if (car.speed < 100) {
                car.speed += 0.5f;  // Gradual acceleration
                if (car.speed > 100) {
                    car.speed = 100;
                }
                car.autoAccelEnabled = true;  // Re-enable auto-acceleration when manually accelerating
            }
        }
        
        // Theme switching (but not for exit - that's handled by GameBase)
        if (button_d && debounce(current_time)) {
            auto themes = road->getThemes();
            road->nextTheme();
        }
        
        // Manual tunnel trigger: A + B buttons pressed together
        if (button_a && button_b && debounce(current_time)) {
            road->triggerTunnel();
        }
    }
    
    bool update() override {
        // Check for exit condition using GameBase method
        bool button_d = cosmic->is_pressed(CosmicUnicorn::SWITCH_D);
        if (checkExitCondition(button_d)) {
            return false;  // Exit game
        }
        
        // Handle other input
        handleInput(
            cosmic->is_pressed(CosmicUnicorn::SWITCH_A),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_B),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_C),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_D),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_UP),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_DOWN),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_UP),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_DOWN)
        );
        
        // Sync speed between car and road
        road->speed = car.speed;
        
        // Update the road (this was missing - the road needs to update to move scenery and spawn objects!)
        road->update();
        
        // Check for collisions with oncoming cars
        uint32_t current_time = time_us_64();
        if (road->checkCollisions(car)) {
            if (!collision_detected) {
                collision_detected = true;
                collision_time = current_time;
                
                // Collision effects - slow down and add bounce
                car.speed *= 0.5f;  // Reduce speed by half
                car.velocity += (rand() % 2 == 0 ? 0.1f : -0.1f);  // Random bounce left/right
            }
        }
        
        // Reset collision flash after duration
        if (collision_detected && (current_time - collision_time > COLLISION_FLASH_DURATION)) {
            collision_detected = false;
        }
        
        return true;  // Continue game
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        
        road->draw(car);
        car.draw(graphics);


        // Add collision flash effect - red overlay
        if (collision_detected) {
            graphics.set_pen(255, 0, 0);  // Red
            // Flash with reducing intensity over time
            uint32_t time_since_collision = time_us_64() - collision_time;
            float flash_intensity = 1.0f - (float)time_since_collision / COLLISION_FLASH_DURATION;
            
            // Draw red pixels around the border for flash effect
            for (int x = 0; x < CosmicUnicorn::WIDTH; x++) {
                if ((x % 4) == 0 && flash_intensity > 0.5f) {  // Sparse red pixels
                    graphics.pixel(Point(x, 0));
                    graphics.pixel(Point(x, CosmicUnicorn::HEIGHT - 1));
                }
            }
            for (int y = 0; y < CosmicUnicorn::HEIGHT; y++) {
                if ((y % 4) == 0 && flash_intensity > 0.5f) {  // Sparse red pixels
                    graphics.pixel(Point(0, y));
                    graphics.pixel(Point(CosmicUnicorn::WIDTH - 1, y));
                }
            }
        }
    }
    
    const char* getName() const override {
        return "Arcade Racer";
    }
    
    const char* getDescription() const override {
        return "3D racing game with multiple themes and oncoming cars";
    }
};
