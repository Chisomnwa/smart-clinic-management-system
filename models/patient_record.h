#pragma once

#include "models/appointable.h"
#include "models/contact_information.h"
#include "models/patient_category.h"

#include <QString>

#include <memory>
#include <optional>

namespace scapms::models {

class Appointment;

class PatientRecord : public Appointable {
public:
    PatientRecord(QString patientId, QString fullName, int age,
                  ContactInformation contactInformation);
    ~PatientRecord() override = default;

    const QString& patientId() const noexcept { return m_patientId; }
    const QString& fullName() const noexcept { return m_fullName; }
    int age() const noexcept { return m_age; }
    const ContactInformation& contactInformation() const noexcept { return m_contactInformation; }

    virtual PatientCategory category() const noexcept = 0;
    virtual QString categoryLabel() const = 0;
    virtual QString priorityLabel() const = 0;
    virtual std::shared_ptr<const PatientRecord> withUpdatedDetails(
        QString fullName, int age, ContactInformation contactInformation,
        std::optional<QString> followUpReason = std::nullopt) const = 0;

    void scheduleAppointment(const Appointment& appointment,
                             const QDateTime& now) const override;
    void cancelAppointment(Appointment& appointment) const override;

protected:
    static QString requireFollowUpReason(QString reason);

private:
    QString m_patientId;
    QString m_fullName;
    int m_age;
    ContactInformation m_contactInformation;
};

} // namespace scapms::models
