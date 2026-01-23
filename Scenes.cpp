#include "Scenes.h"
#include "Game.h"
#include <SDL.h>
#include "ScrollItemComponent.h"
#include "json.hpp"
#include <sstream>

// =============== TitleScene 実装 ===============
TitleScene::TitleScene(Game* game, SDL_Renderer* renderer)
    : mGame(game), mRenderer(renderer) {
    
}

TitleScene::~TitleScene() {
    if (mFont) TTF_CloseFont(mFont);
}

void TitleScene::LoadContent()
{
    StartSound = Mix_LoadWAV("Assets/Audio/StartSound.wav");
    mTextureBack = mGame->GetTexture("Assets/TitleBack.png");
    mTextureTitle = mGame->GetTexture("Assets/Title.png");
    if (!mTextureBack||!mTextureTitle) {
        SDL_Log("Failed to load title image");
    }
    mFont = TTF_OpenFont("Assets/NSJP.ttf", 48);
    if (!mFont) { SDL_Log("Failed to load font: %s", TTF_GetError()); }
    mBlinkingText = std::make_unique<BlinkingTextComponent>(
        mRenderer,
        mFont,
        "Press Space to Start", // テキスト
        0,                // X座標 (中央寄せされる)
        600,              // Y座標
        0.7f              // 点滅間隔
    );
}

void TitleScene::ProcessInput(const Uint8* keyState) {
    if (keyState[SDL_SCANCODE_SPACE]) {
        if (!mIsFadingOut) {
            Mix_PlayChannel(-1, StartSound, 0);
        }
        mIsFadingOut = true;
    }
}

void TitleScene::Update(float deltaTime) {

    if (mIsFadingOut) {
        mFadeAlpha = static_cast<Uint8>(std::min(255.0f, mFadeAlpha + 255.0f * deltaTime));
        if (mBlinkingText) {
            mBlinkingText->SetBlinkingActive(false);
            mBlinkingText->SetVisible(true);
        }
    }
    else {
        if (mBlinkingText) {
            mBlinkingText->Update(deltaTime);
        }
    }
}

void TitleScene::Draw(SDL_Renderer* renderer) {
    if (mTextureBack) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mTextureBack, nullptr, &destRect);
    }
    if (mTextureTitle) {
        SDL_Rect destRect = { 212, 0, 630, 420 };
        SDL_RenderCopy(renderer, mTextureTitle, nullptr, &destRect);
    }

    if (mBlinkingText) {
        mBlinkingText->Draw();
    }

    if (mIsFadingOut) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, mFadeAlpha);
        SDL_Rect fadeRect = { 0, 0, 1024, 768 };
        SDL_RenderFillRect(renderer, &fadeRect);
    }
}

bool TitleScene::IsFinished() const {
    return mIsFadingOut && mFadeAlpha >= 255;
}

std::unique_ptr<Scene> TitleScene::NextScene() {
    return std::make_unique<MenuScene>(mGame, mGame->GetRenderer());
}
// ==============================================





// =============== MenuScene 実装 ===============
MenuScene::MenuScene(Game* game, SDL_Renderer* renderer)
    : mGame(game)
    , mRenderer(renderer)
{
    
}


MenuScene::~MenuScene(){
    
}

void MenuScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/techno_03.mp3", -1);
    mTexture = mGame->GetTexture("Assets/Base.png");
    if (!mTexture) {
        SDL_Log("Failed to load title image");
    }

    mFont = TTF_OpenFont("Assets/NSJP.ttf", 40);
    if (!mFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
    }

    std::string weaponName = mGame->mPendingWeapon.name;
    WeaponData data;
    data.LoadFromJSON("Assets/Data/WeaponData.json", weaponName);
    mWeaponTexture = mGame->GetTexture(data.weaponIcon);
    mBackTexture = mGame->GetTexture("Assets/BackGround.png");



    startButton = std::make_unique<UIButton>(700, 450, 200, 60, "Battle", mRenderer, mFont);
    storeButton = std::make_unique<UIButton>(100, 450, 200, 60, "Store", mRenderer, mFont);
    customizeButton = std::make_unique<UIButton>(350, 450, 200, 60, "Customize", mRenderer, mFont);



    int currency = mGame->GetDataManager().GetCurrency();

    SDL_Color white = { 255, 255, 255, 255 };
    std::string text = "CREDITS: " + std::to_string(currency); 

    SDL_Surface* surface = TTF_RenderUTF8_Blended(mFont, text.c_str(), white);
    if (!surface) {
        SDL_Log("Failed to create currency surface: %s", TTF_GetError());
    }
    else {
        mCurrencyText = SDL_CreateTextureFromSurface(mRenderer, surface);
        mCurrencyRect = { 50, 50, surface->w, surface->h };
        SDL_FreeSurface(surface);
    }
}

void MenuScene::UnloadContent() {
    if (mFont) {
        TTF_CloseFont(mFont);
        mFont = nullptr;
    }

    if (mCurrencyText) {
        SDL_DestroyTexture(mCurrencyText);
        mCurrencyText = nullptr;
    }
}

void MenuScene::ProcessEvent(const SDL_Event& e) {
    if (startButton) startButton->HandleEvent(e);
    if (storeButton) storeButton->HandleEvent(e);
    if (customizeButton) customizeButton->HandleEvent(e);
}


void MenuScene::ProcessInput(const Uint8* keyState) {
    if (startButton && startButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<MissionScene>(mGame , mRenderer));
        return;
    }
    if (storeButton && storeButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<ShopScene>(mGame, mRenderer));
        return;
    }
    if (customizeButton && customizeButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<CustomizeScene>(mGame, mRenderer));
        return;
    }
}

void MenuScene::Update(float deltaTime) {
    
}

void MenuScene::Draw(SDL_Renderer* renderer) {
    if (mTexture) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mTexture, nullptr, &destRect);
    }

    if (mBackTexture) {
        SDL_Rect destRect = { 0, 620, 276, 148 };
        SDL_RenderCopy(renderer, mBackTexture, nullptr, &destRect);
    }

    if (mWeaponTexture) {
        SDL_Rect destRect = { 10, 630, 256, 128 };
        SDL_RenderCopy(renderer, mWeaponTexture, nullptr, &destRect);
    }

    // ボタンは1回だけ描画
    if (startButton) startButton->Draw(renderer);
    if (storeButton) storeButton->Draw(renderer);
    if (customizeButton) customizeButton->Draw(renderer);

    if (mCurrencyText) {
        SDL_RenderCopy(renderer, mCurrencyText, nullptr, &mCurrencyRect);
    }
}

// ==============================================




// =============== CustomizeScene 実装 ===============
CustomizeScene::CustomizeScene(Game* game, SDL_Renderer* renderer)
    : mGame(game)
    , mRenderer(renderer) {

}

CustomizeScene::~CustomizeScene() {
    UnloadContent();
}

std::string GetIconPath(const std::string& weaponName)
{
    WeaponData data;
    if (data.LoadFromJSON("Assets/Data/WeaponData.json", weaponName)) {
        return data.weaponIcon;
    }
    else {
        SDL_Log("Icon path could not be loaded for %s", weaponName.c_str());
        return "Assets/WeaponDefault.png";
    }
}

void CustomizeScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/techno_02.mp3", -1);
    mTexture = mGame->GetTexture("Assets/Base.png");
    if (!mTexture) {
        SDL_Log("Failed to load title image");
    }

    mFont = TTF_OpenFont("Assets/NSJP.ttf", 32);
    if (!mFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
    }

    quitButton = std::make_unique<UIButton>(800, 700, 200, 60, "Quit", mRenderer, mFont);

    mHUDActor = std::make_unique<HUDActor>(mGame);
    mHUDActor->SetPosition(Vector2(0.0f, 0.0f));
    mGame->AddActor(mHUDActor.get());

    mScrollList = std::make_unique<ScrollItemComponent>(mHUDActor.get(), mRenderer, mFont);
    mScrollList->SetItemSize(128, 128);
    mScrollList->SetViewport({ 150, 100, 700, 600 });

    if (mGame->mHasPendingWeapon) {
        mCurrentWeaponName = mGame->mPendingWeapon.name;
    }

    std::ifstream file("Assets/Data/WeaponData.json");
    if (file.is_open()) {
        json j;
        file >> j;
        file.close();

        if (j.contains("weapons")) {
            for (auto& [weaponName, weaponInfo] : j["weapons"].items()) {
                if (weaponInfo.value("GetWeapon", false)) {
                    SDL_Texture* tex = mGame->GetTexture(weaponInfo.value("weaponIcon", "Assets/Bullet.png"));
                    std::string description = weaponInfo.value("description", "説明文がありません");

                    mScrollList->AddItem(
                        tex,
                        weaponName,
                        [this, weaponName]() {
                            WeaponData data;
                            if (data.LoadFromJSON("Assets/Data/WeaponData.json", weaponName)) {
                                mGame->mPendingWeapon = data;
                                mGame->mHasPendingWeapon = true;

                                if (!mCurrentWeaponName.empty()) {
                                    mScrollList->UpdateButtonText(mCurrentWeaponName, "Equip");
                                }

                                mCurrentWeaponName = weaponName;
                                mScrollList->UpdateButtonText(mCurrentWeaponName, "Equipped");
                            }
                        },
                        "Equip",
                        description 
                    );
                }
            }
        }
    }

    if (!mCurrentWeaponName.empty()) {
        mScrollList->UpdateButtonText(mCurrentWeaponName, "Equipped");
    }
}

void CustomizeScene::UnloadContent() {
    if (mFont) {
        TTF_CloseFont(mFont);
        mFont = nullptr;
    }

    if (mTexture) {
        mTexture = nullptr;
    }
}

void CustomizeScene::ProcessEvent(const SDL_Event& e) {
    if (quitButton) quitButton->HandleEvent(e);
    if (mScrollList) mScrollList->ProcessEvent(e);
}


void CustomizeScene::ProcessInput(const Uint8* keyState) {
    if (quitButton && quitButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<MenuScene>(mGame, mGame->GetRenderer()));
        return;
    }
}

void CustomizeScene::Update(float deltaTime) {

}

void CustomizeScene::Draw(SDL_Renderer* renderer) {
    if (mTexture) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mTexture, nullptr, &destRect);
        quitButton->Draw(renderer);
    }

    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect scrollBG = { 150, 100, 700, 600 };
    SDL_RenderFillRect(renderer, &scrollBG);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    if (mScrollList) {
        mScrollList->Draw(renderer, Vector2(0, 0));
    }
}
// ==============================================



// ============== = ShopScene 実装 ============== =
using json = nlohmann::json;

ShopScene::ShopScene(Game* game, SDL_Renderer* renderer)
    : mGame(game)
    , mRenderer(renderer) {
}

ShopScene::~ShopScene() {
    UnloadContent();
}

void ShopScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/techno_02.mp3", -1);
    mTexture = mGame->GetTexture("Assets/Shop.png");
    mSoldOutTexture = mGame->GetTexture("Assets/SOLD OUT.png");
    BuySound = Mix_LoadWAV("Assets/Audio/BuySound.wav");
    mFont = TTF_OpenFont("Assets/NSJP.ttf", 32);
    quitButton = std::make_unique<UIButton>(800, 700, 200, 60, "Quit", mRenderer, mFont);
    mHUDActor = std::make_unique<HUDActor>(mGame);
    mHUDActor->SetPosition(Vector2(0.0f, 0.0f));
    mGame->AddActor(mHUDActor.get());
    mScrollList = std::make_unique<ScrollItemComponent>(mHUDActor.get(), mRenderer, mFont);
    mScrollList->SetItemSize(128, 128); 
    mScrollList->SetViewport({ 150, 100, 700, 600 });
    Refresh();
}

