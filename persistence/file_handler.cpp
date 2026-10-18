#include "persistence/file_handler.h"

#include "models/emergency_patient.h"
#include "models/follow_up_patient.h"
#include "models/general_patient.h"
#include "utils/domain_exception.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace scapms::persistence {
namespace {

QString categoryToken(models::PatientCategory category)
{
    switch (category) {
    case models::PatientCategory::General: return QStringLiteral("General");
    case models::PatientCategory::Emergency: return QStringLiteral("Emergency");
    case models::PatientCategory::FollowUp: return QStringLiteral("FollowUp");
    }
    throw utils::DomainException("Cannot save an unknown patient category.");
}

models::PatientCategory parseCategory(const QString& token)
{
    if (token == QStringLiteral("General")) return models::PatientCategory::General;
    if (token == QStringLiteral("Emergency")) return models::PatientCategory::Emergency;
    if (token == QStringLiteral("FollowUp")) return models::PatientCategory::FollowUp;
    throw utils::DomainException("Unsupported patient category: " + token.toStdString());
}

QString statusToken(models::AppointmentStatus status)
{
    switch (status) {
    case models::AppointmentStatus::Pending: return QStringLiteral("Pending");
    case models::AppointmentStatus::Completed: return QStringLiteral("Completed");
    case models::AppointmentStatus::Cancelled: return QStringLiteral("Cancelled");
    }
    throw utils::DomainException("Cannot save an unknown appointment status.");
}

models::AppointmentStatus parseStatus(const QString& token)
{
    if (token == QStringLiteral("Pending")) return models::AppointmentStatus::Pending;
    if (token == QStringLiteral("Completed")) return models::AppointmentStatus::Completed;
    if (token == QStringLiteral("Cancelled")) return models::AppointmentStatus::Cancelled;
    throw utils::DomainException("Unsupported appointment status: " + token.toStdString());
}

QString requiredString(const QJsonObject& object, const QString& key, const QString& context)
{
    const auto value = object.value(key);
    if (!value.isString()) {
        throw utils::DomainException((context + QStringLiteral(" requires string field '") + key + "'.").toStdString());
    }
    return value.toString();
}

int requiredInteger(const QJsonObject& object, const QString& key, const QString& context)
{
    const auto value = object.value(key);
    if (!value.isDouble()) {
        throw utils::DomainException((context + QStringLiteral(" requires integer field '") + key + "'.").toStdString());
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number ||
        number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max()) {
        throw utils::DomainException((context + QStringLiteral(" field '") + key + "' must be an integer.").toStdString());
    }
    return static_cast<int>(number);
}

QJsonObject patientToJson(const models::PatientRecord& patient)
{
    QJsonObject object{
        {QStringLiteral("patientId"), patient.patientId()},
        {QStringLiteral("fullName"), patient.fullName()},
        {QStringLiteral("age"), patient.age()},
        {QStringLiteral("category"), categoryToken(patient.category())},
        {QStringLiteral("phoneNumber"), patient.contactInformation().phoneNumber},
        {QStringLiteral("email"), patient.contactInformation().email}
    };
    if (patient.category() == models::PatientCategory::FollowUp) {
        const auto* followUp = dynamic_cast<const models::FollowUpPatient*>(&patient);
        if (!followUp) throw utils::DomainException("Follow-Up category does not match its patient subtype.");
        object.insert(QStringLiteral("followUpReason"), followUp->followUpReason());
    }
    return object;
}

data::ClinicDatabase::PatientPointer patientFromJson(const QJsonObject& object, int index)
{
    const QString context = QStringLiteral("Patient at index %1").arg(index);
    const QString id = requiredString(object, QStringLiteral("patientId"), context);
    const QString name = requiredString(object, QStringLiteral("fullName"), context);
    const int age = requiredInteger(object, QStringLiteral("age"), context);
    const auto category = parseCategory(requiredString(object, QStringLiteral("category"), context));
    const auto emailValue = object.value(QStringLiteral("email"));
    if (!emailValue.isUndefined() && !emailValue.isString())
        throw utils::DomainException((context + QStringLiteral(" field 'email' must be a string when provided.")).toStdString());
    models::ContactInformation contact{
        requiredString(object, QStringLiteral("phoneNumber"), context),
        emailValue.toString()
    };
    switch (category) {
    case models::PatientCategory::General:
        if (object.contains(QStringLiteral("followUpReason")))
            throw utils::DomainException("General patient must not contain followUpReason.");
        return std::make_shared<models::GeneralPatient>(id, name, age, contact);
    case models::PatientCategory::Emergency:
        if (object.contains(QStringLiteral("followUpReason")))
            throw utils::DomainException("Emergency patient must not contain followUpReason.");
        return std::make_shared<models::EmergencyPatient>(id, name, age, contact);
    case models::PatientCategory::FollowUp: {
        const QString reason = requiredString(object, QStringLiteral("followUpReason"), context);
        return std::make_shared<models::FollowUpPatient>(id, name, age, contact, reason);
    }
    }
    throw utils::DomainException("Patient category is not recognized.");
}

QJsonObject appointmentToJson(const models::Appointment& appointment)
{
    return {
        {QStringLiteral("appointmentId"), appointment.appointmentId()},
        {QStringLiteral("patientId"), appointment.patientId()},
        {QStringLiteral("scheduledAt"), appointment.scheduledAt().toString(Qt::ISODateWithMs)},
        {QStringLiteral("status"), statusToken(appointment.status())},
        {QStringLiteral("notes"), appointment.notes()}
    };
}

models::Appointment appointmentFromJson(const QJsonObject& object, int index)
{
    const QString context = QStringLiteral("Appointment at index %1").arg(index);
    const QString id = requiredString(object, QStringLiteral("appointmentId"), context);
    const QString patientId = requiredString(object, QStringLiteral("patientId"), context);
    const QString timestamp = requiredString(object, QStringLiteral("scheduledAt"), context);
    const QDateTime scheduledAt = QDateTime::fromString(timestamp, Qt::ISODateWithMs);
    if (!scheduledAt.isValid()) {
        throw utils::DomainException((context + QStringLiteral(" has an invalid ISO date/time.")).toStdString());
    }
    const auto status = parseStatus(requiredString(object, QStringLiteral("status"), context));
    const QString notes = requiredString(object, QStringLiteral("notes"), context);
    // restore intentionally permits historical Pending appointments.
    return models::Appointment::restore(id, patientId, scheduledAt, status, notes);
}

data::ClinicDatabase decodeAndValidate(const QJsonObject& root)
{
    const auto versionValue = root.value(QStringLiteral("schemaVersion"));
    if (!versionValue.isDouble() || std::floor(versionValue.toDouble()) != versionValue.toDouble())
        throw utils::DomainException("schemaVersion must be an integer.");
    const int version = versionValue.toInt(-1);
    if (version != FileHandler::currentSchemaVersion)
        throw utils::DomainException(QStringLiteral("Unsupported schema version %1; supported version is %2.")
                                     .arg(version).arg(FileHandler::currentSchemaVersion).toStdString());

    const auto patientsValue = root.value(QStringLiteral("patients"));
    const auto appointmentsValue = root.value(QStringLiteral("appointments"));
    const auto queueValue = root.value(QStringLiteral("waitingQueue"));
    if (!patientsValue.isArray() || !appointmentsValue.isArray() || !queueValue.isArray())
        throw utils::DomainException("Clinic data must contain patients, appointments, and waitingQueue arrays.");

    data::ClinicDatabase candidate;
    int index = 0;
    for (const auto value : patientsValue.toArray()) {
        if (!value.isObject()) throw utils::DomainException("Each patient entry must be a JSON object.");
        candidate.addPatient(patientFromJson(value.toObject(), index++));
    }
    index = 0;
    for (const auto value : appointmentsValue.toArray()) {
        if (!value.isObject()) throw utils::DomainException("Each appointment entry must be a JSON object.");
        candidate.addAppointment(appointmentFromJson(value.toObject(), index++));
    }
    for (const auto value : queueValue.toArray()) {
        if (!value.isString()) throw utils::DomainException("Each waitingQueue entry must be a patient ID string.");
        candidate.enqueuePatient(value.toString());
    }
    return candidate;
}

} // namespace

