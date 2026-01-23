#include "HighSpeedPassComponent.h"
#include "Actor.h"
#include "Game.h"
#include "Player.h"
#include "MoveComponent.h"
#include "BulletComponent.h" 
#include "RangeAttackComponent.h" 
#include "Interceptor.h"
HighSpeedPassComponent::HighSpeedPassComponent(Actor* owner)
    : Component(owner)
    , mCurrentState(Approach)
    , mTimer(0.0f)
{
    mPlayer = mOwner->GetGame()->GetPlayer();
}

void HighSpeedPassComponent::Update(float deltaTime)
{
    if (!mPlayer || mOwner->GetState() != Actor::EAlive) return;

    auto mc = mOwner->GetComponent<MoveComponent>();
    if (!mc) return;

    Vector2 vToPlayer = mPlayer->GetPosition() - mOwner->GetPosition();
    float distToPlayer = vToPlayer.Length();

    switch (mCurrentState)
    {
    case Approach:
    {
        mc->SetForwardSpeed(mMaxSpeed);

        float targetAngle = Math::Atan2(vToPlayer.y, vToPlayer.x);
        float currentRot = mOwner->GetRotation();
        mOwner->SetRotation(Math::Lerp1(currentRot, currentRot + Math::WrapAngle(targetAngle - currentRot), 0.1f));

        if (distToPlayer < mAttackRange)
        {
            auto interceptor = dynamic_cast<Interceptor*>(mOwner);
            if (interceptor)
            {
                interceptor->FireMachineGun();
            }
            if (IsMissile) {
                auto rc = mOwner->GetComponent<RangedAttackComponent>();
                if (rc)
                {
                    float rot = mOwner->GetRotation();
                    Vector2 forward(Math::Cos(rot), Math::Sin(rot));
                    rc->FireBullet(forward);
                }
                IsMissile = false;
            }
        }

        if (distToPlayer < mPassDistance)
        {
            mCurrentState = Withdraw;
            mTimer = 1.2f;
        }
    }
    break;

    case Withdraw:
        mc->SetForwardSpeed(mMaxSpeed * 1.1f);
        mTimer -= deltaTime;
        if (mTimer <= 0.0f)
        {
            mCurrentState = Recover;
        }
        IsMissile = true;
        break;

    case Recover:
    {
        mc->SetForwardSpeed(mMaxSpeed * 0.7f);

        float angleToPlayer = Math::Atan2(vToPlayer.y, vToPlayer.x);
        float angleDiff = Math::WrapAngle(angleToPlayer - mOwner->GetRotation());

        if (Math::Abs(angleDiff) > 0.15f)
        {
            float rotDir = (angleDiff > 0) ? 1.0f : -1.0f;
            mOwner->SetRotation(mOwner->GetRotation() + rotDir * Math::ToRadians(mTurnSpeed) * deltaTime);
        }
        else
        {
            mCurrentState = Approach;
        }
    }
    break;
    }
}