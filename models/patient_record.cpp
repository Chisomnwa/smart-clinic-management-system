#include "models/patient_record.h"

#include "models/appointment.h"
#include "utils/domain_exception.h"

#include <utility>

namespace scapms::models {

PatientRecord::PatientRecord(QString patientId, QString fullName, int age,
                             ContactInformation contactInformation)
    : m_patientId(std::move(patientId))
    , m_fullName(std::move(fullName))
    , m_age(age)
    , m_contactInformation(std::move(contactInformation))
{
    m_patientId = m_patientId.trimmed();
    m_fullName = m_fullName.trimmed();
    m_contactInformation.phoneNumber = m_contactInformation.phoneNumber.trimmed();
    m_contactInformation.email = m_contactInformation.email.trimmed();

    if (m_patientId.isEmpty()) {
        throw utils::DomainException("Patient ID cannot be empty.");
    }
    if (m_fullName.isEmpty()) {
        throw utils::DomainException("Patient name cannot be empty.");
    }
    if (m_age < 0 || m_age > 120) {
        throw utils::DomainException("Patient age must be between 0 and 120.");
    }
    if (m_contactInformation.phoneNumber.isEmpty()) {
        throw utils::DomainException("Patient contact number cannot be empty.");
    }
}

void PatientRecord::scheduleAppointment(const Appointment& appointment,
                                        const QDateTime& now) const
{
    if (appointment.patientId() != m_patientId) {
        throw utils::DomainException("Appointment patient ID does not match this patient.");
    }
    appointment.validateScheduling(now);
}

void PatientRecord::cancelAppointment(Appointment& appointment) const
{
    if (appointment.patientId() != m_patientId) {
        throw utils::DomainException("Appointment patient ID does not match this patient.");
    }
    appointment.cancel();
}

QString PatientRecord::requireFollowUpReason(QString reason)
{
    reason = reason.trimmed();
    if (reason.isEmpty()) {
        throw utils::DomainException("Follow-Up patients must have a follow-up reason.");
    }
    return reason;
}

} // namespace scapms::models