void ShopScene::UnloadContent() {
    if (mFont) {
        TTF_CloseFont(mFont);
        mFont = nullptr;
    }
    if (mCurrencyText) {
        SDL_DestroyTexture(mCurrencyText);
        mCurrencyText = nullptr;
    }
    if (BuySound) {
        Mix_FreeChunk(BuySound);
        BuySound = nullptr;
    }
    mTexture = nullptr;
}

void ShopScene::Refresh() {
    if (mCurrencyText) {
        SDL_DestroyTexture(mCurrencyText);
        mCurrencyText = nullptr;
    }
    int currency = mGame->GetDataManager().GetCurrency();
    std::string text = "CREDITS: " + std::to_string(currency);
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderUTF8_Blended(mFont, text.c_str(), white);
    if (surface) {
        mCurrencyText = SDL_CreateTextureFromSurface(mRenderer, surface);
        mCurrencyRect = { 50, 50, surface->w, surface->h };
        SDL_FreeSurface(surface);
    }
    bool allWeaponsSoldOut = true;
    if (mScrollList) {
        mScrollList->ClearItems();
        std::ifstream file("Assets/Data/WeaponData.json");
        if (file.is_open()) {
            json j;
            file >> j;
            file.close();
            if (j.contains("weapons")) {
                for (auto& [weaponName, weaponInfo] : j["weapons"].items()) {
                    bool isWeaponObtained = weaponInfo.value("GetWeapon", false);
                    if (!isWeaponObtained) {
                        allWeaponsSoldOut = false;

                        int price = weaponInfo.value("price", 0);
                        std::string iconPath = weaponInfo.value("weaponIcon", "Assets/Bullet.png");
                        SDL_Texture* tex = mGame->GetTexture(iconPath);
                        std::string description = weaponInfo.value("description", "説明文がありません");

                        mScrollList->AddItem(
                            tex,
                            weaponName,
                            [this, weaponName, price]() { 
                                if (mGame->GetDataManager().GetCurrency() >= price) {
                                    if (this->mGame->GetDataManager().UseCurrency(price)) {
                                        this->mGame->AddObtainedWeapon(weaponName);
                                        if (BuySound) Mix_PlayChannel(-1, BuySound, 0);
                                        this->Refresh();
                                    }
                                }
                            },
                            "Buy",
                            price,
                            description
                        );

                    }
                }
            }
        }
        mScrollList->SortItemsByPrice();
    }
    mAllWeaponsSoldOut = allWeaponsSoldOut;
}

void ShopScene::SelectWeapon(const std::string& weaponName, SDL_Texture* texture, const std::string& description) {
    WeaponData data;
    if (!data.LoadFromJSON("Assets/Data/WeaponData.json", weaponName)) {
        return;
    }
    int price = data.price;
    mScrollList->AddItem(
        texture,
        weaponName,
        [this, weaponName]() {
            WeaponData data;
            if (data.LoadFromJSON("Assets/Data/WeaponData.json", weaponName)) {
                if (this->mGame->GetDataManager().UseCurrency(data.price)) {
                    Mix_PlayChannel(-1, this->BuySound, 0);
                    this->mGame->AddObtainedWeapon(weaponName);
                    this->Refresh();
                }
            }
        },
        "Buy",
        price,
        description
    );
}

void ShopScene::ProcessEvent(const SDL_Event& e) {
    if (quitButton) quitButton->HandleEvent(e);
    if (mScrollList) {
        mScrollList->ProcessEvent(e);
    }
}

void ShopScene::ProcessInput(const Uint8* keyState) {
    if (quitButton && quitButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<MenuScene>(mGame, mRenderer));
        return;
    }
}

void ShopScene::Update(float deltaTime) {
}

