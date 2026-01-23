#include "Game.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <random>
#include "vector"
#include "memory"
#include "algorithm"
#include "Actor.h"
#include "SpriteComponent.h"
#include "LowLevelEnemy.h"
#include "Random.h"
#include "Player.h"
#include "TileMapComponent.h"
#include "CameraComponent.h"
#include "Scenes.h"
#include "HUDSpriteComponent.h"
#include "DroneGuard.h"
#include "SentryGun.h"
#include "AllyAIComponent.h" 
#include "AllyUnit.h"
#include "MissionDataLoader.h"
#include "LineComponent.h"
#include "Enemy.h"
#include "TargetActor.h"
#include "BattleShip.h"
#include "PatrolComponent.h"
#include "Interceptor.h"

using namespace std;

Game::Game()
	: mWindow(nullptr)
	, mRenderer(nullptr)
	, mIsRunning(true)
	, mUpdatingActors(false) {
}

bool Game::Initialize() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		SDL_Log("初期化エラー：SDLが初期化できませんでした _ %s", SDL_GetError());
		return false;
	}

	if (TTF_Init() != 0) {
		SDL_Log("初期化エラー：フォントシステムの初期化ができませんでした _ %s", TTF_GetError());
		return false;
	}

	mWindow = SDL_CreateWindow("VORTEXER", 100, 100, 1024, 768, 0);
	if (!mWindow) {
		SDL_Log("初期化エラー：ウィンドウが作成できませんでした _ %s", SDL_GetError());
		return false;
	}

	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!mRenderer) {
		SDL_Log("初期化エラー：レンダラーが作成できませんでした _ %s", SDL_GetError());
		return false;
	}

	mFont = TTF_OpenFont("Assets/NSJP.ttf", 32);
	if (!mFont) {
		SDL_Log("初期化エラー：共通フォントのロードに失敗しました _ %s", TTF_GetError());
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		SDL_Log("初期化エラー：音声システムの初期化ができませんでした _ %s", Mix_GetError());
		return false;
	}

	int loadedCurrency = 0;
	std::string equippedWeaponName;
	if (dataManager.LoadGameData("Assets/Data/SaveData.json", equippedWeaponName)) {
		if (!equippedWeaponName.empty()) {
			WeaponData weapon;
			if (weapon.LoadFromJSON("Assets/Data/WeaponData.json", equippedWeaponName)) {
				mPendingWeapon = weapon;
				mHasPendingWeapon = true;
			}
		}
	}

	Random::Init();
	mTicksCount = SDL_GetTicks();
	ChangeScene(std::make_unique<TitleScene>(this, mRenderer));

	return true;
}

void Game::RunLoop() {
	Uint32 lastTicks = SDL_GetTicks();

	while (mIsRunning) {
		Uint32 currentTicks = SDL_GetTicks();
		float deltaTime = (currentTicks - lastTicks) / 1000.0f;
		if (deltaTime > 0.05f) deltaTime = 0.05f;
		lastTicks = currentTicks;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				mIsRunning = false;
			}

			if (mCurrentScene) {
				mCurrentScene->ProcessEvent(e);
			}
		}

		const Uint8* keyState = SDL_GetKeyboardState(NULL);

		if (mCurrentScene) {
			mCurrentScene->ProcessInput(keyState);
			mCurrentScene->Update(deltaTime);

			SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
			SDL_RenderClear(mRenderer);

			mCurrentScene->Draw(mRenderer);
			SDL_RenderPresent(mRenderer);

			if (mCurrentScene->IsFinished()) {
				ChangeScene(mCurrentScene->NextScene());
			}
		}
	}
}

