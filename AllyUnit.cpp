#include "AllyUnit.h"
#include "Game.h"
#include "MoveComponent.h"
#include "AllyAIComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "BulletComponent.h"
#include <iostream>
#include "DeathEffect.h"

AllyUnit::AllyUnit(Game* game, AllyAIComponent::EAllyMode mode, const Vector2& position)
    : Actor(game)
    , mMode("EStay")
    , mHP(new HPComponent(this , 30))
    , mSprite(new SpriteComponent(this, 50, "Assets/AllyDrone.png", true, Vector2{ 4,4 }))
    , mMove(new MoveComponent(this, 0, 300.0f, 180.0f))
    , mAllyAI(new AllyAIComponent(this))
    , mCollision(new CollisionComponent(this, 25.0f))
{
    SetPosition(position);

    if (mAllyAI)
    {
        mAllyAI->SetMode(mode);
    }

    BulletComponent* bc = new BulletComponent(this);
    bc->SetFireInterval(0.4f); 
    bc->SetBulletSpeed(600.0f);
    bc->SetDamage(10.0f);


    new HPComponent(this, 50);
    new HPBarComponent(this, mHP);

    GetGame()->AddAlly(this);
}

AllyUnit::~AllyUnit() {
    GetGame()->RemoveAlly(this);
}

void AllyUnit::UpdateActor(float deltaTime)
{
    Actor::UpdateActor(deltaTime);
}

void AllyUnit::TakeDamage(int amount)
{
    mHP->TakeDamage(amount);

    if (mHP && mHP->IsDead()) {
        if (GetState() == EAlive) {
            DeathEffect* deathEffect = new DeathEffect(GetGame(), 1);
            deathEffect->SetPosition(GetPosition());
            SetState(EStop);
        }
    }
}