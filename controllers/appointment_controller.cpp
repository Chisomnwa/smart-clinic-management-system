#include "controllers/appointment_controller.h"

#include "utils/domain_exception.h"

namespace scapms::controllers {

models::Appointment AppointmentController::scheduleAppointment(
    QString appointmentId, const QString& patientId, QDateTime when, QString notes, QDateTime now)
{
    if (m_database.findAppointment(appointmentId.trimmed()))
        throw utils::DomainException("Appointment ID already exists: " + appointmentId.toStdString());
    auto patient = m_database.requirePatient(patientId);
    auto candidate = models::Appointment::schedule(std::move(appointmentId), patientId,
                                                    std::move(when), std::move(notes), now);
    patient->scheduleAppointment(candidate, now); // runtime dispatch through Appointable
    for (const auto& existing : m_database.appointments()) {
        if (existing.patientId() == patientId && existing.scheduledAt() == candidate.scheduledAt() &&
            existing.status() == models::AppointmentStatus::Pending)
            throw utils::DomainException("Patient already has a pending appointment at that time.");
    }
    m_database.pushUndoSnapshot();
    m_database.addAppointment(std::move(candidate));
    return m_database.appointments().back();
}

void AppointmentController::cancelAppointment(const QString& appointmentId)
{
    const auto& appointment = m_database.requireAppointment(appointmentId);
    auto patient = m_database.requirePatient(appointment.patientId());
    // Validate transition before snapshotting so failed requests do not create undo entries.
    if (appointment.status() != models::AppointmentStatus::Pending)
        throw utils::DomainException("Only Pending appointments can be cancelled.");
    m_database.pushUndoSnapshot();
    auto& mutableAppointment = m_database.requireAppointment(appointmentId);
    patient->cancelAppointment(mutableAppointment);
}

void AppointmentController::completeAppointment(const QString& appointmentId, QDateTime now)
{
    const auto& appointment = m_database.requireAppointment(appointmentId);
    if (appointment.status() != models::AppointmentStatus::Pending ||
        !now.isValid() || appointment.scheduledAt() > now)
        throw utils::DomainException("Appointment cannot be completed in its current state or before its scheduled time.");
    m_database.pushUndoSnapshot();
    m_database.requireAppointment(appointmentId).markCompleted(now);
}

void AppointmentController::deleteAppointment(const QString& appointmentId)
{
    (void)m_database.requireAppointment(appointmentId);
    m_database.pushUndoSnapshot();
    m_database.removeAppointment(appointmentId);
}

models::Appointment AppointmentController::getAppointment(const QString& appointmentId) const
{
    return m_database.requireAppointment(appointmentId);
}

std::vector<models::Appointment> AppointmentController::listAppointments() const
{
    return m_database.appointments();
}

} // namespace scapms::controllers
