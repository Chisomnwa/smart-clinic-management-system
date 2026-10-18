#include "models/appointment.h"

#include "utils/domain_exception.h"

#include <utility>

namespace scapms::models {

Appointment::Appointment(QString appointmentId,
                         QString patientId,
                         QDateTime scheduledAt,
                         AppointmentStatus status,
                         QString notes)
    : m_appointmentId(std::move(appointmentId))
    , m_patientId(std::move(patientId))
    , m_scheduledAt(std::move(scheduledAt))
    , m_status(status)
    , m_notes(std::move(notes))
{
    m_appointmentId = m_appointmentId.trimmed();
    m_patientId = m_patientId.trimmed();
    m_notes = m_notes.trimmed();
    validateCommonFields();
}

Appointment Appointment::schedule(QString appointmentId,
                                   QString patientId,
                                   QDateTime scheduledAt,
                                   QString notes,
                                   QDateTime now)
{
    Appointment appointment(std::move(appointmentId), std::move(patientId),
                            std::move(scheduledAt), AppointmentStatus::Pending,
                            std::move(notes));
    appointment.validateScheduling(now);
    return appointment;
}

Appointment Appointment::restore(QString appointmentId,
                                  QString patientId,
                                  QDateTime scheduledAt,
                                  AppointmentStatus status,
                                  QString notes)
{
    return Appointment(std::move(appointmentId), std::move(patientId),
                       std::move(scheduledAt), status, std::move(notes));
}

void Appointment::validateCommonFields() const
{
    if (m_appointmentId.isEmpty()) {
        throw utils::DomainException("Appointment ID cannot be empty.");
    }
    if (m_patientId.isEmpty()) {
        throw utils::DomainException("Appointment must reference a patient ID.");
    }
    if (!m_scheduledAt.isValid()) {
        throw utils::DomainException("Appointment date and time must be valid.");
    }
    switch (m_status) {
    case AppointmentStatus::Pending:
    case AppointmentStatus::Completed:
    case AppointmentStatus::Cancelled:
        break;
    default:
        throw utils::DomainException("Appointment status is not recognized.");
    }
}

void Appointment::validateScheduling(const QDateTime& now) const
{
    if (!now.isValid()) {
        throw utils::DomainException("Current date and time must be valid.");
    }
    if (m_status != AppointmentStatus::Pending) {
        throw utils::DomainException("A new appointment must have Pending status.");
    }
    if (m_scheduledAt <= now) {
        throw utils::DomainException("New appointments must be scheduled in the future.");
    }
}

void Appointment::cancel()
{
    if (m_status != AppointmentStatus::Pending) {
        throw utils::DomainException("Only Pending appointments can be cancelled.");
    }
    m_status = AppointmentStatus::Cancelled;
}

void Appointment::markCompleted(const QDateTime& now)
{
    if (m_status != AppointmentStatus::Pending) {
        throw utils::DomainException("Only Pending appointments can be completed.");
    }
    if (!now.isValid() || m_scheduledAt > now) {
        throw utils::DomainException("An appointment cannot be completed before its scheduled time.");
    }
    m_status = AppointmentStatus::Completed;
}

} // namespace scapms::models
