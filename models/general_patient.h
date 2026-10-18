#pragma once

#include "models/patient_record.h"

namespace scapms::models {

class GeneralPatient final : public PatientRecord {
public:
    using PatientRecord::PatientRecord;

    PatientCategory category() const noexcept override { return PatientCategory::General; }
    QString categoryLabel() const override { return QStringLiteral("General"); }
    QString priorityLabel() const override { return QStringLiteral("Normal"); }
    std::shared_ptr<const PatientRecord> withUpdatedDetails(
        QString fullName, int age, ContactInformation contactInformation,
        std::optional<QString> followUpReason = std::nullopt) const override;
};

} // namespace scapms::models
