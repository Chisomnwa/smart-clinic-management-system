#include "gui/main_window.h"
#include "models/follow_up_patient.h"

#include <QComboBox>
#include <QApplication>
#include <QAction>
#include <QAbstractItemView>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QFontMetrics>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTimer>
#include <QTimeZone>
#include <QtTest>

using namespace scapms;

namespace {
bool comboPopupHasReadableStates(const QComboBox* combo)
{
    if (!combo || !combo->view()) return false;
    const auto* view = combo->view();
    const auto palette = view->palette();
    const QString style = view->styleSheet();
    return palette.color(QPalette::Base).lightness() > 240
        && palette.color(QPalette::Text).lightness() < 100
        && palette.color(QPalette::Highlight).lightness() < 150
        && palette.color(QPalette::HighlightedText).lightness() > 240
        && style.contains(QStringLiteral("QAbstractItemView::item {"))
        && style.contains(QStringLiteral("QAbstractItemView::item:hover"))
        && style.contains(QStringLiteral("QAbstractItemView::item:selected"));
}
}

class GuiIntegrationTests final : public QObject {
    Q_OBJECT
private:
    static QDateTime future()
    { return QDateTime(QDate::currentDate().addDays(30), QTime(10, 0), QTimeZone(QByteArrayLiteral("UTC"))); }
private slots:
    void endToEndRegisterScheduleQueueSaveLoadEditUndo()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        gui::MainWindow window;
        QString error;
        QVERIFY2(window.registerPatient(QStringLiteral("General"), QStringLiteral("Ada Clinic"), 34,
                                       QStringLiteral("08011223344"), QStringLiteral("ada@example.test"), {}, &error),
                 qPrintable(error));
        QCOMPARE(window.database().patients().size(), std::size_t(1));
        QCOMPARE(window.database().patients().front()->patientId(), QStringLiteral("P-000001"));
        auto* patientTable = window.findChild<QTableWidget*>(QStringLiteral("patientsTable"));
        QVERIFY(patientTable); QCOMPARE(patientTable->rowCount(), 1);

        QVERIFY2(window.scheduleAppointment(QStringLiteral("P-000001"), future(), QStringLiteral("First visit"), &error),
                 qPrintable(error));
        QCOMPARE(window.database().appointments().size(), std::size_t(1));
        QVERIFY(window.enqueuePatient(QStringLiteral("P-000001"), &error));
        QCOMPARE(window.database().waitingQueue(), std::vector<QString>({QStringLiteral("P-000001")}));
        auto* appointmentTable = window.findChild<QTableWidget*>(QStringLiteral("appointmentsTable"));
        auto* queueTable = window.findChild<QTableWidget*>(QStringLiteral("queueTable"));
        QVERIFY(appointmentTable); QVERIFY(queueTable);
        QCOMPARE(appointmentTable->rowCount(), 1); QCOMPARE(queueTable->rowCount(), 1);

        const QString path = directory.filePath(QStringLiteral("integration.json"));
        QVERIFY2(window.saveToFile(path).success, "Save should succeed.");
        QVERIFY(!window.isDirty());
        window.newDataset();
        QVERIFY(window.database().patients().empty());
        QVERIFY2(window.loadFromFile(path).success, "Load should succeed.");
        QCOMPARE(window.database().patients().size(), std::size_t(1));
        QCOMPARE(window.database().appointments().size(), std::size_t(1));
        QCOMPARE(window.database().waitingQueue(), std::vector<QString>({QStringLiteral("P-000001")}));
        QCOMPARE(patientTable->rowCount(), 1); QCOMPARE(appointmentTable->rowCount(), 1); QCOMPARE(queueTable->rowCount(), 1);

