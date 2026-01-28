#include "AllyBuilding.h"
#include "Game.h"
#include "Actor.h"
#include "BulletComponent.h"

AllyBuilding::AllyBuilding(Game* game, const Vector2& position, int hp, const std::string& texture)
    : Actor(game)
{
    SetPosition(position);
    mHP = new HPComponent(this, hp);
    mSprite = new SpriteComponent(this, 50, texture);
    mCollision = new CollisionComponent(this, 80.0f);

    if (mSprite && !texture.empty()) {
        mSprite->SetTexture(GetGame()->GetTexture(texture));
    }
}

void AllyBuilding::TakeDamage(int amount) {
    if (mHP) {
        mHP->TakeDamage(amount);
        if (mHP->IsDead()) {
            SetState(EStop); // 破壊されたら動作停止（Game側で判定に使用）
        }
    }
}

void AllyBuilding::UpdateActor(float deltaTime) {
    Actor::UpdateActor(deltaTime);
}