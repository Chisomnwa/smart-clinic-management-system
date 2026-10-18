#pragma once

#include "data/clinic_database.h"

#include <optional>

namespace scapms::controllers {

class ClinicManager {
public:
    explicit ClinicManager(data::ClinicDatabase& database) : m_database(database) {}

    void registerPatient(data::ClinicDatabase::PatientPointer patient);
    QString nextPatientId() const { return m_database.nextPatientId(); }
    void editPatient(const QString& patientId, QString fullName, int age,
                     models::ContactInformation contact,
                     std::optional<QString> followUpReason = std::nullopt);
    void deletePatient(const QString& patientId);
    data::ClinicDatabase::PatientPointer getPatient(const QString& patientId);
    std::vector<data::ClinicDatabase::PatientPointer> listPatients() const;
    void enqueuePatient(const QString& patientId) { m_database.enqueuePatient(patientId); }
    std::vector<QString> waitingQueue() const { return m_database.waitingQueue(); }
    std::optional<QString> callNextPatient() { return m_database.callNextPatient(); }
    bool undoLastAdministrativeOperation() { return m_database.undoLastOperation(); }

private:
    data::ClinicDatabase& m_database;
};

} // namespace scapms::controllers
