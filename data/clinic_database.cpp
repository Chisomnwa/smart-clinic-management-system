#include "data/clinic_database.h"

#include "utils/domain_exception.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace scapms::data {
namespace {

template <typename Range, typename IdAccessor>
std::uint64_t largestNumericSuffix(const Range& records, const QString& prefix, IdAccessor idOf)
{
    std::uint64_t largest = 0;
    for (const auto& record : records) {
        const QString id = idOf(record);
        if (!id.startsWith(prefix, Qt::CaseSensitive)) continue;
        bool parsed = false;
        const auto suffix = id.mid(prefix.size()).toULongLong(&parsed, 10);
        if (parsed && suffix > largest) largest = suffix;
    }
    return largest;
}

std::uint64_t numericSuffix(const QString& id, const QString& prefix)
{
    if (!id.startsWith(prefix, Qt::CaseSensitive)) return 0;
    bool parsed = false;
    const auto suffix = id.mid(prefix.size()).toULongLong(&parsed, 10);
    return parsed ? suffix : 0;
}

template <typename Exists>
QString generateId(const QString& prefix, std::uint64_t largest, const Exists& exists)
{
    if (largest == std::numeric_limits<std::uint64_t>::max()) {
        throw utils::DomainException("No more numeric IDs are available for prefix " + prefix.toStdString());
    }
    auto candidateNumber = largest + 1;
    while (true) {
        const QString candidate = prefix + QStringLiteral("%1").arg(
            static_cast<qulonglong>(candidateNumber), 6, 10, QLatin1Char('0'));
        if (!exists(candidate)) return candidate;
        if (candidateNumber == std::numeric_limits<std::uint64_t>::max()) {
            throw utils::DomainException("No more numeric IDs are available for prefix " + prefix.toStdString());
        }
        ++candidateNumber;
    }
}

} // namespace

ClinicDatabase::PatientPointer ClinicDatabase::findPatient(const QString& patientId) const noexcept
{
    for (const auto& patient : m_patients) {
        if (patient->patientId() == patientId) {
            return patient;
        }
    }
    return {};
}

ClinicDatabase::PatientPointer ClinicDatabase::requirePatient(const QString& patientId) const
{
    const auto patient = findPatient(patientId);
    if (!patient) {
        throw utils::DomainException("Patient was not found: " + patientId.toStdString());
    }
    return patient;
}

const models::Appointment* ClinicDatabase::findAppointment(const QString& appointmentId) const noexcept
{
    for (const auto& appointment : m_appointments) {
        if (appointment.appointmentId() == appointmentId) {
            return &appointment;
        }
    }
    return nullptr;
}

models::Appointment* ClinicDatabase::findAppointment(const QString& appointmentId) noexcept
{
    for (auto& appointment : m_appointments) {
        if (appointment.appointmentId() == appointmentId) {
            return &appointment;
        }
    }
    return nullptr;
}

const models::Appointment& ClinicDatabase::requireAppointment(const QString& appointmentId) const
{
    const auto* appointment = findAppointment(appointmentId);
    if (!appointment) {
        throw utils::DomainException("Appointment was not found: " + appointmentId.toStdString());
    }
    return *appointment;
}

models::Appointment& ClinicDatabase::requireAppointment(const QString& appointmentId)
{
    auto* appointment = findAppointment(appointmentId);
    if (!appointment) {
        throw utils::DomainException("Appointment was not found: " + appointmentId.toStdString());
    }
    return *appointment;
}

void ClinicDatabase::addPatient(PatientPointer patient)
{
    if (!patient) {
        throw utils::DomainException("Cannot register an empty patient record.");
    }
    if (findPatient(patient->patientId())) {
        throw utils::DomainException("Patient ID already exists: " + patient->patientId().toStdString());
    }
    m_patientIdSequence = std::max(m_patientIdSequence,
                                    numericSuffix(patient->patientId(), QStringLiteral("P-")));
    m_patients.push_back(std::move(patient));
}

void ClinicDatabase::replacePatient(const QString& patientId, PatientPointer replacement)
{
    if (!replacement || replacement->patientId() != patientId) {
        throw utils::DomainException("Replacement patient must retain the original patient ID.");
    }
    for (auto& patient : m_patients) {
        if (patient->patientId() == patientId) {
            patient = std::move(replacement);
            invalidatePatientCache(patientId);
            return;
        }
    }
    throw utils::DomainException("Patient was not found: " + patientId.toStdString());
}

