# 클라이언트 수정 내역 · 2026-10-15

## 변경 파일

- `client/src/native/main.cpp`: 메인 Windows 프레임 복원에 맞춘 창 클래스, 로고만 표시하는 별도 무테 로딩 창, 정상적인 메시지 루프 종료 처리.
- `client/src/native/app.cpp`: 이동·크기 변경 중 다시 그리기, 중복 창 제어 제거, 버튼 호버·누름·릴리스 반응, 중지 버튼 상태, 읽기 실패 표시.
- `client/src/native/app.hpp`: 다크 프로젝트 이름 입력 창과 빈 이름 검사, 편집 불가능한 생성 옵션 콤보, 배율 변경 폰트 갱신, 이미지 형식 확인 및 수정 시 갱신, 캐시 재요청 처리.
- `client/src/native/graphics.hpp`: 화면 내용 유지 옵션, 이미지 큐의 누락 방지와 실패 결과 전달, 크기·형식 변환 오류 검사, 선택 이미지 우선 처리.
- `client/src/native/editor.hpp`, `prompt_tags.hpp`: 글자 단위로 갈라지지 않는 일관된 색, 쉼표 기준 태그 가중치 편집. Ctrl+위/아래 또는 우클릭 메뉴로 0.1씩 조절하며 초기화와 실행 취소를 지원.
- `client/src/native/common.hpp`: Windows에서 읽을 수 있는 추가 이미지 확장자 허용. 라이브러리는 확장자뿐 아니라 실제 디코더 인식도 확인.
- `assets/splash.png`, `client/tools/prepare_splash.py`: 로고와 중앙 정렬 SNUWCAVNAS 문구만 표시.
- `runtime_tests.hpp`, `client/tests/test_native_client.py`: 태그 경계/가중치, 이미지 100개 처리와 손상 파일 후 복구 회귀 검사.
- `client/tools/verify_exe_icon.py`: 실제 배포 아이콘 원본을 한곳에서 사용.

## 정리

이전 검증 PNG, 이전 UI 로그, 중복 개발 아이콘, 수동 테스트 임시 디렉토리, Python 캐시, 재생성 가능한 리소스 중간 파일을 제거했습니다. 조사 문서는 docs/research로 이동했습니다. 사용자 설정·이미지·백업과 필요한 빌드 도구는 보존했습니다.

## 검사

메인 클라이언트 대상 unittest 5개 통과. 기존 프로젝트는 임시 복사본으로 검사했고 원본 변경 여부도 확인했습니다. 실제 하드웨어 GPU에서 창 없는 렌더링과 픽셀 읽기를 검사했습니다. 로컬 테스트 API로 업로드·2장 생성·편집·Steps 보존을 확인했습니다. 배포 EXE 아이콘과 ZIP 단일 EXE 구성을 정적으로 확인했습니다.

사용자 요청에 따라 프로그램 창을 띄우지 않았습니다. 실제 창 드래그와 새 대화상자의 화면상 동작은 육안 검증하지 않았습니다. 서버와 로그인·가입·모니터링 코드는 변경하지 않았습니다.
