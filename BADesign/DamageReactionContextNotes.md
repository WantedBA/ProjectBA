# Damage / Hit Reaction 작업 인수인계 메모

작성일: 2026-05-20
작업 브랜치: `feat/player-knockback`

이 문서는 피격 방향, `FBADamageEvent`, `FBACharacterDamageContext`, 가드 브레이크, 넉다운 반응에 대해 오늘 논의하고 반영한 내용을 다음 작업자가 바로 이어받을 수 있도록 정리한 메모다.

## 관련 커밋

- `5a3e5db8` 넉다운 피격 반응을 추가하고 가드 파괴 조건을 분리
- `aeef7a70` 대미지 컨텍스트를 제거하고 피격 상태 갱신을 정리
- `6a7a1556` 커스텀 대미지 이벤트의 역할을 주석으로 보강

## 핵심 결론

### 1. `ImpactNormal`은 가드 각도 판정의 주 입력으로 쓰지 않는다

`FHitResult::ImpactNormal`은 “공격이 어느 방향에서 왔는가”가 아니라, 충돌이 발생한 컴포넌트 표면의 법선이다.

현재 플레이어 생성자에서 `GetMesh()->SetCollisionProfileName(TEXT("NoCollision"))`가 설정되어 있으므로, 플레이어 피격이 캐릭터 캡슐을 통해 발생한다면 `ImpactNormal`은 대체로 캡슐 표면의 노멀일 가능성이 높다. 만약 나중에 스켈레탈 메시나 PhysicsAsset으로 충돌을 받도록 바뀌면 그때는 해당 메시/바디 표면의 노멀이 될 수 있다.

그래서 `ImpactNormal`은 다음 용도에는 적합하다.

- 피격 VFX 방향
- 표면 반응
- 디버그 시 실제 접촉면 확인

하지만 다음 용도에는 안정적이지 않다.

- 플레이어 기준 앞/뒤/좌/우 피격 방향 판별
- 가드 각도 판정
- 넉백 방향의 주 소스

가드 판정은 “피격자 기준으로 공격자가 어느 방향에 있는가”가 필요하다. 따라서 현재 방향성은 `DamageDirection` 또는 `FPointDamageEvent::ShotDirection` 같은 게임플레이 방향을 명시적으로 넘겨 쓰는 것이다.

### 2. `ShotDirection`은 자동 계산값이 아니다

`FPointDamageEvent::ShotDirection`은 엔진이 알아서 계산해주는 값이 아니다.

- `UGameplayStatics::ApplyPointDamage`를 쓰면 `HitFromDirection` 인자로 넘긴 값이 `ShotDirection`에 들어간다.
- `FPointDamageEvent`를 직접 만들면 코드에서 직접 `ShotDirection`을 세팅해야 한다.

따라서 근접 sweep 판정에서도 팀 규약만 정하면 `ShotDirection`을 사용할 수 있다. 예를 들면:

```cpp
PointDamageEvent.ShotDirection =
    (Victim->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
```

이 방향은 충돌면 노멀이 아니라 “공격자에서 피격자로 향하는 게임플레이 방향”이다. 현재 가드 각도와 넉백은 이 의미의 방향을 기대한다.

### 3. `FPointDamageEvent`는 총알 전용으로 제한하지 않아도 된다

이름 때문에 투사체/총알 전용처럼 보이지만, 실질적으로는 “한 지점의 피해 + HitResult + 방향”을 표현하는 이벤트로 볼 수 있다. 근접 sweep도 최종적으로 특정 대상에게 특정 타격 지점과 방향을 전달할 수 있으므로 `FPointDamageEvent`와 잘 맞는다.

다만 현재 코드는 `FPointDamageEvent`로 완전히 전환하지 않았다. 이유는 `FPointDamageEvent`에는 `EBADamageReactionType`을 담을 필드가 없기 때문이다.

## 현재 코드 구조

### `FBADamageEvent`

위치: `Source/BAProject/Combat/BADamageTypes.h`

현재 `FBADamageEvent`는 유지되어 있다. 남긴 이유는 다음과 같다.

- `FPointDamageEvent`는 `HitResult`와 `ShotDirection`을 이미 제공한다.
- 그러나 `HitReact`, `LargeHitReact`, `KnockDown` 같은 BA 전용 피격 반응 메타데이터는 UE 기본 `FDamageEvent` 계열에 없다.
- 따라서 공격자가 피격 반응 강도를 런타임으로 지정해야 할 때 `DamageReactionType`을 `TakeDamage` 경계로 함께 넘기기 위한 최소 확장점으로 남겨두었다.

현재 필드:

- `DamageReactionType`
- `DamageDirection`
- `HitResult`

주의할 점:

