#include "EnemyStateComponent.h"
#include "Actor.h"
#include "Player.h"
#include "Math.h"
#include "PatrolComponent.h"
#include "OrbitAttackComponent.h"
#include "RangeAttackComponent.h"
#include "Enemy.h"

EnemyStateComponent::EnemyStateComponent(class Actor* owner, class Player* player)
	: Component(owner)
	, mPlayer(player)
	, mCurrentState(EState::Patrol)
{
}

void EnemyStateComponent::StartBehavior()
{
	SetState(EState::Patrol);
}

void EnemyStateComponent::Update(float dt)
{
	if (mCurrentState == EState::Attack)
	{
		mAttackTimer -= dt;
	}

	CheckTransitions();

	HandleState(dt);
}

void EnemyStateComponent::OnDamageTaken()
{
	if (mCurrentState == EState::Patrol)
	{
		SetState(EState::Attack);
	}
	mAttackTimer = mAttackDuration;
}

void EnemyStateComponent::CheckTransitions()
{
	if (!mPlayer) return;

	if (mPatrolComp == nullptr && mCurrentState == EState::Attack)
	{
		return;
	}

	Vector2 toPlayer = mPlayer->GetPosition() - mOwner->GetPosition();
	float dist = toPlayer.Length();
	float chaseRange = 0.0f;

	if (mOrbitAttackComp) {
		chaseRange = mOrbitAttackComp->mMaxChaseDistance;
	}
	else if (mRangedAttackComp) {
		chaseRange = mRangedAttackComp->mChaseRange;
	}

	if (mCurrentState == EState::Patrol)
	{
		if (dist <= mSearchRange)
		{
			SetState(EState::Attack);
		}
	}
	else if (mCurrentState == EState::Attack)
	{
		if (mAttackTimer > 0.0f)
		{
			return;
		}

		if (chaseRange > 0.0f && dist > chaseRange)
		{
			SetState(EState::Patrol);
		}
	}
}

void EnemyStateComponent::HandleState(float dt)
{
	if (mCurrentState == EState::Patrol)
	{
		if (mPatrolComp && mPatrolComp->IsActive())
		{
			mPatrolComp->Execute(dt);
		}
	}
	else if (mCurrentState == EState::Attack)
	{
		if (mOrbitAttackComp && mOrbitAttackComp->IsActive())
		{
			mOrbitAttackComp->Execute(dt);
		}
		else if (mRangedAttackComp && mRangedAttackComp->IsActive())
		{
			mRangedAttackComp->Execute(dt);
		}
	}
}

void EnemyStateComponent::SetState(EState newState)
{
	if (mRangedAttackComp) mRangedAttackComp->SetIsActive(false);
	if (mOrbitAttackComp) mOrbitAttackComp->SetIsActive(false);
	if (mPatrolComp) mPatrolComp->SetIsActive(false);

	mCurrentState = newState;

	Enemy* enemyOwner = dynamic_cast<Enemy*>(mOwner);

	if (mCurrentState == EState::Patrol && mPatrolComp)
	{
		mPatrolComp->SetIsActive(true);

		if (enemyOwner) {
			enemyOwner->SetActiveBehavior(mPatrolComp);
		}
	}
	else if (mCurrentState == EState::Attack)
	{
		EnemyBehaviorComponent* activeBehavior = nullptr;

		if (mOrbitAttackComp)
		{
			mOrbitAttackComp->SetIsActive(true);
			activeBehavior = mOrbitAttackComp;
		}

		if (mRangedAttackComp)
		{
			mRangedAttackComp->SetIsActive(true);
			if (!activeBehavior) {
				activeBehavior = mRangedAttackComp;
			}
		}

		mAttackTimer = mAttackDuration;

		if (enemyOwner && activeBehavior) {
			enemyOwner->SetActiveBehavior(activeBehavior);
		}
	}
}