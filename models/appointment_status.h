#pragma once

#include <QString>

namespace scapms::models {

enum class AppointmentStatus {
    Pending,
    Completed,
    Cancelled
};

inline QString appointmentStatusName(AppointmentStatus status)
{
    switch (status) {
    case AppointmentStatus::Pending: return QStringLiteral("Pending");
    case AppointmentStatus::Completed: return QStringLiteral("Completed");
    case AppointmentStatus::Cancelled: return QStringLiteral("Cancelled");
    }
    return QStringLiteral("Unknown");
}

} // namespace scapms::models
