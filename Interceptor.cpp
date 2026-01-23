#include "Interceptor.h"
#include "Game.h"
#include "Random.h"
#include "MoveComponent.h"
#include "SpriteComponent.h"
#include "CollisionComponent.h"
#include "HPBarComponent.h"
#include "HPComponent.h"
#include "Player.h"
#include "CameraComponent.h"
#include "EnemyBullet.h"
#include "RangeAttackComponent.h"
#include "HighSpeedPassComponent.h"
#include "Math.h"

Interceptor::Interceptor(Game* game)
    : Enemy(game)
    , mFireTimer(0.0f)
    , mFireInterval(0.12f)
{
    mHPComponent = new HPComponent(this, 50);
    mPlayer = game->GetPlayer();

    Vector2 minPos(CameraComponent::MapOffset, CameraComponent::MapOffset);
    Vector2 maxPos(CameraComponent::mMapWidth - CameraComponent::MapOffset, CameraComponent::mMapHeight - CameraComponent::MapOffset);
    SetPosition(GetRandomPositionInRange(minPos, maxPos));
    SetRotation(Random::GetFloatRange(0.0f, Math::TwoPi));

    new MoveComponent(this, 0, 0.0f, 0.0f);

    mTexturePath = "Assets/Interceptor.png";
    SpriteComponent* sc = new SpriteComponent(this, 1000, mTexturePath, true, Vector2{ 40, 40 });
    sc->SetColor(0.8f, 0.9f, 1.0f);

    mCollision = new CollisionComponent(this, 22.0f);
    new HPBarComponent(this, mHPComponent);

    // ミサイル設定
    mMissile = new RangedAttackComponent(this, mPlayer, true);
    mMissile->mIsMissile = true;
    mMissile->mFireRate = 5.0f;
    mMissile->mBulletSpeed = 900.0f;
    mMissile->mBulletDamage = 15;
    mMissile->mAttackRange = 900.0f;

    // AI設定
    mAI = new HighSpeedPassComponent(this);
    mAI->mMaxSpeed = 400.0f;
    mAI->mTurnSpeed = 20.0f;
    mAI->mAttackRange = 600.0f;
    mAI->mPassDistance = 180.0f;
}

void Interceptor::FireMachineGun()
{
    if (mFireTimer <= 0.0f)
    {
        float rot = GetRotation();
        Vector2 rightSideDir(-Math::Sin(rot), Math::Cos(rot));
        float offsetDist = 20.0f;

        // 左側の銃口位置
        Vector2 leftGunPos = GetPosition() + (rightSideDir * -offsetDist);
        new EnemyBullet(
            GetGame(),
            leftGunPos,
            rot,
            950.0f,
            1.5f,
            4,
            "Assets/Bullet.png"
        );

        // 右側の銃口位置
        Vector2 rightGunPos = GetPosition() + (rightSideDir * offsetDist);
        new EnemyBullet(
            GetGame(),
            rightGunPos,
            rot,
            950.0f,
            1.5f,
            4,
            "Assets/Bullet.png"
        );

        mFireTimer = mFireInterval;
    }
}

void Interceptor::UpdateActor(float deltaTime)
{
    Enemy::UpdateActor(deltaTime);
    mFireTimer -= deltaTime;

    Vector2 pos = GetPosition();
    float width = CameraComponent::mMapWidth;
    float height = CameraComponent::mMapHeight;
    const float margin = 0.0f;

    bool teleported = false;

    if (pos.x < -margin) {
        pos.x = width + margin - 10.0f;
        teleported = true;
    }
    else if (pos.x > width + margin) {
        pos.x = -margin + 10.0f;
        teleported = true;
    }

    if (pos.y < -margin) {
        pos.y = height + margin - 10.0f;
        teleported = true;
    }
    else if (pos.y > height + margin) {
        pos.y = -margin + 10.0f;
        teleported = true;
    }

    if (teleported)
    {
        SetPosition(pos);
    }
}