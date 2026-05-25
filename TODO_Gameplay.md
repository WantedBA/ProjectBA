# 게임플레이 할일 목록

작성일: 2026-05-25
기준 브랜치: `origin/develop` / `9abec5e7` Feature/death montage flow (#104)
진행 중 참고 브랜치: `develop` / 낙하 착지 작업

정책, 결정사항, 현재 동작 요약은 [GAMEPLAY_POLICY.md](GAMEPLAY_POLICY.md)에만 기록한다. 이 문서는 진행 중 작업과 남은 할일만 기록한다.

## 진행 중

### 1. 낙하, 착지, 낙사

담당 브랜치: `develop`

- C++ 기본 흐름은 구현됐다.
  - `BAPlayerCharacter.Movement.Falling.cpp`를 추가했다.
  - `Falling()`에서 낙하 시작 높이를 저장한다.
  - `Landed()`에서 낙하 거리, 낙하 데미지, 낙사, 강착지 회복을 처리한다.
  - KnockDown/Airborne 같은 피격 런치는 낙하 데미지로 해석하지 않게 분리했다.
  - `K2_OnFallStarted`, `K2_OnLandedFromFall`, `K2_OnLandingRecoveryStarted`, `K2_OnLandingRecoveryEnded`로 BP 연출 연결점을 열었다.
- 남은 확인:
  - `BP_PlayerCharacter`에서 `SafeFallDistance`, `FatalFallDistance`, `FallDamageMinCurrentHPPercent`, `FallDamageMaxCurrentHPPercent` 값 튜닝.
  - 연출 구간에서 `BAFallDamageSuppressionVolume` 배치 후 낙하 피해 억제 확인.
  - `ABP_Player`에서 LandLight/LandHeavy 상태와 `CompleteLandingRecoveryAnimation` Notify 연결 확인.
  - PIE에서 짧은 낙하, 데미지 낙하, 낙사, KnockDown/Airborne 착지 충돌 여부 확인.

## 남은 작업

### 2. 카메라 충돌, 에임, 공격 셰이크

- 카메라 에임 조정.
- 근접 전투에서 적 메쉬 또는 벽에 비빌 때 카메라가 과하게 당겨지거나 땅을 보는 문제를 수정한다.
- 플레이어/적 캡슐 크기가 아니라 SpringArm 충돌, 카메라 충돌 채널, 적 메쉬의 Camera 응답을 분리해서 확인한다.
- 적 메쉬가 카메라만 불필요하게 막는 경우 Camera 채널 응답을 조정한다.
- 벽 충돌은 유지하되 근접 전투에서 시야가 무너지지 않도록 SpringArm `ProbeSize`, `ProbeChannel`, 카메라 보정 규칙을 정한다.
- 카메라 상하 회전각이 너무 자유로워 시야가 흔들리는 문제를 줄이기 위해 Pitch 최소/최대 각도를 좁힌다.
- 공격 시 카메라 셰이크를 추가한다.
- 남은 셰이크 작업은 공격 시 카메라 셰이크만 추적한다.

### 3. 피격/공격 후딜 탈출구간

- KnockDown/Airborne 기립 탈출은 `feature/death-montage-flow`에서 진행 중이다.
- 일반 피격 리액션 후딜에서 입력 가능한 탈출구간을 추가할지 결정한다.
- 공격 후딜에서 다음 행동으로 넘어갈 수 있는 탈출구간을 추가한다.
- 탈출 가능한 행동 범위를 먼저 정한다.
  - 예: 회피, 가드, 다음 공격, 이동 복귀.
- `ActionWindowData`를 사용할지, 리액션/공격 도메인별 별도 타이밍 정책으로 둘지 결정한다.

### 4. 공격 처치 후 떨림

- 공격으로 적을 죽인 직후 플레이어가 짧게 떨리는 문제를 확인한다.
- 락온 사망 지연, 공격 루트모션, 적 사망 충돌 비활성화, ControllerRotationExtension 회전 보정 중 어느 경로인지 분리한다.
- 락온 상태와 비락온 상태를 각각 재현해서 락온 브랜치 영향인지 먼저 판정한다.

### 5. Strafe 백스탭

- 백스탭 애니메이션을 migrate한다.
- 백스탭 몽타주를 루트모션 기준으로 연결한다.
- Strafe 상태에서 `S` 입력을 누른 채 회피를 요청하면 뒷구르기가 아니라 백스탭이 나가야 한다.
- 현재 Strafe 회피는 입력 방향 애니메이션을 고르도록 정리되어 있으므로, 남은 작업은 `Backward` 방향 데이터와 몽타주 연결이다.

### 6. 락온 대상 체력바

- 락온 대상이 일반 몬스터일 때 머리 위에 체력바를 표시한다.
- 보스는 기존 보스 UI 정책과 충돌하지 않게 별도 처리한다.
- 락온 대상이 스위칭되거나 해제되어도 기존 대상 체력바는 1초 동안 유지한다.
- 1초 유지 시간 동안 다시 같은 대상을 락온하면 유지 타이머를 취소하고 현재 락온 체력바 상태로 복귀한다.
- 체력바 표시 상태는 `OnTargetLocked`, `OnTargetUnlocked`, `OnSocketChanged` 흐름과 연결한다.

### 7. 게임패드 입력

- 게임패드용 IMC 추가.
- Input Action 추가 또는 기존 Input Action에 게임패드 매핑 추가.
- 키보드/마우스 테스트 입력과 게임패드 기준 조작이 충돌하지 않는지 확인.
- 락온 중 오른쪽 스틱 X/Y 입력이 타겟 전환으로 연결되는지 확인한다.

## 완료/병합됨

### 일반 몬스터 피격 화면 깜빡임

- 완료되어 병합됐다.
- 남은 작업에서 제외한다.

### 콤보 버퍼 만료

- #100 공격 몽타주 중단 상태 정리와 함께 완료되어 병합됐다.
- `NextAttackMontage`, `NextAttackActionType`, `NextComboTransitionTid` 정리 흐름에 포함됐다.

### 피격/가드 카메라 셰이크

- `feature/death-montage-flow`에서 완료됐다.
- 남은 셰이크 작업은 공격 시 카메라 셰이크만 추적한다.

### 사망 몽타주, 기립

- #104로 완료되어 병합됐다.
- 사망 몽타주, LargeHit 사망 루트모션, KnockDown/Airborne 기립, 피격/가드 셰이크 흐름을 포함한다.

## 다음 작업 추천 순서

1. 낙하, 착지, 낙사 PIE 검증 및 BP 값 튜닝
2. 피격/공격 후딜 탈출구간
3. 카메라 충돌/상하 회전각/에임/공격 셰이크 폴리싱
4. 공격 처치 후 떨림
5. 락온 대상 체력바
6. Strafe 백스탭
7. 게임패드 IMC/Input Action

공격 방향과 공격 스태미너 비용은 현재 상태가 의도에 맞으므로 다음 작업에서 건드리지 않는다.
