#pragma once
#include "Component.h"
#include "Math.h"

class HighSpeedPassComponent : public Component
{
public:
    enum EAIState { Approach, Withdraw, Recover };

    HighSpeedPassComponent(class Actor* owner);
    void Update(float deltaTime) override;

    float mMaxSpeed = 700.0f;       // 接近・離脱時の最高速度
    float mTurnSpeed = 120.0f;      // 旋回時の回転速度
    float mAttackRange = 500.0f;    // 射撃を開始する距離
    float mPassDistance = 150.0f;   // 離脱に移行する自機との接近閾値
    float mWithdrawTime = 1.5f;     // 離脱直進を続ける時間

private:
    EAIState mCurrentState;
    float mTimer;
    class Player* mPlayer;
    bool IsMissile = true;
};