void Game::GameUpdate() {
	if (!mPlayer || !mPlayer->mHPComponent || !mHPBar || !mStaminaBar) {
		return;
	}

	float hpPercent = static_cast<float>(mPlayer->mHPComponent->GetHP()) / mPlayer->mHPComponent->GetMaxHP();
	mHPBar->SetHPPercent(hpPercent);

	float staminaPercent = (mPlayer->ic->mBoostTimer / mPlayer->ic->mBoostDuration);
	mStaminaBar->SetHPPercent(staminaPercent);

	if (mPlayer && mLevelBar) {
		int currentExp = mPlayer->GetCurrentEXP();
		int expToNextLevel = mPlayer->GetEXPToNextLevel();

		float expPercent = (expToNextLevel > 0) ? (static_cast<float>(currentExp) / expToNextLevel) : 0.0f;
		mLevelBar->SetHPPercent(expPercent);
	}

	while (!SDL_TICKS_PASSED(SDL_GetTicks(), mTicksCount + 16));

	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	if (deltaTime > 0.05f) {
		deltaTime = 0.05f;
	}
	mTicksCount = SDL_GetTicks();

	if (mPlayer && mPlayer->GetState() != Actor::EAlive && !mSceneChangeRequested) {
		SDL_Log("GameOver: Player is dead. Requesting Result Scene.");

		const bool isWin = false;
		const int earnedCurrency = 0;

		mSceneChangeRequested = true;
		mNextScene = std::make_unique<ResultScene>(this, mRenderer, isWin, earnedCurrency);
		return;
	}

	mUpdatingActors = true;
	for (auto actor : mActors) {
		actor->Update(deltaTime);
	}
	mUpdatingActors = false;

	for (auto pending : mWaitingActors) {
		mActors.emplace_back(pending);
	}
	mWaitingActors.clear();

	std::vector<Actor*> deadActors;
	for (auto actor : mActors) {
		if (actor->GetState() == Actor::EStop) {
			if (std::find(deadActors.begin(), deadActors.end(), actor) == deadActors.end()) {
				deadActors.emplace_back(actor);
			}
		}
	}

	for (auto actor : deadActors) {
		if (actor == mPlayer) {
			continue;
		}
		RemoveActor(actor);
	}

	bool missionObjectiveAchieved = false;

	if (mMissionData.targetType == "Elimination") {
		bool allEnemiesDead = true;
		for (auto actor : mActors) {
			if (dynamic_cast<Enemy*>(actor) && actor->GetState() == Actor::EAlive) {
				allEnemiesDead = false;
				break;
			}
		}
		missionObjectiveAchieved = allEnemiesDead;
	}
	else if (mMissionData.targetType == "Destruction") {
		if (mTargetActors.empty()) {
			missionObjectiveAchieved = true;
		}
		else {
			bool anyTargetAlive = false;
			for (auto target : mTargetActors) {
				if (target->GetState() == Actor::EAlive) {
					anyTargetAlive = true;
					break;
				}
			}
			missionObjectiveAchieved = !anyTargetAlive;
		}
	}
	else if (mMissionData.targetType == "Defense") {
		if (!mTargetActors.empty() && mTargetActors[0]->GetState() == Actor::EStop) {
			SDL_Log("Mission Failed: Defense Target Destroyed! Requesting Result Scene.");

			const bool isWin = false;
			const int earnedCurrency = 0;

			mSceneChangeRequested = true;
			mNextScene = std::make_unique<ResultScene>(this, mRenderer, isWin, earnedCurrency);
			return;
		}

		mDefenseTimer += deltaTime;
		if (mDefenseTimer >= mMissionData.defenseDuration) {
			missionObjectiveAchieved = true;
		}
	}

	if (missionObjectiveAchieved && !mMissionCompleted) {
		mMissionCompleted = true;
		mMissionCompleteTime = SDL_GetTicks();
	}

	if (mMissionCompleted) {
		if (mMissionTextComponent) {
			mMissionTextComponent->Update(deltaTime);
		}

		Uint32 currentTime = SDL_GetTicks();
		if (currentTime - mMissionCompleteTime >= 5000) {
			if (!mSceneChangeRequested) {
				mSceneChangeRequested = true;

				const bool isWin = true;
				const int earnedCurrency = mCreditsEarned;
				mNextScene = std::make_unique<ResultScene>(this, mRenderer, isWin, earnedCurrency);
			}
		}
	}

	if (mHUDActor && mHUDActor->GetMiniMapLogic()) {
		std::vector<Vector2> enemyPositions;
		for (auto actor : mActors) {
			for (auto enemy : mEnemies) {
				if (enemy->GetState() == Actor::EAlive) {
					enemyPositions.push_back(enemy->GetPosition());
				}
			}
		}
		mHUDActor->GetMiniMapLogic()->SetTargets(enemyPositions);
		mHUDActor->GetMiniMapLogic()->SetPlayerPosition(mPlayer->GetPosition());
	}

	if (mSceneChangeRequested) {
		ChangeScene(std::move(mNextScene));
		mSceneChangeRequested = false;
	}

	mOceanTime += deltaTime;

	if (mCurrentScene && dynamic_cast<GameScene*>(mCurrentScene.get())) {
		TileMapComponent* tileMap = nullptr;
		for (auto actor : mActors) {
			tileMap = actor->GetComponent<TileMapComponent>();
			if (tileMap) break;
		}

		if (tileMap) {
			tileMap->UpdateFade(deltaTime);
		}

		if (tileMap && tileMap->IsAnimated()) {
			const float TIME_PER_FRAME = 1.50f;
			const int TOTAL_FRAMES = 2;

			if (TOTAL_FRAMES > 0) {
				int currentFrame = (int)(mOceanTime / TIME_PER_FRAME);
				currentFrame = currentFrame % TOTAL_FRAMES;
				tileMap->SetAnimationFrame(currentFrame);
			}
		}
	}

	for (auto& drawFunc : mImmediateDraws) {
		drawFunc(mRenderer, mCameraPos);
	}
	mImmediateDraws.clear();
}