- `DamageDirection`은 표면 법선이 아니다.
- 공격자에서 피격자 방향으로 향하는 게임플레이 방향이다.
- 가드 각도, 피격 방향 몽타주, 넉백 방향에 사용된다.

### `FBACharacterDamageContext`

제거됨.

기존에는 `TakeDamage` 이후 내부 표준 컨텍스트로 `FinalDamage`, `DamageDirection`, `HitDirection`, `HitResult`, `DamageCauser`, `EventInstigator` 등을 묶어 넘겼다.

하지만 현재 소비처가 Player/Enemy 정도이고, `OnDamaged` 인자와 helper 함수로 충분하다고 판단해서 제거했다.

현재 `ACharacterBase::OnDamaged` 시그니처:

```cpp
virtual void OnDamaged(
    float FinalDamage,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser);
```

### `CharacterBase`의 DamageEvent 해석 helper

위치: `Source/BAProject/Character/CharacterBase.h/.cpp`

현재 helper:

- `ResolveDamageReactionType(FDamageEvent const& DamageEvent)`
- `ResolveDamageDirection(const AActor& DamagedActor, FDamageEvent const& DamageEvent, const AActor* DamageCauser)`
- `ResolveDamageHitResult(FDamageEvent const& DamageEvent)`
- `ResolveHitDirection(const AActor& DamagedActor, const FVector& DamageDirection)`

`DamageEvent.IsOfType(...)` 분기가 있는 이유:

- `TakeDamage`와 `OnDamaged`는 UE 표준 시그니처 때문에 `const FDamageEvent&`만 받는다.
- 실제 런타임 객체가 `FBADamageEvent`인지 `FPointDamageEvent`인지 확인해야 해당 타입의 필드를 안전하게 읽을 수 있다.
- `FDamageEvent` 계열은 `UObject`가 아니므로 `Cast<>`가 아니라 `GetTypeID` / `IsOfType` 기반 자체 RTTI를 사용한다.

타입별 해석:

```cpp
FBADamageEvent:
- DamageReactionType
- DamageDirection
- HitResult

FPointDamageEvent:
- ShotDirection
- HitInfo
```

fallback:

- 방향이 없고 `DamageCauser`가 있으면 `(DamagedActor.Location - DamageCauser.Location).GetSafeNormal()`을 사용한다.
- 반응 타입이 없으면 `EBADamageReactionType::HitReact`로 본다.
- HitResult가 없으면 빈 `FHitResult()`를 반환한다.

## 플레이어 피격 반응 상태

위치:

- `Source/BAProject/Player/BAPlayerCharacterTypes.h`
- `Source/BAProject/Player/BAPlayerCharacter.h`
- `Source/BAProject/Player/BAPlayerCharacter.Damage.cpp`

현재 `EPlayerDamageReactionState`:

- `None`
- `HitReact`
- `LargeHitReact`
- `KnockDown`
- `GuardHit`
- `GuardBreak`

`EBADamageReactionType::GuardBreak`는 제거했다.

이유:

- “가드를 깨는 공격”이라는 공격자 주문형 피격 타입은 쓰지 않기로 했다.
- 가드 브레이크는 오직 플레이어가 가드 중 피격되고, 그 결과 스태미너가 0 이하가 됐을 때만 발생해야 한다.
- 따라서 가드 브레이크는 공격 데이터가 아니라 플레이어 방어 처리 결과다.

현재 `GuardBreak`는 플레이어 내부 반응 상태 및 몽타주 선택용으로만 남아 있다.

## 넉다운 반응

`EBADamageReactionType::KnockDown`을 추가했다.

의도:

- `LargeHitReact`보다 더 멀리 날아가는 피격 반응이다.
- 날아가는 애니메이션 중 추가 피격을 막기 위한 상태다.
- 현재는 `KnockDown` 반응 시작 시 `SetInvincible(true)`를 호출하고, 반응 종료 시 `SetInvincible(false)`로 되돌린다.

관련 설정:

- `KnockDownKnockbackStrength`
- `KnockDownReactMontages`

전용 몽타주가 없으면 `LargeHitReactMontages`를 fallback으로 사용한다.

## `BAPlayerState` 변경

위치: `Source/BAProject/Player/BAPlayerCharacter.h/.cpp/.Damage.cpp`

`BAPlayerState` 직접 대입을 줄이기 위해 setter를 추가했다.

```cpp
UFUNCTION(BlueprintCallable, Category = "Combat")
void SetBAPlayerState(EBAPlayerState NewState);
```

현재 코드에서 공격/피격/사망/피격 종료 시 상태 변경은 setter를 통해 수행한다.

아직 완전한 상태 머신은 아니다. `BAPlayerState`는 ActionComponent/CombatComponent/Movement 입력 정책이 충돌하는 현재 구조에서 임시 중재 상태로 쓰이고 있다. 추후 시간이 있으면 PlayerCharacter 단일 정책 상태 또는 ActionComponent 통합 상태로 정리하는 것이 좋다.

