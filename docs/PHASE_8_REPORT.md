# Phase 8 구현 보고 — 모델 설정 가드·GPU 실패 처리

## 변경 요약·주요 파일
model_guards.json을 서버와 네이티브 빌드가 공유한다. SDXL/Illustrious/FLUX 증류형·Base의 해상도 정렬/크기/설정 범위/권장값/필수값/지원하지 않는 로더를 정의했다. 모델 변경 시 기본값을 적용하고 필수 항목을 비활성화해 이유 툴팁을 표시한다. 서버에서도 강제 적용하므로 클라이언트 수정으로 우회할 수 없다. 원시 node_overrides는 모델 정책 우회 방지를 위해 거부한다.
backend_errors.py는 backend 전체 payload 대신 안전한 오류 안내를 반환한다. backend_execution.py는 선택적 1회 OOM 재시도와 취소를 처리한다. 기존 단일 GPU scheduler lock 안에서 실행되며 모델을 내리지 않는다.
developer_preferences와 권한 ContextVar를 추가하고 개발자 화면에 자신의 1·2차 검사 토글을 연결했다. 일반 safe/admin은 사전, strict는 사전+사후, free는 없음으로 서버에서 고정한다. 학습 실행은 기존 서버 develop 권한 강제를 유지하고 일반 계정의 학습 시작/중지·자동 학습 UI를 숨겼다.

## DB·env
accounts DB에 developer_preferences(account/pre/post) 테이블을 추가한다. 기본 두 값은 0이고 해당 개발자 계정의 새 요청에만 적용된다. 다른 계정 정책은 바뀌지 않으며 이전에 큐에 들어간 작업은 캡처한 설정을 따른다. 기존 DB 값은 덮어쓰지 않는다.
GENERATION_OOM_RETRY=0을 예제에 추가했다. 1이면 확인된 GPU OOM에만 작은 VAE 타일로 한 번 재시도한다. seed·출력 크기를 조용히 바꾸지 않는다. 실제 env·설치 EXE·운영 모델은 유지했다.

## 검증
전체 회귀 196개 통과(30.207초). 보정 단계 잠금을 보완한 최종 빌드와 관련 집중 검사 17개도 통과(4.519초). 모델 필수값·정렬·범위·SDXL/FLUX 로더 혼합·원시 덮어쓰기 차단, 실제 숨겨진 네이티브 컨트롤 비활성화, 개발자 검사 계정 격리/등급 강제, OOM 단일 재시도·중간 취소·원본 graph/seed 보존·오류 payload 비노출을 확인했다. 실제 ComfyUI/Ollama/학습/GPU 생성은 실행하지 않았다.

## 리소스·설계 근거
Windows 로컬 Python에서 설정 가드 10,000회 0.0603초, tracemalloc peak 7,896 bytes였다. GPU 수치가 아니다. 모델 상주·기존 단일 GPU 작업·FLUX FP8 파일 기본값과 타일 VAE를 유지했다. ComfyUI가 하드웨어/파일에 맞춰 연산 정밀도·offload를 정하며, 이 구현이 VRAM 부족을 RAM으로 무조건 해결한다고 주장하지 않는다.
[Comfy 공식 안내](https://docs.comfy.org/tutorials/flux/flux-2-klein)의 증류형 4-step과 Base 구분을 적용했다. [BFL Base 모델 카드](https://huggingface.co/black-forest-labs/FLUX.2-klein-base-4B)는 Base가 step/guidance 증류 모델이 아님을 명시한다. 기본 20 steps/CFG 5는 현재 서비스의 효율 기본값이며 모든 이미지에 대한 최고 품질 보장은 아니다.

## 추가 기능·한계
재시도는 실패가 확인된 backend 작업에만 적용한다. 성공 이미지의 중복 생성이나 자동 해상도 축소는 하지 않는다. 실패 이미지는 기존 quota 정산에서 해제된다. 사용자에게 backend prompt/traceback을 반사하지 않는다.
실제 RTX 3060 12GB/32GB의 VRAM/RAM·지연 측정과 모델별 이미지 품질 QA는 미수행이다. 자동 체크포인트 아키텍처 추론 및 추가 정밀도 선택 UI는 미구현이며 지원 프로필과 Comfy 기본 로더를 사용한다. 실제 장비 검증 전 리소스 안정화 완료로 간주하지 않는다. 다음은 Phase 9 관리 CLI 강화다.

법률 검토를 받은 문서가 아니므로 서비스 공개 전 전문가 검토 필요.
