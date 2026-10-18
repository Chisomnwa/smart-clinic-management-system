#include "controllers/patient_algorithms.h"

#include "utils/domain_exception.h"

namespace scapms::controllers {
namespace {

QString normalized(const QString& value) { return value.trimmed().toCaseFolded(); }

int compare(const data::ClinicDatabase::PatientPointer& lhs,
            const data::ClinicDatabase::PatientPointer& rhs, PatientSortField field)
{
    switch (field) {
    case PatientSortField::Name: {
        const int result = QString::compare(lhs->fullName(), rhs->fullName(), Qt::CaseInsensitive);
        if (result != 0) return result;
        return QString::compare(lhs->patientId(), rhs->patientId(), Qt::CaseSensitive);
    }
    case PatientSortField::Id:
        return QString::compare(lhs->patientId(), rhs->patientId(), Qt::CaseSensitive);
    case PatientSortField::Age:
        if (lhs->age() < rhs->age()) return -1;
        if (lhs->age() > rhs->age()) return 1;
        return QString::compare(lhs->patientId(), rhs->patientId(), Qt::CaseSensitive);
    }
    return 0;
}

bool comesBefore(const data::ClinicDatabase::PatientPointer& lhs,
                 const data::ClinicDatabase::PatientPointer& rhs,
                 PatientSortField field, bool ascending)
{
    const int order = compare(lhs, rhs, field);
    return ascending ? order < 0 : order > 0;
}

void searchNameAt(const std::vector<data::ClinicDatabase::PatientPointer>& records,
                  std::size_t index, const QString& query,
                  std::vector<data::ClinicDatabase::PatientPointer>& results)
{
    if (index >= records.size()) return;
    if (normalized(records[index]->fullName()).contains(query)) results.push_back(records[index]);
    searchNameAt(records, index + 1, query, results);
}

} // namespace

PatientAlgorithms::PatientPointer PatientAlgorithms::linearSearchById(
    const std::vector<PatientPointer>& records, const QString& patientId)
{
    const QString id = patientId.trimmed();
    if (id.isEmpty()) throw utils::DomainException("Patient ID search text cannot be empty.");
    for (const auto& patient : records)
        if (patient && patient->patientId() == id) return patient;
    return {};
}

PatientAlgorithms::PatientPointer PatientAlgorithms::binarySearchById(
    const SortedPatientRecords& records, const QString& patientId)
{
    const QString id = patientId.trimmed();
    if (id.isEmpty()) throw utils::DomainException("Patient ID search text cannot be empty.");
    if (records.field() != PatientSortField::Id || !records.ascending())
        throw utils::DomainException("Binary search requires patient records sorted by ID ascending.");
    std::size_t low = 0, high = records.records().size();
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        const int order = QString::compare(records.records()[mid]->patientId(), id, Qt::CaseSensitive);
        if (order == 0) return records.records()[mid];
        if (order < 0) low = mid + 1;
        else high = mid;
    }
    return {};
}

std::vector<PatientAlgorithms::PatientPointer> PatientAlgorithms::recursiveSearchByName(
    const std::vector<PatientPointer>& records, const QString& nameQuery)
{
    const QString query = normalized(nameQuery);
    if (query.isEmpty()) throw utils::DomainException("Patient name search text cannot be empty.");
    std::vector<PatientPointer> results;
    searchNameAt(records, 0, query, results);
    return results;
}

std::vector<PatientAlgorithms::PatientPointer> PatientAlgorithms::searchByCategory(
    const std::vector<PatientPointer>& records, models::PatientCategory category)
{
    switch (category) {
    case models::PatientCategory::General:
    case models::PatientCategory::Emergency:
    case models::PatientCategory::FollowUp:
        break;
    default:
        throw utils::DomainException("Unknown patient category search value.");
    }
    std::vector<PatientPointer> matches;
    for (const auto& patient : records)
        if (patient && patient->category() == category) matches.push_back(patient);
    return matches;
}

SortedPatientRecords PatientAlgorithms::sort(const std::vector<PatientPointer>& records,
                                              PatientSortField field, SortAlgorithm algorithm,
                                              bool ascending)
{
    switch (field) {
    case PatientSortField::Name:
    case PatientSortField::Id:
    case PatientSortField::Age:
        break;
    default:
        throw utils::DomainException("Unknown patient sort field.");
    }
    auto result = records;
    for (const auto& patient : result)
        if (!patient) throw utils::DomainException("Cannot sort an empty patient record.");

    if (algorithm == SortAlgorithm::Insertion) {
        for (std::size_t i = 1; i < result.size(); ++i) {
            auto key = result[i];
            std::size_t j = i;
            while (j > 0 && comesBefore(key, result[j - 1], field, ascending)) {
                result[j] = result[j - 1];
                --j;
            }
            result[j] = std::move(key);
        }
    } else if (algorithm == SortAlgorithm::Selection) {
        for (std::size_t i = 0; i < result.size(); ++i) {
            std::size_t selected = i;
            for (std::size_t j = i + 1; j < result.size(); ++j)
                if (comesBefore(result[j], result[selected], field, ascending)) selected = j;
            if (selected != i) std::swap(result[i], result[selected]);
        }
    } else {
        throw utils::DomainException("Unknown patient sort algorithm.");
    }
    return SortedPatientRecords(std::move(result), field, ascending);
}

} // namespace scapms::controllers
