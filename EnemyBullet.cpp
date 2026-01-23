#include "EnemyBullet.h"
#include "SpriteComponent.h"
#include "MoveComponent.h"
#include "CollisionComponent.h"
#include "Player.h"
#include "Game.h"
#include "CameraComponent.h" 
#include "Math.h" 
#include "AllyUnit.h"

EnemyBullet::EnemyBullet(class Game* game, const Vector2& startPos, float rotation, float speed, float lifeTime, int damage, const std::string& texturePath)
    : Actor(game)
    , mDeathTimer(lifeTime)
    , mCollision(nullptr)
    , mAudioComponent(nullptr)
    , Damage(damage)
    , mBulletPath(texturePath)
{
    SetPosition(startPos);
    SetRotation(rotation);

    SpriteComponent* sc = new SpriteComponent(this);
    sc->SetTexture(game->GetTexture(mBulletPath));
    MoveComponent* mc = new MoveComponent(this, 0, speed, 0.0f);

    mAudioComponent = new AudioComponent(this);
    mAudioComponent->LoadSE("shot", "Assets/Audio/shotgun.mp3");

    mCollision = new CollisionComponent(this , 11.0f);

    mAudioComponent->PlaySE("shot");
}

void EnemyBullet::UpdateActor(float deltaTime) {
	mDeathTimer -= deltaTime;
	if (mDeathTimer <= 0.0f)
	{
		SetState(EStop);
	}
	else
	{
		Vector2 pos = GetPosition();
		if (pos.x <= 0.0f || pos.x >= CameraComponent::mMapWidth ||
			pos.y <= 0.0f || pos.y >= CameraComponent::mMapHeight)
		{
			SetState(EStop);
		}

		Player* player = GetGame()->GetPlayer();
		if (player && Intersect(*mCollision, *(player->GetCircle())))
		{
			SetState(EStop);
			player->TakeDamage(Damage);
			return;
		}

        const std::vector<AllyUnit*>& allies = GetGame()->GetAllies();
        for (AllyUnit* ally : allies)
        {
            if (ally && ally->GetState() == Actor::EAlive)
            {
                if (Intersect(*mCollision, *(ally->GetCircle())))
                {
                    SetState(EStop);
                    ally->TakeDamage(Damage);
                    return;
                }
            }
        }
	}
}