#include <iiFileProvider.h>
#include <QtCore/QJsonDocument>
#include <QtTest/QTest>
#include <stdexcept>

using namespace iiFileProvider;

class AuthorshipTest : public QObject {
    Q_OBJECT
private slots:
    void immediateDumpAndRoundTrip()
    {
        const auto at = QDateTime::fromString("2026-09-07T12:00:00Z", Qt::ISODate);
        auto author = FileAuthor::fromIisaccAccount({{"sub", "writer-1"}, {"email", "writer@example.com"}},
            QUrl("https://iisacc.com"), at);
        QVERIFY(author);
        Authorship history;
        QVERIFY(history.setAuthor(*author, at));
        const auto first = history.dump();
        QCOMPARE(history.revision(), quint64(1));
        QVERIFY(!history.setAuthor(*author, at));
        QCOMPARE(history.dump(), first);
        history.recordChange(at.addSecs(1));
        QCOMPARE(history.revision(), quint64(2));
        QVERIFY(history.dump() != first);
        auto loaded = Authorship::fromJson(history.toJson());
        QVERIFY(loaded);
        QCOMPARE(loaded->dump(), history.dump());
        QVERIFY(!loaded->hasActiveAuthor());
        loaded->recordChange(at.addSecs(2));
        QVERIFY(loaded->toJson()["lastAuthor"].isNull());
    }
    void credentialsNeverEnterDump()
    {
        auto at = QDateTime::currentDateTimeUtc();
        auto author = FileAuthor::fromIisaccAccount({{"sub", "writer"}, {"email", "writer@example.com"}},
            QUrl("https://iisacc.com"), at);
        QVERIFY(author);
        AuthenticationTokenInfo info;
        info.subject = "writer"; info.serviceOrigin = QUrl("https://iisacc.com");
        info.sessionId = QString(32, 'a'); info.issuedAt = at; info.expiresAt = at.addDays(1);
        auto token = AuthenticationToken::create(info, QByteArray(43, 's'));
        QVERIFY(token); QVERIFY(author->setAuthenticationToken(*token));
        Authorship history;
        QVERIFY(history.setAuthor(*author, at));
        QVERIFY(!history.dump().contains(QByteArray(43, 's')));
        QVERIFY(!history.dump().contains("token"));
    }
    void invalidReadAndClockRollback()
    {
        Authorship history;
        const auto at = QDateTime::currentDateTimeUtc();
        history.recordChange(at);
        history.recordChange(at.addSecs(-100));
        QCOMPARE(history.toJson()["modifiedAt"].toString(), at.toUTC().toString(Qt::ISODateWithMs));
        auto json = history.toJson(); json["revision"] = 1.5;
        QVERIFY(!Authorship::fromJson(json));
        json = history.toJson(); json["token"] = "forbidden";
        QVERIFY(!Authorship::fromJson(json));
        const auto before = history.dump();
        QVERIFY_EXCEPTION_THROWN(history.recordChange(QDateTime()), std::invalid_argument);
        QCOMPARE(history.dump(), before);
    }
};
QTEST_GUILESS_MAIN(AuthorshipTest)
#include "authorship.moc"
