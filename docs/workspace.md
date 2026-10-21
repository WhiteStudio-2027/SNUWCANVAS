# 모노레포 작업 공간

- client/: Windows C++ 클라이언트 코드·에셋·빌드 도구·테스트. client/.build는 재생성 가능한 결과입니다.
- server/: Ubuntu 서버 코드·관리 CLI·업데이트 도구·테스트·문서.
- tools/: 공통 배포와 전체 검사 도구, 개발 의존성.
- docs/: 공통 안내와 이전 조사 기록.
- SNUWCANVAS.exe: 작업 공간 루트의 유일한 클라이언트 배포 파일. 빌드와 테스트 모두 이 파일을 사용합니다.

루트 SNUWCANVAS.exe는 현재 PC의 설치 실행 파일입니다. data/, config/, prompts/, generated/, archive/는 사용자 자료 또는 백업으로, 기존 절대 참조 경로를 보존하기 위해 이동하지 않았습니다.

자세한 실행·업데이트 방법은 [저장소 안내](../README.md)를 참고하세요. 배포물에는 실제 설정·API 키·사용자 이미지·기록 DB를 넣지 않습니다.
