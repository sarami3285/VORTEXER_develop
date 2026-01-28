#pragma once
#include <SDL.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "Math.h"
#include "CameraComponent.h"
#include "Player.h"
#include <SDL_mixer.h>
#include "SDL_ttf.h"
#include "HUDActor.h"
#include "HPComponent.h"
#include "HUDSpriteComponent.h"
#include "Scene.h"
#include "DataManager.h"
#include "BlinkingTextComponent.h"
#include "WeaponData.h"
#include "Enemy.h"
#include "LineComponent.h"
#include "MissionDataLoader.h"

using namespace std;
class Actor;
class Player;
class Enemy;
class CameraComponent;
class SpriteComponent;
class HUDActor;
class HUDSpriteComponent;
class HPComponent;
class Scene;
class BlinkingTextComponent;
class TargetActor;
struct UpgradeOption;

enum class GameState {
    Title,
    BaseMenu,
    MissionStart,
    InMission,
    Result,
    ReturnToBase
};

class Game {
public:
    Game();
    bool Initialize();
    void RunLoop();
    void Shutdown();

    class Enemy* GetNearestEnemy(const Vector2& pos);
    class Actor* GetNearestTarget(const Vector2& pos);

    void ChangeScene(std::unique_ptr<Scene> newScene);

    void AddActor(class Actor* actor);
    void RemoveActor(class Actor* actor);

    void AddSprite(class SpriteComponent* sprite);
    void RemoveSprite(class SpriteComponent* sprite);

    void AddEnemy(class Enemy* enemy);
    void RemoveEnemy(class Enemy* enemy);
    void AddTargetActor(class TargetActor* target);
    void RemoveTargetActor(class TargetActor* target);

    SDL_Texture* GetTexture(const std::string& fileName);

    vector<class Actor*>& GetActors() { return mActors; }

    vector<class Enemy*>& GetEnemies() { return mEnemies; }

    DataManager& GetDataManager() { return dataManager; }

    void GameInput();
    void GameUpdate();
    void GameGenerate();

    void PlayBGM(const std::string& fileName, int loop = -1);
    void StopBGM();

    void LoadData(int missionID);
    void UnloadData();
    void AddAlly(class AllyUnit* ally);
    void RemoveAlly(class AllyUnit* ally);
    void InstanceEnemy(const EnemyConfig& config , Enemy*& enemy,Vector2 pos);

    SDL_Renderer* GetRenderer() const { return mRenderer; }
    bool IsRunning() const { return mIsRunning; }

    std::vector<std::function<void(SDL_Renderer*, const Vector2&)>> mImmediateDraws;
    std::vector<class Actor*> mDeadActors;

    void AddImmediateDraw(std::function<void(SDL_Renderer*, const Vector2&)> func) {
        mImmediateDraws.emplace_back(func);
    }

    void RequestSceneChange(std::unique_ptr<Scene> newScene);

    void UpdateObtainedWeapons();
    const std::vector<std::string>& GetObtainedWeaponNames()const;
    void AddObtainedWeapon(const std::string& weaponName);

    WeaponData mPendingWeapon;
    bool mHasPendingWeapon = false;
    int enemyCount;
    std::vector<std::string> mObtainedWeaponNames;
    std::vector<std::string> mObtainedWeapons;

    bool mMissionCompleted = false;
    Uint32 mMissionCompleteTime = 0;
    bool mSceneChangeRequested = false;
    std::unique_ptr<Scene> mNextScene;


    Vector2 mCameraPos;
    CameraComponent* GetCamera() const { return mCamera; }
    void SetCamera(CameraComponent* camera) { mCamera = camera; }
    Player* GetPlayer()const { return mPlayer; }
    void SetPlayer(Player* player) { mPlayer = player; }
    vector<class AllyUnit*> GetAllies() const { return mAllies; }
    class Actor* GetNearestAttackTarget(const Vector2& pos);


    void AddLineComponent(class LineComponent* line);
    void RemoveLineComponent(class LineComponent* line);

    void SetCurrentMissionID(int missionID) { mCurrentMissionID = missionID; }
    int GetLastMissionID() const { return mCurrentMissionID; }

    std::vector<UpgradeOption> GetRandomUpgrades(int count);

private:
    std::vector<class LineComponent*> mLineComponents;
    std::vector<class CollisionComponent*> mCollisionComponents;
    void ResetGameSceneState();

    vector<class Actor*> mActors;
    vector<class Actor*> mWaitingActors;
    vector<class SpriteComponent*> mSprites;

    vector<class Enemy*> mEnemies;
    vector<class AllyUnit*> mAllies;
    std::vector<class Actor*> mTargetActors;

    int mCurrentMissionID = -1;
    MissionData mMissionData; 
    float mDefenseTimer = 0.0f;
    float mGameTimer = 0.0f;

    std::unique_ptr<Scene> mCurrentScene;

    unordered_map<std::string, SDL_Texture*> mTextures;

    Mix_Music* mCurrentBGM = nullptr;

    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    TTF_Font* mFont;
    Uint32 mTicksCount;
    bool mIsRunning;
    bool mUpdatingActors;
    class Player* mPlayer;
    CameraComponent* mCamera;
    HUDSpriteComponent* mHPBar;
    HUDSpriteComponent* mHPBack;
    HUDSpriteComponent* mStaminaBar;
    HUDSpriteComponent* mStaminaBack;
    HUDSpriteComponent* mLevelBar;
    HUDSpriteComponent* mLevelBack;
    HPComponent* mHPComponent;
    DataManager dataManager;

    SDL_Surface* mMissionCompleteSurface = nullptr;
    SDL_Texture* mMissionCompleteTexture = nullptr;

    std::unique_ptr<BlinkingTextComponent> mMissionTextComponent;

    HUDActor* mHUDActor;

    float mOceanTime;
    enum EFadeState
    {
        ENone,      // フェードなし（通常時）
        EFadeOut,   // 画面を暗くする処理中
        EFadeIn     // 画面を明るくする処理中
    };
    EFadeState mFadeState = ENone;
    float mFadeTimer = 0.0f;      
    float mFadeDuration = 0.5f;

    float mMissionStartTime = 0.0f; 
    int mEnemiesKilledCount = 0; 
    int mCreditsEarned = 0;
};