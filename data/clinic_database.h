#pragma once

#include "models/appointment.h"
#include "models/patient_record.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <vector>

namespace scapms::data {

class ClinicDatabase {
public:
    using PatientPointer = std::shared_ptr<const models::PatientRecord>;
    static constexpr std::size_t recentPatientCacheCapacity = 10;

    struct CacheEntry {
        QString patientId;
        std::uint64_t lastAccess = 0;
        bool occupied = false;
    };

    const std::vector<PatientPointer>& patients() const noexcept { return m_patients; }
    const std::vector<models::Appointment>& appointments() const noexcept { return m_appointments; }

    PatientPointer findPatient(const QString& patientId) const noexcept;
    PatientPointer requirePatient(const QString& patientId) const;
    const models::Appointment* findAppointment(const QString& appointmentId) const noexcept;
    models::Appointment* findAppointment(const QString& appointmentId) noexcept;
    const models::Appointment& requireAppointment(const QString& appointmentId) const;
    models::Appointment& requireAppointment(const QString& appointmentId);

    void addPatient(PatientPointer patient);
    void replacePatient(const QString& patientId, PatientPointer replacement);
    void removePatientAndRelatedState(const QString& patientId);
    QString nextPatientId() const;

    void addAppointment(models::Appointment appointment);
    void removeAppointment(const QString& appointmentId);
    QString nextAppointmentId() const;

    void enqueuePatient(const QString& patientId);
    std::vector<QString> waitingQueue() const;
    std::optional<QString> callNextPatient();

    void recordPatientAccess(const QString& patientId);
    const std::array<CacheEntry, recentPatientCacheCapacity>& cacheEntries() const noexcept
    {
        return m_recentPatients;
    }
    bool isPatientCached(const QString& patientId) const noexcept;

    void pushUndoSnapshot();
    bool canUndo() const noexcept { return !m_undoSnapshots.empty(); }
    bool undoLastOperation();

    // Installs a separately validated database as one transaction. Session-only
    // undo snapshots and recent-access cache entries are deliberately cleared.
    void replaceWithValidatedState(ClinicDatabase&& validatedState);

private:
    struct Snapshot {
        std::vector<PatientPointer> patients;
        std::vector<models::Appointment> appointments;
        std::queue<QString> waitingQueue;
        std::array<CacheEntry, recentPatientCacheCapacity> recentPatients;
        std::uint64_t accessCounter = 0;
    };

    void invalidatePatientCache(const QString& patientId) noexcept;

    std::vector<PatientPointer> m_patients;
    std::vector<models::Appointment> m_appointments;
    std::queue<QString> m_waitingQueue;
    std::stack<Snapshot> m_undoSnapshots;
    std::array<CacheEntry, recentPatientCacheCapacity> m_recentPatients{};
    std::uint64_t m_accessCounter = 0;
    // High-water marks are intentionally not part of undo snapshots: undoing a
    // deletion or an insertion must never make an issued ID reusable.
    std::uint64_t m_patientIdSequence = 0;
    std::uint64_t m_appointmentIdSequence = 0;
};

} // namespace scapms::data
