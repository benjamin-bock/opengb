#include "launcher.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "font5x7.hpp"

namespace fs = std::filesystem;

namespace {

constexpr int WinW = 480;
constexpr int WinH = 220;
constexpr int CharW = 6;   // 5 px glyph + 1 px spacing
constexpr int CharH = 8;   // 7 px glyph + 1 px spacing

struct FileBrowserResult {
    bool done = false;
    std::string pickedPath;
    std::string currentDir;
};

std::string baseName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

bool hasRomExtension(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    return lower.size() > 3 &&
           (lower.substr(lower.size() - 3) == ".gb" ||
            lower.substr(lower.size() - 4) == ".gbc");
}

std::string parentOf(const std::string& dir) {
    fs::path p(dir);
    if (p.has_parent_path() && !p.parent_path().empty() &&
        p.parent_path() != p) {
        return p.parent_path().string();
    }
    return dir;
}

bool pointIn(const SDL_Rect& rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
           y < rect.y + rect.h;
}

void fillRect(SDL_Renderer* r, const SDL_Rect& rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}

void drawRect(SDL_Renderer* r, const SDL_Rect& rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(r, &rect);
}

int textWidth(const std::string& s, int scale = 1) {
    return static_cast<int>(s.size()) * CharW * scale;
}

// Draws text with the embedded 5x7 bitmap font. scale=2 gives a chunky
// retro look; (r,g,b) is the fill colour.
void drawText(SDL_Renderer* r, const std::string& text, int x, int y,
              SDL_Color c, int scale = 1) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
    int cx = x;
    for (char ch : text) {
        unsigned char u = static_cast<unsigned char>(ch);
        if (u >= 32 && u <= 126) {
            const uint8_t* glyph =
                &opengb::kFont5x7[5 * (static_cast<size_t>(u) - 32)];
            for (int col = 0; col < 5; col++) {
                uint8_t bits = glyph[col];
                for (int row = 0; row < 7; row++) {
                    if (bits & (1u << row)) {
                        SDL_Rect px{cx + col * scale, y + row * scale, scale,
                                    scale};
                        SDL_RenderFillRect(r, &px);
                    }
                }
            }
        }
        cx += CharW * scale;
    }
}

std::string ellipsizeLeft(const std::string& s, size_t maxChars) {
    if (s.size() <= maxChars) return s;
    return "~" + s.substr(s.size() - maxChars + 1);
}

const SDL_Color Bg{24, 26, 32, 255};
const SDL_Color Panel{40, 44, 52, 255};
const SDL_Color Border{80, 86, 98, 255};
const SDL_Color Text{235, 235, 235, 255};
const SDL_Color Muted{140, 146, 158, 255};
const SDL_Color Accent{80, 170, 255, 255};
const SDL_Color AccentDim{60, 130, 200, 255};
const SDL_Color Green{46, 170, 110, 255};
const SDL_Color GreenDim{36, 140, 92, 255};
const SDL_Color Hover{55, 62, 74, 255};
const SDL_Color DirColor{130, 180, 255, 255};

void drawButton(SDL_Renderer* r, const SDL_Rect& rect, bool hovered,
                SDL_Color base, const std::string& label) {
    fillRect(r, rect, hovered ? Hover : base);
    drawRect(r, rect, Border);
    drawText(r, label, rect.x + (rect.w - textWidth(label)) / 2,
             rect.y + (rect.h - 7) / 2, Text);
}

