# SNUWCANVAS

Windows 네이티브 C++20 클라이언트입니다. Python/Tkinter/PyInstaller를 실행하지 않습니다.
이미지 캔버스와 패널은 Direct2D, 글자는 DirectWrite, 통신은 WinHTTP를 사용합니다.
텍스트 편집·파일 선택은 Windows 기본 컨트롤을 사용합니다.

`dist/SNUWCANVAS.exe`를 실행하세요. 배포 ZIP에는 EXE 하나만 포함합니다.
서버와 ComfyUI의 Python 환경 및 API는 기존대로 유지됩니다.

## 사용

- 프로젝트는 왼쪽에서 선택하고, 새로 만들기·저장·관리는 같은 사이드바에서 합니다.
- 프로젝트를 드래그하면 순서가 바뀝니다. 폴더 헤더에 놓으면 폴더로 이동합니다. 왼쪽 상단 ▱+ 또는 우클릭 → 새 폴더로 정리하며, 폴더 클릭은 접기/펼치기입니다.
- 폴더 우클릭으로 이름 변경·폴더 해제가 가능하며, 프로젝트 우클릭 → 폴더로 이동도 지원합니다. 목록 끝의 폴더 밖으로 이동 영역에 놓으면 루트로 이동합니다.
- 생성 모델·스타일·품질은 중앙 하단에서 선택합니다. 생성 중 다시 누르면 예약합니다.
- 이미지 라이브러리는 휠, 빈 영역/썸네일 위 드래그, 오른쪽 스크롤 손잡이로 이동합니다. 패널 폭을 늘리면 이미지 열 수와 목록 높이가 함께 늘어납니다. 최근 썸네일은 256개까지 캐시합니다.
- 썸네일을 클릭하면 미리보기, 더블클릭 또는 자세히 보기는 Windows 기본 이미지 앱입니다.
- 프롬프트의 쉼표 단위 태그에 커서를 놓고 Ctrl+↑/↓로 가중치를 0.1씩 조절합니다. 우클릭 메뉴에서 조절·초기화할 수 있습니다.
- 메인 창은 Windows 기본 상단 바를 사용합니다.
- 시작 로딩 창은 고정 크기·타이틀바 없는 별도 창이며 실제 초기화가 끝나면 닫힙니다.
- Ctrl+S 저장, Ctrl+N 새 프로젝트, Ctrl+O 프로젝트 파일 열기.

## 데이터

EXE 옆 `data/config`, `data/prompts`, `data/generated`를 사용합니다.
기존 JSON 형식을 읽고 저장하며, 별도 출력 폴더와 참조 이미지 경로는 유지합니다.
기존 `config`와 `prompts`만 있고 `data/config`가 없는 경우 최초 실행에 복사하여 가져옵니다.
프로젝트 삭제는 이미지 파일을 삭제하지 않습니다.

## 소스와 빌드

클라이언트 소스: `src/native/` (이전 Python 클라이언트 소스는 제거했습니다.)
서버는 별도 저장소로 관리합니다. 클라이언트 빌드는 서버 폴더에 의존하지 않습니다.

빌드 도구: MSYS2 UCRT64 MinGW-w64 g++ / windres.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_native.ps1 -Publish
```

창을 띄우지 않는 정적 배포 검증: `python tools/verify_exe_icon.py`
로컬 API 호환성 검사: `python tools/test_native_api.py` (테스트 서버만 사용, GUI 없음).
네이티브 `--self-test`, `--graphics-test`, `--migration-test`, `--api-test`는 창을 만들지 않습니다.

[네이티브 전환 안내](docs/native-client.md) · [모델과 보정](docs/model_workflows.md) · [편집](docs/editing.md)

폴더 구성과 보존 기준: [작업 공간 안내](docs/workspace.md).

배포 결과는 저장소 루트 releases/에 고정 이름으로 저장하며 업데이트 시 기존 EXE를 덮어씁니다. 앱 실행 데이터 경로는 변경하지 않습니다.

## 독립 저장소와 검사

이 폴더 자체를 Git 저장소 루트로 사용합니다. `assets`, `src`, `tools`, `tests`, `docs`를 함께 올립니다.
실제 설정·키·프로젝트·생성 이미지·빌드 결과는 .gitignore로 제외합니다.
`python -m pip install -r requirements-dev.txt` 후 빌드하고 `python -m unittest discover -s tests -v`로 검사합니다.
`-Publish`는 dist 안의 고정 이름 EXE/ZIP을 덮어씁니다. 기존 설치에 적용하려면 `-InstallDir "기존 설치 폴더"`를 함께 지정합니다.
각 생성 작업은 기본 2시간의 전체 제한 시간이 있으며 설정 JSON의 `job_timeout_seconds`로 조절할 수 있습니다.
중지 시 로컬 대기열과 통신을 취소하고 서버 취소 확인 결과를 표시합니다. 서버 연결이 끊기면 원격 중지는 확인할 수 없습니다.

## 상태 알림과 다운로드 변경

클라이언트는 /jobs/{id}/events 연결 하나로 상태 변경 알림을 받으며 상태 GET을 반복하지 않습니다.
서버와 클라이언트를 함께 업데이트해야 합니다. 연결이 끊기면 오류를 표시하고 자동 반복 조회는 하지 않습니다.
서버는 출력 경로가 ComfyUI와 다르면 소유권을 확인한 작업 결과를 ComfyUI /view에서 가져옵니다.
