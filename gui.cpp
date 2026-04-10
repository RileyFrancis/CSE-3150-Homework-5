#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <random>
#include <cmath>
#include <map>

using namespace std;

// Game constants - adjust these to change window size
const int WINDOW_WIDTH = 800;      // Increased from 600
const int WINDOW_HEIGHT = 900;     // Increased from 700
const int GRID_SIZE = 4;
const int TILE_SIZE = 160;         // Increased from 120
const int GRID_PADDING = 25;       // Increased from 20
const int ANIMATION_DURATION = 150; // milliseconds

// These will be calculated to center the grid
int GRID_OFFSET_X;
int GRID_OFFSET_Y;

// Color scheme - Modern dark with vibrant accent colors
struct Color {
    Uint8 r, g, b, a;
    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0, Uint8 a = 255) : r(r), g(g), b(b), a(a) {}
};

const Color BG_COLOR(26, 26, 46);           // Deep dark blue
const Color GRID_BG(40, 40, 64);            // Slightly lighter dark
const Color EMPTY_TILE(52, 52, 78);         // Empty cell color
const Color TEXT_LIGHT(245, 245, 250);      // Light text
const Color TEXT_DARK(26, 26, 46);          // Dark text
const Color ACCENT_GOLD(255, 193, 84);      // Gold accent
const Color ACCENT_PURPLE(139, 92, 246);    // Purple accent

// Tile color map - vibrant gradient-inspired colors
map<int, Color> TILE_COLORS = {
    {2, Color(94, 129, 172)},      // Slate blue
    {4, Color(129, 140, 248)},     // Indigo
    {8, Color(167, 139, 250)},     // Purple
    {16, Color(232, 121, 249)},    // Magenta
    {32, Color(251, 113, 133)},    // Pink
    {64, Color(251, 146, 60)},     // Orange
    {128, Color(250, 204, 21)},    // Yellow
    {256, Color(163, 230, 53)},    // Lime
    {512, Color(74, 222, 128)},    // Green
    {1024, Color(45, 212, 191)},   // Teal
    {2048, Color(96, 165, 250)},   // Blue
    {4096, Color(139, 92, 246)},   // Deep purple
    {8192, Color(236, 72, 153)}    // Hot pink
};

// Animation state for a tile
struct TileAnimation {
    int value;
    float x, y;          // Current position
    float target_x, target_y;  // Target position
    float scale;         // For merge animation
    bool is_new;         // For spawn animation
    bool is_merging;     // For merge animation
    Uint32 start_time;
    
    TileAnimation() : value(0), x(0), y(0), target_x(0), target_y(0), 
                     scale(1.0f), is_new(false), is_merging(false), start_time(0) {}
};

class Game2048 {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_large;
    TTF_Font* font_medium;
    TTF_Font* font_small;
    
    vector<vector<int>> board;
    vector<vector<TileAnimation>> tile_animations;
    stack<vector<vector<int>>> history;
    
    int score;
    bool game_over;
    bool animating;
    Uint32 animation_start_time;
    
public:
    Game2048() : window(nullptr), renderer(nullptr), 
                font_large(nullptr), font_medium(nullptr), font_small(nullptr),
                board(4, vector<int>(4, 0)),
                tile_animations(4, vector<TileAnimation>(4)),
                score(0), game_over(false), animating(false), animation_start_time(0) {
        // Calculate centered grid position
        int grid_width = TILE_SIZE * 4 + GRID_PADDING * 5;
        int grid_height = TILE_SIZE * 4 + GRID_PADDING * 5;
        
        GRID_OFFSET_X = (WINDOW_WIDTH - grid_width) / 2;
        GRID_OFFSET_Y = (WINDOW_HEIGHT - grid_height) / 2 + 50;  // +50 to leave room for header
    }
    
    ~Game2048() {
        cleanup();
    }
    
    bool init() {
        // Force X11 on Linux to avoid Wayland issues
        #ifdef __linux__
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
        #endif
        
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "SDL init failed: " << SDL_GetError() << endl;
            cerr << "Try running with: SDL_VIDEODRIVER=x11 ./gui" << endl;
            return false;
        }
        
