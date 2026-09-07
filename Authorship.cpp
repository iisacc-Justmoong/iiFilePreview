#include "Authorship.h"
#include "JsonContract.h"
#include <QtCore/QCryptographicHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QSet>
#include <limits>
#include <stdexcept>

namespace iiFileProvider {
namespace {
QString keyFor(const FileAuthor& author)
{
    const auto& metadata = author.metadata();
    return QString::fromLatin1(QCryptographicHash::hash(
        metadata.serviceOrigin.toEncoded() + '\n' + metadata.account.sub.toUtf8(),
        QCryptographicHash::Sha256).toHex());
}
}
Authorship::Authorship() { rebuildDump(); }
quint64 Authorship::revision() const noexcept { return m_revision; }
bool Authorship::isEmpty() const noexcept { return m_revision == 0 && m_authors.isEmpty(); }
void Authorship::clearActiveAuthor() noexcept { m_activeAuthor.clear(); }
bool Authorship::hasActiveAuthor() const noexcept { return !m_activeAuthor.isEmpty(); }
QByteArray Authorship::dump() const { return m_dump; }
QJsonObject Authorship::toJson() const
{
    return {{"schemaVersion", 1}, {"revision", QString::number(m_revision)},
        {"modifiedAt", detail::dateJson(m_modifiedAt)}, {"authors", m_authors},
        {"lastAuthor", m_lastAuthor.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(m_lastAuthor)}};
}
void Authorship::rebuildDump()
{
    auto serialized = QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
    if (serialized.size() > MaximumBytes) throw std::length_error("Authorship metadata exceeds 1 MiB");
    m_dump = std::move(serialized);
}
bool Authorship::setAuthor(const FileAuthor& author, const QDateTime& at)
{
    const auto key = keyFor(author);
    const auto profile = author.toJson();
    auto next = *this;
    next.m_activeAuthor = key;
    for (qsizetype index = 0; index < next.m_authors.size(); ++index) {
        auto record = next.m_authors[index].toObject();
        if (record["key"].toString() != key) continue;
        if (record["author"].toObject() == profile) {
            m_activeAuthor = key;
            return false;
        }
        record["author"] = profile;
        next.m_authors[index] = record;
        next.recordChange(at);
        *this = std::move(next);
        return true;
    }
    if (next.m_authors.size() >= 256) throw std::length_error("Authorship exceeds 256 authors");
    next.m_authors.append(QJsonObject{{"key", key}, {"author", profile}});
    next.recordChange(at);
    *this = std::move(next);
    return true;
}
void Authorship::recordChange(const QDateTime& at)
{
    if (!at.isValid()) throw std::invalid_argument("Invalid authorship change time");
    if (m_revision == std::numeric_limits<quint64>::max())
        throw std::overflow_error("Authorship revision is exhausted");
    auto next = *this;
    ++next.m_revision;
    next.m_modifiedAt = m_modifiedAt.isValid() && at < m_modifiedAt ? m_modifiedAt : at.toUTC();
    next.m_lastAuthor = m_activeAuthor;
    // Contribution time belongs to this ledger, not a guessed source-file creation time.
    for (qsizetype index = 0; index < next.m_authors.size(); ++index) {
        auto record = next.m_authors[index].toObject();
        if (record["key"].toString() != m_activeAuthor) continue;
        if (!record.contains("firstChangedAt")) record["firstChangedAt"] = detail::dateJson(next.m_modifiedAt);
        record["lastChangedAt"] = detail::dateJson(next.m_modifiedAt);
        next.m_authors[index] = record;
        break;
    }
    next.rebuildDump();
    *this = std::move(next);
}
std::optional<Authorship> Authorship::fromDump(const QByteArray& bytes, QString* error)
{
    if (error) error->clear();
    if (bytes.size() > MaximumBytes) { if (error) *error = "Invalid authorship size."; return std::nullopt; }
    QJsonParseError parseError;
    const auto parsed = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
        if (error) *error = "Invalid authorship JSON.";
        return std::nullopt;
    }
    return fromJson(parsed.object(), error);
}
std::optional<Authorship> Authorship::fromJson(const QJsonObject& json, QString* error)
{
    if (error) error->clear();
    try {
        using namespace detail;
        require(QJsonDocument(json).toJson(QJsonDocument::Compact).size() <= MaximumBytes, "authorship.size");
        knownKeys(json, {"schemaVersion", "revision", "modifiedAt", "authors", "lastAuthor"}, "authorship.fields");
        require(json["schemaVersion"].isDouble() && json["schemaVersion"].toDouble() == 1, "authorship.schemaVersion");
        const auto revision = text(json, "revision", 20, true);
        require(matches(revision, "\\A(?:0|[1-9][0-9]*)\\z"), "authorship.revision");
        Authorship result;
        bool valid = false;
        result.m_revision = revision.toULongLong(&valid);
        require(valid, "authorship.revision");
        result.m_modifiedAt = timestamp(json, "modifiedAt", result.m_revision != 0);
        require(json["authors"].isArray() && json["authors"].toArray().size() <= 256, "authorship.authors");
        QSet<QString> keys;
        for (const auto& item : json["authors"].toArray()) {
            require(item.isObject(), "authorship.author");
            auto record = item.toObject();
            knownKeys(record, {"key", "author", "firstChangedAt", "lastChangedAt"}, "authorship.author.fields");
            const auto author = FileAuthor::fromJson(object(record, "author"));
            require(author.has_value(), "authorship.author");
            const auto key = text(record, "key", 64, true);
            require(key == keyFor(*author) && !keys.contains(key), "authorship.author.key");
            keys.insert(key);
            const auto first = timestamp(record, "firstChangedAt", true);
            const auto last = timestamp(record, "lastChangedAt", true);
            require(first <= last && last <= result.m_modifiedAt, "authorship.author.timestamps");
            record["author"] = author->toJson();
            record["firstChangedAt"] = dateJson(first);
            record["lastChangedAt"] = dateJson(last);
            result.m_authors.append(record);
        }
        const auto last = json["lastAuthor"];
        require(last.isNull() || last.isString(), "authorship.lastAuthor");
        if (last.isString()) {
            result.m_lastAuthor = last.toString();
            require(keys.contains(result.m_lastAuthor), "authorship.lastAuthor");
        }
        require(result.m_revision != 0 || (result.m_authors.isEmpty() && !result.m_modifiedAt.isValid()), "authorship.empty");
        result.rebuildDump();
        return result;
    } catch (const detail::InvalidField& invalid) {
        detail::finishError(error, invalid);
        return std::nullopt;
    } catch (const std::length_error&) {
        if (error) *error = "Invalid authorship size.";
        return std::nullopt;
    }
}
} // namespace iiFileProvider
