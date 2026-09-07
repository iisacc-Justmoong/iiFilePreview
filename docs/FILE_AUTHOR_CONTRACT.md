# 파일 작성자와 인증 토큰 계약

2026-09-07 `/Volumes/Storage/Workspace/Service/iisacc.com`의 현재 작업 트리를 관측하여 작성했다. 웹 서비스에는 별도로 진행 중인 미커밋 계정·세션 변경이 포함되어 있다. 아래는 로컬 구현의 데이터 계약이며 운영 배포나 실제 계정 조회를 확인했다는 의미가 아니다. 서비스 소스나 사용자 레코드는 이 SDK 작업에서 변경하지 않는다.

## 관측한 사용자 모델

| 서비스 파일 | 확인한 필드와 조건 |
| --- | --- |
| `backend/providers/accounts/cognito.js`, `verifiedAccount()` | 서버가 JWT를 검증하고 `email_verified === true`일 때 정규화한 `email`, 안정적인 `sub`를 반환 |
| `src/lib/server/auth/snapshot.js`, `publicAccountSnapshot()` | 공개 account는 `sub`, `email`, `displayName`, `userId`, `societyCloudMembership`, `avatarUrl` 여섯 필드 |
| `backend/app/services/accounts/profile_service.rb`, `present()` | `account_profiles[verified_sub]`의 이름·User ID·아바타·멤버십을 조회. 비밀번호 설정·마케팅 동의·약관 시각은 내부 정보 |
| `backend/app/services/accounts/registration_service.rb` | User ID는 `@[a-z0-9_]{3,30}`이며 등록·동의 시각과 비밀번호 설정 여부를 별도로 보관 |
| `backend/app/services/accounts/society_cloud_membership.rb` | 멤버십 값은 정확히 `Free`, `Plus`, `Pro`, `Enterprise` |
| `backend/app/services/accounts/login_session_service.rb` | 세션 관리 ID, 앱 디바이스, 생성·최근 접속·만료 시각. registry 비밀값은 43자리 base64url, Redis에는 SHA-256만 저장 |
| `backend/app/services/auth/service.rb`, `issue_session`, `resolve_session` | Cognito ID/refresh 쿠키와 registry 쿠키를 함께 사용. registry 만료·해제는 유효한 Cognito 토큰만으로 복구하지 않음 |
| `src/routes/Account/Session/App/+server.js` | 코드 검증·refresh 성공의 JSON은 `{account, session, limits}`이며 인증 원문은 JSON에 없음 |

파일의 작성자 귀속은 멤버십·제품 소유권·라이선스·로그인 허용과 별개이다. `sub`를 email이나 User ID로 대체하지 않는다. 서버가 이메일을 검증한다는 사실만으로 로컬에서 입력받은 JSON에 인증 성공 상태를 부여하지 않는다.

## 객체 구성

`FileAuthor`는 아래의 `AuthorMetadata`와 선택적인 `AuthorLoginSession`, `AuthenticationToken`을 가진다. `metadata()`는 const 참조를 반환하고, 수정은 복사본을 편집한 뒤 `setMetadata()`로 원자적으로 적용한다. 검증 실패 시 이전 데이터와 토큰을 유지한다. `sub`·서비스 origin·디바이스 중 하나가 바뀌면 기존 로그인 세션과 토큰을 해제한다.

