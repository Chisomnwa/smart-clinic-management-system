#pragma once

#include "models/patient_record.h"

namespace scapms::models {

class FollowUpPatient final : public PatientRecord {
public:
    FollowUpPatient(QString patientId, QString fullName, int age,
                    ContactInformation contactInformation, QString followUpReason);

    PatientCategory category() const noexcept override { return PatientCategory::FollowUp; }
    QString categoryLabel() const override { return QStringLiteral("Follow-Up"); }
    QString priorityLabel() const override { return QStringLiteral("Follow-Up"); }
    const QString& followUpReason() const noexcept { return m_followUpReason; }
    std::shared_ptr<const PatientRecord> withUpdatedDetails(
        QString fullName, int age, ContactInformation contactInformation,
        std::optional<QString> followUpReason = std::nullopt) const override;

private:
    QString m_followUpReason;
};

} // namespace scapms::models