void Game::GameGenerate() {
	SDL_SetRenderDrawColor(mRenderer, 100, 200, 100, 255);
	SDL_RenderClear(mRenderer);

	if (mPlayer) {
		mCameraPos = mPlayer->GetCameraComponent()->GetCameraPos();
	}

	for (auto sprite : mSprites) {
		if (sprite && sprite->GetOwner()->GetState() != Actor::EStop) {
			sprite->Draw(mRenderer, mCameraPos);
		}
	}

	for (auto line : mLineComponents) {
		if (line && line->GetOwner()->GetState() != Actor::EStop) {
			line->Draw(mRenderer, mCameraPos);
		}
	}

	if (mMissionCompleted && mMissionTextComponent) {
		mMissionTextComponent->Draw();
	}

	SDL_RenderPresent(mRenderer);
}

void Game::GameInput() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			mIsRunning = false;
			break;
		}
	}

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_ESCAPE]) {
		mIsRunning = false;
	}

	mUpdatingActors = true;
	for (auto actor : mActors) {
		actor->ProcessInput(keyState);
	}
	mUpdatingActors = false;
}

void Game::LoadData(int missionID) {
	SetCurrentMissionID(missionID);

	mPlayer = new Player(this);

	if (mHasPendingWeapon) {
		WeaponData baseData;
		if (baseData.LoadFromJSON("Assets/Data/WeaponData.json", mPendingWeapon.name)) {
			mPlayer->SetWeapon(baseData);
		}
		else {
			mPlayer->SetWeapon(mPendingWeapon);
		}
	}

	mMissionStartTime = SDL_GetTicks() / 1000.0f;
	mEnemiesKilledCount = 0;

	if (!MissionDataLoader::LoadMission(missionID, mMissionData)) {
		SDL_Log("ERROR: Failed to load mission data for ID %d. Using fallback.", missionID);
	}

	PlayBGM("Assets/Audio/Drumnbass_03.mp3", -1);
	mPlayer->SetPosition(Vector2(512.0f, 384.0f));
	mPlayer->SetRotation(Math::PiOver2);

	std::string equippedWeaponName;
	if (dataManager.LoadGameData("Assets/Data/SaveData.json", equippedWeaponName)) {
		if (!equippedWeaponName.empty()) {
			WeaponData baseWeapon;
			if (baseWeapon.LoadFromJSON("Assets/Data/WeaponData.json", equippedWeaponName)) {
				mPlayer->SetWeapon(baseWeapon);
			}
		}
	}

	if (mHasPendingWeapon) {
		mPlayer->SetWeapon(mPendingWeapon);
		mHasPendingWeapon = true;
	}

	CameraComponent::mMapWidth = mMissionData.mapWidth;
	CameraComponent::mMapHeight = mMissionData.mapHeight;

	int tileSize = mMissionData.mapTileConfig.tileSize;
	int tileNumWidth = static_cast<int>((CameraComponent::mMapWidth + tileSize - 1.0f) / tileSize);
	int tileNumHeight = static_cast<int>((CameraComponent::mMapHeight + tileSize - 1.0f) / tileSize);

	SDL_Texture* tileSetTex = GetTexture(mMissionData.mapTileConfig.tilesetTexture);

	Actor* mapActor = new Actor(this);
	TileMapComponent* tileMap = new TileMapComponent(mapActor);
	tileMap->SetTileSet(tileSetTex, tileSize, mMissionData.mapTileConfig.tileSetRows);

	std::vector<std::vector<int>> mapData(tileNumHeight, std::vector<int>(tileNumWidth, 0));
	srand(static_cast<unsigned int>(time(nullptr)));

	if (mMissionData.mapTileConfig.tileType == "City") {
		for (int y = 0; y < tileNumHeight; ++y) {
			for (int x = 0; x < tileNumWidth; ++x) {
				int r = rand() % 100;
				if (y < tileNumHeight / 3 || y > tileNumHeight * 2 / 3 || x < tileNumWidth / 3 || x > tileNumWidth * 2 / 3) {
					if (r < 60) mapData[y][x] = 0;
					else mapData[y][x] = 3;
				}
				else {
					if (r < 25) mapData[y][x] = 1;
					else if (r < 50) mapData[y][x] = 2;
					else if (r < 75) mapData[y][x] = 4;
					else mapData[y][x] = 5;
				}
			}
		}
	}
	else if (mMissionData.mapTileConfig.tileType == "Desert") {
		for (int y = 0; y < tileNumHeight; ++y) {
			for (int x = 0; x < tileNumWidth; ++x) {
				int r = rand() % 100;
				if (r < 20) mapData[y][x] = 1;
				else if (r < 40) mapData[y][x] = 2;
				else if (r < 60) mapData[y][x] = 3;
				else if (r < 80) mapData[y][x] = 4;
				else mapData[y][x] = 5;
			}
		}
	}
	else if (mMissionData.mapTileConfig.tileType == "Ocean") {
		for (int y = 0; y < tileNumHeight; ++y) {
			for (int x = 0; x < tileNumWidth; ++x) {
				mapData[y][x] = 0;
			}
		}
		tileMap->SetIsAnimated(true);
		tileMap->SetAnimationRows(mMissionData.mapTileConfig.tileSetRows);
	}

	tileMap->SetMapData(mapData);
	mapActor->SetPosition(Vector2(0, 0));

	mHUDActor = new HUDActor(this);
	mHPBack = new HUDSpriteComponent(mHUDActor);
	mHPBar = new HUDSpriteComponent(mHUDActor);
	mHPBack->SetTexture(GetTexture("Assets/HPBack.png"));
	mHPBar->SetTexture(GetTexture("Assets/HP.png"));
	mHPBack->SetOffset(Vector2(250.0f, 50.0f));
	mHPBar->SetOffset(Vector2(250.0f, 50.0f));
	mHPBar->SetHPPercent(1.0f);

	mStaminaBack = new HUDSpriteComponent(mHUDActor);
	mStaminaBar = new HUDSpriteComponent(mHUDActor);
	mStaminaBack->SetTexture(GetTexture("Assets/StaminaBack.png"));
	mStaminaBar->SetTexture(GetTexture("Assets/Stamina1.png"));
	mStaminaBack->SetOffset(Vector2(250.0f, 88.0f));
	mStaminaBar->SetOffset(Vector2(250.0f, 84.0f));
	mStaminaBar->SetHPPercent(1.0f);

	mLevelBack = new HUDSpriteComponent(mHUDActor);
	mLevelBar = new HUDSpriteComponent(mHUDActor);
	mLevelBack->SetTexture(GetTexture("Assets/HPBack.png"));
	mLevelBar->SetTexture(GetTexture("Assets/EXPBar.png"));
	const Vector2 LevelBarOffset(250.0f, 126.0f);
	mLevelBack->SetOffset(LevelBarOffset);
	mLevelBar->SetOffset(LevelBarOffset);
	mLevelBar->SetHPPercent(0.5f);

	auto weaponBack = new HUDSpriteComponent(mHUDActor);
	weaponBack->SetTexture(GetTexture("Assets/weaponBackGround.png"));
	weaponBack->SetOffset(Vector2(200.0f, 630.0f));

	auto weaponIcon = new HUDSpriteComponent(mHUDActor);
	if (mPlayer && mPlayer->GetWeaponComponent()) {
		string iconPath = mPlayer->GetWeaponComponent()->GetWeaponIconPath();
		weaponIcon->SetTexture(GetTexture(iconPath));
	}
	else {
		weaponIcon->SetTexture(this->GetTexture("Assets/Weapon1.png"));
	}

	weaponIcon->SetOffset(Vector2(200.0f, 630.0f));
	mHUDActor->SetPosition(Vector2(0.0f, 0.0f));

	if (mMissionData.targetType == "Destruction") {
		for (const auto& config : mMissionData.targetConfigs) {
			TargetActor* target = new TargetActor(this);
			target->SetPosition(config.position);

			if (target->GetHPComponent() && config.hp > 0) {
				target->GetHPComponent()->SetMaxHP(config.hp);
				target->GetHPComponent()->SetHP(config.hp);
			}

			if (auto sc = target->GetComponent<SpriteComponent>()) {
				if (!config.texture.empty()) {
					sc->SetTexture(GetTexture(config.texture));
				}
			}
			mTargetActors.push_back(target);
		}
	}
	else if (mMissionData.targetType == "Defense") {
		TargetActor* core = new TargetActor(this);
		if (mMissionData.defenseTarget.type == "Core") {
			core->SetPosition(mMissionData.defenseTarget.position);
		}

		if (core->GetHPComponent() && mMissionData.defenseTarget.hp > 0) {
			core->GetHPComponent()->SetMaxHP(mMissionData.defenseTarget.hp);
			core->GetHPComponent()->SetHP(mMissionData.defenseTarget.hp);
		}

		mTargetActors.push_back(core);
		mDefenseTimer = 0.0f;
	}

	for (const auto& config : mMissionData.enemyConfigs) {
		float mapW = CameraComponent::mMapWidth;
		float mapH = CameraComponent::mMapHeight;
		Vector2 pos;
		Enemy* enemy = nullptr;

		if (config.type == "Formation") {
			pos = config.centerPosition;
			InstanceEnemy(config, enemy, pos);
		}
		else {
			for (int i = 0; i < config.count; i++) {
				if (!config.fixedPositions.empty() && i < config.fixedPositions.size()) {
					pos = config.fixedPositions[i];
				}
				else {
					float randX = Random::GetFloatRange(100.0f, mapW - 100.0f);
					float randY = Random::GetFloatRange(100.0f, mapH - 100.0f);
					pos = Vector2(randX, randY);
				}

				InstanceEnemy(config, enemy, pos);

				if (enemy) {
					AddEnemy(enemy);
					HUDSpriteComponent* hpBar = new HUDSpriteComponent(mHUDActor);
					hpBar->SetTexture(GetTexture("Assets/HP1.png"));
					hpBar->SetOffset(Vector2(0, -50.0f));
					hpBar->SetTargetActor(enemy);
					enemy = nullptr;
				}
			}
		}
	}

	const int MissionTextY = 500;
	mMissionTextComponent = std::make_unique<BlinkingTextComponent>(
		mRenderer,
		mFont,
		"Mission Completed!  Return to Base",
		0,
		MissionTextY,
		0.8f
	);

	SDL_Log("DEBUG: Actors size after LoadData: %zu", mActors.size());
}

