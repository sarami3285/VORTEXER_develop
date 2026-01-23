#include "OrbitAttackComponent.h"
#include "Actor.h"
#include "Player.h"
#include "MoveComponent.h"
#include "Math.h"
#include "RangeAttackComponent.h"

OrbitAttackComponent::OrbitAttackComponent(Actor* owner, Player* player)
    : RangedAttackComponent(owner, player, false)
{
    mChaseRange = 1000.0f;
    mAttackRange = 300.0f;
    mChaseSpeed = 100.0f;

    mOrbitDistance = 150.0f;
    mOrbitSpeed = Math::Pi / 2.0f;
    mMaxChaseDistance = 1000.0f;
    mChaseTurnSpeed = Math::TwoPi;
}

void OrbitAttackComponent::Execute(float dt)
{
    if (!IsActive() || !mPlayer || mOwner->GetState() != Actor::EAlive) return;

    mFireCooldown -= dt;

    Vector2 toPlayer = mPlayer->GetPosition() - mOwner->GetPosition();
    float dist = toPlayer.Length();

    class MoveComponent* mc = mOwner->GetComponent<MoveComponent>();
    if (!mc) return;

    // ----------------------------------------------------------------
    // 2. 移動制御ブロック (独自の軌道ロジック)
    // ----------------------------------------------------------------
    if (dist > mOrbitDistance)
    {
        Vector2 desiredDirection = toPlayer;
        desiredDirection.Normalize();

        float targetAngle = Math::Atan2(desiredDirection.y, desiredDirection.x);
        float currentAngle = mOwner->GetRotation();

        float diff = Math::WrapAngle(targetAngle - currentAngle);
        float idealAngularSpeed = diff / dt;
        float rotationInput = Math::Clamp(idealAngularSpeed, -mChaseTurnSpeed, mChaseTurnSpeed);

        mc->SetAngularSpeed(rotationInput);

        // 旧SetRotation ブロック
        /*
        float turnAmount = mChaseTurnSpeed * dt;
        const float rotationTolerance = 0.01f;
        if (Math::Abs(diff) < rotationTolerance)
        {
            mOwner->SetRotation(targetAngle);
        }
        else
        {
            mOwner->SetRotation(currentAngle + Math::Sign(diff) * turnAmount);
        }
        */

        mc->SetForwardSpeed(mChaseSpeed);
    }
    else
    {
        // --- 軌道運動 ---
        Vector2 toTarget = mPlayer->GetPosition() - mOwner->GetPosition();
        Vector2 orbitDirection = Vector2(-toTarget.y, toTarget.x);
        orbitDirection.Normalize();

        float speedRatio = Math::Min(1.0f, dist / mOrbitDistance);
        mc->SetForwardSpeed(mOrbitSpeed);

        // 目標角度の補正
        float escapeAngle = Math::Atan2(-toTarget.y, -toTarget.x);
        float baseTargetAngle = Math::Atan2(orbitDirection.y, orbitDirection.x);

        float blendFactor = 1.0f - speedRatio;
        float blendedTargetAngle = baseTargetAngle;

        if (blendFactor > 0.01f)
        {
            blendedTargetAngle = Math::Lerp1(baseTargetAngle, escapeAngle, blendFactor * 1.0f);
        }

        float currentAngle = mOwner->GetRotation();
        float diff = Math::WrapAngle(blendedTargetAngle - currentAngle);
        float idealAngularSpeed = diff / dt;
        float rotationInput = Math::Clamp(idealAngularSpeed, -mChaseTurnSpeed, mChaseTurnSpeed);

        mc->SetAngularSpeed(rotationInput);
    }

    // 3. 攻撃ロジック
    if (dist <= mAttackRange)
    {
        Vector2 fireDir = toPlayer;
        fireDir.Normalize();

        if (mCurrentBurstCount > 0)
        {
            mBurstDelayTimer -= dt;
            if (mBurstDelayTimer <= 0.0f)
            {
                FireBullet(fireDir); 

                mCurrentBurstCount--;

                if (mCurrentBurstCount > 0)
                {
                    mBurstDelayTimer = mBurstDelay;
                }
                else
                {
                    mFireCooldown = mFireRate;
                    mBurstDelayTimer = 0.0f;
                }
            }
        }
        else if (mFireCooldown <= 0.0f)
        {
            mCurrentBurstCount = mBurstCount;
            mBurstDelayTimer = 0.0f;
        }
    }
}