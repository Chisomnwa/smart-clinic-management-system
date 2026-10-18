#include "controllers/clinic_manager.h"

#include "utils/domain_exception.h"

namespace scapms::controllers {

void ClinicManager::registerPatient(data::ClinicDatabase::PatientPointer patient)
{
    if (!patient) throw utils::DomainException("Cannot register an empty patient record.");
    if (m_database.findPatient(patient->patientId()))
        throw utils::DomainException("Patient ID already exists: " + patient->patientId().toStdString());
    m_database.pushUndoSnapshot();
    m_database.addPatient(std::move(patient));
}

void ClinicManager::editPatient(const QString& patientId, QString fullName, int age,
                                models::ContactInformation contact,
                                std::optional<QString> followUpReason)
{
    auto current = m_database.requirePatient(patientId);
    auto replacement = current->withUpdatedDetails(std::move(fullName), age, std::move(contact),
                                                   std::move(followUpReason));
    m_database.pushUndoSnapshot();
    m_database.replacePatient(patientId, std::move(replacement));
}

void ClinicManager::deletePatient(const QString& patientId)
{
    (void)m_database.requirePatient(patientId);
    m_database.pushUndoSnapshot();
    m_database.removePatientAndRelatedState(patientId);
}

data::ClinicDatabase::PatientPointer ClinicManager::getPatient(const QString& patientId)
{
    auto patient = m_database.requirePatient(patientId);
    m_database.recordPatientAccess(patientId);
    return patient;
}

std::vector<data::ClinicDatabase::PatientPointer> ClinicManager::listPatients() const
{
    return m_database.patients();
}

} // namespace scapms::controllers