void Game::UnloadData() {
	mActors.clear();
	mWaitingActors.clear();
	mSprites.clear();

	for (auto& tex : mTextures) {
		SDL_DestroyTexture(tex.second);
	}
	mTextures.clear();

	mPlayer = nullptr;
	mHUDActor = nullptr;
	mHPBack = nullptr;
	mHPBar = nullptr;
	mStaminaBack = nullptr;
	mStaminaBar = nullptr;
}

void Game::ChangeScene(std::unique_ptr<Scene> newScene) {
	if (mCurrentScene && dynamic_cast<GameScene*>(mCurrentScene.get()) != nullptr) {
		if (mPlayer && mPlayer->GetWeaponComponent()) {
			mPendingWeapon = mPlayer->GetWeaponComponent()->GetWeaponData();
			mHasPendingWeapon = true;
		}

		UnloadData();
		ResetGameSceneState();
	}

	if (mCurrentBGM) {
		StopBGM();
	}

	mCurrentScene = std::move(newScene);
	if (mCurrentScene) {
		mCurrentScene->LoadContent();
		mCurrentScene->LoadData();
	}
}

void Game::ResetGameSceneState() {
	mMissionCompleted = false;
	mMissionCompleteTime = 0;
	mSceneChangeRequested = false;
	mNextScene = nullptr;
}

