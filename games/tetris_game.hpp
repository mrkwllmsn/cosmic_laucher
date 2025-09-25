#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <array>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BOARD_OFFSET_X = 4;  // Move board left to use more space
const int BOARD_OFFSET_Y = 2;  // Move board up

enum class TetrominoType {
    I, O, T, S, Z, J, L, NONE
};

struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
};

class Tetromino {
public:
    TetrominoType type;
    std::array<std::array<bool, 4>, 4> shape;
    Position position;
    int rotation;
    uint8_t color_r, color_g, color_b;
    
    Tetromino(TetrominoType t = TetrominoType::NONE) : type(t), position(BOARD_WIDTH/2 - 2, 0), rotation(0) {
        initShape();
        setColor();
    }
    
private:
    void initShape() {
        // Clear the shape first
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                shape[i][j] = false;
            }
        }
        
        switch(type) {
            case TetrominoType::I:
                // I-piece: ####
                shape[1][0] = true;
                shape[1][1] = true;
                shape[1][2] = true;
                shape[1][3] = true;
                break;
            case TetrominoType::O:
                // O-piece: ##
                //          ##
                shape[0][0] = true;
                shape[0][1] = true;
                shape[1][0] = true;
                shape[1][1] = true;
                break;
            case TetrominoType::T:
                // T-piece:  #
                //          ###
                shape[0][1] = true;
                shape[1][0] = true;
                shape[1][1] = true;
                shape[1][2] = true;
                break;
            case TetrominoType::S:
                // S-piece:  ##
                //          ##
                shape[0][1] = true;
                shape[0][2] = true;
                shape[1][0] = true;
                shape[1][1] = true;
                break;
            case TetrominoType::Z:
                // Z-piece: ##
                //           ##
                shape[0][0] = true;
                shape[0][1] = true;
                shape[1][1] = true;
                shape[1][2] = true;
                break;
            case TetrominoType::J:
                // J-piece: #
                //          ###
                shape[0][0] = true;
                shape[1][0] = true;
                shape[1][1] = true;
                shape[1][2] = true;
                break;
            case TetrominoType::L:
                // L-piece:   #
                //          ###
                shape[0][2] = true;
                shape[1][0] = true;
                shape[1][1] = true;
                shape[1][2] = true;
                break;
            default:
                break;
        }
    }
    
    void setColor() {
        switch(type) {
            case TetrominoType::I: color_r = 0; color_g = 255; color_b = 255; break; // Cyan - bright
            case TetrominoType::O: color_r = 255; color_g = 255; color_b = 0; break; // Yellow - bright
            case TetrominoType::T: color_r = 255; color_g = 0; color_b = 255; break; // Magenta - very bright
            case TetrominoType::S: color_r = 0; color_g = 255; color_b = 0; break; // Green - bright
            case TetrominoType::Z: color_r = 255; color_g = 0; color_b = 0; break; // Red - bright
            case TetrominoType::J: color_r = 0; color_g = 150; color_b = 255; break; // Bright Blue
            case TetrominoType::L: color_r = 255; color_g = 165; color_b = 0; break; // Orange - bright
            default: color_r = 255; color_g = 255; color_b = 255; break; // White
        }
    }

private:
    Position getRotationCenter() const {
        switch(type) {
            case TetrominoType::I:
                return Position(2, 1); // I piece rotates around center of line
            case TetrominoType::O:
                return Position(1, 1); // O doesn't rotate anyway
            case TetrominoType::T:
                return Position(1, 1); // T rotates around center block
            case TetrominoType::S:
                return Position(1, 1); // S rotates around middle point
            case TetrominoType::Z:
                return Position(1, 1); // Z rotates around middle point
            case TetrominoType::J:
                return Position(1, 1); // J rotates around center block
            case TetrominoType::L:
                return Position(1, 1); // L rotates around center block
            default:
                return Position(1, 1);
        }
    }

