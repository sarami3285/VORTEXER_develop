#pragma once
#include "Component.h"
#include "Math.h"

class AllyAIComponent : public Component
{
public:
	AllyAIComponent(class Actor* owner);
	void Update(float deltaTime) override;

	enum EAllyMode { EStay, EFollow, EAttack };
	void SetMode(EAllyMode mode) { mMode = mode; }
	void SetOffset(const Vector2& offset) { mOffset = offset; }

private:
	void AttackProcess(float deltaTime);
	void FollowProcess(float deltaTime);
	void OrbitProcess(float deltaTime);
	class Actor* SearchNearestEnemy();

	EAllyMode mMode;
	float mAttackRange;
	float mOrbitAngle;
	float mAttackCooldown = 0.4f;
	Vector2 mOffset = Vector2::Zero;
};