void ShopScene::Draw(SDL_Renderer* renderer) {
    if (mTexture) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mTexture, nullptr, &destRect);
    }
    if (quitButton) {
        quitButton->Draw(renderer);
    }
    if (mAllWeaponsSoldOut) {
        if (mSoldOutTexture) {
            int texW = 0, texH = 0;
            SDL_QueryTexture(mSoldOutTexture, nullptr, nullptr, &texW, &texH);
            SDL_Rect dstRect = { (1024 - texW) / 2, (768 - texH) / 2, texW, texH };
            SDL_RenderCopy(renderer, mSoldOutTexture, nullptr, &dstRect);
        }
    }
    else {
        SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect scrollBG = { 150, 100, 700, 600 };
        SDL_RenderFillRect(renderer, &scrollBG);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (mScrollList) {
            mScrollList->Draw(renderer, Vector2(0, 0));
        }
    }
    if (mCurrencyText) {
        SDL_RenderCopy(renderer, mCurrencyText, nullptr, &mCurrencyRect);
    }
}

// ==============================================


// =============== MissionScene 実装 ===============
MissionScene::MissionScene(Game* game, SDL_Renderer* renderer)
    : mGame(game)
    , mRenderer(renderer)
{
    
}

MissionScene::~MissionScene() {
    if (mFont) TTF_CloseFont(mFont);
}

void MissionScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/techno_03.mp3", -1);
    mTexture = mGame->GetTexture("Assets/Shop.png");
    mFont = TTF_OpenFont("Assets/NSJP.ttf", 24);
    quitButton = std::make_unique<UIButton>(800, 700, 200, 60, "Quit", mRenderer, mFont);
    mDisplayBackTexture = mGame->GetTexture("Assets/DisplayFrame.png");

    int startY = 150;
    int buttonW = 250;
    int buttonH = 80;
    int spacing = 30;

    for (int i = 0; i < 3; ++i) {
        int targetID = (i < 3) ? i : 100;
        mMissionIDs[i] = targetID;

        missionButton[i] = std::make_unique<UIButton>(
            50, startY + i * (buttonH + spacing),
            buttonW, buttonH,
            (targetID < 100 ? "Mission " : "Random ") + std::to_string(i + 1),
            mRenderer, mFont
        );

        MissionData data;
        if (MissionDataLoader::LoadMission(targetID, data)) {
            mMissionIcons[i] = mGame->GetTexture(data.iconPath);
        }
    }
}

void MissionScene::ProcessEvent(const SDL_Event& e) {
    if (quitButton) quitButton->HandleEvent(e);

    for (int i = 0; i < 3; ++i) {
        if (missionButton[i]) {
            missionButton[i]->HandleEvent(e);
            if (e.type == SDL_MOUSEMOTION && missionButton[i]->IsHovered()) {
                mSelectedMissionID = mMissionIDs[i];
            }
        }
    }
}

void MissionScene::ProcessInput(const Uint8* keyState) {
    if (quitButton && quitButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<MenuScene>(mGame, mRenderer));
        return;
    }

    for (int i = 0; i < 3; ++i) {
        if (missionButton[i] && missionButton[i]->IsClicked()) {
            mGame->ChangeScene(std::make_unique<GameScene>(mGame, mRenderer, mMissionIDs[i]));
            return;
        }
    }
}

void MissionScene::Update(float deltaTime) {

}

void DrawMultiLineText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y)
{
    std::stringstream ss(text);
    std::string line;
    int currentY = y;
    int lineHeight = 0;
    SDL_Color white = { 255, 255, 255, 255 };

    while (std::getline(ss, line, '\n'))
    {
        if (line.empty()) {
            if (lineHeight == 0) {
                int w, h;
                TTF_SizeUTF8(font, " ", &w, &h);
                lineHeight = h;
            }
            currentY += lineHeight;
            continue;
        }

        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, line.c_str(), white);
        if (surface)
        {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture)
            {
                SDL_Rect destRect = { x, currentY, surface->w, surface->h };
                SDL_RenderCopy(renderer, texture, nullptr, &destRect);
                SDL_DestroyTexture(texture);
            }

            lineHeight = surface->h;
            currentY += lineHeight;
            SDL_FreeSurface(surface);
        }
    }
}

