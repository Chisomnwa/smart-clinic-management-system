#include "controllers/appointment_controller.h"
#include "controllers/clinic_manager.h"
#include "controllers/clinic_reports.h"
#include "controllers/patient_algorithms.h"
#include "data/clinic_database.h"
#include "models/emergency_patient.h"
#include "models/follow_up_patient.h"
#include "models/general_patient.h"

#include <QtTest>
#include <QTimeZone>

#include <memory>
#include <stdexcept>

using namespace scapms;

class Phase3Tests final : public QObject {
    Q_OBJECT
private:
    using Pointer = data::ClinicDatabase::PatientPointer;
    static models::ContactInformation contact() { return {"08012345678", "test@example.test"}; }
    static QDateTime at(const QDate& date, const QTime& time)
    { return QDateTime(date, time, QTimeZone(QByteArrayLiteral("UTC"))); }
    static QDateTime future(int days = 30) { return at(QDate(2035, 4, 20).addDays(days), QTime(10, 0)); }
    static QDateTime now() { return at(QDate(2035, 4, 1), QTime(10, 0)); }
    static Pointer general(QString id, QString name, int age = 30)
    { return std::make_shared<models::GeneralPatient>(std::move(id), std::move(name), age, contact()); }
    static Pointer emergency(QString id, QString name, int age = 30)
    { return std::make_shared<models::EmergencyPatient>(std::move(id), std::move(name), age, contact()); }
    static Pointer followUp(QString id, QString name, int age = 30)
    { return std::make_shared<models::FollowUpPatient>(std::move(id), std::move(name), age, contact(), "Review"); }

private slots:
    void patientCrudAndUndo()
    {
        data::ClinicDatabase db; controllers::ClinicManager manager(db);
        manager.registerPatient(general("P1", "Ada"));
        QCOMPARE(manager.getPatient("P1")->fullName(), QString("Ada"));
        QVERIFY(db.isPatientCached("P1"));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.registerPatient(general("P1", "Duplicate")));
        manager.editPatient("P1", "Ada Lovelace", 31, contact());
        QCOMPARE(manager.getPatient("P1")->age(), 31);
        QVERIFY(manager.undoLastAdministrativeOperation());
        QCOMPARE(manager.getPatient("P1")->fullName(), QString("Ada"));
        manager.deletePatient("P1");
        QVERIFY(!db.findPatient("P1"));
        QVERIFY(manager.undoLastAdministrativeOperation());
        QVERIFY(db.findPatient("P1"));
    }

    void appointmentsAndTransitions()
    {
        data::ClinicDatabase db; controllers::ClinicManager patients(db);
        controllers::AppointmentController appointments(db);
        patients.registerPatient(general("P1", "Ada"));
        auto scheduled = appointments.scheduleAppointment("A1", "P1", future(), {}, now());
        QCOMPARE(scheduled.status(), models::AppointmentStatus::Pending);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            appointments.scheduleAppointment("A2", "P1", now().addSecs(-1), {}, now()));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            appointments.scheduleAppointment("A1", "P1", future(2), {}, now()));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            appointments.scheduleAppointment("A3", "missing", future(), {}, now()));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            appointments.completeAppointment("A1", now()));
        appointments.cancelAppointment("A1");
        QCOMPARE(appointments.getAppointment("A1").status(), models::AppointmentStatus::Cancelled);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, appointments.cancelAppointment("A1"));
        QVERIFY(patients.undoLastAdministrativeOperation());
        QCOMPARE(appointments.getAppointment("A1").status(), models::AppointmentStatus::Pending);
        appointments.completeAppointment("A1", future());
        QCOMPARE(appointments.getAppointment("A1").status(), models::AppointmentStatus::Completed);
        appointments.deleteAppointment("A1");
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, appointments.getAppointment("A1"));
        QVERIFY(patients.undoLastAdministrativeOperation());
        QCOMPARE(appointments.getAppointment("A1").status(), models::AppointmentStatus::Completed);
    }

    void queueIsFifoAndDeletionCleansReferences()
    {
        data::ClinicDatabase db; controllers::ClinicManager manager(db);
        manager.registerPatient(general("P1", "One")); manager.registerPatient(emergency("P2", "Two"));
        manager.registerPatient(general("P3", "Three"));
        QCOMPARE(db.requirePatient("P2")->priorityLabel(), QString("High Priority"));
        manager.enqueuePatient("P1"); manager.enqueuePatient("P2"); manager.enqueuePatient("P3");
        QCOMPARE(manager.waitingQueue(), std::vector<QString>({"P1", "P2", "P3"}));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.enqueuePatient("P2"));
        QCOMPARE(manager.callNextPatient().value(), QString("P1"));
        QCOMPARE(manager.callNextPatient().value(), QString("P2"));
        QCOMPARE(manager.callNextPatient().value(), QString("P3"));
        QVERIFY(!manager.callNextPatient().has_value());
        appointmentsSetup(db);
        manager.enqueuePatient("P3"); manager.deletePatient("P3");
        QVERIFY(manager.waitingQueue().empty());
        QVERIFY(db.appointments().empty());
        QVERIFY(manager.undoLastAdministrativeOperation());
        QCOMPARE(manager.waitingQueue(), std::vector<QString>({"P3"}));
        QCOMPARE(db.appointments().size(), std::size_t(1));
    }

    void queueOperationsJoinUndoHistoryAndNoOpsDoNot()
    {
        data::ClinicDatabase db;
        controllers::ClinicManager manager(db);

        QVERIFY(!manager.callNextPatient().has_value());
        QVERIFY(!db.canUndo()); // Empty call-next made no change.
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.enqueuePatient("MISSING"));
        QVERIFY(!db.canUndo()); // Invalid enqueue made no change.

        manager.registerPatient(general("P-000001", "Ada"));
        manager.enqueuePatient("P-000001");
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.enqueuePatient("P-000001"));

        // Exactly one successful queue mutation followed registration. The
        // failed duplicate must not add another snapshot.
        QVERIFY(manager.undoLastAdministrativeOperation());
        QVERIFY(manager.waitingQueue().empty());
        QVERIFY(db.findPatient("P-000001"));
        QVERIFY(manager.undoLastAdministrativeOperation());
        QVERIFY(!db.findPatient("P-000001"));
        QVERIFY(!manager.undoLastAdministrativeOperation());
    }

    void mixedPatientAppointmentAndQueueOperationsUndoInReverseOrder()
    {
        data::ClinicDatabase db;
        controllers::ClinicManager patients(db);
        controllers::AppointmentController appointments(db);

        patients.registerPatient(general("P-000001", "Ada"));
        appointments.scheduleAppointment("A-000001", "P-000001", future(), "Review", now());
        patients.enqueuePatient("P-000001");
        patients.editPatient("P-000001", "Ada Updated", 31, contact());
        QCOMPARE(patients.getPatient("P-000001")->fullName(), QString("Ada Updated"));
        QCOMPARE(patients.callNextPatient().value(), QString("P-000001"));
        QVERIFY(patients.waitingQueue().empty());

        // Each undo restores the complete state immediately before the most
        // recent successful mutation, including related records and FIFO data.
        QVERIFY(patients.undoLastAdministrativeOperation()); // call-next
        QCOMPARE(patients.waitingQueue(), std::vector<QString>({"P-000001"}));
        QCOMPARE(db.requirePatient("P-000001")->fullName(), QString("Ada Updated"));

        QVERIFY(patients.undoLastAdministrativeOperation()); // patient edit
        QCOMPARE(db.requirePatient("P-000001")->fullName(), QString("Ada"));
        QCOMPARE(db.requireAppointment("A-000001").status(), models::AppointmentStatus::Pending);
        QCOMPARE(patients.waitingQueue(), std::vector<QString>({"P-000001"}));

        QVERIFY(patients.undoLastAdministrativeOperation()); // enqueue
        QVERIFY(patients.waitingQueue().empty());
        QCOMPARE(db.appointments().size(), std::size_t(1));

        QVERIFY(patients.undoLastAdministrativeOperation()); // schedule
        QVERIFY(db.appointments().empty());
        QVERIFY(db.findPatient("P-000001"));

        QVERIFY(patients.undoLastAdministrativeOperation()); // registration
        QVERIFY(db.patients().empty());
        QVERIFY(!patients.undoLastAdministrativeOperation());

        // Undo must not make issued identifiers reusable.
        QCOMPARE(patients.nextPatientId(), QString("P-000002"));
        QCOMPARE(appointments.nextAppointmentId(), QString("A-000002"));
    }

    void cacheCapacityReplacementAndInvalidation()
    {
        data::ClinicDatabase db; controllers::ClinicManager manager(db);
        for (int i = 0; i < 11; ++i) {
            const auto id = QString("P%1").arg(i);
            manager.registerPatient(general(id, id));
        }
        for (int i = 0; i < 10; ++i) manager.getPatient(QString("P%1").arg(i));
        manager.getPatient("P10");
        std::size_t occupied = 0;
        for (const auto& entry : db.cacheEntries()) if (entry.occupied) ++occupied;
        QCOMPARE(occupied, data::ClinicDatabase::recentPatientCacheCapacity);
        QVERIFY(!db.isPatientCached("P0"));
        QVERIFY(db.isPatientCached("P10"));
        manager.editPatient("P10", "Updated", 30, contact());
        QVERIFY(!db.isPatientCached("P10"));
        manager.getPatient("P10");
        manager.deletePatient("P10");
        QVERIFY(!db.isPatientCached("P10"));
    }

    void searchesAndInputValidation()
    {
        std::vector<Pointer> records{general("P3", "Carol Jones", 40), emergency("P1", "Ada Smith", 20),
                                     followUp("P2", "Ada Lovelace", 60)};
        QCOMPARE(controllers::PatientAlgorithms::linearSearchById(records, "P1")->fullName(), QString("Ada Smith"));
        QVERIFY(!controllers::PatientAlgorithms::linearSearchById(records, "P404"));
        auto sorted = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Id,
                                                            controllers::SortAlgorithm::Insertion);
        QCOMPARE(controllers::PatientAlgorithms::binarySearchById(sorted, "P2")->fullName(), QString("Ada Lovelace"));
        QVERIFY(!controllers::PatientAlgorithms::binarySearchById(sorted, "P404"));
        auto nameMatches = controllers::PatientAlgorithms::recursiveSearchByName(records, "ada");
        QCOMPARE(nameMatches.size(), std::size_t(2));
        QCOMPARE(controllers::PatientAlgorithms::searchByCategory(records, models::PatientCategory::FollowUp).size(), std::size_t(1));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controllers::PatientAlgorithms::linearSearchById(records, " "));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controllers::PatientAlgorithms::recursiveSearchByName(records, " "));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            controllers::PatientAlgorithms::searchByCategory(records, static_cast<models::PatientCategory>(99)));
        auto wrongSort = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Age,
                                                               controllers::SortAlgorithm::Selection);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controllers::PatientAlgorithms::binarySearchById(wrongSort, "P1"));
        auto descending = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Id,
                                                               controllers::SortAlgorithm::Selection, false);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controllers::PatientAlgorithms::binarySearchById(descending, "P1"));
    }

    void bothManualSortsCoverEveryFieldAndDirection()
    {
        std::vector<Pointer> records{general("P3", "Zoe", 40), emergency("P1", "Ada", 20),
                                     followUp("P2", "Mia", 30)};
        for (const auto algorithm : {controllers::SortAlgorithm::Insertion, controllers::SortAlgorithm::Selection}) {
            auto names = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Name, algorithm);
            QCOMPARE(names.records()[0]->fullName(), QString("Ada"));
            auto ids = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Id, algorithm);
            QCOMPARE(ids.records()[0]->patientId(), QString("P1"));
            auto ages = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Age, algorithm);
            QCOMPARE(ages.records()[0]->age(), 20);
            auto descending = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Age, algorithm, false);
            QCOMPARE(descending.records()[0]->age(), 40);
        }
        std::vector<Pointer> empty;
        QCOMPARE(controllers::PatientAlgorithms::sort(empty, controllers::PatientSortField::Id,
            controllers::SortAlgorithm::Insertion).records().size(), std::size_t(0));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            controllers::PatientAlgorithms::sort(records, static_cast<controllers::PatientSortField>(99),
                                                 controllers::SortAlgorithm::Selection));
    }

    void allReportsReflectStoredRecords()
    {
        data::ClinicDatabase db; controllers::ClinicManager manager(db);
        manager.registerPatient(general("P1", "Ada")); manager.registerPatient(emergency("P2", "Bob"));
        manager.registerPatient(followUp("P3", "Cyd"));
        const auto date = QDate(2035, 5, 2);
        db.addAppointment(models::Appointment::restore("A1", "P1", at(date, QTime(9, 0)), models::AppointmentStatus::Completed));
        db.addAppointment(models::Appointment::restore("A2", "P1", at(date, QTime(10, 0)), models::AppointmentStatus::Completed));
        db.addAppointment(models::Appointment::restore("A3", "P2", at(date, QTime(11, 0)), models::AppointmentStatus::Pending));
        db.addAppointment(models::Appointment::restore("A4", "P3", at(date, QTime(12, 0)), models::AppointmentStatus::Cancelled));
        const auto attended = controllers::ClinicReports::mostFrequentlyAttended(db);
        QCOMPARE(attended.size(), std::size_t(1)); QCOMPARE(attended[0].patient->patientId(), QString("P1"));
        QCOMPARE(attended[0].completedAppointments, std::size_t(2));
        const auto pending = controllers::ClinicReports::patientsWithPendingAppointments(db);
        QCOMPARE(pending.size(), std::size_t(1)); QCOMPARE(pending[0].patient->patientId(), QString("P2"));
        const auto distribution = controllers::ClinicReports::patientCategoryDistribution(db);
        QCOMPARE(distribution.total(), std::size_t(3));
        QCOMPARE(distribution.count(models::PatientCategory::General), std::size_t(1));
        QCOMPARE(distribution.count(models::PatientCategory::Emergency), std::size_t(1));
        QCOMPARE(distribution.count(models::PatientCategory::FollowUp), std::size_t(1));
        const auto daily = controllers::ClinicReports::dailyAppointmentSummary(db, date);
        QCOMPARE(daily.total, std::size_t(4)); QCOMPARE(daily.completed, std::size_t(2));
        QCOMPARE(daily.pending, std::size_t(1)); QCOMPARE(daily.cancelled, std::size_t(1));
        QCOMPARE(controllers::ClinicReports::dailyAppointmentSummary(db, date.addDays(1)).total, std::size_t(0));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            controllers::ClinicReports::dailyAppointmentSummary(db, QDate()));
    }

    void invalidOperationsDoNotChangeState()
    {
        data::ClinicDatabase db; controllers::ClinicManager manager(db);
        QVERIFY(!manager.undoLastAdministrativeOperation());
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.deletePatient("missing"));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.getPatient("missing"));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, manager.enqueuePatient("missing"));
        QCOMPARE(db.patients().size(), std::size_t(0));
        QVERIFY(manager.waitingQueue().empty());
        QVERIFY(!manager.callNextPatient().has_value());
    }

private:
    static void appointmentsSetup(data::ClinicDatabase& db)
    {
        db.addAppointment(models::Appointment::restore("RELATED", "P3", future(), models::AppointmentStatus::Pending));
    }
};

QTEST_GUILESS_MAIN(Phase3Tests)
#include "test_phase3.moc"