        if (TTF_Init() < 0) {
            cerr << "TTF init failed: " << TTF_GetError() << endl;
            return false;
        }
        
        window = SDL_CreateWindow("2048", 
                                  SDL_WINDOWPOS_CENTERED, 
                                  SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, 
                                  SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Window creation failed: " << SDL_GetError() << endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            cerr << "Renderer creation failed: " << SDL_GetError() << endl;
            return false;
        }
        
        // Load fonts - try multiple common paths (increased sizes for larger window)
        const char* font_paths[] = {
            "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",  // Fedora
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",    // Ubuntu/Debian
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",        // macOS
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",                // Arch
            nullptr
        };
        
        for (int i = 0; font_paths[i] != nullptr && !font_large; i++) {
            font_large = TTF_OpenFont(font_paths[i], 96);  // Increased from 72
        }
        
        for (int i = 0; font_paths[i] != nullptr && !font_medium; i++) {
            font_medium = TTF_OpenFont(font_paths[i], 64);  // Increased from 48
        }
        
        const char* regular_paths[] = {
            "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",       // Fedora
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",         // Ubuntu/Debian
            "/System/Library/Fonts/Supplemental/Arial.ttf",             // macOS
            "/usr/share/fonts/TTF/DejaVuSans.ttf",                     // Arch
            nullptr
        };
        
        for (int i = 0; regular_paths[i] != nullptr && !font_small; i++) {
            font_small = TTF_OpenFont(regular_paths[i], 28);  // Increased from 24
        }
        
        if (!font_large || !font_medium || !font_small) {
            cerr << "Font loading failed: " << TTF_GetError() << endl;
            cerr << "Please install DejaVu fonts:" << endl;
            cerr << "  Fedora: sudo dnf install dejavu-sans-fonts" << endl;
            cerr << "  Ubuntu: sudo apt-get install fonts-dejavu" << endl;
            return false;
        }
        
        // Initialize game
        cout << "Initializing game board..." << endl;
        spawn_tile();
        spawn_tile();
        init_animations();
        
        cout << "Game initialized successfully!" << endl;
        cout << "Controls:" << endl;
        cout << "  Arrow Keys or WASD - Move tiles" << endl;
        cout << "  U - Undo" << endl;
        cout << "  N - New game" << endl;
        cout << "  ESC or close window - Quit" << endl;
        
