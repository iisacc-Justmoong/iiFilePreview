# 이름과 URL 파일 메타데이터

iiFileProvider 0.4.0은 `FileLink`로 `[이름|URL]` 쌍을 표현하고 `Authorship`의 파일 메타데이터에 선택적인 `links` 목록으로 저장한다. 파일 자체의 주소·출처·참고 자료를 위한 필드이며 작성자의 계정 프로필 링크와는 독립적이다.

## 입력과 저장

```cpp
#include <iiFileProvider.h>

QString error;
auto society = iiFileProvider::FileLink::fromString(
    "[Society 원본|society://container/Files/artwork.iisc]", &error);
auto reference = iiFileProvider::FileLink::create(
    "로컬 자료", QUrl::fromLocalFile("/Volumes/Storage/Workspace/Assets/그림 파일.png"), &error);

iiFileProvider::Authorship history;
if (society && reference) {
    // author는 호스트가 제공한 FileAuthor이다. 작성자와 링크를 한 번에 기록한다.
    history.setAuthor(author, {*society, *reference}, actualEditTime);
}

// 작성자 등록과 별도로 [이름|URL] 목록을 전달할 수도 있다.
history.setLinksFromStrings({
    "[Society 원본|society://container/Files/artwork.iisc]",
    "[네트워크 자료|smb://nas.local/shared/reference.png]",
    "[선택적 참고|../reference/project.json#asset-1]"
}, actualEditTime);

const QList<iiFileProvider::FileLink> links = history.links();
const QByteArray metadata = history.dump(); // 호스트의 파일 저장 경로에 전달한다.
auto restored = iiFileProvider::Authorship::fromDump(metadata, &error);
```

`FileLink::create(name, QString)`는 이름과 URL 원문을, `create(name, QUrl)`는 이름과 이미 해석한 QUrl을, `fromString("[이름|URL]")`은 문자열 표현을 받는다. 오류 시 `std::nullopt`와 입력 원문이 포함되지 않는 오류를 반환한다. `name()`·`urlText()`로 저장 값을 조회하며, `url()`은 QUrl로 해석한 편의용 복사본이다. `toString()`으로 문자열 표현, `toJson()`으로 `{name, url}` 객체를 얻는다. 파일에서는 구분자를 다시 해석할 필요가 없도록 아래의 구조화된 배열을 사용한다.

```json
"links": [
  {"name": "Society 원본", "url": "society://container/Files/artwork.iisc"},
  {"name": "네트워크 자료", "url": "smb://nas.local/shared/reference.png"}
]
```

## URL 범위

스킴 허용 목록이나 HTTP(S)·호스트 필수 조건을 두지 않는다. `society:` 같은 사용자 정의 스킴, `file:`, `smb:`, `ftp:`, `sftp:`, `ipfs:`, `urn:`, `mailto:`, `tel:`, `data:`, `content:`, `qrc:` 및 상대 URL·프래그먼트 참조를 받는다. Society의 실제 주소는 호스트가 넘긴 값을 사용하며 예제 스킴의 등록이나 Society 서버 연결을 이 SDK가 수행하는 것은 아니다.

