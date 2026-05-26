# 게임플레이 할일 목록

작성일: 2026-05-25
기준 브랜치: `origin/develop` / `9abec5e7` Feature/death montage flow (#104)
진행 중 참고 브랜치: `feature/camera-polishing` / 카메라 폴리싱

정책, 결정사항, 현재 동작 요약은 [GAMEPLAY_POLICY.md](GAMEPLAY_POLICY.md)에만 기록한다. 이 문서는 진행 중 작업과 남은 할일만 기록한다.

## 진행 중

### 2. 카메라 충돌, 에임, 공격 히트스톱

담당 브랜치: `feature/camera-polishing`

- C++ 기본 흐름은 구현됐다.
  - `ABAPlayerController`에서 카메라 Pitch 최소/최대 기본값을 좁혔다.
  - `BAPlayerCharacter`에서 SpringArm `ProbeSize`, `ProbeChannel`, 충돌 활성화 값을 설정으로 관리한다.
  - `AEnemyBase`에서 적 Capsule/Mesh의 `Camera` 채널 응답을 `Ignore`로 분리했다.
  - 락온 화면 높이 보정 기본값을 `LockOnAdditionalControllerPitchOffset -10`으로 맞췄다.
  - 카메라 기본값, 락온 보정, 피격/가드/착지/사망 셰이크를 `BAPlayerCharacter.Camera.cpp`로 분리했다.
  - 공격 히트 피드백은 실제 충돌 시점에 플레이어 공격 몽타주만 잠깐 멈추는 방식으로 변경했다.
  - 히트스톱은 실제 적 계열 피해 대상에 피해가 적용되고, 피해 적용 후 적이 살아 있는 경우에만 발동한다.
  - 일반공격/강공격/차지 공격 모두 같은 `AttackHitStopDuration` 정책을 사용한다.
  - `DamageReactionCameraShakeScale`은 에디터에서 숨기고, HitReact `0.15`, LargeHitReact `1.0` 기본값을 적용했다.
  - 피격/가드/사망/강착지 셰이크 스케일 정책을 적용했다.
- 남은 확인:
  - PIE에서 벽 근접, 적 근접, 락온/비락온 근접 전투 시 카메라가 과하게 당겨지지 않는지 확인.
  - Pitch 제한값 `-50/35`, SpringArm `ProbeSize 8` 체감 확인.
  - 일반공격/강공격/차지 공격 히트스톱, 처치 시 히트스톱 생략, HitReact/LargeHit/KnockDown, 일반 가드/퍼펙트 가드/가드 브레이크, LandHeavy, 사망 셰이크 체감 확인.

## 남은 작업

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

- 다른 동료가 작업 중이다.
- 백스탭 애니메이션을 migrate한다.
- 백스탭 몽타주를 루트모션 기준으로 연결한다.
- Strafe 상태에서 `S` 입력을 누른 채 회피를 요청하면 뒷구르기가 아니라 백스탭이 나가야 한다.
- 현재 Strafe 회피는 입력 방향 애니메이션을 고르도록 정리되어 있으므로, 남은 작업은 `Backward` 방향 데이터와 몽타주 연결이다.

### 7. 게임패드 입력

- 다른 동료가 작업 중이다.
- 게임패드용 IMC 추가.
- Input Action 추가 또는 기존 Input Action에 게임패드 매핑 추가.
- 키보드/마우스 테스트 입력과 게임패드 기준 조작이 충돌하지 않는지 확인.
- 락온 중 오른쪽 스틱 X/Y 입력이 타겟 전환으로 연결되는지 확인한다.

## 완료/병합됨

### 낙하, 착지, 낙사

- 완료되어 남은 작업에서 제외한다.

### 락온 대상 체력바

- 완료되어 남은 작업에서 제외한다.

### 일반 몬스터 피격 화면 깜빡임

- 완료되어 병합됐다.
- 남은 작업에서 제외한다.

### 콤보 버퍼 만료

- #100 공격 몽타주 중단 상태 정리와 함께 완료되어 병합됐다.
- `NextAttackMontage`, `NextAttackActionType`, `NextComboTransitionTid` 정리 흐름에 포함됐다.

### 피격/가드 카메라 셰이크

- `feature/death-montage-flow`에서 완료됐다.
- 공격 카메라 셰이크는 히트스톱 정책으로 대체됐다.

### 사망 몽타주, 기립

- #104로 완료되어 병합됐다.
- 사망 몽타주, LargeHit 사망 루트모션, KnockDown/Airborne 기립, 피격/가드 셰이크 흐름을 포함한다.

## 다음 작업 추천 순서

1. 카메라 충돌/상하 회전각/에임/공격 히트스톱 폴리싱
2. 피격/공격 후딜 탈출구간
3. 공격 처치 후 떨림
4. Strafe 백스탭 동료 작업 확인
5. 게임패드 IMC/Input Action 동료 작업 확인

공격 방향과 공격 스태미너 비용은 현재 상태가 의도에 맞으므로 다음 작업에서 건드리지 않는다.