PersistenceResult FileHandler::save(const data::ClinicDatabase& database, const QString& filePath)
{
    try {
    QJsonArray patients;
    for (const auto& patient : database.patients()) {
        if (!patient) return {false, QStringLiteral("Cannot save an empty patient record.")};
        patients.append(patientToJson(*patient));
    }
    QJsonArray appointments;
    for (const auto& appointment : database.appointments())
        appointments.append(appointmentToJson(appointment));
    QJsonArray waitingQueue;
    for (const auto& patientId : database.waitingQueue()) waitingQueue.append(patientId);
    const QJsonObject root{
        {QStringLiteral("schemaVersion"), currentSchemaVersion},
        {QStringLiteral("patients"), patients},
        {QStringLiteral("appointments"), appointments},
        {QStringLiteral("waitingQueue"), waitingQueue}
    };

    QSaveFile file(filePath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return {false, QStringLiteral("Could not open '%1' for saving: %2").arg(filePath, file.errorString())};
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        const QString error = file.errorString();
        file.cancelWriting();
        return {false, QStringLiteral("Could not write '%1': %2").arg(filePath, error)};
    }
    if (!file.commit())
        return {false, QStringLiteral("Could not atomically commit '%1': %2").arg(filePath, file.errorString())};
    return {true, QStringLiteral("Clinic data saved to '%1'.").arg(filePath)};
    } catch (const std::exception& error) {
        return {false, QStringLiteral("Could not serialize clinic data: %1")
                       .arg(QString::fromUtf8(error.what()))};
    }
}

PersistenceResult FileHandler::load(data::ClinicDatabase& database, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {false, QStringLiteral("Could not open '%1' for loading: %2").arg(filePath, file.errorString())};
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError)
        return {false, QStringLiteral("Could not read '%1': %2").arg(filePath, file.errorString())};
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return {false, QStringLiteral("Invalid clinic JSON in '%1' at byte %2: %3")
                       .arg(filePath).arg(parseError.offset).arg(parseError.errorString())};
    }
    if (!document.isObject())
        return {false, QStringLiteral("Invalid clinic JSON in '%1': the top-level value must be an object.")
                       .arg(filePath)};
    try {
        auto candidate = decodeAndValidate(document.object());
        database.replaceWithValidatedState(std::move(candidate));
        return {true, QStringLiteral("Clinic data loaded from '%1'.").arg(filePath)};
    } catch (const std::exception& error) {
        return {false, QStringLiteral("Clinic data in '%1' is invalid: %2")
                       .arg(filePath, QString::fromUtf8(error.what()))};
    }
}

} // namespace scapms::persistence