        return true;
    }
    
    void cleanup() {
        if (font_large) TTF_CloseFont(font_large);
        if (font_medium) TTF_CloseFont(font_medium);
        if (font_small) TTF_CloseFont(font_small);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
    
    void spawn_tile() {
        vector<pair<int, int>> empty;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (board[r][c] == 0) {
                    empty.emplace_back(r, c);
                }
            }
        }
        
        if (empty.empty()) return;
        
        static mt19937 gen(time(nullptr));
        uniform_int_distribution<> pos_dist(0, empty.size() - 1);
        uniform_int_distribution<> val_dist(1, 10);
        
        auto [r, c] = empty[pos_dist(gen)];
        board[r][c] = (val_dist(gen) == 1 ? 4 : 2);
        
        // Mark as new for animation
        tile_animations[r][c].is_new = true;
        tile_animations[r][c].start_time = SDL_GetTicks();
        tile_animations[r][c].scale = 0.0f;
    }
    
    void init_animations() {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                tile_animations[r][c].value = board[r][c];
                tile_animations[r][c].x = tile_animations[r][c].target_x = c;
                tile_animations[r][c].y = tile_animations[r][c].target_y = r;
                tile_animations[r][c].scale = 1.0f;
                tile_animations[r][c].is_new = false;
                tile_animations[r][c].is_merging = false;
            }
        }
    }
    
    vector<int> compress_row(const vector<int>& row) {
        vector<int> result;
        copy_if(row.begin(), row.end(), back_inserter(result), [](int x) { return x != 0; });
        result.resize(row.size(), 0);
        return result;
    }
    
    vector<int> merge_row(vector<int> row) {
        for (size_t i = 1; i < row.size(); i++) {
            if (row[i-1] == row[i] && row[i] != 0) {
                row[i] *= 2;
                score += row[i];
                row[i-1] = 0;
                i++;
            }
        }
        return compress_row(row);
    }
    
    bool move_left() {
        bool moved = false;
        for (auto& row : board) {
            vector<int> original = row;
            row = compress_row(row);
            row = merge_row(row);
            if (original != row) moved = true;
        }
        return moved;
    }
    
    bool move_right() {
        bool moved = false;
        for (auto& row : board) {
            vector<int> original = row;
            reverse(row.begin(), row.end());
            row = compress_row(row);
            row = merge_row(row);
            reverse(row.begin(), row.end());
            if (original != row) moved = true;
        }
        return moved;
    }
    
    bool move_up() {
        bool moved = false;
        for (int col = 0; col < 4; col++) {
            vector<int> column(4);
            for (int row = 0; row < 4; row++) {
                column[row] = board[row][col];
            }
            vector<int> original = column;
            column = compress_row(column);
            column = merge_row(column);
            for (int row = 0; row < 4; row++) {
                board[row][col] = column[row];
            }
            if (column != original) moved = true;
        }
        return moved;
    }
    
    bool move_down() {
        bool moved = false;
        for (int col = 0; col < 4; col++) {
            vector<int> column(4);
            for (int row = 0; row < 4; row++) {
                column[row] = board[row][col];
            }
            vector<int> original = column;
            reverse(column.begin(), column.end());
            column = compress_row(column);
            column = merge_row(column);
            reverse(column.begin(), column.end());
            for (int row = 0; row < 4; row++) {
                board[row][col] = column[row];
            }
            if (column != original) moved = true;
        }
        return moved;
    }
    
    void handle_move(bool moved) {
        if (moved) {
            animation_start_time = SDL_GetTicks();
            animating = true;
            spawn_tile();
            init_animations();
        }
    }
    
    void undo() {
        if (!history.empty()) {
            board = history.top();
            history.pop();
            init_animations();
        }
    }
    
    void handle_input(SDL_Event& event) {
        if (event.type == SDL_KEYDOWN && !animating) {
            vector<vector<int>> prev = board;
            bool moved = false;
            
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    // Signal to quit - will be handled in run() loop
                    break;
                case SDLK_LEFT:
                case SDLK_a:
                    moved = move_left();
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    moved = move_right();
                    break;
                case SDLK_UP:
                case SDLK_w:
                    moved = move_up();
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    moved = move_down();
                    break;
                case SDLK_u:
                    undo();
                    return;
                case SDLK_n:
                    // New game
                    board = vector<vector<int>>(4, vector<int>(4, 0));
                    history = stack<vector<vector<int>>>();
                    score = 0;
                    spawn_tile();
                    spawn_tile();
                    init_animations();
                    return;
            }
            
            if (moved) {
                history.push(prev);
                handle_move(moved);
            }
        }
    }
    
    void update_animations() {
        Uint32 current_time = SDL_GetTicks();
        bool still_animating = false;
        
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                auto& anim = tile_animations[r][c];
                
                // New tile spawn animation
                if (anim.is_new) {
                    Uint32 elapsed = current_time - anim.start_time;
                    if (elapsed < ANIMATION_DURATION) {
                        float progress = (float)elapsed / ANIMATION_DURATION;
                        anim.scale = progress;
                        still_animating = true;
                    } else {
                        anim.scale = 1.0f;
                        anim.is_new = false;
                    }
                }
            }
        }
        
        animating = still_animating;
    }
    
    void draw_rounded_rect(int x, int y, int w, int h, int radius, Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        // Draw filled rectangle (center)
        SDL_Rect rect = {x + radius, y, w - 2 * radius, h};
        SDL_RenderFillRect(renderer, &rect);
        
        rect = {x, y + radius, w, h - 2 * radius};
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw corners (simple square corners for now - true rounded would need more complex rendering)
        rect = {x, y, radius, radius};
        SDL_RenderFillRect(renderer, &rect);
        rect = {x + w - radius, y, radius, radius};
        SDL_RenderFillRect(renderer, &rect);
        rect = {x, y + h - radius, radius, radius};
        SDL_RenderFillRect(renderer, &rect);
        rect = {x + w - radius, y + h - radius, radius, radius};
        SDL_RenderFillRect(renderer, &rect);
    }
    
    void draw_text(const string& text, int x, int y, TTF_Font* font, Color color, bool centered = true) {
        SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), sdl_color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        SDL_Rect rect;
        rect.w = surface->w;
        rect.h = surface->h;
        rect.x = centered ? x - rect.w / 2 : x;
        rect.y = centered ? y - rect.h / 2 : y;
        
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
    
    void render() {
        // Clear background
        SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
        SDL_RenderClear(renderer);
        
        // Calculate header positions relative to grid
        int header_y = GRID_OFFSET_Y - 100;
        int score_y = GRID_OFFSET_Y - 40;
        
        // Draw header
        draw_text("2048", WINDOW_WIDTH / 2, header_y, font_large, ACCENT_GOLD);
        
        // Draw score
        string score_text = "Score: " + to_string(score);
        draw_text(score_text, WINDOW_WIDTH / 2, score_y, font_small, TEXT_LIGHT);
        
        // Draw grid background
        int grid_width = TILE_SIZE * 4 + GRID_PADDING * 5;
        int grid_height = TILE_SIZE * 4 + GRID_PADDING * 5;
        draw_rounded_rect(GRID_OFFSET_X, GRID_OFFSET_Y, grid_width, grid_height, 8, GRID_BG);
        
        // Draw tiles
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                int x = GRID_OFFSET_X + GRID_PADDING + c * (TILE_SIZE + GRID_PADDING);
                int y = GRID_OFFSET_Y + GRID_PADDING + r * (TILE_SIZE + GRID_PADDING);
                
                // Draw empty cell
                draw_rounded_rect(x, y, TILE_SIZE, TILE_SIZE, 6, EMPTY_TILE);
                
                // Draw tile if present
                int value = board[r][c];
                if (value > 0) {
                    auto& anim = tile_animations[r][c];
                    
                    // Apply scale animation
                    float scale = anim.scale;
                    int scaled_size = (int)(TILE_SIZE * scale);
                    int offset = (TILE_SIZE - scaled_size) / 2;
                    
                    // Get tile color
                    Color tile_color = TILE_COLORS.count(value) ? 
                                      TILE_COLORS[value] : 
                                      Color(100, 100, 150);
                    
                    draw_rounded_rect(x + offset, y + offset, scaled_size, scaled_size, 6, tile_color);
                    
                    // Draw value
                    if (scale > 0.3f) {  // Only show text when tile is big enough
                        string value_text = to_string(value);
                        TTF_Font* tile_font = value >= 1000 ? font_medium :   // Changed from font_small
                                            value >= 100 ? font_large : 
                                            font_large;  // Use largest font for small numbers
                        
                        Color text_color = value <= 4 ? TEXT_LIGHT : TEXT_DARK;
                        draw_text(value_text, x + TILE_SIZE / 2, y + TILE_SIZE / 2, tile_font, text_color);
                    }
                }
            }
        }
        
        // Draw controls help - position below grid
        int grid_height = TILE_SIZE * 4 + GRID_PADDING * 5;
        int help_y = GRID_OFFSET_Y + grid_height + 40;
        
        draw_text("Arrow Keys or WASD to move | U to undo | N for new game", 
                 WINDOW_WIDTH / 2, help_y, font_small, Color(150, 150, 170), true);
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        
        cout << "Game loop starting..." << endl;
        
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else {
                    handle_input(event);
                }
            }
            
            update_animations();
            render();
            SDL_Delay(16); // ~60 FPS
        }
        
        cout << "Game loop ended. Final score: " << score << endl;
    }
};

int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;
    
    cout << "Starting 2048 GUI..." << endl;
    
    Game2048 game;
    
    if (!game.init()) {
        cerr << "Failed to initialize game" << endl;
        cerr << "\nIf you see SDL video driver errors, try running:" << endl;
        cerr << "  SDL_VIDEODRIVER=x11 ./gui" << endl;
        return 1;
    }
    
    game.run();
    
    cout << "Thanks for playing!" << endl;
    
    return 0;
}