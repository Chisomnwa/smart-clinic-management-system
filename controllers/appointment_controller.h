#pragma once

#include "data/clinic_database.h"

namespace scapms::controllers {

class AppointmentController {
public:
    explicit AppointmentController(data::ClinicDatabase& database) : m_database(database) {}

    QString nextAppointmentId() const { return m_database.nextAppointmentId(); }
    models::Appointment scheduleAppointment(QString appointmentId, const QString& patientId,
                                           QDateTime when, QString notes = {},
                                           QDateTime now = QDateTime::currentDateTime());
    void cancelAppointment(const QString& appointmentId);
    void completeAppointment(const QString& appointmentId,
                             QDateTime now = QDateTime::currentDateTime());
    void deleteAppointment(const QString& appointmentId);
    models::Appointment getAppointment(const QString& appointmentId) const;
    std::vector<models::Appointment> listAppointments() const;

private:
    data::ClinicDatabase& m_database;
};

} // namespace scapms::controllers