void MissionScene::Draw(SDL_Renderer* renderer) {
    if (mTexture) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mTexture, nullptr, &destRect);
    }

    if (quitButton) quitButton->Draw(renderer);
    for (int i = 0; i < 3; ++i) {
        if (missionButton[i]) {
            missionButton[i]->Draw(renderer);
        }
    }

    SDL_Rect displayRect = { 400, 100, 580, 500 };
    if (mDisplayBackTexture) {
        SDL_RenderCopy(renderer, mDisplayBackTexture, nullptr, &displayRect);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &displayRect);
    }

    int currentID = mSelectedMissionID;
    static MissionData cachedData;
    static int lastLoadedID = -1;

    if (currentID != lastLoadedID && currentID >= 0) {
        if (MissionDataLoader::LoadMission(currentID, cachedData)) {
            lastLoadedID = currentID;
        }
    }

    if (lastLoadedID != -1) {
        int textX = 420;
        int descY = 400;
        DrawMultiLineText(renderer, mFont, cachedData.description, textX, descY);
        std::string rewardText = "REWARDS : \n" + std::to_string(cachedData.Rewards.BaseCredits) + "\n";
        DrawMultiLineText(renderer, mFont, rewardText, textX, 500);

        if (currentID >= 0 && currentID < 3 && mMissionIcons[currentID]) {
            SDL_Texture* iconTex = mMissionIcons[currentID];
            int originalW, originalH;
            SDL_QueryTexture(iconTex, nullptr, nullptr, &originalW, &originalH);
            const int MAX_W = 480;
            const int MAX_H = 200;
            const int START_X = 450;
            const int START_Y = 150;
            SDL_Rect iconRect;
            if (originalW > 0 && originalH > 0) {
                float aspect = (float)originalW / originalH;
                int targetW = MAX_W;
                int targetH = (int)(targetW / aspect);
                if (targetH > MAX_H) {
                    targetH = MAX_H;
                    targetW = (int)(targetH * aspect);
                }
                iconRect.w = targetW;
                iconRect.h = targetH;
                iconRect.x = START_X + (MAX_W - targetW) / 2;
                iconRect.y = START_Y + (MAX_H - targetH) / 2;
                SDL_RenderCopy(renderer, iconTex, nullptr, &iconRect);
            }
        }
    }
}
// ==============================================

// =============== GameScene 実装 ===============
GameScene::GameScene(Game* game, SDL_Renderer* renderer, int missionID)
    : mGame(game)
    , mMissionID(missionID)
{

}

void GameScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/bgm.mp3", -1);
}

void GameScene::LoadData() {
    mGame->LoadData(mMissionID);
}

void GameScene::ProcessInput(const Uint8* keyState) {
    mGame->GameInput();
}

void GameScene::Update(float deltaTime) {
    mGame->GameUpdate();
}

void GameScene::Draw(SDL_Renderer* renderer) {
    mGame->GameGenerate();
}
// ==============================================


// ============== ResultScene 実装 ==============
ResultScene::ResultScene(Game* game, SDL_Renderer* renderer, bool isWin, int earnedCurrency)
    : mGame(game)
    , mRenderer(renderer)
    , mIsWin(isWin)
    , mEarnedCurrency(earnedCurrency)
    , mFont(nullptr)
    , mResultTextTexture(nullptr)
    , mCurrencyTextTexture(nullptr)
{
}

ResultScene::~ResultScene() {

}

