#include "AllyAIComponent.h"
#include "Actor.h"
#include "Game.h"
#include "Enemy.h"
#include "BulletComponent.h"
#include "MoveComponent.h"
#include "Player.h"
#include "Random.h"

AllyAIComponent::AllyAIComponent(Actor* owner)
	: Component(owner)
	, mMode(EStay)
	, mAttackRange(400.0f)
	, mOrbitAngle(0.0f)
{
	mOrbitAngle = Random::GetFloatRange(0.0f,360.0f);
}

void AllyAIComponent::Update(float deltaTime)
{
	AttackProcess(deltaTime);
	OrbitProcess(deltaTime);
}

void AllyAIComponent::OrbitProcess(float deltaTime)
{
	Player* player = mOwner->GetGame()->GetPlayer();
	if (!player) return;

	auto mc = mOwner->GetComponent<MoveComponent>();
	if (mc)
	{
		mc->SetForwardSpeed(0.0f);
		mc->SetAngularSpeed(0.0f);
	}

	const float orbitRadius = 170.0f;
	const float orbitSpeed = 0.9f;
	mOrbitAngle += orbitSpeed * deltaTime;
	mOrbitAngle = Math::WrapAngle(mOrbitAngle);

	Vector2 playerPos = player->GetPosition();
	Vector2 targetPos;
	targetPos.x = playerPos.x + orbitRadius * Math::Cos(mOrbitAngle);
	targetPos.y = playerPos.y + orbitRadius * Math::Sin(mOrbitAngle);

	// ステージ端クランプ
	targetPos.x = Math::Clamp(targetPos.x, 50.0f, 3190.0f);
	targetPos.y = Math::Clamp(targetPos.y, 50.0f, 3190.0f);

	Vector2 currentPos = mOwner->GetPosition();
	Vector2 toTarget = (targetPos + mOffset) - currentPos;
	float distToTarget = toTarget.Length();

	if (distToTarget > 1.0f)
	{
		toTarget.Normalize();
		float baseSpeed = (orbitRadius * orbitSpeed) * 1.5f;
		if (distToTarget > 100.0f) baseSpeed *= 2.0f;
		Vector2 nextPos = currentPos + toTarget * baseSpeed * deltaTime;
		mOwner->SetPosition(Vector2::Lerp(currentPos, nextPos, 0.4f));

		if (!SearchNearestEnemy())
		{
			float targetAngle = Math::Atan2(toTarget.y, toTarget.x);
			float currentAngle = mOwner->GetRotation();
			float angleDiff = Math::WrapAngle(targetAngle - currentAngle);
			mOwner->SetRotation(Math::Lerp1(currentAngle, currentAngle + angleDiff, 0.1f));
		}
	}
}

void AllyAIComponent::AttackProcess(float deltaTime)
{
	Actor* target = SearchNearestEnemy();
	if (!target) return;

	Vector2 currentPos = mOwner->GetPosition();
	Vector2 diff = target->GetPosition() - currentPos;
	float distSq = diff.LengthSq();

	if (distSq <= mAttackRange * mAttackRange)
	{
		float targetAngle = Math::Atan2(diff.y, diff.x);
		float currentAngle = mOwner->GetRotation();
		float angleDiff = Math::WrapAngle(targetAngle - currentAngle);
		mOwner->SetRotation(Math::Lerp1(currentAngle, currentAngle + angleDiff, 0.2f));

		auto bc = mOwner->GetComponent<BulletComponent>();
		if (bc)
		{
			bc->Fire(deltaTime);
		}
	}
}

void AllyAIComponent::FollowProcess(float deltaTime)
{
	Player* player = mOwner->GetGame()->GetPlayer();
	if (!player) return;

	Vector2 playerPos = player->GetPosition();
	Vector2 myPos = mOwner->GetPosition();
	Vector2 diff = playerPos - myPos;
	float distSq = diff.LengthSq();

	auto mc = mOwner->GetComponent<MoveComponent>();
	if (!mc) return;

	if (distSq > 150.0f * 150.0f)
	{
		diff.Normalize();
		mc->SetForwardSpeed(300.0f);
		// プレイヤーの方へ回転を向ける
		float angle = Math::Atan2(diff.y, diff.x);
		mOwner->SetRotation(angle);
	}
	else
	{
		mc->SetForwardSpeed(0.0f);
	}
}

Actor* AllyAIComponent::SearchNearestEnemy()
{
	Actor* nearest = nullptr;
	float nearestDistSq = 1e38f;

	for (auto actor : mOwner->GetGame()->GetActors())
	{
		if (dynamic_cast<Enemy*>(actor) && actor->GetState() == Actor::EAlive)
		{
			float distSq = (actor->GetPosition() - mOwner->GetPosition()).LengthSq();
			if (distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				nearest = actor;
			}
		}
	}
	return nearest;
}