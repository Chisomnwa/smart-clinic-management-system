#pragma once

#include "data/clinic_database.h"

#include <array>

namespace scapms::controllers {

struct AttendanceReportRow {
    data::ClinicDatabase::PatientPointer patient;
    std::size_t completedAppointments = 0;
};
struct PendingAppointmentsReportRow {
    data::ClinicDatabase::PatientPointer patient;
    std::size_t pendingAppointments = 0;
};
struct CategoryDistribution {
    std::array<std::size_t, 3> counts{}; // General, Emergency, FollowUp
    std::size_t count(models::PatientCategory category) const;
    std::size_t total() const noexcept { return counts[0] + counts[1] + counts[2]; }
};
struct DailyAppointmentSummary {
    QDate date;
    std::size_t total = 0;
    std::size_t pending = 0;
    std::size_t completed = 0;
    std::size_t cancelled = 0;
};

class ClinicReports {
public:
    static std::vector<AttendanceReportRow> mostFrequentlyAttended(
        const data::ClinicDatabase& database);
    static std::vector<PendingAppointmentsReportRow> patientsWithPendingAppointments(
        const data::ClinicDatabase& database);
    static CategoryDistribution patientCategoryDistribution(
        const data::ClinicDatabase& database);
    static DailyAppointmentSummary dailyAppointmentSummary(
        const data::ClinicDatabase& database, const QDate& date);
};

} // namespace scapms::controllers