public:
    void rotate() {
        if (type == TetrominoType::O) return; // O piece doesn't rotate
        
        Position center = getRotationCenter();
        std::array<std::array<bool, 4>, 4> rotated = {};
        
        // Rotate around center point
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shape[i][j]) {
                    // Translate to origin relative to center
                    int relY = i - center.y;
                    int relX = j - center.x;
                    
                    // Rotate 90 degrees clockwise: (x,y) -> (y,-x)
                    int newRelX = relY;
                    int newRelY = -relX;
                    
                    // Translate back from center
                    int newY = newRelY + center.y;
                    int newX = newRelX + center.x;
                    
                    // Check bounds and set
                    if (newY >= 0 && newY < 4 && newX >= 0 && newX < 4) {
                        rotated[newY][newX] = true;
                    }
                }
            }
        }
        
        shape = rotated;
        rotation = (rotation + 1) % 4;
    }
    
    std::array<Position, 4> getBlocks() const {
        std::array<Position, 4> blocks;
        int blockIndex = 0;
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shape[i][j]) {
                    blocks[blockIndex++] = Position(position.x + j, position.y + i);
                }
            }
        }
        return blocks;
    }
};

class TetrisGame : public GameBase {
private:
    // Game board and pieces
    std::array<std::array<TetrominoType, BOARD_WIDTH>, BOARD_HEIGHT> board;
    std::array<std::array<uint8_t, 3>, 7> pieceColors;
    Tetromino currentPiece;
    Tetromino nextPiece;
    
    // Game state
    uint32_t score;
    uint32_t lines;
    uint32_t level;
    uint32_t dropTimer;
    uint32_t dropDelay;
    bool gameOver;
    bool paused;
    
    // Input handling
    bool lastBrightUpPressed;
    
    // Animation states
    bool clearingLines;
    std::vector<int> linesToClear;
    uint32_t clearAnimationTimer;
    uint32_t clearAnimationFrame;

