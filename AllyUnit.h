#pragma once
#include "Actor.h"
#include <memory>
#include <string>
#include "Math.h"
#include "AllyAIComponent.h" 
#include "CollisionComponent.h"
#include "HPBarComponent.h"
#include "HPComponent.h"

class MoveComponent;
class AllyAIComponent;
class SpriteComponent;
class CollisionComponent;

class AllyUnit : public Actor
{
public:
    AllyUnit(Game* game, AllyAIComponent::EAllyMode mode, const Vector2& position);
    ~AllyUnit();

    void UpdateActor(float deltaTime) override;
    virtual void TakeDamage(int amount);
    class CollisionComponent* GetCircle() { return mCollision; }

protected:
    MoveComponent* mMove;
    AllyAIComponent* mAllyAI;
    HPComponent* mHP;
    CollisionComponent* mCollision;
    SpriteComponent* mSprite;

private:
    std::string mMode;
    HPBarComponent* mHPBar;
};