## 현재 빌드 검증

마지막 C++ 구조 변경 후 다음 빌드를 성공시켰다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat' BAProjectEditor Win64 Development -Project='C:\Users\mindo\Workspace\ProjectBA\BAProject.uproject' -WaitMutex
```

빌드 결과:

- 성공
- 기존 경고는 남아 있음
  - `AN_PlayerAttackEnd.cpp`의 deprecated `UAnimNotify::Notify`
  - `InteractorComponent.cpp`의 관련 없는 `Cast<>` static warning

## 오늘 다 못한 작업 / 다음 작업 후보

### 1. 가드 중 피격 스태미너 처리 구현

아직 구현되지 않은 핵심 작업이다.

현재 코드에는 “가드 중 피격 → 스태미너 감소 → 0 이하이면 `SetGuardState(EGuardState::GuardBroken)`” 흐름이 없다. 사용자가 확인한 것처럼 실제 진입 지점이 비어 있다.

현재 `ABAPlayerCharacter::OnDamaged`는 대략 다음 순서다.

1. `StatComponent->ApplyDamage(FinalDamage)`로 HP 피해 적용
2. 생존 여부 확인
3. `DamageReactionType`, `DamageDirection`, `HitDirection` 계산
4. `IsGuardingAgainstDamage(DamageDirection)` 확인
5. `ShouldPlayGuardBreakReaction()` 확인
6. 피격 반응 재생

문제:

- 가드 성공 여부를 판단하기 전에 이미 HP 피해가 적용된다.
- 가드 성공 시 HP 피해를 막거나 줄이는 정책이 아직 없다.
- 가드 성공 시 스태미너를 얼마나 깎을지 정책이 없다.
- 스태미너 0일 때 `GuardBroken`으로 바꾸는 로직이 없다.

다음 구현 방향 예시:

```cpp
const EBADamageReactionType DamageReactionType = ResolveDamageReactionType(DamageEvent);
const FVector DamageDirection = ResolveDamageDirection(*this, DamageEvent, DamageCauser);
const EActionDirection HitDirection = ResolveHitDirection(*this, DamageDirection);

const bool bGuarding = IsGuardingAgainstDamage(DamageDirection);

if (bGuarding)
{
    const float StaminaDamage = ResolveGuardStaminaDamage(FinalDamage, DamageEvent);
    StatComponent->ConsumeStamina(StaminaDamage);

    if (StatComponent->GetCurrentStamina() <= 0.f)
    {
        ActionComponent->SetGuardState(EGuardState::GuardBroken);
    }
}
else
{
    StatComponent->ApplyDamage(FinalDamage);
}

