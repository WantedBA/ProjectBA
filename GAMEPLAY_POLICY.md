# 게임플레이 정책 및 현재 동작

작성일: 2026-05-22
문서 분리: 2026-05-25
기준 브랜치: `origin/develop` / `debf5d8e` Features/skill apply element (#103)
진행 중 참고 브랜치: `feature/death-montage-flow` / `b01e9613`
최근 업데이트: 2026-05-25 / TODO 4번, 8번 완료/병합 및 피격/가드 셰이크 완료 반영

이 문서는 정책, 결정사항, 현재 동작, 완료 이력만 기록한다. 남은 작업과 우선순위는 [TODO_Gameplay.md](TODO_Gameplay.md)에만 기록한다.

## 현재 결정사항

- 게임은 게임패드 기준으로 개발한다.
- 키보드/마우스 입력으로 테스트 중이더라도, 실제 의도는 왼쪽 스틱 이동 방향을 기준으로 공격과 회피 방향이 결정되는 구조다.
- 공격 방향은 현재 동작을 유지한다.
  - 왼쪽 스틱으로 이동하면서 공격 버튼을 누르면 이동 방향 기준으로 캐릭터가 회전하고 공격하는 것이 맞다.
  - 콤보 공격 사이의 버퍼 방향도 현재 방식이 좋다.
  - 회피처럼 재생 직전에 공격 방향을 별도 정책으로 강제 재해석하는 작업은 하지 않는다.
- 회피는 이동 입력이 있을 때와 없을 때를 다르게 취급한다.
  - 이동 입력 있음: 컨트롤러 yaw 기준 입력 방향으로 캐릭터를 회전하고, 전방 구르기 몽타주를 재생한다.
  - 이동 입력 없음: 캐릭터 현재 정면 기준으로 뒤로 백스텝한다.
- 공격 스태미너 비용은 `ActionData.StaminaCost`와 `ActionData.StaminaCostType`을 사용한다.
  - `Instant`: 액션 시작 비용.
  - `OnDemand`: 가드 피격/퍼펙트 가드 같은 명시 소비 비용.
  - 별도 소비 컨텍스트 enum은 두지 않는다.
- 피격/가드 카메라 셰이크는 기본 C++ 셰이크로 연결되어 있다.
- 공격 카메라 셰이크는 별도 폴리싱 작업으로 남긴다.

## 완료된 작업

### 연속 구르기 방향 전환

- 연속 구르기 중 이동 입력을 바꿨을 때 방향이 잘 바뀌지 않던 문제를 수정했다.
- 버퍼된 회피는 실제 재생 직전에 현재 이동 입력을 다시 읽어 방향을 갱신한다.
- 관련 커밋:
  - `65216137` 연속 구르기 방향 전환을 개선
  - `0d3db35d` 버퍼된 회피와 콤보 공격 방향을 재생 직전에 갱신

### 컨트롤러 기준 방향 판정

- 캐릭터 가속/정면 기준이 아니라 컨트롤러 yaw 기준 입력 방향으로 액션 방향을 해석하도록 수정했다.
- `S` 입력은 캐릭터가 어느 방향을 보고 있든 컨트롤러 기준 뒤 입력으로 해석한다.
- 관련 커밋:
  - `5b50236b` 액션 방향 판정을 컨트롤러 기준으로 변경
  - `b5d8c3f5` 회피 루트모션 기준을 컨트롤러 방향으로 맞춤

### 구르기 몽타주 선택 정리

- 8방향 구르기 몽타주를 직접 고르는 방식이 아니라, 이동 입력이 있으면 항상 전방 구르기 몽타주를 재생하도록 정리했다.
- 방향성은 몽타주 선택이 아니라 재생 전 캐릭터 회전으로 처리한다.
- 이동 입력이 없으면 백스텝용 `Backward` 애니메이션 방향을 사용한다.
- 관련 커밋:
  - `6525279f` 구르기 몽타주 선택을 전방 중심으로 정리

### 백스텝

- 이동 입력 없이 스페이스/회피 버튼을 누르면 캐릭터 현재 정면 기준 뒤로 백스텝한다.
- 무입력 회피는 액터를 컨트롤러 기준 뒤로 돌리지 않는다.
- 백스텝 몽타주를 회피 `Backward` 데이터에 연결했다.
- 관련 에셋:
  - `Content/Character/Player/Animation/Montages/DodgeRoll/AM_Player_Dodge_Backstep.uasset`
- 관련 데이터:
  - `BADesign/Excel/Action.xlsx`
  - `BADesign/Json/Action.json`
  - `Content/Table/DT_Action_ActionAnimationData.uasset`
- 변경된 `ActionAnimationData` 행:
  - `30023` / `ActionTid 10021` / `Direction Backward`
  - `30032` / `ActionTid 10022` / `Direction Backward`
- 관련 커밋:
  - `4cc1d665` 무입력 회피를 캐릭터 기준 백스텝으로 보정
  - `06762b4b` 백스텝 몽타주를 회피 데이터에 연결
  - `860963d8` 백스텝 회피 데이터 원본을 갱신

### ActionAnimationComponent 책임 정리

- `ActionAnimationComponent` 안에 들어갔던 `DodgeRoll`/`Backstep` 예외처리를 제거했다.
- `ActionAnimationComponent`는 액션 애니메이션 재생, 몽타주 선택, 윈도우 처리만 담당한다.
- 액션별 방향 정책은 각 도메인에서 처리한다.
- 현재 회피 애니메이션 방향 정책은 `BAPlayerCharacter` 쪽에 있다.
- `ActionAnimationComponent`에는 일반 delegate인 `ResolveActionAnimationDirection`만 추가했다.
- 관련 파일:
  - `Source/BAProject/Component/ActionAnimationComponent.h`
  - `Source/BAProject/Component/ActionAnimationComponent.cpp`
  - `Source/BAProject/Player/BAPlayerCharacter.Movement.cpp`
  - `Source/BAProject/Player/BAPlayerCharacter.Movement.Input.cpp`
  - `Source/BAProject/Player/BAPlayerCharacter.h`
- 관련 커밋:
  - `e20addf7` 회피 애니메이션 방향 정책을 플레이어 도메인으로 분리

### 공격 스태미너 소비

- `EActionStaminaConsumeContext`를 제거하고 `ActionData.StaminaCostType`만 비용 종류의 기준으로 사용하도록 정리했다.
- `ActionComponent`에서 사용하지 않던 `ConsumeActiveActionStaminaCost`를 제거했다.
- 외부에서 직접 쓸 필요가 없던 `FindBestMoveset`, `CanStartAction`을 `private`로 내렸다.
- 공격은 기존 레거시 콤보 재생 경로를 유지하면서 `ActionComponent`의 비용 소비 API만 사용한다.
- `ConsumeActionStartStaminaCostByType`을 추가했다.
  - `StaminaCostType == Instant` 비용만 소비한다.
  - LightAttack은 기존 데이터의 `StaminaCost 15`, HeavyAttack은 `StaminaCost 30`을 사용한다.
- `ConsumeActionStaminaCostByType`은 `OnDemand` 소비 전용으로 유지한다.
- 첫 공격은 몽타주가 유효하고 스태미너가 충분할 때만 시작한다.
- 콤보 공격은 입력 시점에 비용을 빼지 않고, `OnNextComboCheck`에서 실제 다음 콤보가 재생되기 직전에 비용을 소비한다.
- 다음 콤보 재생 직전에 스태미너가 부족하면 예약된 다음 콤보를 비우고 현재 공격만 마무리한다.
- 다음 콤보가 Light/Heavy인지 `NextAttackActionType`으로 보관해서 비용과 데미지 배율을 맞춘다.
- 관련 커밋/PR:
  - `add04d12` 액션 스태미너 소비 흐름을 정리
  - `da415a7b` 공격 콤보에 스태미너 비용을 적용
  - `a946a60e` Feature/attack stamina cost (#91)

### 락온 플러그인 포팅

- `LockOnTarget` 플러그인을 프로젝트 플러그인으로 추가했다.
- UE 5.6 기준으로 컴파일되도록 포팅했다.
- `BAProject` 모듈에서 `LockOnTarget` 의존성을 사용한다.

### 락온 대상 상태 처리

- 적 사망 시 `LockOnReleaseDelayOnDeath` 이후 `TargetComponent::SetCanBeCaptured(false)`를 호출한다.
- 기본 지연 시간은 `0.75`초다.
- 현재 락온 대상이 죽으면 지연 시간 동안 기존 타겟을 유지한 뒤 플러그인의 `StateInvalidation` 흐름이 실행된다.
- `WeightedTargetHandler`는 주변 타겟을 다시 찾고, 실패하면 락온을 해제한다.
- 일반 몬스터와 보스 BP에 락온 타겟 컴포넌트가 붙어 있다.

### 락온 입력과 타겟 전환

- 임시 `ToggleStrafe` 입력 경로를 제거했다.
- `LockOnAction` 입력은 `ABAPlayerController::OnLockOnStarted()`로 들어온다.
- 컨트롤러는 `ABAPlayerCharacter::ToggleLockOnTargeting()`만 호출한다.
- 캐릭터는 `ULockOnTargetComponent::EnableTargeting()`을 호출한다.
- `EnableTargeting()`은 타겟이 없으면 탐색하고, 이미 락온 중이면 해제한다.
- `IA_LockOn`은 `IMC_Default`에서 Middle Mouse Button으로 매핑한다.
- 락온 중 `LookAction` 입력은 카메라 회전으로 쓰지 않고 `SwitchLockOnTargetInput()`으로 들어간다.
- `SwitchLockOnTargetInput()`은 플러그인의 `SwitchTargetYaw()`와 `SwitchTargetPitch()`를 호출한다.
- 플러그인은 입력 버퍼가 임계값을 넘으면 `WeightedTargetHandler`로 입력 방향의 다음 후보를 찾는다.

### 락온 이동과 카메라

- 락온 성공 시 현재 locomotion을 저장하고 Strafe로 전환한다.
- 락온 해제 시 락온 때문에 바뀐 경우에만 이전 locomotion으로 복구한다.
- 락온 중 Sprint 요청은 Free 상태와 같은 규칙으로 허용한다.
- Run/Sprint 전환 시 실제 `MaxWalkSpeed`는 즉시 바뀌지 않고 보간된다.
- 보간 속도는 `BP_PlayerCharacter`의 `Movement > Speed Up Interp Rate`, `Slow Down Interp Rate`로 조정한다.
- Strafe Run에서 Sprint로 넘어갈 때 캐릭터 yaw는 `Strafe Sprint Facing Rotation Rate Yaw`로 이동 방향을 향해 보간된다.
- Strafe 상태의 회피는 입력 방향 애니메이션을 그대로 사용한다.
- Free 상태의 회피는 기존처럼 입력 방향으로 캐릭터를 돌리고 전방 구르기 몽타주를 재생한다.
- 구르기 중 락온되면 Strafe 전환은 구르기 몽타주 종료 뒤로 지연된다.
- `ControllerRotationExtension`이 컨트롤러 회전을 타겟 방향으로 돌린다.
- `ConfigureLockOnCameraDefaults()`는 카메라 상대 Pitch를 기준으로 `PitchOffset`을 보정한다.
- 화면 높이 조정은 `BP_PlayerCharacter`의 `LockOn Additional Controller Pitch Offset`으로 한다.
- 현재 테스트 기준 `-10` 값이 원하는 화면 높이에 가깝다.

### 최근 develop 반영

- `1382052c` Features/enemy trail (#99)
  - 적 Trail/VFX 관련 에셋과 에디터 AI/Niagara 분석/익스포트 경로가 보강됐다.
- `75cdaf82` 콤보 공격 중단 상태 정리, 차징 공격 최대 시간 제한 (#100)
  - 공격 몽타주가 중간에 끊겼을 때 콤보 상태가 남는 문제를 정리했다.
  - 차징 공격에 `MaxChargeTime` 제한과 차징 시간 기반 대미지 반영이 추가됐다.
- `91f0e8c4` Features/vfx (#101)
  - 플레이어 리스폰 몽타주 에셋과 리스폰 재생 흐름이 추가됐다.
- `debf5d8e` Features/skill apply element (#103)
  - 스킬 속성 적용 데이터와 무기 Trail/Element VFX 경로가 추가됐다.
  - `PlayerWeaponVFX` 컴포넌트가 추가되고 스킬 적용 흐름과 연결됐다.

### 피격/가드 카메라 셰이크

- `feature/death-montage-flow`에서 기본 C++ 셰이크 연결을 완료했다.
- `UBADamageCameraShake`를 `BAPlayerCharacter` 기본값으로 사용한다.
- 피격 리액션 재생 시 `ClientStartCameraShake()`를 호출한다.
- 일반 피격, 큰 피격, KnockDown, GuardHit, GuardBreak는 같은 기본 셰이크를 사용한다.
- 공격 셰이크는 이 작업에 포함하지 않는다.

### 일반 몬스터 피격 화면 깜빡임

- 완료되어 병합됐다.
- 남은 작업에서 제외했다.

### 콤보 버퍼 만료

- #100 공격 몽타주 중단 상태 정리와 함께 완료되어 병합됐다.
- `NextAttackMontage`, `NextAttackActionType`, `NextComboTransitionTid` 정리 흐름에 포함됐다.

## 진행 중인 정책/구현 메모

### `feature/death-montage-flow`

- 플레이어 사망 처리는 `BAPlayerCharacter.Damage.Death.cpp`로 분리되어 있다.
- 일반 피격 사망과 큰 피격 사망은 방향별 사망 몽타주 맵으로 분리한다.
  - `DeathMontages`
  - `LargeHitDeathMontages`
- 사망 최종 처리는 피격/사망 몽타주 재생 이후로 지연하고, 마지막 자세를 고정한다.
- 사망 시 장착 무기 드롭은 별도 설정 묶음으로 분리한다.
- KnockDown/Airborne 리액션은 `Damage.Hit.cpp`와 `Damage.GetUp.cpp`로 분리되어 있다.
- KnockDown/Airborne은 지면 도착 후 바로 idle로 돌아가지 않고 마지막 프레임 고정, 입력 탈출 창, 기립 몽타주를 거친다.
- 피격/가드 카메라 셰이크는 `UBADamageCameraShake` 기본 C++ 클래스로 연결되어 있다.
  - 기본 클래스는 `BAPlayerCharacter` 생성자에서 `DamageReactionCameraShakeClass = UBADamageCameraShake::StaticClass()`로 설정한다.
  - 피격 리액션 재생 시 `PlayDamageReactionCameraShake()`가 `ClientStartCameraShake()`를 호출한다.
  - 현재 기본 구현은 모든 피격/가드 피격/가드 브레이크에 같은 셰이크 클래스를 사용한다.

## 현재 동작 요약

### 회피

- `TryStartDodgeAction`에서 현재 이동 입력을 액션 방향으로 변환해 `Dodge` 액션을 시작한다.
- 이동 입력이 없는 경우 액션 방향은 `Any`다.
- 버퍼된 회피는 재생 직전 `BAPlayerCharacter::ResolveBufferedActionDirection`에서 현재 이동 입력을 다시 읽는다.
- `BAPlayerCharacter::ResolveActionAnimationDirection`에서 회피 애니메이션 검색 방향을 결정한다.
  - Free 상태: `Any` -> `Backward`, 그 외 방향 -> `Forward`
  - Strafe 상태: `Any` -> `Backward`, 그 외 방향 -> 입력 방향 유지
- `ActionAnimationComponent::ResolveActionOrientationDirection`에서 회피 전 캐릭터 회전 방향을 별도로 정한다.
  - Free 상태: 입력 방향으로 캐릭터를 돌린다.
  - Strafe 상태: 캐릭터를 돌리지 않고 타겟을 바라보는 상태를 유지한다.

### 공격

- 공격 방향은 현재 방식 유지.
- 이동 입력이 있는 상태에서 공격하면 이동 방향 기준으로 공격 방향이 잡힌다.
- 게임패드 기준으로 왼쪽 스틱 이동 방향 + 공격 버튼 조합이 의도된 조작이다.
- 공격 버퍼 방향 정책은 추가 변경하지 않는다.
- 첫 Light/Heavy 공격은 시작 직전에 `Instant` 스태미너 비용을 소비한다.
- 콤보 Light/Heavy 공격은 `OnNextComboCheck`에서 실제 다음 몽타주 재생 직전에 `Instant` 스태미너 비용을 소비한다.
- 스태미너가 부족하면 첫 공격은 시작하지 않고, 다음 콤보는 예약을 취소한다.
- 공격 몽타주가 중간에 끊겨 공격 상태가 아니게 되면 `NowComboTransitionTid`를 초기화한다.
- 차징 공격은 `MaxChargeTime`으로 최대 차징 시간을 제한한다.
- 현재 공격 비용 데이터:
  - LightAttack: `StaminaCost 15`, `StaminaCostType Instant`
  - HeavyAttack: `StaminaCost 30`, `StaminaCostType Instant`

### 락온

- Middle Mouse Button 입력은 `IA_LockOn`과 `LockOnAction`을 통해 C++ 입력 경로로 들어온다.
- 락온 입력은 타겟이 없으면 탐색하고, 타겟이 있으면 해제한다.
- 락온 중 Look 입력은 `SwitchTargetYaw/Pitch`로 전달되어 입력 방향의 다른 타겟을 찾는다.
- 락온 성공 시 Strafe로 전환하고, 락온 해제 시 이전 locomotion으로 돌아간다.
- 락온 중 Sprint 요청은 Free 상태와 같은 규칙으로 Sprint에 진입한다.
- Strafe Run에서 Sprint로 바뀔 때 이동 속도는 보간되어 snapped 느낌을 줄인다.
- Strafe Run에서 Sprint로 바뀔 때 캐릭터 방향도 보간되어 BR/BL 입력에서 즉시 꺾이지 않는다.
- 회피 중 락온을 시작해도 회피 방향은 유지되고, Strafe 전환은 회피 종료 뒤 적용된다.
- 카메라 높이는 `BP_PlayerCharacter`의 `LockOn Additional Controller Pitch Offset`으로 조정한다.

### 피격/사망

- 일반 피격, 큰 피격, KnockDown, GuardHit, GuardBreak는 `EPlayerDamageReactionState`로 분리한다.
- GuardHit는 블렌드아웃 시작 시점에 가드 루프로 복귀해 idle 노출을 줄인다.
- KnockDown/Airborne은 기립이 끝날 때까지 무적을 유지한다.
- KnockDown/Airborne 기립 대기 중 이동 입력은 기립 몽타주 일부 재생 후 이동 복귀로 처리한다.
- KnockDown/Airborne 기립 대기 중 구르기 입력은 기립 몽타주 없이 즉시 회피 탈출로 처리한다.
- 피격/가드 카메라 셰이크는 기본 C++ 셰이크로 연결되어 있다.

## 검증 상태

- 변경된 C++ 파일들은 기존 작업에서 `BAProjectEditor` 빌드 컴파일을 통과했다.
- 에디터가 실행 중이면 `UnrealEditor-BAProject.dll`을 잡고 있어서 링크 단계가 실패할 수 있다.
- 마지막 확인 시 `ActionAnimationComponent` 내부에 `DodgeRoll`/`Backstep` 문자열은 남아있지 않았다.
- 공격 스태미너 작업은 `BAProjectEditor` 빌드 성공을 확인했다.
- 락온 작업은 `BAProjectEditor Win64 Development`와 `BAProject Win64 Development` 빌드 성공을 확인했다.
- PIE에서 Middle Mouse Button 락온/해제, 타겟 전환, 락온 카메라 높이 `-10` 값을 확인했다.
- `feature/death-montage-flow`의 피격/가드 카메라 셰이크는 PIE 체감 확인을 완료했다.