void Game::Shutdown() {
	if (mPlayer) {
		dataManager.SaveGameData("Assets/Data/SaveData.json", mPlayer->GetCurrentWeaponName());
	}

	UnloadData();
	SDL_DestroyWindow(mWindow);
	SDL_DestroyRenderer(mRenderer);
	Mix_CloseAudio();
	SDL_Quit();
}

void Game::AddActor(Actor* actor) {
	if (std::find(mActors.begin(), mActors.end(), actor) != mActors.end() ||
		std::find(mWaitingActors.begin(), mWaitingActors.end(), actor) != mWaitingActors.end()) {
		SDL_Log("Warning: Actor %p is already added!", actor);
		return;
	}

	if (mUpdatingActors) {
		mWaitingActors.emplace_back(actor);
	}
	else {
		mActors.emplace_back(actor);
	}
}

void Game::RemoveActor(Actor* actor) {
	auto iter_waiting = std::find(mWaitingActors.begin(), mWaitingActors.end(), actor);
	if (iter_waiting != mWaitingActors.end()) {
		std::iter_swap(iter_waiting, mWaitingActors.end() - 1);
		mWaitingActors.pop_back();
	}

	auto iter_actors = std::find(mActors.begin(), mActors.end(), actor);
	if (iter_actors != mActors.end()) {
		std::iter_swap(iter_actors, mActors.end() - 1);
		mActors.pop_back();
	}

	if (auto enemy = dynamic_cast<Enemy*>(actor)) {
		RemoveEnemy(enemy);
	}

	auto iter_target = std::find(mTargetActors.begin(), mTargetActors.end(), actor);
	if (iter_target != mTargetActors.end()) {
		std::iter_swap(iter_target, mTargetActors.end() - 1);
		mTargetActors.pop_back();
	}

	if (auto ally = dynamic_cast<AllyUnit*>(actor)) {
		RemoveAlly(ally);
	}

	delete actor;
}

