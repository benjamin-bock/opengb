#include "launcher.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace fs = std::filesystem;

namespace {

constexpr int WinW = 480;
constexpr int WinH = 220;

struct Widget {
    SDL_Rect rect{};
    bool hovered = false;
};

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
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.size() > 3 &&
           (lower.substr(lower.size() - 3) == ".gb" ||
            lower.substr(lower.size() - 4) == ".gbc");
}

std::string parentOf(const std::string& dir) {
    fs::path p(dir);
    if (p.has_parent_path() && !p.parent_path().empty()) {
        return p.parent_path().string();
    }
    return dir;
}

void drawOutlinedRect(SDL_Renderer* r, const SDL_Rect& rect,
                      SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    int x2 = rect.x + rect.w - 1;
    int y2 = rect.y + rect.h - 1;
    SDL_RenderDrawLine(r, rect.x, rect.y, x2, rect.y);
    SDL_RenderDrawLine(r, rect.x, y2, x2, y2);
    SDL_RenderDrawLine(r, rect.x, rect.y, rect.x, y2);
    SDL_RenderDrawLine(r, x2, rect.y, x2, y2);
}

bool pointIn(const SDL_Rect& rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

// Tiny integrated file browser restricted to .gb/.gbc files and folders.
FileBrowserResult runFileBrowser(TTF_Font* font,
                                 const std::string& startDir) {
    FileBrowserResult result;
    result.currentDir =
        fs::exists(startDir) ? startDir : fs::current_path().string();

    SDL_Window* win = SDL_CreateWindow(
        "opengb - Choisir une ROM", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 560, 400, SDL_WINDOW_SHOWN);
    if (!win) return result;
    SDL_Renderer* r =
        SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        SDL_DestroyWindow(win);
        return result;
    }

    const SDL_Color bg{24, 26, 32, 255};
    const SDL_Color panel{40, 44, 52, 255};
    const SDL_Color border{80, 86, 98, 255};
    const SDL_Color text{235, 235, 235, 255};
    const SDL_Color hover{55, 62, 74, 255};
    const SDL_Color dirColor{130, 180, 255, 255};

    bool open = true;
    int scroll = 0;

    while (open && !result.done) {
        std::vector<std::pair<std::string, bool>> entries; // path, isDir
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
            } else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) open = false;
            } else if (ev.type == SDL_MOUSEWHEEL) {
                scroll -= ev.wheel.y * 40;
                if (scroll < 0) scroll = 0;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN &&
                       ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                SDL_Rect upBtn{12, 12, 110, 30};
                SDL_Rect cancelBtn{560 - 100 - 12, 400 - 30 - 12, 100, 30};
                if (pointIn(upBtn, mx, my)) {
                    result.currentDir = parentOf(result.currentDir);
                    scroll = 0;
                } else if (pointIn(cancelBtn, mx, my)) {
                    open = false;
                } else {
                    int idx = (my - 60 + scroll) / 32;
                    if (idx >= 0 && idx < static_cast<int>(entries.size()) &&
                        my >= 60) {
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

        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, 255);
        SDL_RenderClear(r);

        SDL_Rect upBtn{12, 12, 110, 30};
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        drawOutlinedRect(r, upBtn, pointIn(upBtn, mx, my) ? hover : panel,
                         border);
        if (font) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, "Dossier parent", text);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                if (t) {
                    SDL_Rect dst{upBtn.x + 6, upBtn.y + (upBtn.h - s->h) / 2,
                                 s->w, s->h};
                    SDL_RenderCopy(r, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        SDL_Rect cancelBtn{560 - 100 - 12, 400 - 30 - 12, 100, 30};
        drawOutlinedRect(r, cancelBtn,
                         pointIn(cancelBtn, mx, my) ? hover : panel, border);
        if (font) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, "Annuler", text);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                if (t) {
                    SDL_Rect dst{cancelBtn.x + (cancelBtn.w - s->w) / 2,
                                 cancelBtn.y + (cancelBtn.h - s->h) / 2, s->w,
                                 s->h};
                    SDL_RenderCopy(r, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        int maxVisible = (400 - 60 - 30) / 32;
        if (entries.size() > static_cast<size_t>(maxVisible) && scroll > 0 &&
            scroll / 32 >=
                static_cast<int>(entries.size()) - maxVisible) {
            scroll =
                (static_cast<int>(entries.size()) - maxVisible) * 32;
        }
        if (scroll < 0) scroll = 0;

        int y = 60 - scroll;
        for (const auto& [path, isDir] : entries) {
            if (y + 32 < 60) {
                y += 32;
                continue;
            }
            if (y > 400 - 34) break;
            SDL_Rect row{12, y, 560 - 24, 30};
            drawOutlinedRect(r, row, pointIn(row, mx, my) ? hover : panel,
                             border);
            if (font) {
                SDL_Surface* s = TTF_RenderUTF8_Blended(
                    font, baseName(path).c_str(),
                    isDir ? dirColor : text);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    if (t) {
                        SDL_Rect dst{row.x + 8,
                                     row.y + (row.h - s->h) / 2, s->w, s->h};
                        SDL_RenderCopy(r, t, nullptr, &dst);
                        SDL_DestroyTexture(t);
                    }
                    SDL_FreeSurface(s);
                }
            }
            y += 32;
        }

        SDL_RenderPresent(r);
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    return result;
}

} // namespace

std::optional<LauncherConfig> runLauncher() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return std::nullopt;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return std::nullopt;
    }

    TTF_Font* font = nullptr;
    std::string fontPath;
    std::vector<std::string> candidates = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
