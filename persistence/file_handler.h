#pragma once

#include "data/clinic_database.h"

#include <QString>

namespace scapms::persistence {

// Result text is suitable for presenting directly in a future GUI message box.
struct PersistenceResult {
    bool success = false;
    QString message;
    explicit operator bool() const noexcept { return success; }
};

class FileHandler {
public:
    static constexpr int currentSchemaVersion = 1;

    static PersistenceResult save(const data::ClinicDatabase& database,
                                  const QString& filePath);
    static PersistenceResult load(data::ClinicDatabase& database,
                                  const QString& filePath);
};

} // namespace scapms::persistence
