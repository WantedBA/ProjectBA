# Guard / Stamina Recovery 작업 인수인계 메모

작성일: 2026-05-21
작업 브랜치: `feat/guard-stamina-recovery`

이 문서는 가드 입력, 일반 가드/퍼펙트 가드 판정, 스태미너 비용, 가드 피격 리액션 에셋 연결 상태를 다음 작업자가 바로 이어받을 수 있도록 정리한 메모다.

## 현재 목표

- 플레이어가 가드 입력을 유지하는 동안 가드 상태가 유지되어야 한다.
- 일반 가드 피격 시 HP와 스태미너가 함께 감소하고, 짧은 GuardHit 리액션 뒤 다시 가드 Loop로 복귀해야 한다.
- 퍼펙트 가드 피격 시 HP와 넉백은 적용하지 않고, 일반 가드 스태미너 비용의 절반만 소비해야 한다.
- 스태미너가 부족한 일반 가드 피격은 GuardBreak로 처리하고, 긴 리액션 동안 이동을 막아 무방비 패널티를 준다.
- 가드 중에는 스태미너 회복이 막히지 않고 `StaminaRecoveryRateMultiplier = 0.5`로 느리게 회복된다.

## 주요 코드 구조

### 가드 액션과 몽타주

- `ABAPlayerCharacter::TryStartGuard()`
  - 가드 입력 시작 시 호출된다.
  - `ActionComponent->TryStartAction(EActionCommand::Guard)`로 가드 액션을 시작한다.
  - `ConfigureGuardMontageSections()`에서 `Start -> Loop`, `Loop -> Loop` 섹션 연결을 설정한다.
  - `bGuardInputHeld = true`로 입력 유지 상태를 기록한다.

- `ABAPlayerCharacter::StopGuard()`
  - 가드 입력 해제 시 호출된다.
  - `bGuardInputHeld = false`로 입력 유지 상태를 끈다.
  - 실제 가드 판정이 열린 뒤 해제하면 `End` 섹션으로 보내고, `Start` 중 해제하면 몽타주를 끊는다.

- `UActionAnimationComponent`
  - `SetActiveMontageNextSection()`, `JumpActiveMontageToSection()`으로 현재 액션 몽타주의 섹션 전환을 제어한다.

### 가드 윈도우

- `UANS_GuardWindow`
  - 플레이어 가드 몽타주의 `Loop` 구간에 배치한다.
  - Notify Begin에서 `SetGuardWindowActive(true)`.
  - Notify End에서 `SetGuardWindowActive(false)`.
  - `Start`, `End` 섹션에는 넣지 않는다.

- `UANS_PerfectWindow`
  - 플레이어 가드 몽타주의 `Loop` 안에서 매우 짧은 구간에 배치한다.
  - 퍼펙트 가드의 주체는 공격자가 아니라 피격자인 플레이어다.
  - 현재는 NotifyState 유지. 나중에 퍼펙트 윈도우를 대량 관리해야 하면 DataRow/window table로 옮기는 쪽을 검토한다.

### CombatComponent 책임

- `UCombatComponent::ApplyDamage()`는 공격자/피격자 구체 타입에 치우친 처리를 직접 하지 않는다.
- 피격자가 `ACharacterBase`이면 다음만 질의해서 `FBADamageEvent`에 담는다.
  - `IsGuardingAgainstDamage(DamageDirection)`
  - `IsPerfectGuardWindowActive()`
- 실제 HP 보정, 스태미너 소비, 리액션, 성공 피드백은 피격자 클래스 책임이다.

### 플레이어 가드 피격 처리

- 위치: `Source/BAProject/Player/BAPlayerCharacter.Damage.cpp`

- `OnDamaged()`
  - 퍼펙트 가드가 일반 가드보다 우선한다.
  - 퍼펙트 가드 성공 시 `HandlePerfectGuardSucceeded()` 후 조기 반환한다.
  - 일반 가드 시 `ConsumeGuardStaminaForDamage()`를 호출한다.
  - 스태미너 소비 성공: GuardHit
  - 스태미너 소비 실패: GuardBreak
  - 일반 가드와 GuardBreak 모두 임시로 `GuardAbsorptionMultiplier = 0.5`를 적용한다.
  - 이 배율은 임시 하드코딩이며, 주석대로 나중에 무기 테이블로 분리해야 한다.

- `HandlePerfectGuardSucceeded()`
  - 일반 가드 스태미너 비용의 절반만 소비한다.
  - HP 피해와 넉백은 적용하지 않는다.
  - `DamageCauserCharacter->OnAttackPerfectGuarded.Broadcast(...)`로 공격자에게 성공을 알린다.
  - 슬로우모션은 합의되지 않은 스펙이라 주석 처리 상태다.

- `FinishDamageReaction()`
  - GuardHit 종료 후 `bGuardInputHeld`가 true이고 조건이 맞으면 `ResumeGuardAfterGuardHit()`로 가드를 재개한다.
  - GuardHit 몽타주가 BlendOut에 들어가면 슬롯 가중치가 빠지며 idle이 보일 수 있어, 완전 종료를 기다리지 않고 BlendOut 시작 시점에 가드 Loop로 복귀한다.
  - GuardBreak 종료 후에는 가드 입력 유지 플래그를 끄고 가드 상태를 해제한다.

## 데이터와 에셋 상태

### Action 데이터