    void initGame() {
        score = 0;
        lines = 0;
        level = 1;
        dropTimer = 0;
        dropDelay = 500;
        gameOver = false;
        paused = false;
        lastBrightUpPressed = false;
        clearingLines = false;
        clearAnimationTimer = 0;
        clearAnimationFrame = 0;
        
        // Initialize board
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[y][x] = TetrominoType::NONE;
            }
        }
        
        // Initialize piece colors array
        pieceColors[0] = {0, 255, 255};    // I - Cyan
        pieceColors[1] = {255, 255, 0};    // O - Yellow  
        pieceColors[2] = {255, 0, 255};    // T - Magenta (very bright)
        pieceColors[3] = {0, 255, 0};      // S - Green
        pieceColors[4] = {255, 0, 0};      // Z - Red
        pieceColors[5] = {0, 150, 255};    // J - Bright Blue
        pieceColors[6] = {255, 165, 0};    // L - Orange
        
        spawnNextPiece();
        spawnNewPiece();
    }
    
    void spawnNewPiece() {
        currentPiece = nextPiece;
        currentPiece.position = Position(BOARD_WIDTH/2 - 2, 0);
        spawnNextPiece();
        
        if (isCollision(currentPiece)) {
            gameOver = true;
        }
    }
    
    void spawnNextPiece() {
        TetrominoType types[] = {TetrominoType::I, TetrominoType::O, TetrominoType::T, 
                                TetrominoType::S, TetrominoType::Z, TetrominoType::J, TetrominoType::L};
        nextPiece = Tetromino(types[rand() % 7]);
    }
    
    bool isCollision(const Tetromino& piece) const {
        auto blocks = piece.getBlocks();
        for (const auto& block : blocks) {
            if (block.x < 0 || block.x >= BOARD_WIDTH || 
                block.y >= BOARD_HEIGHT || 
                (block.y >= 0 && board[block.y][block.x] != TetrominoType::NONE)) {
                return true;
            }
        }
        return false;
    }
    
    void placePiece() {
        auto blocks = currentPiece.getBlocks();
        for (const auto& block : blocks) {
            if (block.y >= 0) {
                board[block.y][block.x] = currentPiece.type;
            }
        }
        
        checkAndStartClearLines();
        if (!clearingLines) {
            spawnNewPiece();
        }
    }
    
    void checkAndStartClearLines() {
        linesToClear.clear();
        
        for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
            bool fullLine = true;
            for (int x = 0; x < BOARD_WIDTH; x++) {
                if (board[y][x] == TetrominoType::NONE) {
                    fullLine = false;
                    break;
                }
            }
            
            if (fullLine) {
                linesToClear.push_back(y);
            }
        }
        
        if (!linesToClear.empty()) {
            clearingLines = true;
            clearAnimationTimer = to_ms_since_boot(get_absolute_time());
            clearAnimationFrame = 0;
        }
    }
    
    void updateClearAnimation() {
        if (!clearingLines) return;
        
        uint32_t currentTime = to_ms_since_boot(get_absolute_time());
        
        if (currentTime - clearAnimationTimer >= 50) { // Animation speed
            clearAnimationFrame++;
            clearAnimationTimer = currentTime;
            
            if (clearAnimationFrame >= 10) { // Animation duration
                // Actually clear the lines now
                for (int lineY : linesToClear) {
                    // Move all lines above down
                    for (int moveY = lineY; moveY > 0; moveY--) {
                        for (int x = 0; x < BOARD_WIDTH; x++) {
                            board[moveY][x] = board[moveY - 1][x];
                        }
                    }
                    // Clear top line
                    for (int x = 0; x < BOARD_WIDTH; x++) {
                        board[0][x] = TetrominoType::NONE;
                    }
                }
                
                // Update score and level
                int linesCleared = linesToClear.size();
                lines += linesCleared;
                level = (lines / 10) + 1;
                dropDelay = 500 - (level * 30);
                if (dropDelay < 50) dropDelay = 50;
                
                // Score calculation (standard Tetris scoring)
                int lineScore[] = {0, 40, 100, 300, 1200};
                score += lineScore[linesCleared] * level;
                
                clearingLines = false;
                linesToClear.clear();
            }
        }
    }
    
    void moveLeft() {
        Tetromino testPiece = currentPiece;
        testPiece.position.x--;
        if (!isCollision(testPiece)) {
            currentPiece = testPiece;
        }
    }
    
    void moveRight() {
        Tetromino testPiece = currentPiece;
        testPiece.position.x++;
        if (!isCollision(testPiece)) {
            currentPiece = testPiece;
        }
    }
    
    void moveDown() {
        Tetromino testPiece = currentPiece;
        testPiece.position.y++;
        if (!isCollision(testPiece)) {
            currentPiece = testPiece;
        } else {
            placePiece();
        }
    }
    
    void rotatePiece() {
        Tetromino testPiece = currentPiece;
        testPiece.rotate();
        if (!isCollision(testPiece)) {
            currentPiece = testPiece;
        }
    }
    
    void hardDrop() {
        while (true) {
            Tetromino testPiece = currentPiece;
            testPiece.position.y++;
            if (isCollision(testPiece)) {
                placePiece();
                break;
            } else {
                currentPiece = testPiece;
                score += 2; // Hard drop bonus
            }
        }
    }
    
    void gameUpdate() {
        if (gameOver) {
            // Handle restart when game is over
            static bool lastAPressed = false;
            bool aPressed = cosmic->is_pressed(cosmic->SWITCH_A);
            
            if (aPressed && !lastAPressed) {
                restart();
            }
            lastAPressed = aPressed;
            return;
        }
        
        // Update line clearing animation
        updateClearAnimation();
        
        // If clearing animation just finished, spawn new piece
        if (!clearingLines && linesToClear.empty() && clearAnimationFrame > 0) {
            clearAnimationFrame = 0;
            spawnNewPiece();
        }
        
        if (paused) {
            // Only handle pause button when paused
            if (cosmic->is_pressed(cosmic->SWITCH_BRIGHTNESS_UP)) {
                if (!lastBrightUpPressed) {
                    paused = false;
                    lastBrightUpPressed = true;
                }
            } else {
                lastBrightUpPressed = false;
            }
            return;
        }
        
        // Don't update game logic during line clearing animation
        if (clearingLines) return;
        
        uint32_t currentTime = to_ms_since_boot(get_absolute_time());
        
        // Handle drop timing - make sure this works!
        if (dropTimer == 0) {
            dropTimer = currentTime; // Initialize on first call
        }
        
        if (currentTime - dropTimer >= dropDelay) {
            moveDown();
            dropTimer = currentTime;
        }
        
        // Simplified button handling
        static bool lastA = false, lastB = false, lastVolUp = false, lastVolDown = false;
        static bool lastBrightUp = false, lastBrightDown = false;
        
        bool aPressed = cosmic->is_pressed(cosmic->SWITCH_A);
        bool bPressed = cosmic->is_pressed(cosmic->SWITCH_B);
        bool volUpPressed = cosmic->is_pressed(cosmic->SWITCH_VOLUME_UP);
        bool volDownPressed = cosmic->is_pressed(cosmic->SWITCH_VOLUME_DOWN);
        bool brightUpPressed = cosmic->is_pressed(cosmic->SWITCH_BRIGHTNESS_UP);
        bool brightDownPressed = cosmic->is_pressed(cosmic->SWITCH_BRIGHTNESS_DOWN);
        
        // Move left
        if (aPressed && !lastA) {
            moveLeft();
        }
        
        // Move right  
        if (volUpPressed && !lastVolUp) {
            moveRight();
        }
        
        // Rotate
        if (bPressed && !lastB) {
            rotatePiece();
        }
        
        // Soft drop
        if (volDownPressed && !lastVolDown) {
            moveDown();
            // No points for soft drop - just faster movement
        }
        
        // Hard drop
        if (brightDownPressed && !lastBrightDown) {
            hardDrop();
        }
        
        // Pause
        if (brightUpPressed && !lastBrightUp) {
            paused = true;
        }
        
        lastA = aPressed;
        lastB = bPressed;
        lastVolUp = volUpPressed;
        lastVolDown = volDownPressed;
        lastBrightUp = brightUpPressed;
        lastBrightDown = brightDownPressed;
    }
    
    void drawBlock(PicoGraphics_PenRGB888& graphics, int x, int y, uint8_t r, uint8_t g, uint8_t b, bool highlight = false) {
        // Main block color
        graphics.set_pen(r, g, b);
        graphics.pixel(Point(x, y));
        
        // Add highlight for 3D effect
        if (highlight && x > 0 && y > 0) {
            graphics.set_pen(r + 40, g + 40, b + 40); // Lighter edge
            graphics.pixel(Point(x - 1, y)); // Left highlight
            graphics.pixel(Point(x, y - 1)); // Top highlight
        }
    }
    
    void drawDigit(PicoGraphics_PenRGB888& graphics, int digit, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        graphics.set_pen(r, g, b);
        
        // Compact 3x3 digit patterns optimized for small display
        bool pattern[10][9] = {
            {1,1,1,1,0,1,1,1,1}, // 0
            {0,1,0,0,1,0,0,1,0}, // 1
            {1,1,1,0,0,1,1,1,1}, // 2
            {1,1,1,0,0,1,0,1,1}, // 3
            {1,0,1,1,1,1,0,0,1}, // 4
            {1,1,1,1,0,0,1,1,1}, // 5
            {1,1,1,1,0,0,1,1,1}, // 6
            {1,1,1,0,0,1,0,0,1}, // 7
            {1,1,1,1,1,1,1,1,1}, // 8
            {1,1,1,1,1,1,0,1,1}  // 9
        };
        
        if (digit < 0 || digit > 9) return;
        
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (pattern[digit][i*3 + j]) {
                    graphics.pixel(Point(x + j, y + i));
                }
            }
        }
    }
    
    void drawNumber(PicoGraphics_PenRGB888& graphics, uint32_t number, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (number == 0) {
            drawDigit(graphics, 0, x, y, r, g, b);
            return;
        }
        
        // Count digits to determine starting position
        uint32_t temp = number;
        int digits = 0;
        while (temp > 0) {
            temp /= 10;
            digits++;
        }
        
        // Draw digits from right to left with tight spacing
        int currentX = x + (digits - 1) * 4; // 4 pixels spacing between digits
        while (number > 0) {
            int digit = number % 10;
            drawDigit(graphics, digit, currentX, y, r, g, b);
            number /= 10;
            currentX -= 4;
        }
    }
    
    void drawGameOverText(PicoGraphics_PenRGB888& graphics) {
        graphics.set_font("sans");
        
        // Set red color for GAME OVER text
        graphics.set_pen(255, 0, 0);
        
        // Draw "GAME" on first line (centered)
        std::string gameText = "GAME";
        float scale = 0.4f;  // Smaller text
        int gameWidth = graphics.measure_text(gameText, scale);
        int gameX = (32 - gameWidth) / 2;
        int gameY = 8;  // Moved up slightly
        graphics.text(gameText, Point(gameX, gameY), -1, scale);
        
        // Draw "OVER" on second line (centered)
        std::string overText = "OVER";
        int overWidth = graphics.measure_text(overText, scale);
        int overX = (32 - overWidth) / 2;
        int overY = 20;  // Increased spacing from 18 to 20
        graphics.text(overText, Point(overX, overY), -1, scale);
    }
    
    void drawAnimatedBackground(PicoGraphics_PenRGB888& graphics) {
        static uint32_t frame = 0;
        frame++;
        
        // Animated starfield background
        for (int i = 0; i < 20; i++) {
            int x = (i * 7 + frame / 4) % 32;
            int y = (i * 11 + frame / 6) % 32;
            int brightness = 20 + (sinf(frame * 0.1f + i) * 15);
            graphics.set_pen(brightness, brightness, brightness + 10);
            graphics.pixel(Point(x, y));
        }
        
        // Side panel gradient
        for (int y = 0; y < 32; y++) {
            int intensity = 5 + y / 4;
            graphics.set_pen(0, 0, intensity);
            graphics.pixel(Point(0, y));
            graphics.pixel(Point(1, y));
            graphics.pixel(Point(16, y));
            graphics.pixel(Point(17, y));
        }
    }
    
    void restart() {
        score = 0;
        lines = 0;
        level = 1;
        dropTimer = 0;
        dropDelay = 500;
        gameOver = false;
        paused = false;
        
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[y][x] = TetrominoType::NONE;
            }
        }
        
        spawnNewPiece();
    }