| 객체/필드 | 의미와 검증 |
| --- | --- |
| `IisaccAccount.sub` | 필수 `[A-Za-z0-9_-]{1,128}`. 계정의 안정적인 연결키 |
| `email` | 필수. trim·NFKC·소문자 정규화, 전체 254자와 local part 64자 제한, 서버 이메일 형식 |
| `displayName` | 선택적, NFC·trim 후 Unicode code point 80개. 기본 빈 문자열 |
| `userId` | 선택적. 비어 있거나 정확히 `@[a-z0-9_]{3,30}` |
| `societyCloudMembership` | 네 값의 C++ enum. 필드 누락은 서버의 기본값 `Free`, 잘못된 명시 값은 거절 |
| `avatarUrl` | null/누락 또는 `/media/avatars/<64자리 소문자 SHA-256>.webp`. 서비스 origin으로 해석하여 절대 QUrl로 보관 |
| `AuthorDetails.fullName` | 선택적 전체 이름, 160자. 표시 이름에서 추론하지 않음 |
| `givenName`, `additionalName`, `familyName`, `pseudonym` | 각 80자. 호스트가 명시한 이름 구성/필명 |
| `biography` | 4,096자. LF 줄바꿈 허용 |
| `organization`, `department`, `team`, `jobTitle` | 소속·부서·팀·직책, 각 160자 |
| `organizationId` | 호스트가 명시한 소속 식별자, 128자 |
| `contactEmail`, `phoneNumber` | 작성자 연락용 이메일·전화 문자열. 계정 이메일과 독립적이며 연락 동의나 검증 상태를 뜻하지 않음 |
| `locale`, `timeZone` | locale 태그 형식과 Qt가 인식하는 시간대 ID. 빈 값 허용 |
| `countryCode`, `region`, `city` | 대문자 2자 국가 코드 형식·지역·도시. IP나 기기 정보에서 추정하지 않음 |
| `links[]` | 최대 32개 `{relation, label, url}`. 종류 40자·표시명 160자·HTTPS URL 2,048자 |
| `identifiers[]` | 최대 32개 `{scheme, value}`. 종류 40자·값 256자. ORCID·ISNI 등 외부 ID를 명시하며 진위는 확인하지 않음 |
| `FileAttribution.roles[]` | 최대 16개, 각 40자, 중복 없는 역할 문자열. 기본 빈 배열이며 creator/editor/translator 등 실제 기여를 명시 |
| `credit`, `copyrightNotice` | 크레딧과 저작권 표시, 각 1,024자 |
| `licenseIdentifier`, `licenseUrl` | 라이선스 명칭 128자·HTTPS URL. 사용 권한이나 소유권을 검증하는 증서가 아님 |
| `documentId`, `projectId`, `workspaceId` | 호스트가 명시하는 파일·프로젝트·작업 공간 식별자, 각 128자 |
| `createdAt`, `modifiedAt` | 실제 파일 작성/수정에 기여한 시각. 선택적이며 둘 다 있으면 수정 시각이 작성 시각 이후여야 함 |
| `serviceOrigin`, `capturedAt` | 명시적인 HTTPS 서비스 origin과 해당 account snapshot을 확보한 시각. 필수 |
| `AuthorDevice` | 아래의 앱 보고 작성 환경. 선택적이며 하드웨어 인증을 뜻하지 않음 |

이름·연락처·소속·기여 정보의 선택적 필드는 계정에 실제 존재한다고 주장하지 않는다. 기본은 빈 값이며 호스트가 확보한 정보만 추가한다. 실명 검증, 이메일 인증 boolean, 관리자 여부, 자동 생성한 파일 작성 시각은 넣지 않는다. 표시 라벨은 displayName → User ID → email 순서이다.

앱 디바이스는 서버와 같은 `id`(lowercase SHA-256 64자리), `type`(`pc` 또는 `tablet`), `name`(80자), `platform`(40자), `osVersion`(80자), `appId`(128자), `appVersion`(40자)이다. 모두 필수이며 OS만으로 form factor를 추론하지 않는다. 디바이스 ID는 호스트가 이미 해시한 식별자만 받으며 실제 머신 ID를 자동 조회하지 않는다.

## 명시적 입력 어댑터

- `fromIisaccAccount(account, serviceOrigin, capturedAt)`는 공개 account 객체를 읽는다. `sub`·email이 없는 부분 프로필은 거절하고, 내부 필드와 알 수 없는 추가 필드는 복사하지 않는다. 오래된 account에서 누락된 선택 필드는 빈 값/Free/null로 구성한다.
- `fromIisaccAppSession(response, serviceOrigin, capturedAt)`는 account에 더해 `session.id`, `client: app`, `current: true`, 모든 device 필드와 세 시각을 요구한다. `createdAt <= lastSeenAt <= capturedAt < expiresAt`를 검사하고 디바이스를 작성 환경으로 복사한다. 세션 생성 시각을 파일 작성 시각으로 사용하지 않는다.
- `fromJson(metadata)`는 아래의 schemaVersion 1 파일 메타데이터만 읽는다. 이때 토큰과 로그인 세션은 항상 비어 있다.

모든 입력에서 필드의 JSON 타입을 먼저 검사한다. 숫자를 문자열로 바꾸거나 null을 빈 이름으로 바꾸지 않는다. 문자열은 NFC·trim 후 code point 개수를 제한하며 제어 문자·줄/문단 구분자를 거절한다. biography의 LF만 허용한다. 시간 문자열에는 `Z` 또는 `±HH:MM`이 필요하며 UTC ISO 8601 밀리초로 저장한다. account 입력은 32 KiB, 앱 응답은 64 KiB, 파일 메타데이터는 128 KiB 이하이다. 크기는 compact JSON의 UTF-8 바이트 기준이다.

