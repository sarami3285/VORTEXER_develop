#pragma once
#include <SDL.h>
#include "SDL_ttf.h"
#include <string>
#include <SDL_mixer.h> 

class UIButton {
public:
    UIButton(int x, int y, int w, int h, const std::string& labelText, SDL_Renderer* renderer, TTF_Font* font);
    ~UIButton();

    void HandleEvent(const SDL_Event& e);
    void Draw(SDL_Renderer* renderer);
    bool IsClicked();
    void SetRect(int x, int y, int w, int h);
    void SetText(const std::string& newText);
    void ResetClick() { mClicked = false; }

    bool IsHovered() const { return mIsHovered; }

private:
    SDL_Rect mRect;
    std::string mLabel;
    SDL_Texture* mLabelTexture;
    SDL_Renderer* mRenderer;
    TTF_Font* mFont;
    bool mClicked;

    bool mIsHovered;        // ホバー判定用フラグ
    bool mSoundPlayed;      // 効果音再生判定
    Mix_Chunk* hoverSound;  // 効果音
    Mix_Chunk* clickSound;  // 効果音
};