const bool bGuardBreak = bGuarding && ShouldPlayGuardBreakReaction();
```

정해야 할 정책:

- 가드 성공 시 HP 피해를 완전히 막을지, 일부만 받을지
- 스태미너 피해량은 `FinalDamage` 기반인지, 공격별 별도 데이터인지
- 스태미너가 0이 된 프레임에 GuardHit와 GuardBreak 중 무엇을 우선할지
- GuardBroken 상태를 언제 `None` 또는 `Guarding`으로 복구할지
- GuardBroken일 때 입력/이동/가드 재시도 제한 시간

### 2. `FBADamageEvent`와 `FPointDamageEvent` 통합 방향 결정

현재는 `FBADamageEvent : FDamageEvent`를 유지한다.

하지만 추후 더 정리하려면 두 가지 선택지가 있다.

선택 A: 현재 유지

- `FBADamageEvent`가 `DamageReactionType`, `DamageDirection`, `HitResult`를 보관
- `FPointDamageEvent`로 들어오는 외부 대미지도 helper에서 처리
- 현재 코드와 가장 가까움

선택 B: `FBADamageEvent : FPointDamageEvent`로 변경

- UE 기본 `HitInfo`, `ShotDirection`을 그대로 사용
- BA 확장 필드는 `DamageReactionType` 하나만 추가
- 논의 방향과 가장 잘 맞는 장기 구조
- 변경 시 `ClassID`, `IsOfType`, 생성부, helper를 다시 점검해야 함

선택 C: `FPointDamageEvent` + `UDamageType` 파생으로 반응 타입 표현

- UE 패턴에는 더 가깝지만, 공격마다 런타임 반응 타입을 바꾸는 용도에는 오히려 불편할 수 있음
- `UDamageType` 클래스가 데이터 조합마다 늘어날 수 있음

현재 추천은 B다. 다만 마감이 가깝다면 현재 구조를 유지해도 문제는 없다.

### 3. `CombatComponent::ApplyDamage`에서 방향 규약 명시

현재 위치: `Source/BAProject/Component/CombatComponent.cpp`

현재 BA 전투 코드는 `FBADamageEvent`를 만들고:

- `DamageReactionType = CurrentDamageReactionType`
- `HitResult = HitResult`
- `DamageDirection = VictimLocation - OwnerLocation`

으로 넘긴다.

다음 작업자는 이 방향 규약을 유지할지 결정해야 한다.

후보:

- `VictimLocation - AttackerLocation`
  - 가드 각도와 넉백에 안정적
  - 무기 궤적의 실제 이동 방향과는 다를 수 있음
- `CurrentMid - PrevMid`
  - 무기 sweep 이동 방향에 가까움
  - 큰 무기/회전 공격에서 공격자 위치보다 타격 궤적을 더 잘 반영할 수 있음
- `HitResult.TraceEnd - HitResult.TraceStart`
  - Trace 기반 방향
  - sweep 구현 방식에 따라 기대와 다를 수 있음

가드 판정이 중요하면 우선 `VictimLocation - AttackerLocation` 유지가 단순하고 안정적이다.

### 4. 에디터/블루프린트 확인 필요

C++ 빌드는 성공했지만 에디터 에셋 검증은 하지 않았다.

확인할 것:

- `EBADamageReactionType::GuardBreak` 제거로 Blueprint, DataTable, Animation Notify, 에셋 참조가 깨지지 않았는지
- `KnockDownReactMontages`에 필요한 몽타주가 설정되어 있는지
- `KnockDownKnockbackStrength` 기본값이 실제 플레이 감각에 맞는지
- `KnockDown` 반응 중 `SetInvincible(true)`가 다른 무적 윈도우와 충돌하지 않는지
- 피격 종료 시 `SetInvincible(false)`가 액션 무적 윈도우를 의도치 않게 끄지 않는지

특히 마지막 항목은 중요하다. 현재 `SetInvincible`은 단일 상태값이라 여러 무적 source를 reference count로 관리하지 않는다. KnockDown 무적과 ActionAnimationComponent의 iframe 무적이 겹치면 서로 덮어쓸 수 있다. 현재 구조에서는 실제 충돌 가능성이 낮을 수 있지만, 장기적으로는 무적 source/count 관리가 필요하다.

### 5. `BAPlayerState` 정리

현재 `BAPlayerState`는 임시 중재 상태다.

문제:

- 이름은 “현재 재생 중인 애니메이션 상태”처럼 보이지만 실제로는 입력/이동/피격 정책에도 사용된다.
- `ActionComponent`의 `EActionRuntimeState`, `DamageReactionState`, `MovementRuntime.Phase`와 역할이 겹친다.

단기적으로는 setter를 통해 변경 지점을 통제한다.

장기적으로는 다음 중 하나를 선택하는 것이 좋다.

- `EBAPlayerState`를 `EBAPlayerControlState` 또는 `EBAPlayerActionState`로 이름 변경
- 공격/가드/회피/피격 상태를 모두 ActionComponent에 통합
- PlayerCharacter가 “입력 전환 정책 관리자” 역할을 명시적으로 갖고, component 상태를 읽어 파생 상태를 계산

현재 마감 상황에서는 PlayerCharacter가 중재하는 현재 방향이 현실적이다.

## 다음 작업자가 먼저 보면 좋은 파일

- `Source/BAProject/Combat/BADamageTypes.h`
- `Source/BAProject/Character/CharacterBase.h`
- `Source/BAProject/Character/CharacterBase.cpp`
- `Source/BAProject/Component/CombatComponent.cpp`
- `Source/BAProject/Player/BAPlayerCharacter.h`
- `Source/BAProject/Player/BAPlayerCharacter.Damage.cpp`
- `Source/BAProject/Component/ActionComponent.h`
- `Source/BAProject/Component/ActionComponent.cpp`
- `Source/BAProject/Component/StatComponent.h`
- `Source/BAProject/Component/StatComponent.cpp`

## 이어서 작업할 때 추천 순서

1. 에디터에서 `EBADamageReactionType::GuardBreak` 제거로 깨진 Blueprint/DataTable 참조가 있는지 확인한다.
2. `ABAPlayerCharacter::OnDamaged`의 HP 적용 순서를 가드 판정 이후로 옮길지 결정한다.
3. 가드 성공 시 스태미너 소비량 정책을 정한다.
4. 스태미너가 0 이하가 되면 `ActionComponent->SetGuardState(EGuardState::GuardBroken)`을 호출한다.
5. GuardBreak 반응 종료 후 guard state 복구 정책을 넣는다.
6. 필요하면 `FBADamageEvent : FPointDamageEvent` 구조로 줄인다.
7. `BAProjectEditor Win64 Development` 빌드 후 PIE에서 일반 피격, 가드 피격, 가드 브레이크, 넉다운을 각각 확인한다.
