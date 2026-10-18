#pragma once

#include <QString>

namespace scapms::models {

enum class PatientCategory {
    General,
    Emergency,
    FollowUp
};

inline QString patientCategoryName(PatientCategory category)
{
    switch (category) {
    case PatientCategory::General: return QStringLiteral("General");
    case PatientCategory::Emergency: return QStringLiteral("Emergency");
    case PatientCategory::FollowUp: return QStringLiteral("Follow-Up");
    }
    return QStringLiteral("Unknown");
}

} // namespace scapms::models
