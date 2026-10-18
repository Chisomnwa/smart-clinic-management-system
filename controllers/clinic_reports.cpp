#include "controllers/clinic_reports.h"

#include "utils/domain_exception.h"

namespace scapms::controllers {

std::size_t CategoryDistribution::count(models::PatientCategory category) const
{
    switch (category) {
    case models::PatientCategory::General: return counts[0];
    case models::PatientCategory::Emergency: return counts[1];
    case models::PatientCategory::FollowUp: return counts[2];
    }
    throw utils::DomainException("Unknown patient category.");
}

std::vector<AttendanceReportRow> ClinicReports::mostFrequentlyAttended(
    const data::ClinicDatabase& database)
{
    std::vector<AttendanceReportRow> rows;
    for (const auto& patient : database.patients()) {
        std::size_t completed = 0;
        for (const auto& appointment : database.appointments())
            if (appointment.patientId() == patient->patientId() &&
                appointment.status() == models::AppointmentStatus::Completed) ++completed;
        if (completed > 0) rows.push_back({patient, completed});
    }
    // Manual insertion ordering, preserving tied patients in the report.
    for (std::size_t i = 1; i < rows.size(); ++i) {
        auto key = rows[i];
        std::size_t j = i;
        while (j > 0 && rows[j - 1].completedAppointments < key.completedAppointments) {
            rows[j] = rows[j - 1]; --j;
        }
        rows[j] = std::move(key);
    }
    return rows;
}

std::vector<PendingAppointmentsReportRow> ClinicReports::patientsWithPendingAppointments(
    const data::ClinicDatabase& database)
{
    std::vector<PendingAppointmentsReportRow> rows;
    for (const auto& patient : database.patients()) {
        std::size_t pending = 0;
        for (const auto& appointment : database.appointments())
            if (appointment.patientId() == patient->patientId() &&
                appointment.status() == models::AppointmentStatus::Pending) ++pending;
        if (pending > 0) rows.push_back({patient, pending});
    }
    return rows;
}

CategoryDistribution ClinicReports::patientCategoryDistribution(
    const data::ClinicDatabase& database)
{
    CategoryDistribution result;
    for (const auto& patient : database.patients()) {
        switch (patient->category()) {
        case models::PatientCategory::General: ++result.counts[0]; break;
        case models::PatientCategory::Emergency: ++result.counts[1]; break;
        case models::PatientCategory::FollowUp: ++result.counts[2]; break;
        }
    }
    return result;
}

DailyAppointmentSummary ClinicReports::dailyAppointmentSummary(
    const data::ClinicDatabase& database, const QDate& date)
{
    if (!date.isValid()) throw utils::DomainException("Report date must be valid.");
    DailyAppointmentSummary result;
    result.date = date;
    for (const auto& appointment : database.appointments()) {
        if (appointment.scheduledAt().date() != date) continue;
        ++result.total;
        switch (appointment.status()) {
        case models::AppointmentStatus::Pending: ++result.pending; break;
        case models::AppointmentStatus::Completed: ++result.completed; break;
        case models::AppointmentStatus::Cancelled: ++result.cancelled; break;
        }
    }
    return result;
}

} // namespace scapms::controllers