서비스 origin에는 사용자정보·경로·query·fragment를 허용하지 않으며 마지막 `/`와 기본 HTTPS 포트를 정규화한다. 계정 아바타는 정확한 서버 경로만 허용한다. 상세 프로필의 외부 링크·라이선스 URL에는 HTTPS만 허용한다. 이 SDK는 URL에 접속하거나 이미지를 다운로드하지 않는다.

## 저장 형식과 토큰 경계

```json
{
  "schemaVersion": 1,
  "serviceOrigin": "https://iisacc.com",
  "capturedAt": "2026-09-07T10:00:00.000Z",
  "account": {
    "sub": "example-subject",
    "email": "author@example.com",
    "displayName": "Example Author",
    "userId": "@example_author",
    "societyCloudMembership": "Free",
    "avatarUrl": null
  },
  "details": { "organization": "Example Studio" },
  "attribution": { "roles": ["creator"], "createdAt": null, "modifiedAt": null },
  "device": null
}
```

읽기에서 누락된 details/attribution의 선택 필드는 기본값으로 채우고, 쓰기에서는 정의된 모든 필드를 명시한다. schemaVersion 누락·문자열·소수·다른 버전과 알 수 없는 저장 필드는 거절한다. 공용 API 응답 어댑터의 추가 필드 무시와 파일 저장 스키마의 엄격한 검사는 의도적으로 다르다.

`AuthenticationTokenInfo`는 종류(`IisaccSession`, `CognitoId`, `CognitoAccess`, `CognitoRefresh`), `subject`, `serviceOrigin`, 선택적 issuer·audience·scopes, 세션 관리 ID와 issuedAt/notBefore/expiresAt을 담는다. issuedAt·expiresAt은 필수이고 종료 시각은 시작 시각보다 커야 한다. notBefore가 있으면 발급 시각 이상·만료 시각 미만이다. 범위 검사는 만료 시각 자체를 제외한다.

원문은 별도의 private QByteArray에 보유한다. 1~16,384바이트의 공백 없는 출력 가능한 ASCII를 받고 registry 종류는 정확히 43자리 base64url과 32자리 hex 세션 관리 ID를 요구한다. 토큰 내부 JWT claim을 해독해 subject·issuer·audience·시각을 추론하지 않는다. 호스트 인증 계층이 검증한 메타데이터를 명시적으로 전달해야 한다. 서명 검증·갱신·취소·서버 접근 허용은 호스트 인증 계층의 책임이다.

```cpp
iiFileProvider::AuthenticationTokenInfo info;
info.kind = iiFileProvider::AuthenticationTokenKind::IisaccSession;
info.subject = author->metadata().account.sub;
info.serviceOrigin = author->metadata().serviceOrigin;
info.sessionId = author->loginSession()->id;
info.issuedAt = author->loginSession()->createdAt;
info.expiresAt = author->loginSession()->expiresAt;
auto token = iiFileProvider::AuthenticationToken::create(info, registryCookieValue, &error);
if (token && author->setAuthenticationToken(*token, &error)) {
    // 원문이 필요한 인증 전송 계층에서만 명시적으로 secret()에 접근한다.
}
```

이 예제는 유효한 앱 응답으로 만든 author와 호스트가 관리하는 쿠키 값이 있다는 전제이다. 서비스 앱 API는 이 쿠키를 Bearer 헤더로 받는 계약이 아니며 SDK가 임의로 요청 헤더를 생성하지 않는다. 공개 세션 ID는 로그인 비밀값을 대신할 수 없다. 멤버십은 인증 토큰의 scope로 자동 변환하지 않는다.

`FileAuthor::toJson()`에는 토큰 종류·원문·로그인 세션 관리 ID 자체가 들어가지 않는다. `AuthenticationToken::toRedactedJson()`은 별도 진단용 메타데이터와 `[REDACTED]`만 반환하고 QDebug도 원문을 출력하지 않는다. `secret()` 호출 및 값 객체의 명시적 복사는 호스트가 관리해야 한다. clear/destruction이 모든 Qt implicit-sharing 복사본의 메모리를 암호학적으로 소거한다고 보장하지 않는다.