URL 문법 검사에는 기존 Qt 6.8.3 Core의 [QUrl](https://doc.qt.io/qt-6.8/qurl.html)의 `StrictMode`를 재사용하며, 검사 결과로 원문을 덮어쓰지 않는다. 문자열과 JSON으로 받은 URL은 대소문자·percent encoding·Unicode·query/fragment/userinfo를 그대로 저장한다. 예를 들어 IPFS의 대소문자 구분 식별자, Society의 사용자 정의 authority, 서명 URL의 인코딩을 QUrl의 정규화된 출력으로 바꾸지 않는다. 스킴 추정, HTTPS 변환, 상대 주소 해석, 경로 정규화, URL용 Unicode NFC 변환도 하지 않는다. 상대 주소의 기준은 호스트가 결정한다.

`create(name, QUrl)`는 호출자가 이미 해석한 QUrl의 `FullyEncoded` 값을 받으므로 그 이전 원문은 복구할 수 없다. 정확한 원문 유지가 필요한 주소는 QString 또는 `[이름|URL]` 입력을 사용한다. 저장된 원문은 `urlText()`로 읽고, 정규화된 QUrl이 필요한 경우에만 `url()`을 사용한다.

공백이 있는 로컬 경로는 `QUrl::fromLocalFile()`로 전달하거나 문자열 URL에서 `%20`으로 인코딩한다. `[이름|URL]`은 첫 `|`를 구분자로, 맨 바깥의 `[`·`]`를 묶음으로 사용하므로 IPv6 주소의 대괄호는 손상하지 않는다. 이름의 `|`는 허용하지 않으며 URL의 해당 문자는 `%7C`로 표현한다. 이름은 NFC·trim 후 Unicode code point 최대 160개이며 비어 있을 수 없다. URL은 비어 있지 않은 유효한 QUrl이어야 하며 원문 UTF-8과 fully encoded 표현이 각각 최대 16 KiB이다. 잘못된 percent encoding·UTF-16·원문 제어 문자는 거절한다.

이 SDK는 입력 URL을 열거나 네트워크·파일 시스템에 접근하지 않는다. 런타임 인증 토큰과 로그인 세션을 자동 복사하지 않지만, 호스트가 명시적으로 전달한 URL의 query/userinfo 등은 그대로 파일에 저장한다. 작성자 계정 프로필의 HTTPS 검증 규칙은 기존 계약을 유지한다.

## 갱신과 영구 편집자 명단

`setAuthor(author, links, at)`는 작성자 선택/프로필과 파일 링크를 한 번의 원자적 변경으로 기록하고 revision을 한 번만 증가시킨다. 같은 프로필과 같은 순서의 링크는 no-op이며 활성 작성자만 선택한다. 기존 `setAuthor(author, at)` 호출은 저장된 링크를 유지한다. 추가 인자를 생략하는 것과 명시적으로 빈 목록을 전달하는 것은 다르다.

`setLinks(links, at)`와 `setLinksFromStrings(entries, at)`는 링크 목록 전체를 교체한다. 빈 목록은 선택적인 링크만 비우며 최초 편집자·참여자는 삭제하지 않는다. 조회 목록은 복사본이며 원본을 변경할 수 없다. 이름이 같아도 URL 원문이 다르면 별도 항목을 허용하고, 정규화한 이름과 URL 원문이 모두 같은 중복 쌍은 거절한다. 입력 순서를 유지한다.

최대 64개 링크와 전체 compact JSON 2 MiB 한도를 적용한다. 잘못된 항목·중복·잘못된 기록 시각은 `std::invalid_argument`, 인원/용량 한도 초과는 `std::length_error`, revision 소진은 `std::overflow_error`로 거절한다. 전체 목록을 검증한 뒤 적용하므로 실패 시 기존 링크·편집자 명단·활성 편집자·revision·덤프를 유지한다. 파일 I/O와 성공한 덤프의 실제 저장은 호스트의 책임이다.

## 버전과 소비자

현재 Authorship 저장 스키마는 3이며 `links` 배열이 필수이다. 링크가 없는 파일은 빈 배열을 쓴다. 스키마 1·2는 기존 필드와 기존 크기 한도를 엄격하게 검사한 뒤 명단과 revision을 그대로 유지하고 빈 링크 목록을 추가하여 읽는다. 이후 출력은 스키마 3이다. 기존 한도까지 채운 스키마 1·2 파일도 전체 작성자를 보존한다.

파일 수준 링크 저장 멤버가 추가되어 `Authorship` 값의 메모리 배치가 달라졌다. 공유 라이브러리 ABI 식별자는 `0`에서 `0.4`로 변경했으며, 소비자와 Authorship을 포함하는 SDK는 iiFileProvider 0.4.0 헤더·라이브러리로 다시 빌드해야 한다. 이전 ABI 라이브러리 파일을 새 바이너리로 덮어써 교체하지 않는다. CMake의 공개 타깃은 `iiFileProvider::iiFileProvider`를 유지한다.

`tests/file_links.cpp`는 스킴·상대 주소·인코딩·문자열/JSON/파일 왕복, 잘못된 입력, 원자적 작성자+링크 기록, 생략과 비우기 구분, 편집자 명단 보존, 목록 한도와 실패 원자성을 검증한다. 같은 테스트를 설치된 헤더와 공유 라이브러리만 사용하는 별도 소비자에서도 빌드·실행한다. 기존 Authorship 테스트는 스키마 1·2 최대 크기의 무손실 변환과 전체 크기 한도에서의 명단 보존을 함께 검사한다.

2026-09-08 macOS arm64 / Qt 6.8.3에서 0.4.0 Release 빌드와 소스 CTest 4/4, `build/stage/` 설치본을 사용하는 별도 소비자 CTest 4/4이 통과했다. 양쪽의 FileLinks Qt Test는 각각 57 passed, Authorship은 40 passed(초기화/정리 포함)이다. IPFS/Society 식별자의 대소문자와 서명 URL 인코딩 원문을 보존하는 회귀 테스트도 포함한다. 설치 소비자는 실제 `libiiFileProvider.0.4.dylib`에 링크하며 새 API의 export를 확인했다. 설치 로그는 `build/file-links-install.log`, 파일·링크 경로 검증은 `build/file-links-artifact-audit.json`에 기록한다. 이번 검증은 iiFileProvider와 별도 설치 소비자 범위이며 다른 SDK·제품의 재빌드나 배포를 포함하지 않는다.
