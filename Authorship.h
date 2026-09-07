#pragma once

#include "FileAuthor.h"
#include <QtCore/QJsonArray>

namespace iiFileProvider {

// Copyable file metadata. No credentials, callbacks, writable file or owner pointers.
class IIFILEPROVIDER_EXPORT Authorship final {
public:
    static constexpr qsizetype MaximumBytes = 1024 * 1024;
    static constexpr const char* MetadataKey = "iisacc:authorship";
    Authorship();
    [[nodiscard]] static std::optional<Authorship> fromJson(const QJsonObject&, QString* error = nullptr);
    [[nodiscard]] static std::optional<Authorship> fromDump(const QByteArray&, QString* error = nullptr);
    // Returns false only for an identical profile; still selects that author for future changes.
    // Stored profiles are sanitized through FileAuthor::toJson(). Failure throws before mutation.
    bool setAuthor(const FileAuthor&, const QDateTime& at = QDateTime::currentDateTimeUtc());
    void clearActiveAuthor() noexcept;
    [[nodiscard]] bool hasActiveAuthor() const noexcept;
    // Call synchronously at a successful mutation boundary, never for a rejected edit/no-op.
    void recordChange(const QDateTime& at = QDateTime::currentDateTimeUtc());
    [[nodiscard]] quint64 revision() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] QByteArray dump() const; // Already serialized by the last mutation.
private:
    quint64 m_revision = 0;
    QDateTime m_modifiedAt;
    QJsonArray m_authors;
    QString m_lastAuthor;
    QString m_activeAuthor; // Local editing context, never restored from a file.
    QByteArray m_dump;
    void rebuildDump();
};
} // namespace iiFileProvider
