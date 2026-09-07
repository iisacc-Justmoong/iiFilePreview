# iiFileProvider

C++20과 Qt 6.8.3 Core를 사용하는 버전 0.2.0의 동적 라이브러리이다. iisacc.com 계정 모델을 확장한 `FileAuthor`가 파일 작성자의 신원·상세 프로필·기여 정보·작성 디바이스를 기록하고, `AuthenticationToken`이 별도의 런타임 인증 토큰을 보유한다. 파일 읽기·쓰기, 로그인 HTTP 요청과 JWT 서명 검증은 이 객체의 역할에 포함되지 않는다.

공개 저장소는 [iisacc-Justmoong/iiFileProvider](https://github.com/iisacc-Justmoong/iiFileProvider)이다. 2026-09-07에 헤더·네임스페이스·CMake 패키지·공유 라이브러리·설치 경로의 SDK 식별자를 `iiFileProvider`로 통일했다. 소비자는 아래의 새 헤더와 CMake 타깃을 사용하고 기존 빌드 캐시를 다시 구성해야 한다.

## 공개 API

`<iiFileProvider.h>` 하나로 작성자 모델과 인증 토큰을 사용할 수 있다. 공개 헤더는 `FileAuthor.h`, `AuthenticationToken.h`, `Authorship.h`이다. `Authorship`은 작성자별 최초·최근 기여 시각과 변경 번호를 보관하며 변경 직후 JSON 덤프를 갱신한다. 클래스는 QObject가 아닌 C++ 값 객체이며 Qt Network나 계정 매니저의 수명에 의존하지 않는다.

```cpp
#include <iiFileProvider.h>

QString error;
auto author = iiFileProvider::FileAuthor::fromIisaccAccount(
    accountJson, QUrl("https://iisacc.com"), QDateTime::currentDateTimeUtc(), &error);
if (author) {
    auto metadata = author->metadata();
    metadata.details.organization = "Example Studio";
    metadata.attribution.roles = {"creator", "editor"};
    // 파일 작성 시각은 호스트가 알고 있는 실제 시각을 명시한다.
    metadata.attribution.createdAt = actualFileCreationTime;
    if (author->setMetadata(metadata, &error)) {
        const QJsonObject fileMetadata = author->toJson(); // 토큰·로그인 세션 제외
    }
}
```

`fromIisaccAccount()`는 account 객체를, `fromIisaccAppSession()`은 `{account, session, ...}` 앱 응답을 받는다. 후자는 앱 디바이스와 세션 시각도 검사한다. `fromJson()`은 버전이 명시된 파일 작성자 메타데이터를 읽는다. 모두 오류 시 `std::nullopt`와 값이 포함되지 않은 오류를 반환한다.

`AuthenticationToken::create()`에 토큰 종류·계정 subject·서비스 origin·발급/만료 시각·토큰 원문을 명시한다. 만들어진 토큰은 `setAuthenticationToken()`으로 작성자 객체에 연결한다. `secret()`만 원문을 돌려주며, `toJson()`과 디버그 출력에 원문을 넣지 않는다. `isWithinValidityWindow(at)`는 시간 범위 검사이며 인증 성공이나 접근 권한을 증명하지 않는다. iisacc.com 앱의 토큰은 JSON 응답이 아닌 HttpOnly 쿠키에 있으므로 호스트의 인증 계층이 별도로 관리해야 한다.

필드 목록, 관측한 서버 파일, 검증 조건과 예제는 [파일 작성자 계약](docs/FILE_AUTHOR_CONTRACT.md)에 있다.

기존 소비자와의 호환을 위해 아래의 bootstrap API도 유지한다.

```cpp
#include <iiFileProvider.h>

const QString message = iiFileProvider::helloWorld();
```

`[[nodiscard]] QString iiFileProvider::helloWorld()`는 호출할 때마다 `Hello world!`를 반환한다. 공개 헤더와 구현은 소스 루트에 함께 배치한다. 외부 의존성은 기존 Qt 6.8.3 Core이며, 신규 외부 라이브러리를 도입하지 않았다. Qt의 사용 및 배포 조건은 설치된 Qt 라이선스에 따른다.

## 빌드, 테스트, 설치

CMake 3.24 이상, C++20 컴파일러 및 Qt 6.8.3이 필요하다. macOS에서는 `/Volumes/Storage/Qt/6.8.3/macos`가 존재하면 자동으로 탐색 경로에 추가한다.

```sh
./install.sh
```

단독 프로젝트로 구성할 때만 기본 설치 경로를 설정하므로 `add_subdirectory()`로 포함하는 상위 프로젝트의 설치 경로는 유지한다.

스크립트는 `build/`에서 Release 빌드 및 CTest를 실행하고, 기본 경로 `~/.local/SDK/iiFileProvider`에 설치한 뒤 `build/consumer/build/`에서 설치된 CMake 패키지만 사용하는 별도 실행 파일을 빌드하고 테스트한다. bootstrap 테스트는 반환 문자열, C++20 컴파일 설정, Qt 6.8.3 헤더 버전과 런타임 버전을 검사한다. 작성자 계약 테스트는 계정 매핑·상세 메타데이터 왕복·Unicode·잘못된 형식·세션 만료·토큰 바인딩·원문 제외·원자적 갱신을 소스 및 설치 소비자 양쪽에서 검사한다. 테스트 빌드에만 Qt Test를 사용한다.

설치 소비자 구성에는 현재 설치 경로의 패키지 디렉터리를 명시하므로 `INSTALL_PREFIX`를 변경해 재실행해도 이전 패키지 캐시를 사용하지 않는다.

설정은 명령행 인자 대신 환경 변수로 전달한다. `INSTALL_PREFIX`는 절대 경로여야 하며, `CMAKE_PREFIX_PATH`는 세미콜론 또는 콜론으로 구분한 추가 검색 경로를 받는다. 병렬 빌드 개수는 `CMAKE_BUILD_PARALLEL_LEVEL`로 지정하며 기본값은 2이다.

```sh
QT_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos" \
INSTALL_PREFIX="$HOME/.local/SDK/iiFileProvider" \
./install.sh
```

수동 실행 시에도 빌드 디렉터리는 `build/`를 사용한다.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release
cmake -S tests/consumer -B build/consumer/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local/SDK/iiFileProvider;/Volumes/Storage/Qt/6.8.3/macos"
cmake --build build/consumer/build --config Release
ctest --test-dir build/consumer/build -C Release --output-on-failure
```

## 설치 결과와 소비

기본 설치 경로에 `include/`의 umbrella·작성자·인증 토큰·export 헤더, `lib/`의 공유 라이브러리, `lib/cmake/iiFileProvider/`의 CMake 패키지, `share/iiFileProvider/`의 README와 계약 문서가 생성된다. 비공개 `JsonContract.h`는 설치하지 않는다. Windows 공유 라이브러리 실행 파일은 `bin/`에 설치된다. 소비자에게 C++20 및 `Qt6::Core` 링크 요구 사항을 전달한다. Qt를 묶어서 복사하지 않으며 설치된 Qt 런타임이 필요하다. 공유 라이브러리의 설치 RPATH는 링크에 사용한 외부 라이브러리 경로를 포함한다.

```cmake
find_package(iiFileProvider 0.2.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE iiFileProvider::iiFileProvider)
```

`CMAKE_PREFIX_PATH`에 SDK 설치 경로와 Qt 경로를 포함한다. 빌드·테스트·설치까지만 제공하며 커밋, 원격 업로드 또는 배포 단계는 없다.

## License

SPDX-License-Identifier: AGPL-3.0-only

iiFileProvider의 자체 작성 코드와 문서는 GNU Affero General Public License v3.0 전용으로
배포한다. 전체 조건은 [LICENSE](LICENSE)를 따른다.

Qt를 포함한 외부 라이브러리와 별도 고지가 있는 서드파티 코드는 각자의 라이선스를
유지한다. 이 프로젝트의 라이선스 선언은 해당 서드파티 라이선스를 대체하지 않는다.