파일 메타데이터에는 호스트가 설정한 이메일·연락처·디바이스가 포함된다. 파일 공개 범위에 맞는 작성자 정보 선택은 호스트가 한다. 비밀번호·OTP·refresh/ID/registry 토큰·마케팅 동의·약관 기록·token hash를 서버 account에서 자동 복사하지 않는다.

## 의존성과 검증

Qt Core의 JSON, Unicode, QUrl, QDateTime, QTimeZone을 재사용한다. 기존 Qt 6.8.3 외의 런타임 의존성은 추가하지 않는다. 이미 있는 iiAcountManager는 로그인·Qt Network·QObject 수명을 소유하고 현재 Account는 네 표시 필드만 노출하므로, 파일 작성자 값 객체가 그 매니저를 소유하거나 상위 앱에 의존하도록 만들지 않았다. 두 SDK는 명시적인 서버 JSON 계약으로 연결할 수 있다. 형식·문자열·시각의 API 동작은 [QJsonValue](https://doc.qt.io/qt-6.8/qjsonvalue.html), [QString](https://doc.qt.io/qt-6.8/qstring.html), [QDateTime](https://doc.qt.io/qt-6.8/qdatetime.html)의 공식 문서를 참고했다.

`tests/author_contract.cpp`는 실제 서버 필드 형태를 본뜬 합성 fixture로 account/앱 응답, rich metadata 왕복, 이전 모델 호환, 잘못된 필드, Unicode code point 경계, 내부 정보 제외, 세션 만료, 토큰 바인딩·시간 범위·redaction과 실패의 원자성을 검증한다. 같은 계약을 별도 설치 소비자에서 다시 컴파일·실행하여 공개 헤더와 공유 라이브러리 export를 확인한다. 실제 계정·운영 토큰·과금 요청은 사용하지 않는다.

2026-09-07 macOS arm64 / AppleClang 21 / Qt 6.8.3의 Release 빌드와 설치를 완료했다. 소스 CTest 2/2, 별도 설치 소비자 CTest 2/2, 기존 Society의 재빌드 및 CTest 2/2가 통과했다. 작성자 Qt Test 출력은 소스·설치본 각각 39 passed(초기화/정리 포함)이다. 설치된 공개 헤더·문서의 소스 일치, 공개 심볼 export, 런타임의 Qt Test/Network 의존성 부재를 확인했다. 상세 실행 로그는 로컬 `build/author-install.log`, `build/society-regression.log`, `build/author-artifact-audit.json`에 있다.


## 0.2.0 공통 파일 기여 기록

`Authorship`은 iiCSMIDI·iiGeneralDocument·iiSharedCanvas의 파일 메타데이터에 공통으로 쓰는 값 객체이다. `setAuthor()`는 host가 제공한 FileAuthor의 공개 JSON만 복사하고 변경된 프로필을 즉시 기록한다. `recordChange()`는 성공한 실제 변경마다 revision과 기여 시각을 갱신하고 즉시 compact JSON을 생성한다. `dump()`는 미리 갱신된 바이트를 반환하며 타이머·save·소멸자를 기다리지 않는다. 같은 프로필 선택은 revision을 바꾸지 않으며 활성 편집자 문맥만 설정한다.

저장 키는 `iisacc:authorship`, 스키마는 `{schemaVersion:1, revision:"unsigned decimal", modifiedAt, authors:[{key,author,firstChangedAt,lastChangedAt}], lastAuthor}`이다. key는 서비스 origin과 sub의 SHA-256이며 author는 FileAuthor JSON이다. 최대 256명·1 MiB로 제한하고 알 수 없는 필드·중복 ID·잘못된 참조나 시각을 거절한다. 시계가 뒤로 가도 최신 수정 시각은 감소하지 않는다. 오류는 상태를 변경하지 않는다.

토큰·로그인 세션·활성 편집자 문맥은 덤프에 없다. 파일 읽기는 active author를 비우므로 이전 작성자의 신원을 새 편집에 빌려 쓰지 않는다. host가 작성자를 지정하지 않은 변경은 lastAuthor:null로 기록한다. 첫 기여 시각은 파일의 원래 생성 시각으로 추론하지 않는다. 여러 작업을 하나의 파일 트랜잭션으로 묶을 때 실패·no-op 여부를 판정한 뒤 recordChange를 호출하며, 저장에 실패하면 문서와 이 값 객체를 함께 롤백해야 한다.

`tests/authorship.cpp`와 설치 소비자는 즉시 덤프·기여자 선택·JSON 왕복·토큰 제외·타입 오류·시계 역행·실패 원자성을 검증한다.