void Game::AddSprite(SpriteComponent* sprite) {
	int myDrawOrder = sprite->GetDrawOrder();
	auto iter = mSprites.begin();
	for (; iter != mSprites.end(); iter++) {
		if (myDrawOrder < (*iter)->GetDrawOrder()) {
			break;
		}
	}
	mSprites.insert(iter, sprite);
}

SDL_Texture* Game::GetTexture(const std::string& fileName) {
	SDL_Texture* tex = nullptr;
	auto iter = mTextures.find(fileName);

	if (iter != mTextures.end()) {
		tex = iter->second;
	}
	else {
		SDL_Surface* surf = IMG_Load(fileName.c_str());
		if (!surf) {
			SDL_Log("Failed to load texture file %s", fileName.c_str());
			return nullptr;
		}

		tex = SDL_CreateTextureFromSurface(mRenderer, surf);
		SDL_FreeSurface(surf);
		if (!tex) {
			SDL_Log("Failed to convert surface to texture for %s", fileName.c_str());
			return nullptr;
		}

		mTextures.emplace(fileName.c_str(), tex);
	}
	return tex;
}

void Game::RemoveSprite(SpriteComponent* sprite) {
	auto iter = std::find(mSprites.begin(), mSprites.end(), sprite);
	if (iter != mSprites.end()) {
		mSprites.erase(iter);
	}
}

void Game::UpdateObtainedWeapons() {
	std::vector<std::string> allWeaponNames = {
		"MusinGun",
		"ShotGun",
		"RailGun"
	};

	mObtainedWeaponNames.clear();

	for (const std::string& name : allWeaponNames) {
		WeaponData data;
		if (data.LoadFromJSON("Assets/Data/WeaponData.json", name) && data.gotWeapon) {
			mObtainedWeaponNames.push_back(name);
		}
	}
}

const std::vector<std::string>& Game::GetObtainedWeaponNames() const {
	return mObtainedWeaponNames;
}