#endif
    };
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            font = TTF_OpenFont(candidate.c_str(), 16);
            if (font) {
                fontPath = candidate;
                break;
            }
        }
    }
    TTF_Font* titleFont =
        !fontPath.empty() ? TTF_OpenFont(fontPath.c_str(), 24) : nullptr;

    SDL_Window* window = SDL_CreateWindow(
        "opengb", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinW,
        WinH, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError()
                  << std::endl;
        return std::nullopt;
    }
    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError()
                  << std::endl;
        return std::nullopt;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, WinW, WinH);

    const SDL_Color bg{24, 26, 32, 255};
    const SDL_Color panel{40, 44, 52, 255};
    const SDL_Color border{80, 86, 98, 255};
    const SDL_Color text{235, 235, 235, 255};
    const SDL_Color accent{80, 170, 255, 255};
    const SDL_Color accentDim{60, 130, 200, 255};

    LauncherConfig cfg;
    cfg.romPath.clear();
    cfg.volume = 0.8f;

    bool running = true;
    std::string editing;
    bool editFocused = true;
    int caretTimer = 0;

    // Layout
    SDL_Rect romField{20, 70, WinW - 140 - 40, 32};
    SDL_Rect browseBtn{WinW - 110, 70, 90, 32};
    SDL_Rect volTrack{20, 140, WinW - 140 - 40, 8};
    SDL_Rect launchBtn{WinW / 2 - 90, 175, 180, 34};

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_DROPTEXT || event.type == SDL_DROPFILE) {
                if (event.drop.file) {
                    editing = event.drop.file;
                    SDL_free(event.drop.file);
                }
            } else if (event.type == SDL_TEXTINPUT) {
                editing += event.text.text;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE &&
                    !editing.empty()) {
                    editing.pop_back();
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    cfg.romPath = editing;
                    if (!cfg.romPath.empty() && fs::exists(cfg.romPath)) {
                        running = false;
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                if (pointIn(browseBtn, mx, my)) {
                    FileBrowserResult res = runFileBrowser(
                        font,
                        editing.empty()
                            ? fs::current_path().string()
                            : parentOf(fs::absolute(editing).string()));
                    if (res.done) {
                        editing = res.pickedPath;
                    }
                } else if (pointIn(launchBtn, mx, my)) {
                    cfg.romPath = editing;
                    if (!cfg.romPath.empty() && fs::exists(cfg.romPath)) {
                        running = false;
                    } else if (font) {
                        // invalid path: brief visual feedback via title
                        SDL_SetWindowTitle(window,
                            "opengb - ROM introuvable !");
                        SDL_Delay(120);
                        SDL_SetWindowTitle(window, "opengb");
                    }
                } else if (pointIn(volTrack, mx, my) ||
                           pointIn({volTrack.x, volTrack.y - 8,
                                    volTrack.w, volTrack.h + 16},
                                   mx, my)) {
                    float frac = static_cast<float>(mx - volTrack.x) /
                                 static_cast<float>(volTrack.w);
                    cfg.volume = std::clamp(frac, 0.0f, 1.0f);
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                if (SDL_GetMouseState(nullptr, nullptr) &
                        SDL_BUTTON(SDL_BUTTON_LEFT) &&
                    (pointIn({volTrack.x, volTrack.y - 8, volTrack.w,
                              volTrack.h + 16},
                             event.motion.x, event.motion.y))) {
                    float frac =
                        static_cast<float>(event.motion.x - volTrack.x) /
                        static_cast<float>(volTrack.w);
                    cfg.volume = std::clamp(frac, 0.0f, 1.0f);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
        SDL_RenderClear(renderer);

        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        // Title
        if (titleFont) {
            SDL_Surface* s =
                TTF_RenderUTF8_Blended(titleFont, "opengb", accent);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{(WinW - s->w) / 2, 18, s->w, s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        // ROM field label
        if (font) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(
                font, "Chemin de la ROM :", text);
            if (s) {
                SDL_Texture* t =
                    SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{romField.x, romField.y - 22, s->w, s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        // ROM text field
        drawOutlinedRect(renderer, romField, panel, border);
        if (font) {
            std::string shown = editing;
            int textW = 0;
            TTF_SizeText(font, shown.c_str(), &textW, nullptr);
            while (!shown.empty() && textW > romField.w - 20) {
                shown.erase(0, 1);
                if (shown.size() > 1) shown[0] = '~';
                TTF_SizeText(font, shown.c_str(), &textW, nullptr);
            }
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, shown.c_str(), text);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{romField.x + 8,
                                 romField.y + (romField.h - s->h) / 2, s->w,
                                 s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
            caretTimer = (caretTimer + 1) % 60;
            if (editFocused && caretTimer < 30) {
                SDL_SetRenderDrawColor(renderer, accent.r, accent.g,
                                       accent.b, 255);
                SDL_RenderDrawLine(renderer, romField.x + 8 + textW,
                                   romField.y + 6,
                                   romField.x + 8 + textW,
                                   romField.y + romField.h - 6);
            }
        }

        // Browse button
        drawOutlinedRect(renderer, browseBtn,
                         pointIn(browseBtn, mx, my) ? accentDim : accent,
                         border);
        if (font) {
            SDL_Surface* s =
                TTF_RenderUTF8_Blended(font, "Parcourir...", text);
            if (s) {
                SDL_Texture* t =
                    SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{browseBtn.x + (browseBtn.w - s->w) / 2,
                                 browseBtn.y + (browseBtn.h - s->h) / 2, s->w,
                                 s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        // Volume label + value
        if (font) {
            char volText[16];
            std::snprintf(volText, sizeof(volText), "Volume : %d%%",
                          static_cast<int>(cfg.volume * 100.0f + 0.5f));
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, volText, text);
            if (s) {
                SDL_Texture* t =
                    SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{volTrack.x, volTrack.y - 24, s->w, s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        // Volume slider track
        drawOutlinedRect(renderer, {volTrack.x, volTrack.y - 2, volTrack.w,
                                    volTrack.h + 4},
                         panel, border);
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255);
        int volW = static_cast<int>(static_cast<float>(volTrack.w) *
                                    cfg.volume);
        SDL_Rect fill{volTrack.x, volTrack.y, volW, volTrack.h};
        SDL_RenderFillRect(renderer, &fill);
        SDL_Rect knob{volTrack.x + volW - 6, volTrack.y - 6, 12,
                      volTrack.h + 12};
        drawOutlinedRect(renderer, knob, text, border);

        // Launch button
        bool launchHover = pointIn(launchBtn, mx, my);
        drawOutlinedRect(renderer, launchBtn,
                         launchHover ? accentDim
                                     : SDL_Color{46, 170, 110, 255},
                         border);
        if (font) {
            SDL_Surface* s =
                TTF_RenderUTF8_Blended(font, "Lancer", text);
            if (s) {
                SDL_Texture* t =
                    SDL_CreateTextureFromSurface(renderer, s);
                if (t) {
                    SDL_Rect dst{launchBtn.x + (launchBtn.w - s->w) / 2,
                                 launchBtn.y + (launchBtn.h - s->h) / 2, s->w,
                                 s->h};
                    SDL_RenderCopy(renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(8);
    }

    if (titleFont) TTF_CloseFont(titleFont);
    if (font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (cfg.romPath.empty() || !fs::exists(cfg.romPath)) {
        return std::nullopt;
    }
    return cfg;
}