- Guard 액션은 시작 스태미너 비용이 없다.
- `ActionData`의 Guard 행은 다음 의도다.
  - `StaminaCost = 15`
  - `StaminaCostType = OnDemand`
  - `StaminaRecoveryRateMultiplier = 0.5`
  - `MinRequiredStamina = 1`
- `CanStartAction()`은 비용 타입별 세부 분기를 몰라도 되도록 `ConsumeStamina()` / `CanConsumeStamina()` 쪽에서 비용 타입을 해석한다.

### Guard 몽타주

- 몽타주 이름: `AM_Player_Guard`
- 경로: `/Game/Character/Player/Animation/Montages/Guard/AM_Player_Guard`
- 섹션:
  - `Start`
  - `Loop`
  - `End`
- 에디터에서 `Start`, `End` 섹션 시퀀스 Segment PlayRate를 2배로 조정했다.
- `ActionAnimationData.PlayRate`는 몽타주 전체 배율이라 1로 유지하는 것이 맞다.

### GuardHit / GuardBreak 몽타주

- 생성 및 `BP_PlayerCharacter` 연결 완료.
- 예상 경로:
  - `Content/Character/Player/Animation/Montages/Guard/AM_Player_GuardHit.uasset`
  - `Content/Character/Player/Animation/Montages/Guard/AM_Player_GuardBreak.uasset`
- `BP_PlayerCharacter`의 Class Defaults에서 다음 맵에 연결했다.
  - `Guard Hit React Montages`
  - `Guard Break React Montages`
- 우선 `Any` 키로 연결하는 방식이 가장 안전하다.

## 현재 확인 완료

- 가드 입력과 기본 가드 판정은 동작한다.
- 퍼펙트 가드 판정, 일반 가드 넉백, 일반 가드 HP 피해 보정은 PIE에서 확인했다.
- 일반 가드 피격 시 GuardHit 몽타주 재생을 확인했다.
- GuardHit 뒤 입력 유지 상태에서 idle 잡모션 없이 `AM_Player_Guard`의 `Loop` 섹션으로 복귀하는 것을 확인했다.
- 일반 가드/가드브레이크/퍼펙트 가드 판정은 PIE 화면에 디버그 텍스트로 표시된다.
  - `Guard`
  - `Guard Break`
  - `Perfect Guard`

## 남은 이슈

### 1. GuardBreak 몽타주 재생 확인 필요

GuardHit 몽타주 재생은 확인 완료했다. 처음 미재생처럼 보였던 원인은 맵/GameMode가 네이티브 `BAGameMode` 경로를 타면서 `BP_PlayerCharacter`에 설정한 몽타주 맵이 사용되지 않았기 때문이다.

우선 확인할 후보:

- `GuardBreakReactMontages` 맵 키가 실제 `HitDirection`과 맞는지 확인한다.
  - 테스트 중에는 `Any` 키가 가장 안전하다.
- 몽타주 슬롯이 `PlayAnimMontage()`가 재생하는 슬롯과 맞는지 확인한다.
- `K2_OnDamageReaction()` 블루프린트 구현이 있다면 거기서 다른 몽타주/상태를 덮어쓰는지 확인한다.

### 2. GuardHit 후 Loop 복귀 완료

현재 구현은 `FinishDamageReaction()`에서 입력 유지 시 `ResumeGuardAfterGuardHit()`을 호출한다.

동작:

- GuardHit 리액션이 BlendOut에 진입
- 입력 유지 및 조건 확인
- Guard 액션 재시작
- `AM_Player_Guard`의 `Loop` 섹션으로 즉시 점프
- `GuardState = Guarding`, `CombatMode = Block` 복구

### 3. 일반 가드 스태미너 소비 후 회복 딜레이가 적용되지 않음

현상:

- 일반 가드로 스태미너가 소모된 직후 곧바로 0.5배 회복이 시작된다.
- 패널티가 사실상 약하게 느껴진다.

원인 후보:

- `UActionComponent::ConsumeStamina()`는 `EActionStaminaConsumeContext::Start`일 때만 회복 pause/delay 쪽 처리를 한다.
- Guard의 비용은 `OnDemand`라서 `UStatComponent::ConsumeStamina()`만 호출되고, `StaminaRecoveryDelayRemaining`이 설정되지 않는다.

수정 후보:

- `UStatComponent`에 `ApplyStaminaRecoveryDelay()` 또는 `RestartStaminaRecoveryDelay()` 같은 명시 함수 추가.
- `UActionComponent::ConsumeStamina()`에서 실제 스태미너를 소비한 뒤, `OnDemand` 소비도 회복 딜레이를 트리거하도록 한다.
- 단, 가드 중 회복 자체는 막으면 안 된다. 딜레이가 끝난 뒤 `StaminaRecoveryRateMultiplier = 0.5` 상태로 회복되어야 한다.

## 다음 작업 순서 제안

1. GuardBreak 몽타주 재생 확인
2. OnDemand 스태미너 소비에도 회복 딜레이가 적용되도록 `StatComponent` / `ActionComponent` 보강
3. PIE 체크
   - 일반 가드 피격: HP 감소, 스태미너 15 감소, GuardHit 몽타주, 입력 유지 시 Loop 복귀
   - 퍼펙트 가드 피격: HP 미감소, 스태미너 7.5 감소, 넉백 없음
   - 스태미너 부족 일반 가드: GuardBreak 몽타주, 가드 해제, 긴 무방비 리액션
   - 일반 가드 스태미너 소비 직후 회복 딜레이 적용 후 0.5배 회복
