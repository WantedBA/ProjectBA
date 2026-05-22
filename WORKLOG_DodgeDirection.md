# 회피/공격 방향 작업 인수인계

작성일: 2026-05-22  
작업 브랜치: `fix/dodge-chain-direction`  
기준 브랜치: `develop`

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

## 현재 동작 요약

### 회피

- `TryStartDodgeAction`에서 현재 이동 입력을 액션 방향으로 변환해 `Dodge` 액션을 시작한다.
- 이동 입력이 없는 경우 액션 방향은 `Any`다.
- 버퍼된 회피는 재생 직전 `BAPlayerCharacter::ResolveBufferedActionDirection`에서 현재 이동 입력을 다시 읽는다.
- `BAPlayerCharacter::ResolveActionAnimationDirection`에서 회피 애니메이션 검색 방향을 결정한다.
  - `Any` -> `Backward`
  - 그 외 방향 -> `Forward`
- `ActionAnimationComponent::OrientOwnerToActionDirection`은 `Any`면 회전하지 않는다.
  - 그래서 무입력 회피는 현재 캐릭터 방향 기준 백스텝이 된다.

### 공격

- 공격 방향은 현재 방식 유지.
- 이동 입력이 있는 상태에서 공격하면 이동 방향 기준으로 공격 방향이 잡힌다.
- 게임패드 기준으로 왼쪽 스틱 이동 방향 + 공격 버튼 조합이 의도된 조작이다.
- 공격 버퍼 방향 정책은 추가 변경하지 않는다.

## 검증 상태

- 변경된 C++ 파일들은 `BAProjectEditor` 빌드에서 컴파일을 통과했다.
- 에디터가 실행 중이면 `UnrealEditor-BAProject.dll`을 잡고 있어서 링크 단계가 실패할 수 있다.
- 마지막 확인 시 `ActionAnimationComponent` 내부에 `DodgeRoll`/`Backstep` 문자열은 남아있지 않았다.

## 남은 작업

### 1. 공격 스태미너 소모

- 공격도 스태미너를 소모해야 하는데 처리가 누락되어 있다.
- 먼저 `ActionData`의 `StaminaCost`, `StaminaCostType`을 그대로 사용할지 검토한다.
- 필요하면 공격별 비용을 `BADesign/Excel/Action.xlsx`와 `BADesign/Json/Action.json`에 추가하고 DataTable을 갱신한다.
- 구현 후 공격 시작 시점 또는 히트/커밋 시점 중 어느 타이밍에 소모할지 결정해야 한다.

### 2. 사망 몽타주

- 사망 몽타주 재생 추가.
- 사망 시 게임 리셋 타이밍 정리.
- UI 재생/리트라이 흐름 연결.
- 코드와 에디터 에셋 변경은 커밋을 분리하는 것이 좋다.

### 3. 낙하, 착지, 낙사

- 낙하 상태 판정.
- 착지 이벤트 처리.
- 낙하 데미지 또는 낙사 조건 추가.
- 착지/낙사 애니메이션과 상태 전환 연결.

### 4. 카메라

- 카메라 에임 조정.
- 공격 시 카메라 셰이크.
- 피격 시 카메라 셰이크.
- 가드/가드 성공/가드 피격 시 카메라 셰이크.

### 5. 게임패드 입력

- 게임패드용 IMC 추가.
- Input Action 추가 또는 기존 Input Action에 게임패드 매핑 추가.
- 키보드/마우스 테스트 입력과 게임패드 기준 조작이 충돌하지 않는지 확인.

## 다음 작업 추천 순서

1. 공격 스태미너 소모
2. 사망 몽타주와 리셋/UI 흐름
3. 낙하, 착지, 낙사
4. 카메라 에임 및 셰이크
5. 게임패드 IMC/Input Action

공격 방향은 현재 상태가 의도에 맞으므로 다음 작업에서 건드리지 않는다.