        QVERIFY(window.editPatient(QStringLiteral("P-000001"), QStringLiteral("Ada Updated"), 35,
                                   QStringLiteral("08099887766"), QStringLiteral("new@example.test"), {}, &error));
        QCOMPARE(window.database().requirePatient(QStringLiteral("P-000001"))->fullName(), QStringLiteral("Ada Updated"));
        QVERIFY(window.undoLastOperation());
        QCOMPARE(window.database().requirePatient(QStringLiteral("P-000001"))->fullName(), QStringLiteral("Ada Clinic"));
        QCOMPARE(window.database().appointments().size(), std::size_t(1));
        QCOMPARE(window.database().waitingQueue().size(), std::size_t(1));
    }

    void navigationAndSearchControlsAreConnected()
    {
        gui::MainWindow window;
        QString error;
        QVERIFY(window.registerPatient(QStringLiteral("General"), QStringLiteral("Zola Nwosu"), 34,
                                       QStringLiteral("08011223344"), {}, {}, &error));
        QVERIFY(window.registerPatient(QStringLiteral("Emergency"), QStringLiteral("Ada Obi"), 42,
                                       QStringLiteral("08022334455"), {}, {}, &error));
        QVERIFY(window.registerPatient(QStringLiteral("Follow-Up"), QStringLiteral("Emeka Obi"), 39,
                                       QStringLiteral("08033445566"), {}, QStringLiteral("Review"), &error));
        window.show();
        QApplication::processEvents();
        QVERIFY(window.isVisible());
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("mainPages"));
        QVERIFY(pages);
        auto* reportsNav = window.findChild<QPushButton*>(QStringLiteral("navReports"));
        QVERIFY(reportsNav); reportsNav->click(); QCOMPARE(pages->currentIndex(), 5);
        auto* searchNav = window.findChild<QPushButton*>(QStringLiteral("navSearch"));
        QVERIFY(searchNav); searchNav->click(); QCOMPARE(pages->currentIndex(), 4);
        auto* field = window.findChild<QComboBox*>(QStringLiteral("searchFieldCombo"));
        auto* algorithm = window.findChild<QComboBox*>(QStringLiteral("searchAlgorithmCombo"));
        QVERIFY(field); QVERIFY(algorithm);
        QVERIFY(field->minimumWidth() >= 142);
        QVERIFY(algorithm->minimumWidth() >= 158);
        QVERIFY(field->minimumHeight() >= 40);
        QVERIFY(algorithm->minimumHeight() >= 40);
        QVERIFY(field->styleSheet().contains(QStringLiteral("QComboBox::drop-down")));
        QVERIFY(algorithm->styleSheet().contains(QStringLiteral("QComboBox::drop-down")));
        QVERIFY(comboPopupHasReadableStates(field));
        QVERIFY(comboPopupHasReadableStates(algorithm));

        auto* searchButton = window.findChild<QPushButton*>(QStringLiteral("runSearchButton"));
        QVERIFY(searchButton);
        QCOMPARE(searchButton->text(), QStringLiteral("Search"));
        QVERIFY(searchButton->isVisible());
        QVERIFY(searchButton->minimumWidth() >= 100);
        QVERIFY(searchButton->minimumHeight() >= 40);
        QVERIFY(searchButton->styleSheet().contains(QStringLiteral("color: #ffffff")));
        QVERIFY(searchButton->styleSheet().contains(QStringLiteral("background-color: #176b87")));

        QCOMPARE(algorithm->count(), 2); // Linear and Binary for patient IDs.
        field->setCurrentIndex(1); QCOMPARE(algorithm->count(), 1);
        QCOMPARE(algorithm->currentText(), QStringLiteral("Recursive Search"));
        field->setCurrentIndex(2); QCOMPARE(algorithm->currentText(), QStringLiteral("Linear Category Search"));
        field->setCurrentIndex(0);
        QCOMPARE(algorithm->currentText(), QStringLiteral("Linear Search"));
        auto* query = window.findChild<QLineEdit*>(QStringLiteral("searchQueryEdit"));
        auto* runSearch = window.findChild<QPushButton*>(QStringLiteral("runSearchButton"));
        auto* results = window.findChild<QTableWidget*>(QStringLiteral("searchResultsTable"));
        QVERIFY(query && runSearch && results);
        query->setText(QStringLiteral("P-000002"));
        runSearch->click();
        QCOMPARE(results->rowCount(), 1);
        QCOMPARE(results->item(0, 0)->text(), QStringLiteral("P-000002"));

        auto* sortField = window.findChild<QComboBox*>(QStringLiteral("sortFieldCombo"));
        auto* sortAlgorithm = window.findChild<QComboBox*>(QStringLiteral("sortAlgorithmCombo"));
        auto* sortButton = window.findChild<QPushButton*>(QStringLiteral("runSortButton"));
        QVERIFY(sortField && sortAlgorithm && sortButton);
        QVERIFY(sortField->minimumWidth() >= 150);
        QVERIFY(sortAlgorithm->minimumWidth() >= 170);
        QVERIFY(sortField->styleSheet().contains(QStringLiteral("QComboBox::drop-down")));
        QVERIFY(sortAlgorithm->styleSheet().contains(QStringLiteral("QComboBox::drop-down")));
        QVERIFY(comboPopupHasReadableStates(sortField));
        QVERIFY(comboPopupHasReadableStates(sortAlgorithm));
        QVERIFY(sortButton->minimumWidth() >= 150);
        QVERIFY(sortButton->minimumHeight() >= 40);
        sortField->setCurrentIndex(0); // Patient name
        sortAlgorithm->setCurrentIndex(0); // Insertion Sort
        sortButton->click();
        QCOMPARE(results->item(0, 1)->text(), QStringLiteral("Ada Obi"));
        QCOMPARE(results->item(1, 1)->text(), QStringLiteral("Emeka Obi"));
        QCOMPARE(results->item(2, 1)->text(), QStringLiteral("Zola Nwosu"));
        sortAlgorithm->setCurrentIndex(1); // Selection Sort
        sortButton->click();
        QCOMPARE(results->item(0, 1)->text(), QStringLiteral("Ada Obi"));
        QCOMPARE(results->item(1, 1)->text(), QStringLiteral("Emeka Obi"));
        QCOMPARE(results->item(2, 1)->text(), QStringLiteral("Zola Nwosu"));
    }

    void comboDropdownsUseReadableThemeAcrossPages()
    {
        gui::MainWindow window;
        QString error;
        QVERIFY(window.registerPatient(QStringLiteral("General"), QStringLiteral("Ada Clinic"), 34,
                                       QStringLiteral("08011223344"), {}, {}, &error));
        QVERIFY(window.registerPatient(QStringLiteral("Emergency"), QStringLiteral("Emeka Obi"), 44,
                                       QStringLiteral("08066778812"), {}, {}, &error));

        auto* queue = window.findChild<QComboBox*>(QStringLiteral("queuePatientCombo"));
        QVERIFY(queue);
        QCOMPARE(queue->count(), 2);
        const QString emergencyEntry = QStringLiteral("P-000002 · Emeka Obi (High Priority)");
        QCOMPARE(queue->itemText(1), emergencyEntry);
        const int longestEntryWidth = QFontMetrics(queue->view()->font()).horizontalAdvance(emergencyEntry);
        QVERIFY(queue->minimumWidth() >= longestEntryWidth + 58);
        QVERIFY(queue->view()->minimumWidth() >= longestEntryWidth + 48);
        QVERIFY(comboPopupHasReadableStates(queue));

        auto* searchNavigation = window.findChild<QPushButton*>(QStringLiteral("navSearch"));
        QVERIFY(searchNavigation);
        searchNavigation->click();
        const QStringList searchAndSortNames{
            QStringLiteral("searchFieldCombo"), QStringLiteral("searchAlgorithmCombo"),
            QStringLiteral("searchCategoryCombo"), QStringLiteral("sortFieldCombo"),
            QStringLiteral("sortAlgorithmCombo")};
        for (const auto& name : searchAndSortNames) {
            auto* combo = window.findChild<QComboBox*>(name);
            QVERIFY2(comboPopupHasReadableStates(combo), qPrintable(name));
        }

        auto* reportsNavigation = window.findChild<QPushButton*>(QStringLiteral("navReports"));
        QVERIFY(reportsNavigation);
        reportsNavigation->click();
        auto* reportSelector = window.findChild<QComboBox*>(QStringLiteral("reportTypeCombo"));
        QVERIFY(comboPopupHasReadableStates(reportSelector));
    }

    void patientAndAppointmentDialogsSaveRealInputs()
    {
        gui::MainWindow window;
        window.show();
        QApplication::processEvents();
        auto* registerButton = window.findChild<QPushButton*>(QStringLiteral("registerPatientButton"));
        QVERIFY(registerButton);
        QTimer::singleShot(0, &window, [&window] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            QVERIFY(dialog->palette().color(QPalette::Window).lightness() > 200);
            QVERIFY(dialog->palette().color(QPalette::WindowText).lightness() < 100);
            QVERIFY(dialog->styleSheet().contains(QStringLiteral("QDialog QLabel")));
            auto* category = dialog->findChild<QComboBox*>(QStringLiteral("patientCategoryCombo"));
            auto* reason = dialog->findChild<QLineEdit*>(QStringLiteral("followUpReasonEdit"));
            auto* name = dialog->findChild<QLineEdit*>(QStringLiteral("patientNameEdit"));
            auto* phone = dialog->findChild<QLineEdit*>(QStringLiteral("patientPhoneEdit"));
            auto* age = dialog->findChild<QSpinBox*>(QStringLiteral("patientAgeSpin"));
            QVERIFY(category && reason && name && phone && age);
            category->setCurrentIndex(static_cast<int>(models::PatientCategory::FollowUp));
            QVERIFY(reason->isVisible());
            name->setText(QStringLiteral("Follow-up Form Patient"));
            phone->setText(QStringLiteral("08066778899"));
            age->setValue(39);
            reason->setText(QStringLiteral("Clinic follow-up"));
            auto* buttons = dialog->findChild<QDialogButtonBox*>(); QVERIFY(buttons);
            buttons->button(QDialogButtonBox::Save)->click();
        });
        registerButton->click();
        QCOMPARE(window.database().patients().size(), std::size_t(1));
        const auto* followUp = dynamic_cast<const models::FollowUpPatient*>(window.database().patients().front().get());
        QVERIFY(followUp); QCOMPARE(followUp->followUpReason(), QStringLiteral("Clinic follow-up"));
        QVERIFY(window.registerPatient(QStringLiteral("Emergency"), QStringLiteral("Emeka Obi"), 44,
                                       QStringLiteral("08066778812"), {}, {}, nullptr));

        auto* scheduleButton = window.findChild<QPushButton*>(QStringLiteral("scheduleAppointmentButton"));
        QVERIFY(scheduleButton);
        bool appointmentLayoutComfortable = false;
        bool patientEntryFullyReadable = false;
        bool calendarControlsAvailable = false;
        QTimer::singleShot(0, &window, [&window, &appointmentLayoutComfortable,
                                        &patientEntryFullyReadable, &calendarControlsAvailable] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            QVERIFY(dialog->palette().color(QPalette::Window).lightness() > 200);
            QVERIFY(dialog->palette().color(QPalette::WindowText).lightness() < 100);
            QVERIFY(dialog->styleSheet().contains(QStringLiteral("QDialog QLabel")));
            auto* dateTime = dialog->findChild<QDateTimeEdit*>(QStringLiteral("appointmentDateTimeEdit"));
            auto* patient = dialog->findChild<QComboBox*>(QStringLiteral("appointmentPatientCombo"));
            auto* notes = dialog->findChild<QLineEdit*>(QStringLiteral("appointmentNotesEdit"));
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            QVERIFY(dateTime && patient && notes && buttons);
            const QString emergencyText = QStringLiteral("P-000002 · Emeka Obi (High Priority)");
            const int fullEntryWidth = QFontMetrics(patient->font()).horizontalAdvance(emergencyText);
            appointmentLayoutComfortable = dialog->minimumWidth() >= 680
                && patient->minimumWidth() >= 390 && dateTime->minimumWidth() >= 360
                && notes->minimumWidth() >= 360;
            patientEntryFullyReadable = patient->findText(emergencyText) >= 0
                && patient->view()->minimumWidth() >= fullEntryWidth + 48
                && comboPopupHasReadableStates(patient);
            auto* calendar = dateTime->calendarWidget();
            QVERIFY(calendar);
            QVERIFY(calendar->palette().color(QPalette::Base).lightness() > 240);
            QVERIFY(calendar->palette().color(QPalette::Text).lightness() < 100);
            QVERIFY(calendar->palette().color(QPalette::Highlight).lightness() < 150);
            QVERIFY(calendar->styleSheet().contains(QStringLiteral("qt_calendar_navigationbar")));
            QVERIFY(calendar->styleSheet().contains(QStringLiteral("QHeaderView::section")));
            QVERIFY(calendar->styleSheet().contains(QStringLiteral("selection-background-color: #176b87")));
            QVERIFY(calendar->styleSheet().contains(QStringLiteral("QTableView::item:disabled")));
            QVERIFY(calendar->styleSheet().contains(QStringLiteral("color: #8b98a8")));
            QVERIFY(calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth")));
            QVERIFY(calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth")));
            auto* monthButton = calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton"));
            if (monthButton && monthButton->menu()) {
                const QMenu* monthMenu = monthButton->menu();
                calendarControlsAvailable = monthButton->minimumWidth() >= 100
                    && monthMenu->minimumWidth() >= 130
                    && monthMenu->palette().color(QPalette::Window).lightness() > 240
                    && monthMenu->palette().color(QPalette::WindowText).lightness() < 100
                    && monthMenu->styleSheet().contains(QStringLiteral("QMenu::item:selected"));
            }
            dateTime->setDateTime(QDateTime::currentDateTime().addDays(4));
            notes->setText(QStringLiteral("Dialog scheduled visit"));
            buttons->button(QDialogButtonBox::Save)->click();
        });
        scheduleButton->click();
        QVERIFY(appointmentLayoutComfortable);
        QVERIFY(patientEntryFullyReadable);
        QVERIFY(calendarControlsAvailable);
        QCOMPARE(window.database().appointments().size(), std::size_t(1));
        QCOMPARE(window.database().appointments().front().notes(), QStringLiteral("Dialog scheduled visit"));
    }

    void invalidActionsAndFileFailuresAreRecoverable()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        gui::MainWindow window;
        QString error;
        QVERIFY(!window.registerPatient(QStringLiteral("Follow-Up"), QStringLiteral("Missing reason"), 40,
                                        QStringLiteral("08012345678"), {}, {}, &error));
        QVERIFY(!error.isEmpty()); QVERIFY(window.database().patients().empty());
        QVERIFY(window.registerPatient(QStringLiteral("Emergency"), QStringLiteral("Emergency Patient"), 45,
                                       QStringLiteral("08012345678"), {}, {}, &error));
        QVERIFY(!window.scheduleAppointment(QStringLiteral("P-000001"), QDateTime::currentDateTime().addDays(-1), {}, &error));
        QVERIFY(!window.scheduleAppointment(QStringLiteral("P-MISSING"), future(), {}, &error));
        const auto appointmentTime = future();
        QVERIFY(window.scheduleAppointment(QStringLiteral("P-000001"), appointmentTime, {}, &error));
        QVERIFY(!window.scheduleAppointment(QStringLiteral("P-000001"), appointmentTime, {}, &error));
        QVERIFY(window.enqueuePatient(QStringLiteral("P-000001"), &error)); // First insertion succeeds.
        QVERIFY(!window.enqueuePatient(QStringLiteral("P-000001"), &error)); // Duplicate is rejected.
        QVERIFY(!window.cancelAppointment(QStringLiteral("A-MISSING"), &error));
        const auto before = window.database().patients().size();
        const auto failedLoad = window.loadFromFile(directory.filePath(QStringLiteral("missing.json")));
        QVERIFY(!failedLoad.success);
        QCOMPARE(window.database().patients().size(), before);
        QVERIFY(window.completeAppointment(QStringLiteral("A-000001"), appointmentTime.addDays(1), &error));
        QVERIFY(!window.completeAppointment(QStringLiteral("A-000001"), future(), &error));
        QVERIFY(window.database().requireAppointment(QStringLiteral("A-000001")).status() == models::AppointmentStatus::Completed);
    }

    void reportDateCalendarUsesLightReadableTheme()
    {
        gui::MainWindow window;
        auto* reports = window.findChild<QPushButton*>(QStringLiteral("navReports"));
        QVERIFY(reports);
        reports->click();
        auto* dateEdit = window.findChild<QDateEdit*>(QStringLiteral("reportDateEdit"));
        QVERIFY(dateEdit);
        auto* calendar = dateEdit->calendarWidget();
        QVERIFY(calendar);
        QVERIFY(calendar->palette().color(QPalette::Base).lightness() > 240);
        QVERIFY(calendar->palette().color(QPalette::Text).lightness() < 100);
        QVERIFY(calendar->styleSheet().contains(QStringLiteral("QCalendarWidget QToolButton")));
        QVERIFY(calendar->styleSheet().contains(QStringLiteral("QCalendarWidget QHeaderView::section")));
        auto* monthButton = calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton"));
        QVERIFY(monthButton);
        QVERIFY(monthButton->menu());
        QVERIFY(monthButton->minimumWidth() >= 100);
        QVERIFY(monthButton->menu()->minimumWidth() >= 130);
        QVERIFY(monthButton->menu()->palette().color(QPalette::Window).lightness() > 240);
        QVERIFY(monthButton->menu()->palette().color(QPalette::WindowText).lightness() < 100);
        QVERIFY(monthButton->menu()->actions().size() >= 12);
        const int originalMonth = calendar->monthShown();
        const int originalYear = calendar->yearShown();
        auto* nextMonth = calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth"));
        auto* previousMonth = calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth"));
        QVERIFY(nextMonth && previousMonth);
        nextMonth->click();
        const bool advancedMonth = calendar->monthShown() != originalMonth
            || calendar->yearShown() != originalYear;
        previousMonth->click();
        QCOMPARE(calendar->monthShown(), originalMonth);
        QCOMPARE(calendar->yearShown(), originalYear);
        QVERIFY(advancedMonth);
    }

    void newDatasetHonorsUnsavedDiscardAndCancelChoices()
    {
        gui::MainWindow window;
        QString error;
        QVERIFY(window.registerPatient(QStringLiteral("General"), QStringLiteral("Unsaved"), 29,
                                       QStringLiteral("08044556677"), {}, {}, &error));
        auto* newAction = window.findChild<QAction*>(QStringLiteral("actionNew"));
        QVERIFY(newAction); QVERIFY(window.isDirty());
        QTimer::singleShot(0, &window, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(box);
            QVERIFY(box->palette().color(QPalette::Window).lightness() > 200);
            QVERIFY(box->palette().color(QPalette::WindowText).lightness() < 100);
            QVERIFY(box->styleSheet().contains(QStringLiteral("QMessageBox QLabel")));
            for (auto* button : box->findChildren<QPushButton*>()) {
                if (button->text() == QStringLiteral("Discard")) { button->click(); return; }
            }
            QFAIL("Discard button was not found.");
        });
        newAction->trigger();
        QVERIFY(window.database().patients().empty()); QVERIFY(!window.isDirty());

        QVERIFY(window.registerPatient(QStringLiteral("General"), QStringLiteral("Keep me"), 30,
                                       QStringLiteral("08044556678"), {}, {}, &error));
        QTimer::singleShot(0, &window, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(box);
            for (auto* button : box->findChildren<QPushButton*>()) {
                if (button->text() == QStringLiteral("Cancel")) { button->click(); return; }
            }
            QFAIL("Cancel button was not found.");
        });
        newAction->trigger();
        QCOMPARE(window.database().patients().size(), std::size_t(1)); QVERIFY(window.isDirty());
    }
};

QTEST_MAIN(GuiIntegrationTests)
#include "test_gui_integration.moc"