void ClinicDatabase::removePatientAndRelatedState(const QString& patientId)
{
    std::size_t patientIndex = m_patients.size();
    for (std::size_t i = 0; i < m_patients.size(); ++i) {
        if (m_patients[i]->patientId() == patientId) {
            patientIndex = i;
            break;
        }
    }
    if (patientIndex == m_patients.size()) {
        throw utils::DomainException("Patient was not found: " + patientId.toStdString());
    }

    m_patients.erase(m_patients.begin() + static_cast<std::ptrdiff_t>(patientIndex));

    for (std::size_t i = 0; i < m_appointments.size();) {
        if (m_appointments[i].patientId() == patientId) {
            m_appointments.erase(m_appointments.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    std::queue<QString> retained;
    while (!m_waitingQueue.empty()) {
        const QString queuedId = m_waitingQueue.front();
        m_waitingQueue.pop();
        if (queuedId != patientId) {
            retained.push(queuedId);
        }
    }
    m_waitingQueue = std::move(retained);
    invalidatePatientCache(patientId);
}

QString ClinicDatabase::nextPatientId() const
{
    const auto largest = std::max(m_patientIdSequence, largestNumericSuffix(
        m_patients, QStringLiteral("P-"), [](const PatientPointer& patient) { return patient->patientId(); }));
    return generateId(QStringLiteral("P-"), largest,
                      [this](const QString& id) { return static_cast<bool>(findPatient(id)); });
}

void ClinicDatabase::addAppointment(models::Appointment appointment)
{
    if (findAppointment(appointment.appointmentId())) {
        throw utils::DomainException("Appointment ID already exists: " +
                                     appointment.appointmentId().toStdString());
    }
    if (!findPatient(appointment.patientId())) {
        throw utils::DomainException("Appointment references an unregistered patient.");
    }
    m_appointmentIdSequence = std::max(m_appointmentIdSequence,
        numericSuffix(appointment.appointmentId(), QStringLiteral("A-")));
    m_appointments.push_back(std::move(appointment));
}

void ClinicDatabase::removeAppointment(const QString& appointmentId)
{
    for (std::size_t i = 0; i < m_appointments.size(); ++i) {
        if (m_appointments[i].appointmentId() == appointmentId) {
            m_appointments.erase(m_appointments.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
    throw utils::DomainException("Appointment was not found: " + appointmentId.toStdString());
}

QString ClinicDatabase::nextAppointmentId() const
{
    const auto largest = std::max(m_appointmentIdSequence, largestNumericSuffix(
        m_appointments, QStringLiteral("A-"),
        [](const models::Appointment& appointment) { return appointment.appointmentId(); }));
    return generateId(QStringLiteral("A-"), largest, [this](const QString& id) {
        return findAppointment(id) != nullptr;
    });
}

void ClinicDatabase::enqueuePatient(const QString& patientId)
{
    if (!findPatient(patientId)) {
        throw utils::DomainException("Cannot queue an unregistered patient.");
    }
    auto currentQueue = m_waitingQueue;
    while (!currentQueue.empty()) {
        const auto queuedId = currentQueue.front();
        currentQueue.pop();
        if (queuedId == patientId) {
            throw utils::DomainException("Patient is already in the waiting queue.");
        }
    }
    // Queue changes share the same administrative undo history as patient and
    // appointment changes. Snapshot only after validation has succeeded.
    pushUndoSnapshot();
    m_waitingQueue.push(patientId);
}

std::vector<QString> ClinicDatabase::waitingQueue() const
{
    auto copy = m_waitingQueue;
    std::vector<QString> result;
    result.reserve(copy.size());
    while (!copy.empty()) {
        result.push_back(copy.front());
        copy.pop();
    }
    return result;
}

std::optional<QString> ClinicDatabase::callNextPatient()
{
    if (m_waitingQueue.empty()) {
        return std::nullopt;
    }
    QString patientId = m_waitingQueue.front();
    pushUndoSnapshot();
    m_waitingQueue.pop();
    return patientId;
}

void ClinicDatabase::recordPatientAccess(const QString& patientId)
{
    if (!findPatient(patientId)) {
        throw utils::DomainException("Cannot cache a patient that is not registered.");
    }

    if (m_accessCounter == std::numeric_limits<std::uint64_t>::max()) {
        std::uint64_t next = 1;
        for (auto& entry : m_recentPatients) {
            if (entry.occupied) {
                entry.lastAccess = next++;
            }
        }
        m_accessCounter = next - 1;
    }
    const auto accessTime = ++m_accessCounter;

    for (auto& entry : m_recentPatients) {
        if (entry.occupied && entry.patientId == patientId) {
            entry.lastAccess = accessTime;
            return;
        }
    }
    for (auto& entry : m_recentPatients) {
        if (!entry.occupied) {
            entry = {patientId, accessTime, true};
            return;
        }
    }

    auto* leastRecent = &m_recentPatients.front();
    for (auto& entry : m_recentPatients) {
        if (entry.lastAccess < leastRecent->lastAccess) {
            leastRecent = &entry;
        }
    }
    *leastRecent = {patientId, accessTime, true};
}

bool ClinicDatabase::isPatientCached(const QString& patientId) const noexcept
{
    for (const auto& entry : m_recentPatients) {
        if (entry.occupied && entry.patientId == patientId) {
            return true;
        }
    }
    return false;
}

void ClinicDatabase::pushUndoSnapshot()
{
    m_undoSnapshots.push({m_patients, m_appointments, m_waitingQueue,
                          m_recentPatients, m_accessCounter});
}

bool ClinicDatabase::undoLastOperation()
{
    if (m_undoSnapshots.empty()) {
        return false;
    }
    Snapshot snapshot = std::move(m_undoSnapshots.top());
    m_undoSnapshots.pop();
    m_patients = std::move(snapshot.patients);
    m_appointments = std::move(snapshot.appointments);
    m_waitingQueue = std::move(snapshot.waitingQueue);
    m_recentPatients = std::move(snapshot.recentPatients);
    m_accessCounter = snapshot.accessCounter;
    return true;
}

void ClinicDatabase::replaceWithValidatedState(ClinicDatabase&& validatedState)
{
    if (this == &validatedState) {
        m_undoSnapshots = {};
        m_recentPatients = {};
        m_accessCounter = 0;
        return;
    }
    m_patients = std::move(validatedState.m_patients);
    m_appointments = std::move(validatedState.m_appointments);
    m_waitingQueue = std::move(validatedState.m_waitingQueue);
    m_patientIdSequence = validatedState.m_patientIdSequence;
    m_appointmentIdSequence = validatedState.m_appointmentIdSequence;
    m_undoSnapshots = {};
    m_recentPatients = {};
    m_accessCounter = 0;
}

void ClinicDatabase::invalidatePatientCache(const QString& patientId) noexcept
{
    for (auto& entry : m_recentPatients) {
        if (entry.occupied && entry.patientId == patientId) {
            entry = {};
        }
    }
}

} // namespace scapms::data
