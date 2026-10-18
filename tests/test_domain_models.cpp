#include "models/appointment.h"
#include "models/emergency_patient.h"
#include "models/follow_up_patient.h"
#include "models/general_patient.h"
#include "models/patient_record.h"

#include <QtTest>

#include <memory>
#include <stdexcept>
#include <vector>

using namespace scapms::models;

class DomainModelTests final : public QObject {
    Q_OBJECT

private:
    static ContactInformation contact()
    {
        return {QStringLiteral("08012345678"), QStringLiteral("person@example.test")};
    }

    static QDateTime time(const QString& value)
    {
        return QDateTime::fromString(value, Qt::ISODate);
    }

private slots:
    void patientFieldsAreTrimmedAndValidated()
    {
        GeneralPatient patient(QStringLiteral(" P-1 "), QStringLiteral(" Ada Patient "),
                              25, contact());
        QCOMPARE(patient.patientId(), QStringLiteral("P-1"));
        QCOMPARE(patient.fullName(), QStringLiteral("Ada Patient"));
        QCOMPARE(patient.age(), 25);

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            GeneralPatient(QStringLiteral("P-2"), QStringLiteral("No Age"), 121, contact()));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            GeneralPatient(QStringLiteral("P-3"), QStringLiteral("No Phone"), 20,
                           {QString(), QString()}));
    }

    void categoryBehaviorIsDistinct()
    {
        GeneralPatient general(QStringLiteral("G-1"), QStringLiteral("General"), 30, contact());
        EmergencyPatient emergency(QStringLiteral("E-1"), QStringLiteral("Emergency"), 40, contact());
        FollowUpPatient followUp(QStringLiteral("F-1"), QStringLiteral("Follow Up"), 50,
                                 contact(), QStringLiteral("Post-treatment review"));

        QCOMPARE(general.categoryLabel(), QStringLiteral("General"));
        QCOMPARE(general.priorityLabel(), QStringLiteral("Normal"));
        QCOMPARE(emergency.categoryLabel(), QStringLiteral("Emergency"));
        QCOMPARE(emergency.priorityLabel(), QStringLiteral("High Priority"));
        QCOMPARE(followUp.categoryLabel(), QStringLiteral("Follow-Up"));
        QCOMPARE(followUp.followUpReason(), QStringLiteral("Post-treatment review"));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            FollowUpPatient(QStringLiteral("F-2"), QStringLiteral("Missing Reason"), 50,
                            contact(), QStringLiteral("  ")));
    }

    void baseReferencesDispatchCategoryBehavior()
    {
        std::vector<std::unique_ptr<PatientRecord>> patients;
        patients.emplace_back(std::make_unique<GeneralPatient>(
            QStringLiteral("G-1"), QStringLiteral("General"), 30, contact()));
        patients.emplace_back(std::make_unique<EmergencyPatient>(
            QStringLiteral("E-1"), QStringLiteral("Emergency"), 40, contact()));
        patients.emplace_back(std::make_unique<FollowUpPatient>(
            QStringLiteral("F-1"), QStringLiteral("Follow Up"), 50, contact(),
            QStringLiteral("Review")));

        QCOMPARE(patients.at(0)->category(), PatientCategory::General);
        QCOMPARE(patients.at(1)->priorityLabel(), QStringLiteral("High Priority"));
        QCOMPARE(patients.at(2)->categoryLabel(), QStringLiteral("Follow-Up"));
    }

    void appointmentSchedulingRequiresFutureTimeAndPatient()
    {
        const auto now = time(QStringLiteral("2026-10-17T10:00:00Z"));
        auto appointment = Appointment::schedule(QStringLiteral("A-1"), QStringLiteral("P-1"),
                                                  now.addSecs(3600), QString(), now);
        GeneralPatient patient(QStringLiteral("P-1"), QStringLiteral("Patient"), 25, contact());
        const Appointable& appointable = patient;
        appointable.scheduleAppointment(appointment, now);
        QCOMPARE(appointment.status(), AppointmentStatus::Pending);

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            Appointment::schedule(QStringLiteral("A-2"), QStringLiteral("P-1"),
                                  now.addSecs(-1), QString(), now));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
            EmergencyPatient(QStringLiteral("E-1"), QStringLiteral("Emergency"), 35, contact())
                .scheduleAppointment(appointment, now));
    }

    void historicalAppointmentsCanBeRestored()
    {
        const auto now = time(QStringLiteral("2026-10-17T10:00:00Z"));
        auto completed = Appointment::restore(QStringLiteral("A-C"), QStringLiteral("P-1"),
                                               now.addDays(-2), AppointmentStatus::Completed,
                                               QStringLiteral("Visit"));
        auto cancelled = Appointment::restore(QStringLiteral("A-X"), QStringLiteral("P-1"),
                                               now.addDays(-1), AppointmentStatus::Cancelled,
                                               QString());
        QCOMPARE(completed.status(), AppointmentStatus::Completed);
        QCOMPARE(cancelled.status(), AppointmentStatus::Cancelled);

        auto pending = Appointment::restore(QStringLiteral("A-P"), QStringLiteral("P-1"),
                                            now.addSecs(-3600), AppointmentStatus::Pending,
                                            QString());
        QCOMPARE(pending.status(), AppointmentStatus::Pending);
        QCOMPARE(pending.scheduledAt(), now.addSecs(-3600));
    }

    void appointmentStatusTransitionsAndAssociationAreEnforced()
    {
        const auto now = time(QStringLiteral("2026-10-17T10:00:00Z"));
        auto due = Appointment::restore(QStringLiteral("A-1"), QStringLiteral("P-1"),
                                        now.addSecs(-3600), AppointmentStatus::Pending,
                                        QString());
        GeneralPatient patient(QStringLiteral("P-1"), QStringLiteral("Patient"), 25, contact());
        patient.cancelAppointment(due);
        QCOMPARE(due.status(), AppointmentStatus::Cancelled);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, due.cancel());

        auto completed = Appointment::restore(QStringLiteral("A-2"), QStringLiteral("P-1"),
                                              now.addSecs(-3600), AppointmentStatus::Pending,
                                              QString());
        completed.markCompleted(now);
        QCOMPARE(completed.status(), AppointmentStatus::Completed);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, completed.cancel());

        auto future = Appointment::schedule(QStringLiteral("A-3"), QStringLiteral("P-1"),
                                            now.addSecs(3600), QString(), now);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.markCompleted(now));
    }
};

QTEST_GUILESS_MAIN(DomainModelTests)
#include "test_domain_models.moc"
