// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerSkillComponent.generated.h"


class UActionComponent;
class UBATableManager;
class USkillTreeSubsystem;
/*
 * 
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BAPROJECT_API UPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
// 재정의 함수
	// Sets default values for this component's properties
	UPlayerSkillComponent();
	// Called when the game starts
	virtual void BeginPlay() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 전체 스킬 새로고침
	UFUNCTION(BlueprintCallable, Category="Skill")
	void RefreshAllSkills();
	
	// 스킬 모두 삭제
	UFUNCTION(BlueprintCallable, Category="Skill")
	void ClearAllSkills();
	
	// 스킬 적용 구현 종류에 따라 분기
	UFUNCTION(BlueprintCallable, Category="Skill")
	void ApplySkill(int32 SkillId);
	
private:
// 서브시스템 포인터
	UPROPERTY(Transient)
	TObjectPtr<USkillTreeSubsystem> SkillTreeSubsystem = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UBATableManager> TableManager = nullptr;
	
// 액터컴포넌트 포인터
	UPROPERTY(Transient)
	TObjectPtr<UActionComponent> ActionComponent = nullptr;
};