public:
    TetrisGame() {}
    
    virtual ~TetrisGame() = default;
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        cosmic->set_brightness(0.5f);
        initGame();
    }
    
    bool update() override {
        // Check for exit condition using GameBase method
        bool button_d = cosmic->is_pressed(CosmicUnicorn::SWITCH_D);
        if (checkExitCondition(button_d)) {
            return false;  // Exit game
        }
        
        // Handle brightness controls directly
        if (cosmic->is_pressed(cosmic->SWITCH_BRIGHTNESS_UP)) {
            cosmic->adjust_brightness(+0.01);
        }
        if (cosmic->is_pressed(cosmic->SWITCH_BRIGHTNESS_DOWN)) {
            cosmic->adjust_brightness(-0.01);
        }
        
        // Update game logic
        gameUpdate();
        
        return true;  // Continue game
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        
        // Draw animated background
        drawAnimatedBackground(graphics);
        
        // Draw enhanced board border with glow effect
        graphics.set_pen(80, 120, 255); // Blue glow
        for (int x = BOARD_OFFSET_X - 2; x <= BOARD_OFFSET_X + BOARD_WIDTH + 1; x++) {
            graphics.pixel(Point(x, BOARD_OFFSET_Y - 2));
            graphics.pixel(Point(x, BOARD_OFFSET_Y + BOARD_HEIGHT + 1));
        }
        for (int y = BOARD_OFFSET_Y - 1; y <= BOARD_OFFSET_Y + BOARD_HEIGHT; y++) {
            graphics.pixel(Point(BOARD_OFFSET_X - 2, y));
            graphics.pixel(Point(BOARD_OFFSET_X + BOARD_WIDTH + 1, y));
        }
        
        // Inner border
        graphics.set_pen(160, 200, 255); // Bright blue
        for (int x = BOARD_OFFSET_X - 1; x <= BOARD_OFFSET_X + BOARD_WIDTH; x++) {
            graphics.pixel(Point(x, BOARD_OFFSET_Y - 1));
            graphics.pixel(Point(x, BOARD_OFFSET_Y + BOARD_HEIGHT));
        }
        for (int y = BOARD_OFFSET_Y; y < BOARD_OFFSET_Y + BOARD_HEIGHT; y++) {
            graphics.pixel(Point(BOARD_OFFSET_X - 1, y));
            graphics.pixel(Point(BOARD_OFFSET_X + BOARD_WIDTH, y));
        }
        
        // Draw placed pieces with enhanced 3D effect
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                if (board[y][x] != TetrominoType::NONE) {
                    int colorIndex = static_cast<int>(board[y][x]);
                    
                    // Check if this line is being cleared
                    bool isClearing = false;
                    for (int clearY : linesToClear) {
                        if (y == clearY) {
                            isClearing = true;
                            break;
                        }
                    }
                    
                    if (isClearing && clearingLines) {
                        // Flashing animation for clearing lines
                        int flash = sinf(clearAnimationFrame * 0.8f) * 127 + 128;
                        graphics.set_pen(flash, flash, flash);
                        graphics.pixel(Point(BOARD_OFFSET_X + x, BOARD_OFFSET_Y + y));
                    } else {
                        // Simple solid pixel - no 3D effects
                        graphics.set_pen(pieceColors[colorIndex][0], 
                                       pieceColors[colorIndex][1], 
                                       pieceColors[colorIndex][2]);
                        graphics.pixel(Point(BOARD_OFFSET_X + x, BOARD_OFFSET_Y + y));
                    }
                }
            }
        }
        
        // Draw current piece (solid colors, no effects)
        if (!gameOver && !paused && !clearingLines) {
            graphics.set_pen(currentPiece.color_r, currentPiece.color_g, currentPiece.color_b);
            auto blocks = currentPiece.getBlocks();
            for (const auto& block : blocks) {
                if (block.y >= 0 && block.y < BOARD_HEIGHT && block.x >= 0 && block.x < BOARD_WIDTH) {
                    graphics.pixel(Point(BOARD_OFFSET_X + block.x, BOARD_OFFSET_Y + block.y));
                }
            }
        }
        
        // Enhanced next piece preview with frame
        graphics.set_pen(100, 100, 150); // Frame color
        for (int i = 18; i < 30; i++) {
            graphics.pixel(Point(i, 0));
            graphics.pixel(Point(i, 6));
        }
        for (int i = 1; i < 6; i++) {
            graphics.pixel(Point(18, i));
            graphics.pixel(Point(29, i));
        }
        
        // Next piece title
        graphics.set_pen(200, 200, 255);
        graphics.pixel(Point(20, 1));
        graphics.pixel(Point(22, 1));
        graphics.pixel(Point(24, 1));
        graphics.pixel(Point(26, 1));
        
        // Draw next piece (simple pixels)
        graphics.set_pen(nextPiece.color_r, nextPiece.color_g, nextPiece.color_b);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (nextPiece.shape[i][j]) {
                    graphics.pixel(Point(21 + j, 2 + i));
                }
            }
        }
        
        // Compact score display in right area
        // Score label and value
        graphics.set_pen(150, 150, 150);
        graphics.pixel(Point(17, 8)); // "S" indicator
        drawNumber(graphics, score, 18, 8, 255, 255, 0); // Yellow score
        
        // Level label and value
        graphics.set_pen(150, 150, 150);
        graphics.pixel(Point(17, 12)); // "L" indicator
        drawNumber(graphics, level, 18, 12, 0, 255, 100); // Green level
        
        // Lines value (no label needed)
        drawNumber(graphics, lines, 18, 16, 255, 0, 200); // Magenta lines
        
        // Game over effect with flashing border and text
        if (gameOver) {
            static uint32_t flashFrame = 0;
            flashFrame++;
            int flash = sinf(flashFrame * 0.5f) * 127 + 128;
            
            graphics.set_pen(flash, 0, 0);
            // Animated border flash
            for (int i = 0; i < 32; i++) {
                if ((i + flashFrame/4) % 4 == 0) {
                    graphics.pixel(Point(i, 0));
                    graphics.pixel(Point(i, 31));
                    graphics.pixel(Point(0, i));
                    graphics.pixel(Point(31, i));
                }
            }
            
            // Draw "GAME OVER" text centered
            drawGameOverText(graphics);
            
            // "A" indicator for restart (below the text)
            graphics.set_pen(255, 255, 0); // Yellow
            graphics.pixel(Point(15, 24)); 
            graphics.pixel(Point(14, 25)); graphics.pixel(Point(16, 25));
            graphics.pixel(Point(13, 26)); graphics.pixel(Point(17, 26));
            
        } else if (paused) {
            // Pause effect with pulsing
            static uint32_t pauseFrame = 0;
            pauseFrame++;
            int pulse = sinf(pauseFrame * 0.2f) * 100 + 155;
            
            graphics.set_pen(pulse, pulse, 0);
            // Pause bars
            for (int y = 8; y < 24; y++) {
                graphics.pixel(Point(12, y)); graphics.pixel(Point(13, y));
                graphics.pixel(Point(15, y)); graphics.pixel(Point(16, y));
            }
        }
    }
    
    const char* getName() const override {
        return "Cosmic Tetris";
    }
    
    const char* getDescription() const override {
        return "Classic Tetris with falling tetromino blocks on the Cosmic Unicorn";
    }
};