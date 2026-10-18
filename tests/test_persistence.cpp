#include "controllers/appointment_controller.h"
#include "controllers/clinic_manager.h"
#include "controllers/clinic_reports.h"
#include "data/clinic_database.h"
#include "models/emergency_patient.h"
#include "models/follow_up_patient.h"
#include "models/general_patient.h"
#include "persistence/file_handler.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QtTest>

#include <memory>
#include <stdexcept>

using namespace scapms;

class PersistenceTests final : public QObject {
    Q_OBJECT
private:
    static models::ContactInformation contact(const QString& phone = "08000000001")
    { return {phone, "clinic@example.test"}; }
    static QDateTime at(int year, int month, int day, int hour)
    { return QDateTime(QDate(year, month, day), QTime(hour, 15), QTimeZone(QByteArrayLiteral("UTC"))); }
    static void populate(data::ClinicDatabase& db)
    {
        db.addPatient(std::make_shared<models::GeneralPatient>("P-000007", "Ada General", 35, contact()));
        db.addPatient(std::make_shared<models::EmergencyPatient>("P-000009", "Eve Emergency", 42, contact("08000000002")));
        db.addPatient(std::make_shared<models::FollowUpPatient>("P-000008", "Uma Follow", 28,
                                                                contact("08000000003"), "Post-surgery review"));
        db.addAppointment(models::Appointment::restore("A-000003", "P-000007", at(2020, 1, 4, 9),
                                                        models::AppointmentStatus::Pending, "Old pending visit"));
        db.addAppointment(models::Appointment::restore("A-000006", "P-000009", at(2020, 1, 5, 10),
                                                        models::AppointmentStatus::Completed, "Completed visit"));
        db.addAppointment(models::Appointment::restore("A-000009", "P-000008", at(2020, 1, 6, 11),
                                                        models::AppointmentStatus::Cancelled, "Cancelled visit"));
        db.enqueuePatient("P-000008");
        db.enqueuePatient("P-000007");
    }
    static bool writeBytes(const QString& path, const QByteArray& bytes)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;
        return file.write(bytes) == bytes.size();
    }
    static bool writeJson(const QString& path, const QJsonObject& object)
    { return writeBytes(path, QJsonDocument(object).toJson(QJsonDocument::Compact)); }
    static QJsonObject readJson(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        return QJsonDocument::fromJson(file.readAll()).object();
    }

