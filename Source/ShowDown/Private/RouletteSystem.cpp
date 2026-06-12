#include "RouletteSystem.h"

URouletteSystem::URouletteSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool URouletteSystem::RollRoulette(int32 BulletCount) const
{
	const int32 ClampedBulletCount = FMath::Clamp(BulletCount, 0, 6);
	return FMath::RandRange(1, 6) <= ClampedBulletCount;
}

float URouletteSystem::GetHitChance(int32 BulletCount) const
{
	return static_cast<float>(FMath::Clamp(BulletCount, 0, 6)) / 6.0f;
}