// Integrated file browser restricted to .gb/.gbc files and folders.
FileBrowserResult runFileBrowser(SDL_Window* parent, const std::string& startDir) {
    FileBrowserResult result;
    result.currentDir = fs::exists(startDir) ? startDir
                                             : fs::current_path().string();

    SDL_Window* win = SDL_CreateWindow(
        "opengb - Choisir une ROM", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 560, 400, SDL_WINDOW_SHOWN);
    if (!win) return result;
    SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        SDL_DestroyWindow(win);
        return result;
    }

    constexpr int RowH = 18;
    constexpr int ListY = 54;
    constexpr int ListBottom = 400 - 44;

    bool open = true;
    int scroll = 0;

    while (open && !result.done) {
        std::vector<std::pair<std::string, bool>> entries;
        std::error_code ec;
        if (!fs::exists(result.currentDir)) {
            result.currentDir = fs::current_path().string();
        }
        fs::directory_iterator it(result.currentDir, ec);
        if (!ec) {
            for (const auto& entry : it) {
                if (entry.is_directory(ec)) {
                    entries.emplace_back(entry.path().string(), true);
                } else if (hasRomExtension(entry.path().filename().string())) {
                    entries.emplace_back(entry.path().string(), false);
                }
            }
        }
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      if (a.second != b.second) return a.second;
                      return baseName(a.first) < baseName(b.first);
                  });

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                open = false;
            } else if (ev.type == SDL_KEYDOWN &&
                       ev.key.keysym.sym == SDLK_ESCAPE) {
                open = false;
            } else if (ev.type == SDL_MOUSEWHEEL) {
                scroll -= ev.wheel.y * RowH;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN &&
                       ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                SDL_Rect upBtn{12, 12, 110, 26};
                SDL_Rect cancelBtn{560 - 100 - 12, 400 - 30 - 10, 100, 26};
                if (pointIn(upBtn, mx, my)) {
                    result.currentDir = parentOf(result.currentDir);
                    scroll = 0;
                } else if (pointIn(cancelBtn, mx, my)) {
                    open = false;
                } else {
                    int idx = (my - ListY + scroll) / RowH;
                    if (idx >= 0 && my >= ListY &&
                        idx < static_cast<int>(entries.size())) {
                        if (entries[idx].second) {
                            result.currentDir = entries[idx].first;
                            scroll = 0;
                        } else {
                            result.pickedPath = entries[idx].first;
                            result.done = true;
                        }
                    }
                }
            }
        }

        fillRect(r, {0, 0, 560, 400}, Bg);

        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        SDL_Rect upBtn{12, 12, 110, 26};
        drawButton(r, upBtn, pointIn(upBtn, mx, my), Panel, "Dossier parent");

        SDL_Rect cancelBtn{560 - 100 - 12, 400 - 30 - 10, 100, 26};
        drawButton(r, cancelBtn, pointIn(cancelBtn, mx, my), Panel, "Annuler");

        std::string dirLabel =
            ellipsizeLeft(result.currentDir, (560 - 24) / CharW);
        drawText(r, dirLabel, 130, 22, Muted);

        int maxVisible = (ListBottom - ListY) / RowH;
        if (scroll > static_cast<int>(entries.size()) * RowH -
                         maxVisible * RowH) {
            scroll = std::max(0, static_cast<int>(entries.size()) * RowH -
                                          maxVisible * RowH);
        }
        if (scroll < 0) scroll = 0;

        int y = ListY - scroll;
        for (const auto& [path, isDir] : entries) {
            if (y + RowH > ListY + scroll && y < ListBottom) {
                SDL_Rect row{12, y, 560 - 24, RowH - 2};
                if (pointIn(row, mx, my)) fillRect(r, row, Hover);
                std::string label =
                    isDir ? "[" + baseName(path) + "]" : baseName(path);
                drawText(r, ellipsizeLeft(label, (560 - 40) / CharW),
                         row.x + 10, y + 5, isDir ? DirColor : Text);
            }
            y += RowH;
        }

        SDL_RenderPresent(r);
        SDL_Delay(8);
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    return result;
}

} // namespace