void ResultScene::LoadContent() {
    mGame->PlayBGM("Assets/Audio/techno_03.mp3", -1);

    if (mIsWin) {
        mBackTexture = mGame->GetTexture("Assets/GameClear.png");
    }
    else {
        mBackTexture = mGame->GetTexture("Assets/GameOver.png");
    }

    mFont = TTF_OpenFont("Assets/NSJP.ttf", 40);
    if (!mFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
    }

    int missionID = mGame->GetLastMissionID();
    MissionData missionData;

    if (MissionDataLoader::LoadMission(missionID, missionData))
    {
        if (mIsWin)
        {
            int baseCredits = missionData.Rewards.BaseCredits;
            float difficultyMultiplier = missionData.Rewards.BonusMultiplier;
            mEarnedCurrency = (int)(baseCredits * difficultyMultiplier);
        }
        else
        {
            mEarnedCurrency = 0;
        }
    }
    else
    {
        SDL_Log("ERROR: Mission Data Load Failed for reward calculation. ID: %d", missionID);
        mEarnedCurrency = 0;
    }

    mGame->GetDataManager().AddCurrency(mEarnedCurrency);

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color red = { 255, 0, 0, 255 };
    SDL_Color green = { 0, 255, 0, 255 };

    std::string resultStr = mIsWin ? "MISSION SUCCESS" : "MISSION FAILED";
    SDL_Color resultColor = mIsWin ? green : red;

    SDL_Surface* resultSurface = TTF_RenderUTF8_Blended(mFont, resultStr.c_str(), resultColor);
    if (!resultSurface) {
        SDL_Log("Failed to create result surface: %s", TTF_GetError());
    }
    else {
        mResultTextTexture = SDL_CreateTextureFromSurface(mRenderer, resultSurface);
        mResultTextRect = { (1024 - resultSurface->w) / 2, 100, resultSurface->w, resultSurface->h };
        SDL_FreeSurface(resultSurface);
    }

    std::string currencyText = "CREDITS EARNED: " + std::to_string(mEarnedCurrency);

    SDL_Surface* currencySurface = TTF_RenderUTF8_Blended(mFont, currencyText.c_str(), white);
    if (!currencySurface) {
        SDL_Log("Failed to create currency surface: %s", TTF_GetError());
    }
    else {
        mCurrencyTextTexture = SDL_CreateTextureFromSurface(mRenderer, currencySurface);
        mCurrencyTextRect = { (1024 - currencySurface->w) / 2, 200, currencySurface->w, currencySurface->h };
        SDL_FreeSurface(currencySurface);
    }

    menuButton = std::make_unique<UIButton>(412, 550, 300, 90, "Back to Menu", mRenderer, mFont);
}

void ResultScene::UnloadContent() {
    if (mFont) {
        TTF_CloseFont(mFont);
        mFont = nullptr;
    }

    if (mResultTextTexture) {
        SDL_DestroyTexture(mResultTextTexture);
        mResultTextTexture = nullptr;
    }

    if (mCurrencyTextTexture) {
        SDL_DestroyTexture(mCurrencyTextTexture);
        mCurrencyTextTexture = nullptr;
    }
}

void ResultScene::ProcessEvent(const SDL_Event& e) {
    if (menuButton) menuButton->HandleEvent(e);
}


void ResultScene::ProcessInput(const Uint8* keyState) {
    if (menuButton && menuButton->IsClicked()) {
        mGame->ChangeScene(std::make_unique<MenuScene>(mGame, mRenderer));
        return;
    }
}

void ResultScene::Update(float deltaTime) {

}

void ResultScene::Draw(SDL_Renderer* renderer) {
    if (mBackTexture) {
        SDL_Rect destRect = { 0, 0, 1024, 768 };
        SDL_RenderCopy(renderer, mBackTexture, nullptr, &destRect);
    }

    if (mResultTextTexture) {
        SDL_RenderCopy(renderer, mResultTextTexture, nullptr, &mResultTextRect);
    }

    if (mCurrencyTextTexture) {
        SDL_RenderCopy(renderer, mCurrencyTextTexture, nullptr, &mCurrencyTextRect);
    }

    if (menuButton) menuButton->Draw(renderer);
}
// ==============================================