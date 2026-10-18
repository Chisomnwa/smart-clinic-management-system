#pragma once

#include "models/appointment_status.h"

#include <QDateTime>
#include <QString>

namespace scapms::models {

class Appointment {
public:
    static Appointment schedule(QString appointmentId,
                                QString patientId,
                                QDateTime scheduledAt,
                                QString notes = {},
                                QDateTime now = QDateTime::currentDateTime());

    // Restoring persisted appointments validates their structure and status,
    // but does not apply the future-date rule used for new appointments.
    static Appointment restore(QString appointmentId,
                               QString patientId,
                               QDateTime scheduledAt,
                               AppointmentStatus status,
                               QString notes = {});

    const QString& appointmentId() const noexcept { return m_appointmentId; }
    const QString& patientId() const noexcept { return m_patientId; }
    const QDateTime& scheduledAt() const noexcept { return m_scheduledAt; }
    AppointmentStatus status() const noexcept { return m_status; }
    QString statusLabel() const { return appointmentStatusName(m_status); }
    const QString& notes() const noexcept { return m_notes; }

    void validateScheduling(const QDateTime& now) const;
    void cancel();
    void markCompleted(const QDateTime& now = QDateTime::currentDateTime());

private:
    Appointment(QString appointmentId,
                QString patientId,
                QDateTime scheduledAt,
                AppointmentStatus status,
                QString notes);

    void validateCommonFields() const;

    QString m_appointmentId;
    QString m_patientId;
    QDateTime m_scheduledAt;
    AppointmentStatus m_status;
    QString m_notes;
};

} // namespace scapms::models