void Game::AddObtainedWeapon(const std::string& weaponName) {
	if (!dataManager.IsWeaponObtained(weaponName)) {
		dataManager.SetWeaponObtained(weaponName, true);
		SDL_Log("INFO: Weapon obtained: %s", weaponName.c_str());
	}
}

void Game::AddEnemy(Enemy* enemy) {
	mEnemies.emplace_back(enemy);
}

void Game::RemoveEnemy(Enemy* enemy) {
	auto iter = std::find(mEnemies.begin(), mEnemies.end(), enemy);
	if (iter != mEnemies.end()) {
		std::iter_swap(iter, mEnemies.end() - 1);
		mEnemies.pop_back();
	}
}

Enemy* Game::GetNearestEnemy(const Vector2& pos) {
	Enemy* nearest = nullptr;
	float minSqDist = FLT_MAX;

	for (Enemy* enemy : mEnemies) {
		if (enemy->GetState() != Actor::EAlive) {
			continue;
		}

		Vector2 diff = enemy->GetPosition() - pos;
		float sqDist = diff.LengthSq();

		if (sqDist < minSqDist) {
			minSqDist = sqDist;
			nearest = enemy;
		}
	}
	return nearest;
}

void Game::AddAlly(AllyUnit* ally) {
	mAllies.emplace_back(ally);
}

void Game::RemoveAlly(AllyUnit* ally) {
	auto iter = std::find(mAllies.begin(), mAllies.end(), ally);
	if (iter != mAllies.end()) {
		std::iter_swap(iter, mAllies.end() - 1);
		mAllies.pop_back();
	}
}

Actor* Game::GetNearestTarget(const Vector2& pos) {
	Actor* nearest = nullptr;
	float minSqDist = FLT_MAX;
	std::vector<Actor*> targets;

	if (mPlayer && mPlayer->GetState() == Actor::EAlive) {
		targets.push_back(mPlayer);
	}

	for (AllyUnit* ally : mAllies) {
		if (ally && ally->GetState() == Actor::EAlive) {
			targets.push_back(ally);
		}
	}

	for (Actor* target : targets) {
		Vector2 diff = target->GetPosition() - pos;
		float sqDist = diff.LengthSq();

		if (sqDist < minSqDist) {
			minSqDist = sqDist;
			nearest = target;
		}
	}
	return nearest;
}

void Game::AddLineComponent(LineComponent* line) {
	mLineComponents.emplace_back(line);
}

void Game::RemoveLineComponent(LineComponent* line) {
	auto iter = std::find(mLineComponents.begin(), mLineComponents.end(), line);
	if (iter != mLineComponents.end()) {
		std::iter_swap(iter, mLineComponents.end() - 1);
		mLineComponents.pop_back();
	}
}

void Game::PlayBGM(const std::string& fileName, int loop) {
	if (mCurrentBGM) {
		Mix_HaltMusic();
		Mix_FreeMusic(mCurrentBGM);
		mCurrentBGM = nullptr;
	}

	mCurrentBGM = Mix_LoadMUS(fileName.c_str());
	if (!mCurrentBGM) {
		SDL_Log("Failed to load BGM: %s", Mix_GetError());
		return;
	}

	Mix_VolumeMusic(24);
	Mix_PlayMusic(mCurrentBGM, loop);
}

void Game::StopBGM() {
	if (mCurrentBGM) {
		Mix_HaltMusic();
		Mix_FreeMusic(mCurrentBGM);
		mCurrentBGM = nullptr;
	}
}

void Game::RequestSceneChange(std::unique_ptr<Scene> newScene) {
	mSceneChangeRequested = true;
	mNextScene = std::move(newScene);
}

std::vector<UpgradeOption> Game::GetRandomUpgrades(int count) {
	std::vector<UpgradeOption> allOptions = {
		{ UpgradeType::DamageBoost, "火力強化", "攻撃力が15%上昇", 1.15f },
		{ UpgradeType::FireRateBoost, "連射強化", "発射間隔が短縮", 0.90f },
		{ UpgradeType::DefenseBoost, "装甲強化", "被ダメージが10%減少", 0.90f },
		{ UpgradeType::SpeedBoost, "機動力強化", "移動速度が10%上昇", 1.10f },
		{ UpgradeType::SummonAlly, "増援要請", "友軍攻撃ヘリを投入", 1.0f }
	};

	std::shuffle(allOptions.begin(), allOptions.end(), Math::generator);

	std::vector<UpgradeOption> selection;
	for (int i = 0; i < count && i < (int)allOptions.size(); ++i) {
		selection.push_back(allOptions[i]);
	}
	return selection;
}

