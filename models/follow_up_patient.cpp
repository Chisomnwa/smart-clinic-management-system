#include "models/follow_up_patient.h"

#include <utility>

namespace scapms::models {

FollowUpPatient::FollowUpPatient(QString patientId, QString fullName, int age,
                                 ContactInformation contactInformation,
                                 QString followUpReason)
    : PatientRecord(std::move(patientId), std::move(fullName), age,
                    std::move(contactInformation))
    , m_followUpReason(requireFollowUpReason(std::move(followUpReason)))
{
}

std::shared_ptr<const PatientRecord> FollowUpPatient::withUpdatedDetails(
    QString fullName, int age, ContactInformation contactInformation,
    std::optional<QString> followUpReason) const
{
    return std::make_shared<FollowUpPatient>(patientId(), std::move(fullName), age,
                                             std::move(contactInformation),
                                             followUpReason.value_or(m_followUpReason));
}

} // namespace scapms::models
