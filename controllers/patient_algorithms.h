#pragma once

#include "data/clinic_database.h"

namespace scapms::controllers {

enum class PatientSortField { Name, Id, Age };
enum class SortAlgorithm { Insertion, Selection };

// Carries sort metadata so binary search cannot accidentally consume an
// unsorted list or a list sorted on a different field.
class SortedPatientRecords {
public:
    const std::vector<data::ClinicDatabase::PatientPointer>& records() const noexcept { return m_records; }
    PatientSortField field() const noexcept { return m_field; }
    bool ascending() const noexcept { return m_ascending; }

private:
    friend class PatientAlgorithms;
    SortedPatientRecords(std::vector<data::ClinicDatabase::PatientPointer> records,
                         PatientSortField field, bool ascending)
        : m_records(std::move(records)), m_field(field), m_ascending(ascending) {}
    std::vector<data::ClinicDatabase::PatientPointer> m_records;
    PatientSortField m_field;
    bool m_ascending;
};

class PatientAlgorithms {
public:
    using PatientPointer = data::ClinicDatabase::PatientPointer;
    static PatientPointer linearSearchById(const std::vector<PatientPointer>& records,
                                           const QString& patientId);
    static PatientPointer binarySearchById(const SortedPatientRecords& records,
                                           const QString& patientId);
    static std::vector<PatientPointer> recursiveSearchByName(
        const std::vector<PatientPointer>& records, const QString& nameQuery);
    static std::vector<PatientPointer> searchByCategory(
        const std::vector<PatientPointer>& records, models::PatientCategory category);
    static SortedPatientRecords sort(const std::vector<PatientPointer>& records,
                                     PatientSortField field, SortAlgorithm algorithm,
                                     bool ascending = true);
};

} // namespace scapms::controllers
