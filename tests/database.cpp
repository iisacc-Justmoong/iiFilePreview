#include "iiFileProvider.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFileInfo>
#include <vector>
#include <cstdlib>
#include <iostream>

using namespace iiFileProvider;
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    const auto path = directory.filePath("arbitrary.document");
    File::create(path, {});
    Database database(path.toStdString());
    database.configureDurable();
    database.execute("CREATE TABLE data (id INTEGER PRIMARY KEY, payload BLOB)");
    {
        Transaction transaction(&database, true);
        Statement insert(&database, "INSERT INTO data VALUES(1,?)");
        const std::vector<std::uint8_t> payload(8192, 0);
        insert.bytes(1, payload.data(), payload.size()); insert.done();
        transaction.commit();
    }
    {
        Transaction transaction(&database, true);
        database.execute("DELETE FROM data");
        // Roll back on scope exit, including exceptions in an upstream codec.
    }
    require(database.scalar("SELECT count(*) FROM data") == 1, "rollback retained record");
    {
        Transaction transaction(&database, true);
        std::vector<std::uint8_t> payload(8192, 0); payload[4097] = 42;
        require(database.patchBlob("data", "payload", 1, payload) == 1, "incremental blob write");
        transaction.commit();
    }
    Statement query(&database, "SELECT payload FROM data WHERE id=1");
    require(query.row() && query.bytes(0).size() == 8192 && query.bytes(0)[4097] == 42,
            "record bytes round trip");
    query.done();
    {
        Transaction transaction(&database, true);
        database.execute("DELETE FROM data"); transaction.commit();
    }
    require(database.scalar("SELECT count(*) FROM data") == 0, "record delete");
    const auto backupPath = directory.filePath("snapshot");
    File::create(backupPath, {});
    Database backup(backupPath.toStdString());
    database.backupTo(backup, 1024 * 1024);
    require(backup.scalar("SELECT count(*) FROM data") == 0, "consistent backup");
    try { Database missing(directory.filePath("missing").toStdString()); return 1; }
    catch (const FileError &) {}
    require(!QFileInfo::exists(directory.filePath("missing")), "open never creates missing file");
}
