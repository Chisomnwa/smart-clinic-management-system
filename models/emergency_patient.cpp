#include "models/emergency_patient.h"

#include "utils/domain_exception.h"

#include <utility>

namespace scapms::models {

std::shared_ptr<const PatientRecord> EmergencyPatient::withUpdatedDetails(
    QString fullName, int age, ContactInformation contactInformation,
    std::optional<QString> followUpReason) const
{
    if (followUpReason.has_value()) {
        throw utils::DomainException("Follow-up reason applies only to Follow-Up patients.");
    }
    return std::make_shared<EmergencyPatient>(patientId(), std::move(fullName), age,
                                              std::move(contactInformation));
}

} // namespace scapms::models