void Game::InstanceEnemy(const EnemyConfig& config, Enemy*& enemy, Vector2 pos) {
	if (config.type == "LowLevelEnemy") {
		enemy = new LowLevelEnemy(this);
		enemy->SetPosition(pos);
	}
	else if (config.type == "DroneGuard") {
		enemy = new DroneGuard(this);
		enemy->SetPosition(pos);
	}
	else if (config.type == "SentryGun") {
		enemy = new SentryGun(this);
		enemy->SetPosition(pos);
	}
	else if (config.type == "BattleShip") {
		enemy = new BattleShip(this, pos);
	}
	else if (config.type == "Interceptor") {
		enemy = new Interceptor(this);
	}
	else if (config.type == "Formation") {
		Enemy* flagship = nullptr;
		Vector2 centerPos = config.centerPosition;

		if (config.mainShipType == "LowLevelEnemy") {
			flagship = new LowLevelEnemy(this);
		}
		else if (config.mainShipType == "DroneGuard") {
			flagship = new DroneGuard(this);
		}
		else if (config.mainShipType == "SentryGun") {
			flagship = new SentryGun(this);
		}
		else if (config.mainShipType == "BattleShip") {
			flagship = new BattleShip(this, centerPos);
		}

		if (flagship) {
			if (config.mainShipType != "BattleShip") {
				flagship->SetPosition(centerPos);
			}

			AddEnemy(flagship);
			HUDSpriteComponent* hpBar = new HUDSpriteComponent(mHUDActor);
			hpBar->SetTexture(GetTexture("Assets/HP1.png"));
			hpBar->SetOffset(Vector2(0, -50.0f));
			hpBar->SetTargetActor(flagship);
		}

		if (!flagship) {
			SDL_Log("ERROR: Flagship type '%s' is unknown or failed to spawn.", config.mainShipType.c_str());
			return;
		}

		float rx = config.radiusX;
		float ry = config.radiusY;
		float formationRot = config.formationRotation;

		for (const auto& detail : config.guardDetails) {
			int guardCount = detail.guardCount;
			const std::string& guardShipType = detail.guardShipType;
			float radiusRatio = detail.radiusRatio;

			float currentRx = rx * radiusRatio;
			float currentRy = ry * radiusRatio;

			for (int i = 0; i < guardCount; i++) {
				float angle = Math::TwoPi * (float)i / guardCount;

				float x = currentRx * Math::Cos(angle);
				float y = currentRy * Math::Sin(angle);

				float cosRot = Math::Cos(formationRot);
				float sinRot = Math::Sin(formationRot);
				float rotatedX = x * cosRot - y * sinRot;
				float rotatedY = x * sinRot + y * cosRot;

				Vector2 offset(rotatedX, rotatedY);
				Vector2 guardPos(centerPos.x + rotatedX, centerPos.y + rotatedY);

				Enemy* guard = nullptr;

				if (guardShipType == "LowLevelEnemy") {
					guard = new LowLevelEnemy(this);
				}
				else if (guardShipType == "DroneGuard") {
					guard = new DroneGuard(this);
				}
				else if (guardShipType == "SentryGun") {
					guard = new SentryGun(this);
				}
				else if (guardShipType == "BattleShip") {
					guard = new BattleShip(this, guardPos);
				}

				if (guard) {
					if (guardShipType != "BattleShip") {
						guard->SetPosition(guardPos);
					}

					AddEnemy(guard);

					if (PatrolComponent* pc = guard->GetComponent<PatrolComponent>()) {
						pc->SetFormationTarget(flagship, offset);
					}

					HUDSpriteComponent* hpBar = new HUDSpriteComponent(mHUDActor);
					hpBar->SetTexture(GetTexture("Assets/HP1.png"));
					hpBar->SetOffset(Vector2(0, -50.0f));
					hpBar->SetTargetActor(guard);
				}
			}
		}
	}
}