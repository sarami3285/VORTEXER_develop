#include "LineComponent.h"
#include "Actor.h"
#include "Game.h"

LineComponent::LineComponent(Actor* owner, int drawOrder)
    : Component(owner)
    , mDrawOrder(drawOrder)
    , mColorR(255), mColorG(255), mColorB(255)
{
}

LineComponent::~LineComponent() {
    mGame->RemoveLineComponent(this);
}

void LineComponent::Draw(SDL_Renderer* renderer, const Vector2& cameraPos) {
    if (!mOwner) return;
    if (mIsVisible) {
        const Vector2& startPosWorld = mOwner->GetPosition();
        const Vector2& endPosWorld = mTargetPos;
        int startX = static_cast<int>(startPosWorld.x - cameraPos.x);
        int startY = static_cast<int>(startPosWorld.y - cameraPos.y); 

        int endX = static_cast<int>(endPosWorld.x - cameraPos.x);
        int endY = static_cast<int>(endPosWorld.y - cameraPos.y);

        SDL_SetRenderDrawColor(renderer, mColorR, mColorG, mColorB, 255);
        SDL_RenderDrawLine(renderer, startX, startY, endX, endY);
    }
}