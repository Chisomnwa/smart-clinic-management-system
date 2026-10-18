#pragma once

#include "controllers/appointment_controller.h"
#include "controllers/clinic_manager.h"
#include "data/clinic_database.h"
#include "persistence/file_handler.h"

#include <QMainWindow>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableWidget;

namespace scapms::gui {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Application actions are shared by the UI and end-to-end tests.
    bool registerPatient(QString category, QString fullName, int age, QString phone,
                         QString email, QString followUpReason, QString* error = nullptr);
    bool editPatient(const QString& patientId, QString fullName, int age, QString phone,
                     QString email, QString followUpReason, QString* error = nullptr);
    bool deletePatient(const QString& patientId, QString* error = nullptr);
    bool scheduleAppointment(QString patientId, QDateTime when, QString notes = {},
                             QString* error = nullptr);
    bool cancelAppointment(const QString& appointmentId, QString* error = nullptr);
    bool completeAppointment(const QString& appointmentId, QDateTime now = QDateTime::currentDateTime(),
                             QString* error = nullptr);
    bool enqueuePatient(const QString& patientId, QString* error = nullptr);
    std::optional<QString> callNextPatient();
    bool undoLastOperation();
    persistence::PersistenceResult saveToFile(const QString& path);
    persistence::PersistenceResult loadFromFile(const QString& path);
    void newDataset();
    bool isDirty() const noexcept { return m_dirty; }
    const data::ClinicDatabase& database() const noexcept { return m_database; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    enum Page { DashboardPage, PatientsPage, AppointmentsPage, QueuePage, SearchPage, ReportsPage };
    enum class UnsavedChoice { Save, Discard, Cancel };

    QWidget* makeDashboardPage();
    QWidget* makePatientsPage();
    QWidget* makeAppointmentsPage();
    QWidget* makeQueuePage();
    QWidget* makeSearchPage();
    QWidget* makeReportsPage();
    void buildMenus();
    void navigateTo(Page page);
    void refreshAll();
    void refreshDashboard();
    void refreshPatients();
    void refreshAppointments();
    void refreshQueue();
    void refreshSearchResults(const std::vector<data::ClinicDatabase::PatientPointer>& records);
    void refreshReport();
    void showPatientDialog(data::ClinicDatabase::PatientPointer patient = {});
    void showAppointmentDialog();
    void showError(const QString& message);
    void showSuccess(const QString& message);
    bool confirmDiscardOrSave(const QString& operation);
    bool saveUsingDialog();
    void loadUsingDialog();
    void searchPatients();
    void sortPatients();
    void updateSearchControls();
    void updateReportControls();
    QString selectedPatientId(const QTableWidget* table) const;
    void setDirty();
    void updateWindowTitle();

    data::ClinicDatabase m_database;
    controllers::ClinicManager m_clinicManager;
    controllers::AppointmentController m_appointmentController;
    QStackedWidget* m_pages = nullptr;
    QTableWidget* m_patientsTable = nullptr;
    QTableWidget* m_appointmentsTable = nullptr;
    QTableWidget* m_queueTable = nullptr;
    QTableWidget* m_resultsTable = nullptr;
    QTableWidget* m_reportsTable = nullptr;
    QLineEdit* m_searchText = nullptr;
    QComboBox* m_searchField = nullptr;
    QComboBox* m_searchAlgorithm = nullptr;
    QComboBox* m_searchCategory = nullptr;
    QLabel* m_searchHelp = nullptr;
    QComboBox* m_sortField = nullptr;
    QComboBox* m_sortAlgorithm = nullptr;
    QComboBox* m_reportType = nullptr;
    QDateEdit* m_reportDate = nullptr;
    QLabel* m_reportSummary = nullptr;
    QLabel* m_patientCount = nullptr;
    QLabel* m_appointmentCount = nullptr;
    QLabel* m_pendingCount = nullptr;
    QLabel* m_queueCount = nullptr;
    QString m_currentFilePath;
    bool m_dirty = false;
};

} // namespace scapms::gui
