#pragma once

#include <QDateTime>

namespace scapms::models {

class Appointment;

// Appointment actions are exposed through this interface so callers can use
// PatientRecord polymorphically. Implementations validate patient association
// and appointment state; persistence/orchestration belongs to controllers.
class Appointable {
public:
    virtual ~Appointable() = default;

    virtual void scheduleAppointment(const Appointment& appointment,
                                     const QDateTime& now) const = 0;
    virtual void cancelAppointment(Appointment& appointment) const = 0;
};

} // namespace scapms::models