private slots:
    void loadedAttendanceFixtureRanksCompletedVisits()
    {
        const QString fixturePath = QFINDTESTDATA("fixtures/most_frequently_attended.json");
        QVERIFY2(!fixturePath.isEmpty(), "Attendance report fixture was not found.");

        data::ClinicDatabase database;
        const auto loaded = persistence::FileHandler::load(database, fixturePath);
        QVERIFY2(loaded.success, qPrintable(loaded.message));
        QCOMPARE(database.patients().size(), std::size_t(3));
        QCOMPARE(database.appointments().size(), std::size_t(3));
        QCOMPARE(database.waitingQueue(), std::vector<QString>({"P-000002", "P-000003"}));

        const auto report = controllers::ClinicReports::mostFrequentlyAttended(database);
        QCOMPARE(report.size(), std::size_t(2)); // Zainab has no completed appointment and is omitted.
        QCOMPARE(report[0].patient->fullName(), QStringLiteral("Ada Okafor"));
        QCOMPARE(report[0].completedAppointments, std::size_t(2));
        QCOMPARE(report[1].patient->fullName(), QStringLiteral("Emeka Obi"));
        QCOMPARE(report[1].completedAppointments, std::size_t(1));
        QVERIFY(report[0].completedAppointments > report[1].completedAppointments);
        QCOMPARE(database.requirePatient(QStringLiteral("P-000003"))->category(), models::PatientCategory::FollowUp);
    }

    void savesAndLoadsCompleteClinicDataset()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.filePath("clinic.json");
        data::ClinicDatabase source; populate(source);
        const auto saved = persistence::FileHandler::save(source, path);
        QVERIFY2(saved.success, qPrintable(saved.message));
        const auto json = readJson(path);
        QCOMPARE(json.value("schemaVersion").toInt(), persistence::FileHandler::currentSchemaVersion);
        QCOMPARE(json.value("patients").toArray().size(), 3);
        QCOMPARE(json.value("appointments").toArray().size(), 3);
        QCOMPARE(json.value("waitingQueue").toArray().size(), 2);

        data::ClinicDatabase loaded;
        const auto result = persistence::FileHandler::load(loaded, path);
        QVERIFY2(result.success, qPrintable(result.message));
        QCOMPARE(loaded.patients().size(), std::size_t(3));
        QCOMPARE(loaded.appointments().size(), std::size_t(3));
        QCOMPARE(loaded.waitingQueue(), std::vector<QString>({"P-000008", "P-000007"}));
        QCOMPARE(loaded.requirePatient("P-000007")->category(), models::PatientCategory::General);
        QCOMPARE(loaded.requirePatient("P-000009")->category(), models::PatientCategory::Emergency);
        QCOMPARE(loaded.requirePatient("P-000008")->category(), models::PatientCategory::FollowUp);
        const auto* followUp = dynamic_cast<const models::FollowUpPatient*>(loaded.findPatient("P-000008").get());
        QVERIFY(followUp);
        QCOMPARE(followUp->followUpReason(), QString("Post-surgery review"));
        QCOMPARE(loaded.requireAppointment("A-000003").status(), models::AppointmentStatus::Pending);
        QCOMPARE(loaded.requireAppointment("A-000003").scheduledAt(), at(2020, 1, 4, 9));
        QCOMPARE(loaded.requireAppointment("A-000003").notes(), QString("Old pending visit"));
        QCOMPARE(loaded.requireAppointment("A-000006").status(), models::AppointmentStatus::Completed);
        QCOMPARE(loaded.requireAppointment("A-000009").status(), models::AppointmentStatus::Cancelled);
    }

    void rejectsDuplicateIdsAndInvalidReferences()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        data::ClinicDatabase source; populate(source);
        const QString path = directory.filePath("input.json");
        QVERIFY(persistence::FileHandler::save(source, path));
        auto json = readJson(path);
        auto patients = json.value("patients").toArray();
        patients.append(patients.at(0));
        json.insert("patients", patients);
        QVERIFY(writeJson(path, json));
        data::ClinicDatabase target;
        auto result = persistence::FileHandler::load(target, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("already exists"));
        QVERIFY(target.patients().empty());

        json = readJsonFromFixture(source, path);
        auto appointments = json.value("appointments").toArray();
        QJsonObject first = appointments.at(0).toObject();
        first.insert("appointmentId", appointments.at(1).toObject().value("appointmentId"));
        appointments[0] = first; json.insert("appointments", appointments);
        QVERIFY(writeJson(path, json));
        result = persistence::FileHandler::load(target, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("already exists"));

        json = readJsonFromFixture(source, path);
        appointments = json.value("appointments").toArray();
        first = appointments.at(0).toObject(); first.insert("patientId", "P-MISSING");
        appointments[0] = first; json.insert("appointments", appointments);
        QVERIFY(writeJson(path, json));
        result = persistence::FileHandler::load(target, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("unregistered patient"));

        json = readJsonFromFixture(source, path);
        auto queue = json.value("waitingQueue").toArray(); queue.append(queue.at(0));
        json.insert("waitingQueue", queue); QVERIFY(writeJson(path, json));
        result = persistence::FileHandler::load(target, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("already in the waiting queue"));

        json = readJsonFromFixture(source, path);
        queue = json.value("waitingQueue").toArray(); queue[0] = "P-MISSING";
        json.insert("waitingQueue", queue); QVERIFY(writeJson(path, json));
        result = persistence::FileHandler::load(target, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("unregistered patient"));
    }

    void rejectsInvalidSubtypeFieldsStatusesAndDates()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        data::ClinicDatabase source; populate(source);
        const QString path = directory.filePath("invalid.json");
        QVERIFY(persistence::FileHandler::save(source, path));
        data::ClinicDatabase target;
        auto json = readJson(path);
        auto patients = json.value("patients").toArray();
        QJsonObject patient = patients.at(0).toObject(); patient.insert("followUpReason", "wrong subtype");
        patients[0] = patient; json.insert("patients", patients); QVERIFY(writeJson(path, json));
        QVERIFY(!persistence::FileHandler::load(target, path).success);

        json = readJson(path); patients = json.value("patients").toArray();
        patient = patients.at(2).toObject(); patient.remove("followUpReason");
        patients[2] = patient; json.insert("patients", patients); QVERIFY(writeJson(path, json));
        QVERIFY(!persistence::FileHandler::load(target, path).success);

        json = readJsonFromFixture(source, path);
        auto appointments = json.value("appointments").toArray();
        QJsonObject appointment = appointments.at(0).toObject(); appointment.insert("status", "InProgress");
        appointments[0] = appointment; json.insert("appointments", appointments); QVERIFY(writeJson(path, json));
        QVERIFY(!persistence::FileHandler::load(target, path).success);

        json = readJsonFromFixture(source, path); appointments = json.value("appointments").toArray();
        appointment = appointments.at(0).toObject(); appointment.insert("scheduledAt", "2020-99-88T47:95:00Z");
        appointments[0] = appointment; json.insert("appointments", appointments); QVERIFY(writeJson(path, json));
        QVERIFY(!persistence::FileHandler::load(target, path).success);

        json = readJsonFromFixture(source, path); patients = json.value("patients").toArray();
        patient = patients.at(0).toObject(); patient.remove("phoneNumber");
        patients[0] = patient; json.insert("patients", patients); QVERIFY(writeJson(path, json));
        const auto missingField = persistence::FileHandler::load(target, path);
        QVERIFY(!missingField.success); QVERIFY(missingField.message.contains("phoneNumber"));

        json = readJsonFromFixture(source, path); patients = json.value("patients").toArray();
        patient = patients.at(0).toObject(); patient.insert("category", "UnknownCategory");
        patients[0] = patient; json.insert("patients", patients); QVERIFY(writeJson(path, json));
        QVERIFY(!persistence::FileHandler::load(target, path).success);
    }

    void malformedJsonAndUnsupportedSchemaAreRejectedTransactionally()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.filePath("broken.json");
        data::ClinicDatabase active; active.addPatient(std::make_shared<models::GeneralPatient>("KEEP", "Keep Me", 20, contact()));
        active.enqueuePatient("KEEP"); active.recordPatientAccess("KEEP"); active.pushUndoSnapshot();
        QVERIFY(writeBytes(path, "{ malformed"));
        auto result = persistence::FileHandler::load(active, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("JSON"));
        QCOMPARE(active.patients().size(), std::size_t(1));
        QCOMPARE(active.waitingQueue(), std::vector<QString>({"KEEP"}));
        QVERIFY(active.isPatientCached("KEEP")); QVERIFY(active.canUndo());

        QJsonObject unsupported{{"schemaVersion", 999}, {"patients", QJsonArray{}},
                                {"appointments", QJsonArray{}}, {"waitingQueue", QJsonArray{}}};
        QVERIFY(writeJson(path, unsupported));
        result = persistence::FileHandler::load(active, path);
        QVERIFY(!result.success); QVERIFY(result.message.contains("Unsupported schema version"));
        QCOMPARE(active.requirePatient("KEEP")->fullName(), QString("Keep Me"));
        QVERIFY(active.isPatientCached("KEEP")); QVERIFY(active.canUndo());
    }

    void missingAndUnwritablePathsReturnMeaningfulErrors()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        data::ClinicDatabase db;
        auto missing = persistence::FileHandler::load(db, directory.filePath("missing.json"));
        QVERIFY(!missing.success); QVERIFY(missing.message.contains("Could not open"));
        auto unwritable = persistence::FileHandler::save(db, directory.filePath("no-parent/clinic.json"));
        QVERIFY(!unwritable.success); QVERIFY(unwritable.message.contains("Could not open"));
    }

    void successfulLoadClearsUndoCacheAndGeneratesCollisionFreeIds()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.filePath("valid.json");
        data::ClinicDatabase source; populate(source);
        QVERIFY(persistence::FileHandler::save(source, path));

        data::ClinicDatabase active; controllers::ClinicManager manager(active);
        controllers::AppointmentController appointments(active);
        manager.registerPatient(std::make_shared<models::GeneralPatient>("OLD", "Old", 25, contact()));
        manager.getPatient("OLD");
        QVERIFY(active.canUndo()); QVERIFY(active.isPatientCached("OLD"));
        const auto loaded = persistence::FileHandler::load(active, path);
        QVERIFY2(loaded.success, qPrintable(loaded.message));
        QVERIFY(!active.canUndo());
        for (const auto& entry : active.cacheEntries()) QVERIFY(!entry.occupied);
        QVERIFY(!manager.undoLastAdministrativeOperation());
        QCOMPARE(manager.nextPatientId(), QString("P-000010"));
        QCOMPARE(appointments.nextAppointmentId(), QString("A-000010"));

        manager.deletePatient("P-000009");
        QCOMPARE(manager.nextPatientId(), QString("P-000010"));
        QVERIFY(manager.undoLastAdministrativeOperation());
        QCOMPARE(manager.nextPatientId(), QString("P-000010"));
        appointments.deleteAppointment("A-000009");
        QCOMPARE(appointments.nextAppointmentId(), QString("A-000010"));
        QVERIFY(manager.undoLastAdministrativeOperation());
        QCOMPARE(appointments.getAppointment("A-000009").status(), models::AppointmentStatus::Cancelled);

        manager.registerPatient(std::make_shared<models::GeneralPatient>(manager.nextPatientId(), "New", 30, contact()));
        appointments.scheduleAppointment(appointments.nextAppointmentId(), "P-000010", at(2040, 2, 1, 9), {}, at(2039, 1, 1, 9));
        QCOMPARE(active.patients().size(), std::size_t(4));
        QCOMPARE(active.appointments().size(), std::size_t(4));
    }

private:
    static QJsonObject readJsonFromFixture(data::ClinicDatabase& source, const QString& path)
    {
        Q_UNUSED(source);
        // Restore a clean valid fixture before each independent mutation.
        data::ClinicDatabase clean;
        populate(clean);
        const auto result = persistence::FileHandler::save(clean, path);
        if (!result.success) return {};
        return readJson(path);
    }
};

QTEST_GUILESS_MAIN(PersistenceTests)
#include "test_persistence.moc"