std::optional<LauncherConfig> runLauncher() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return std::nullopt;
    }

    SDL_Window* window = SDL_CreateWindow(
        "opengb", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinW, WinH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        return std::nullopt;
    }
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        return std::nullopt;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, WinW, WinH);

    LauncherConfig cfg;
    cfg.romPath.clear();
    cfg.volume = 0.8f;

    bool running = true;
    std::string romField;
    bool invalidPath = false;
    int caretTimer = 0;

    SDL_Rect romFieldRect{20, 70, WinW - 140 - 40, 28};
    SDL_Rect browseBtn{WinW - 110, 70, 90, 28};
    SDL_Rect volTrack{20, 140, WinW - 140 - 40, 8};
    SDL_Rect launchBtn{WinW / 2 - 90, 172, 180, 32};

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_DROPTEXT ||
                       event.type == SDL_DROPFILE) {
                if (event.drop.file) {
                    romField = event.drop.file;
                    SDL_free(event.drop.file);
                    invalidPath = false;
                }
            } else if (event.type == SDL_TEXTINPUT) {
                romField += event.text.text;
                invalidPath = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE &&
                    !romField.empty()) {
                    romField.pop_back();
                    invalidPath = false;
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    if (!romField.empty() && fs::exists(romField)) {
                        cfg.romPath = romField;
                        running = false;
                    } else {
                        invalidPath = true;
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                if (pointIn(browseBtn, mx, my)) {
                    FileBrowserResult res = runFileBrowser(
                        window,
                        romField.empty()
                            ? fs::current_path().string()
                            : parentOf(fs::absolute(romField).string()));
                    if (res.done) {
                        romField = res.pickedPath;
                        invalidPath = false;
                    }
                } else if (pointIn(launchBtn, mx, my)) {
                    if (!romField.empty() && fs::exists(romField)) {
                        cfg.romPath = romField;
                        running = false;
                    } else {
                        invalidPath = true;
                    }
                } else if (pointIn({volTrack.x, volTrack.y - 8, volTrack.w,
                                    volTrack.h + 16},
                                   mx, my)) {
                    float frac =
                        static_cast<float>(mx - volTrack.x) /
                        static_cast<float>(volTrack.w);
                    cfg.volume = std::clamp(frac, 0.0f, 1.0f);
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                if (SDL_GetMouseState(nullptr, nullptr) &
                        SDL_BUTTON(SDL_BUTTON_LEFT) &&
                    pointIn({volTrack.x, volTrack.y - 8, volTrack.w,
                             volTrack.h + 16},
                            event.motion.x, event.motion.y)) {
                    float frac =
                        static_cast<float>(event.motion.x - volTrack.x) /
                        static_cast<float>(volTrack.w);
                    cfg.volume = std::clamp(frac, 0.0f, 1.0f);
                }
            }
        }

        fillRect(renderer, {0, 0, WinW, WinH}, Bg);

        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        drawText(renderer, "opengb", (WinW - textWidth("opengb", 2)) / 2, 16,
                 Accent, 2);

        drawText(renderer, "Chemin de la ROM :", romFieldRect.x,
                 romFieldRect.y - 14, Text);

        fillRect(renderer, romFieldRect, Panel);
        drawRect(renderer, romFieldRect, Border);
        drawText(renderer,
                 ellipsizeLeft(romField, (romFieldRect.w - 16) / CharW),
                 romFieldRect.x + 8,
                 romFieldRect.y + (romFieldRect.h - 7) / 2,
                 romField.empty() ? Muted : Text);
        caretTimer = (caretTimer + 1) % 60;
        if (caretTimer < 30) {
            size_t shownLen =
                std::min(romField.size(),
                         static_cast<size_t>((romFieldRect.w - 16) / CharW));
            drawText(renderer, "_",
                     romFieldRect.x + 8 +
                         static_cast<int>(shownLen) * CharW,
                     romFieldRect.y + (romFieldRect.h - 7) / 2, Accent);
        }

        if (invalidPath) {
            drawText(renderer, "ROM introuvable !", WinW - 110 - 20, 105,
                     SDL_Color{220, 90, 90, 255});
        }

        char volText[24];
        std::snprintf(volText, sizeof(volText), "Volume : %d%%",
                      static_cast<int>(cfg.volume * 100.0f + 0.5f));
        drawText(renderer, volText, volTrack.x, volTrack.y - 16, Text);

        fillRect(renderer, {volTrack.x, volTrack.y - 2, volTrack.w,
                            volTrack.h + 4},
                 Panel);
        drawRect(renderer, {volTrack.x, volTrack.y - 2, volTrack.w,
                            volTrack.h + 4},
                 Border);
        int volW = static_cast<int>(static_cast<float>(volTrack.w) *
                                   cfg.volume);
        fillRect(renderer, {volTrack.x, volTrack.y, volW, volTrack.h},
                 Accent);
        fillRect(renderer, {volTrack.x + volW - 2, volTrack.y - 5, 4,
                            volTrack.h + 10},
                 Text);

        bool launchHover = pointIn(launchBtn, mx, my);
        fillRect(renderer, launchBtn, launchHover ? GreenDim : Green);
        drawRect(renderer, launchBtn, Border);
        drawText(renderer, "LANCER",
                 launchBtn.x + (launchBtn.w - textWidth("LANCER", 2)) / 2,
                 launchBtn.y + (launchBtn.h - 14) / 2, Text, 2);

        drawButton(renderer, browseBtn, pointIn(browseBtn, mx, my), Accent,
                   "Parcourir...");

        SDL_RenderPresent(renderer);
        SDL_Delay(8);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (cfg.romPath.empty() || !fs::exists(cfg.romPath)) {
        return std::nullopt;
    }
    return cfg;
}
