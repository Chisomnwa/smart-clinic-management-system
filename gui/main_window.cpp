#include "gui/main_window.h"

#include "controllers/clinic_reports.h"
#include "controllers/patient_algorithms.h"
#include "models/emergency_patient.h"
#include "models/follow_up_patient.h"
#include "models/general_patient.h"

#include <QAction>
#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QFont>
#include <QFontMetrics>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QLineEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

#include <exception>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace scapms::gui {
namespace {

QTableWidget* makeTable(const QStringList& headers, QWidget* parent)
{
    auto* table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setMinimumHeight(280);
    return table;
}

QTableWidgetItem* cell(const QString& text, const QString& id = {})
{
    auto* item = new QTableWidgetItem(text);
    if (!id.isNull()) item->setData(Qt::UserRole, id);
    return item;
}

QWidget* pageHeading(const QString& title, const QString& description)
{
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 8);
    layout->setSpacing(4);
    auto* titleLabel = new QLabel(title, panel);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #17324d;"));
    auto* descriptionLabel = new QLabel(description, panel);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QStringLiteral("color: #64748b; font-size: 13px;"));
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    return panel;
}

QPushButton* actionButton(const QString& text, const QString& objectName, QWidget* parent,
                          bool primary = false)
{
    auto* button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setMinimumHeight(36);
    button->setCursor(Qt::PointingHandCursor);
    if (primary) button->setProperty("primary", true);
    return button;
}

void applyLightDialogTheme(QWidget* dialog)
{
    const QColor surface(QStringLiteral("#f4f7fb"));
    const QColor foreground(QStringLiteral("#243447"));
    const QColor fieldSurface(QStringLiteral("#ffffff"));
    QPalette palette = dialog->palette();
    for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Window, surface);
        palette.setColor(group, QPalette::WindowText, foreground);
        palette.setColor(group, QPalette::Base, fieldSurface);
        palette.setColor(group, QPalette::AlternateBase, QColor(QStringLiteral("#edf2f7")));
        palette.setColor(group, QPalette::Text, foreground);
        palette.setColor(group, QPalette::Button, fieldSurface);
        palette.setColor(group, QPalette::ButtonText, foreground);
        palette.setColor(group, QPalette::Highlight, QColor(QStringLiteral("#176b87")));
        palette.setColor(group, QPalette::HighlightedText, Qt::white);
    }
    dialog->setPalette(palette);
    dialog->setAutoFillBackground(true);
    dialog->setAttribute(Qt::WA_StyledBackground, true);
    dialog->setStyleSheet(QStringLiteral(
        "QDialog, QMessageBox { background-color: #f4f7fb; color: #243447; }"
        "QDialog QLabel, QMessageBox QLabel { color: #243447; background: transparent; }"
        "QDialog QLineEdit, QDialog QComboBox, QDialog QSpinBox, QDialog QDateEdit, QDialog QDateTimeEdit {"
        " color: #243447; background-color: #ffffff; selection-background-color: #176b87; selection-color: #ffffff;"
        " border: 1px solid #ced9e4; border-radius: 6px; padding: 7px; }"
        "QDialog QComboBox QAbstractItemView { color: #243447; background-color: #ffffff;"
        " selection-background-color: #176b87; selection-color: #ffffff; }"
        "QDialog QPushButton, QMessageBox QPushButton { color: #243447; background-color: #ffffff;"
        " border: 1px solid #ced9e4; border-radius: 6px; padding: 7px 13px; min-height: 20px; }"
        "QDialog QPushButton:hover, QMessageBox QPushButton:hover { background-color: #edf4fb; }"
        "QDialog QCalendarWidget { background-color: #ffffff; color: #243447; }"));
}

void applyLightComboPopupTheme(QComboBox* combo, bool fitControlToContents = false,
                               bool toolbarControl = false)
{
    const QColor surface(QStringLiteral("#ffffff"));
    const QColor foreground(QStringLiteral("#243447"));
    const QColor selected(QStringLiteral("#176b87"));
    QPalette palette = combo->palette();
    for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Window, surface);
        palette.setColor(group, QPalette::Base, surface);
        palette.setColor(group, QPalette::AlternateBase, QColor(QStringLiteral("#f4f7fb")));
        palette.setColor(group, QPalette::Text, foreground);
        palette.setColor(group, QPalette::WindowText, foreground);
        palette.setColor(group, QPalette::Button, surface);
        palette.setColor(group, QPalette::ButtonText, foreground);
        palette.setColor(group, QPalette::Highlight, selected);
        palette.setColor(group, QPalette::HighlightedText, Qt::white);
    }
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#8795a5")));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#8795a5")));

    combo->setPalette(palette);
    QString comboStyle = QStringLiteral(
        "QComboBox { color: #243447; background-color: #ffffff; }"
        "QComboBox QAbstractItemView { color: #243447; background-color: #ffffff;"
        " selection-background-color: #176b87; selection-color: #ffffff; outline: 0; }");
    if (toolbarControl) {
        comboStyle += QStringLiteral(
            "QComboBox { min-height: 40px; padding: 6px 34px 6px 11px; border: 1px solid #ced9e4;"
            " border-radius: 6px; }"
            "QComboBox:hover { border-color: #9bb5cb; }"
            "QComboBox:focus { border: 1px solid #176b87; }"
            "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right center; width: 30px;"
            " border-left: 1px solid #ced9e4; background-color: #f4f7fb;"
            " border-top-right-radius: 5px; border-bottom-right-radius: 5px; }"
            "QComboBox::down-arrow { width: 9px; height: 9px; }");
    }
    combo->setStyleSheet(comboStyle);

    auto* view = combo->view();
    view->setPalette(palette);
    view->setAutoFillBackground(true);
    view->setAttribute(Qt::WA_StyledBackground, true);
    view->setStyleSheet(QStringLiteral(
        "QAbstractItemView { color: #243447; background-color: #ffffff; alternate-background-color: #f4f7fb;"
        " selection-background-color: #176b87; selection-color: #ffffff; outline: 0; }"
        "QAbstractItemView::item { color: #243447; background-color: #ffffff; padding: 6px 10px; }"
        "QAbstractItemView::item:hover { color: #243447; background-color: #edf4fb; }"
        "QAbstractItemView::item:selected { color: #ffffff; background-color: #176b87; }"
        "QAbstractItemView::item:disabled { color: #8795a5; }"));

    int widestEntry = 0;
    const QFontMetrics metrics(view->font());
    for (int index = 0; index < combo->count(); ++index)
        widestEntry = std::max(widestEntry, metrics.horizontalAdvance(combo->itemText(index)));
    const int popupWidth = std::max(combo->width(), widestEntry + 48);
    view->setMinimumWidth(popupWidth);
    if (fitControlToContents)
        combo->setMinimumWidth(std::max(combo->minimumWidth(), std::max(390, widestEntry + 58)));
}

void applyLightCalendarTheme(QCalendarWidget* calendar)
{
    const QColor surface(QStringLiteral("#ffffff"));
    const QColor foreground(QStringLiteral("#243447"));
    QPalette palette = calendar->palette();
    for (const auto group : {QPalette::Active, QPalette::Inactive}) {
        palette.setColor(group, QPalette::Window, surface);
        palette.setColor(group, QPalette::Base, surface);
        palette.setColor(group, QPalette::AlternateBase, QColor(QStringLiteral("#f1f5f9")));
        palette.setColor(group, QPalette::Text, foreground);
        palette.setColor(group, QPalette::WindowText, foreground);
        palette.setColor(group, QPalette::Button, QColor(QStringLiteral("#e8eef5")));
        palette.setColor(group, QPalette::ButtonText, foreground);
        palette.setColor(group, QPalette::Highlight, QColor(QStringLiteral("#176b87")));
        palette.setColor(group, QPalette::HighlightedText, Qt::white);
    }
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#8b98a8")));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#8b98a8")));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(QStringLiteral("#8b98a8")));
    calendar->setPalette(palette);
    calendar->setStyleSheet(QStringLiteral(
        "QCalendarWidget { background: #ffffff; color: #243447; }"
        "QCalendarWidget QWidget#qt_calendar_navigationbar { background: #e8eef5; }"
        "QCalendarWidget QToolButton { color: #243447; background: #e8eef5; border: 1px solid #ced9e4;"
        " border-radius: 4px; padding: 4px; }"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton { min-width: 104px; padding-left: 8px; padding-right: 24px; }"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton::menu-indicator { subcontrol-origin: padding;"
        " subcontrol-position: right center; right: 7px; }"
        "QCalendarWidget QToolButton:hover { background: #dce7f1; }"
        "QCalendarWidget QSpinBox { color: #243447; background: #ffffff; selection-background-color: #176b87;"
        " selection-color: #ffffff; border: 1px solid #ced9e4; border-radius: 3px; padding: 2px; }"
        "QCalendarWidget QAbstractItemView { color: #243447; background: #ffffff; alternate-background-color: #f1f5f9;"
        " selection-background-color: #176b87; selection-color: #ffffff; outline: 0; }"
        "QCalendarWidget QTableView::item { color: #243447; background: #ffffff; }"
        "QCalendarWidget QTableView::item:selected { color: #ffffff; background: #176b87; }"
        "QCalendarWidget QTableView::item:disabled { color: #8b98a8; }"
        "QCalendarWidget QHeaderView::section { color: #334a62; background: #edf2f7;"
        " font-weight: 600; border: 0; padding: 4px; }"));

    // Qt creates the month menu as a separate popup window. Style that widget
    // directly so macOS appearance settings cannot apply a dark native palette.
    if (auto* monthButton = calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton"))) {
        monthButton->setMinimumWidth(104);
        monthButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        if (auto* monthMenu = monthButton->menu()) {
            QPalette menuPalette = monthMenu->palette();
            for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
                menuPalette.setColor(group, QPalette::Window, surface);
                menuPalette.setColor(group, QPalette::Base, surface);
                menuPalette.setColor(group, QPalette::Text, foreground);
                menuPalette.setColor(group, QPalette::WindowText, foreground);
                menuPalette.setColor(group, QPalette::Highlight, QColor(QStringLiteral("#176b87")));
                menuPalette.setColor(group, QPalette::HighlightedText, Qt::white);
            }
            monthMenu->setPalette(menuPalette);
            monthMenu->setMinimumWidth(140);
            monthMenu->setStyleSheet(QStringLiteral(
                "QMenu { color: #243447; background-color: #ffffff; border: 1px solid #ced9e4; }"
                "QMenu::item { color: #243447; background-color: #ffffff; padding: 5px 20px; }"
                "QMenu::item:selected { color: #ffffff; background-color: #176b87; }"
                "QMenu::item:disabled { color: #8b98a8; }"));
        }
    }
}

QString categoryString(models::PatientCategory category)
{
    return models::patientCategoryName(category);
}

QString statusColor(models::AppointmentStatus status)
{
    switch (status) {
    case models::AppointmentStatus::Pending: return QStringLiteral("#fff4d6");
    case models::AppointmentStatus::Completed: return QStringLiteral("#dcfce7");
    case models::AppointmentStatus::Cancelled: return QStringLiteral("#fee2e2");
    }
    return QStringLiteral("#f1f5f9");
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_clinicManager(m_database)
    , m_appointmentController(m_database)
{
    setWindowTitle(QStringLiteral("SCAPMS | Smart Clinic Management"));
    setMinimumSize(1020, 680);
    resize(1280, 820);
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f4f7fb; }"
        "QWidget { color: #243447; font-family: 'Avenir Next', 'Helvetica Neue', sans-serif; font-size: 13px; }"
        "QPushButton { border: 1px solid #d6e0ea; border-radius: 7px; padding: 7px 13px; background: white; }"
        "QPushButton:hover { background: #edf4fb; border-color: #aac4dc; }"
        "QPushButton[primary='true'] { color: white; background: #176b87; border-color: #176b87; font-weight: 600; }"
        "QPushButton[primary='true']:hover { background: #12566d; }"
        "QLineEdit, QComboBox, QSpinBox, QDateEdit, QDateTimeEdit { background: white; border: 1px solid #ced9e4; border-radius: 6px; padding: 7px; min-height: 20px; }"
        "QTableWidget { background: white; alternate-background-color: #f7f9fc; border: 1px solid #e0e7ef; border-radius: 8px; padding: 4px; }"
        "QHeaderView::section { background: #edf2f7; color: #475569; border: 0; border-bottom: 1px solid #dce4ed; padding: 9px 6px; font-weight: 600; }"
        "QMenuBar { background: white; } QStatusBar { background: white; color: #476174; }"));

    auto* central = new QWidget(this);
    auto* shell = new QHBoxLayout(central);
    shell->setContentsMargins(14, 14, 14, 10);
    shell->setSpacing(14);

    auto* sidebar = new QWidget(central);
    sidebar->setObjectName(QStringLiteral("navigationSidebar"));
    sidebar->setFixedWidth(205);
    sidebar->setStyleSheet(QStringLiteral("#navigationSidebar { background: #123247; border-radius: 12px; }"));
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(14, 18, 14, 14);
    sidebarLayout->setSpacing(7);
    auto* brand = new QLabel(QStringLiteral("SCAPMS"), sidebar);
    brand->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 800; color: white; letter-spacing: 1px;"));
    auto* brandSub = new QLabel(QStringLiteral("CLINIC WORKSPACE"), sidebar);
    brandSub->setStyleSheet(QStringLiteral("font-size: 9px; font-weight: 700; color: #92b5c8; letter-spacing: 1.5px;"));
    sidebarLayout->addWidget(brand);
    sidebarLayout->addWidget(brandSub);
    sidebarLayout->addSpacing(22);

    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    const QStringList navLabels{QStringLiteral("Dashboard"), QStringLiteral("Patients"),
                                QStringLiteral("Appointments"), QStringLiteral("Waiting Queue"),
                                QStringLiteral("Search & Sort"), QStringLiteral("Reports")};
    const QStringList navNames{QStringLiteral("navDashboard"), QStringLiteral("navPatients"),
                               QStringLiteral("navAppointments"), QStringLiteral("navQueue"),
                               QStringLiteral("navSearch"), QStringLiteral("navReports")};
    for (int i = 0; i < navLabels.size(); ++i) {
        auto* button = new QPushButton(navLabels.at(i), sidebar);
        button->setObjectName(navNames.at(i));
        button->setCheckable(true);
        button->setMinimumHeight(42);
        button->setStyleSheet(QStringLiteral(
            "QPushButton { text-align: left; color: #d8e8f0; border: 0; padding: 8px 11px; background: transparent; }"
            "QPushButton:hover { background: #1d4a61; }"
            "QPushButton:checked { background: #176b87; color: white; font-weight: 700; }"));
        navGroup->addButton(button, i);
        sidebarLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, i] { navigateTo(static_cast<Page>(i)); });
    }
    sidebarLayout->addStretch();
    auto* clinicLabel = new QLabel(QStringLiteral("Patient-first care\nOrganized clinic operations"), sidebar);
    clinicLabel->setStyleSheet(QStringLiteral("color: #9eb9c8; font-size: 11px;"));
    sidebarLayout->addWidget(clinicLabel);

    m_pages = new QStackedWidget(central);
    m_pages->setObjectName(QStringLiteral("mainPages"));
    m_pages->addWidget(makeDashboardPage());
    m_pages->addWidget(makePatientsPage());
    m_pages->addWidget(makeAppointmentsPage());
    m_pages->addWidget(makeQueuePage());
    m_pages->addWidget(makeSearchPage());
    m_pages->addWidget(makeReportsPage());
    shell->addWidget(sidebar);
    shell->addWidget(m_pages, 1);
    setCentralWidget(central);

    buildMenus();
    statusBar()->showMessage(QStringLiteral("Ready · no clinic file loaded"));
    navGroup->button(DashboardPage)->setChecked(true);
    refreshAll();
}

void MainWindow::buildMenus()
{
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto* newAction = fileMenu->addAction(QStringLiteral("&New clinic data"));
    newAction->setObjectName(QStringLiteral("actionNew"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, [this] {
        if (confirmDiscardOrSave(QStringLiteral("create a new dataset"))) {
            newDataset();
            showSuccess(QStringLiteral("New clinic dataset created."));
        }
    });
    auto* saveAction = fileMenu->addAction(QStringLiteral("&Save"));
    saveAction->setObjectName(QStringLiteral("actionSave"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { saveUsingDialog(); });
    auto* loadAction = fileMenu->addAction(QStringLiteral("&Load…"));
    loadAction->setObjectName(QStringLiteral("actionLoad"));
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, [this] { loadUsingDialog(); });
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(QStringLiteral("E&xit"));
    exitAction->setObjectName(QStringLiteral("actionExit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    auto* undoAction = editMenu->addAction(QStringLiteral("Undo Last Administrative Operation"));
    undoAction->setObjectName(QStringLiteral("actionUndo"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this] {
        if (!undoLastOperation()) showError(QStringLiteral("There is no administrative operation to undo."));
    });

    auto* searchAction = new QAction(QStringLiteral("Find Patient"), this);
    searchAction->setObjectName(QStringLiteral("actionFind"));
    searchAction->setShortcut(QKeySequence::Find);
    addAction(searchAction);
    connect(searchAction, &QAction::triggered, this, [this] {
        navigateTo(SearchPage);
        if (m_searchText && m_searchText->isVisible()) m_searchText->setFocus();
        else if (m_searchCategory) m_searchCategory->setFocus();
    });
}

QWidget* MainWindow::makeDashboardPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(18);
    layout->addWidget(pageHeading(QStringLiteral("Clinic dashboard"),
        QStringLiteral("A live overview of registered patients, appointments, and the waiting room.")));

    auto* cards = new QHBoxLayout;
    auto addCard = [page, cards](const QString& title, const QString& objectName, const QString& tint) {
        auto* frame = new QFrame(page);
        frame->setObjectName(QStringLiteral("statCard"));
        frame->setStyleSheet(QStringLiteral("QFrame#statCard { background: white; border: 1px solid #e2e8f0; border-radius: 10px; border-top: 4px solid %1; }").arg(tint));
        auto* inner = new QVBoxLayout(frame);
        inner->setContentsMargins(16, 13, 16, 13);
        auto* titleLabel = new QLabel(title, frame);
        titleLabel->setStyleSheet(QStringLiteral("color: #64748b; font-size: 12px; font-weight: 600;"));
        auto* value = new QLabel(QStringLiteral("0"), frame);
        value->setObjectName(objectName);
        value->setStyleSheet(QStringLiteral("font-size: 30px; font-weight: 800; color: #17324d;"));
        inner->addWidget(titleLabel); inner->addWidget(value);
        cards->addWidget(frame);
        return value;
    };
    m_patientCount = addCard(QStringLiteral("REGISTERED PATIENTS"), QStringLiteral("dashboardPatientCount"), QStringLiteral("#176b87"));
    m_appointmentCount = addCard(QStringLiteral("TOTAL APPOINTMENTS"), QStringLiteral("dashboardAppointmentCount"), QStringLiteral("#5672a5"));
    m_pendingCount = addCard(QStringLiteral("PENDING APPOINTMENTS"), QStringLiteral("dashboardPendingCount"), QStringLiteral("#d69b24"));
    m_queueCount = addCard(QStringLiteral("WAITING IN QUEUE"), QStringLiteral("dashboardQueueCount"), QStringLiteral("#50866d"));
    layout->addLayout(cards);

    auto* welcome = new QFrame(page);
    welcome->setStyleSheet(QStringLiteral("background: white; border: 1px solid #e2e8f0; border-radius: 10px;"));
    auto* welcomeLayout = new QVBoxLayout(welcome);
    welcomeLayout->setContentsMargins(20, 18, 20, 18);
    auto* welcomeTitle = new QLabel(QStringLiteral("Clinic operations, in one place"), welcome);
    welcomeTitle->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 700; color: #17324d; border: 0;"));
    auto* welcomeBody = new QLabel(QStringLiteral(
        "Register patients by care category, manage appointments, preserve FIFO order in the waiting queue, "
        "and review live reports. Emergency patients are highlighted as high priority while queue order stays intact."), welcome);
    welcomeBody->setWordWrap(true);
    welcomeBody->setStyleSheet(QStringLiteral("color: #64748b; line-height: 1.4; border: 0;"));
    welcomeLayout->addWidget(welcomeTitle); welcomeLayout->addWidget(welcomeBody);
    layout->addWidget(welcome);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::makePatientsPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(12);
    layout->addWidget(pageHeading(QStringLiteral("Patients"),
        QStringLiteral("Manage patient profiles and care categories. Emergency patients are clearly flagged.")));
    auto* actions = new QHBoxLayout;
    auto* add = actionButton(QStringLiteral("Register patient"), QStringLiteral("registerPatientButton"), page, true);
    auto* edit = actionButton(QStringLiteral("Edit selected"), QStringLiteral("editPatientButton"), page);
    auto* remove = actionButton(QStringLiteral("Delete selected"), QStringLiteral("deletePatientButton"), page);
    actions->addWidget(add); actions->addWidget(edit); actions->addWidget(remove); actions->addStretch();
    layout->addLayout(actions);
    m_patientsTable = makeTable({QStringLiteral("Patient ID"), QStringLiteral("Full name"), QStringLiteral("Age"),
        QStringLiteral("Category"), QStringLiteral("Priority"), QStringLiteral("Phone"), QStringLiteral("Email")}, page);
    m_patientsTable->setObjectName(QStringLiteral("patientsTable"));
    layout->addWidget(m_patientsTable, 1);
    connect(add, &QPushButton::clicked, this, [this] { showPatientDialog(); });
    connect(edit, &QPushButton::clicked, this, [this] {
        const auto patientId = selectedPatientId(m_patientsTable);
        auto patient = m_database.findPatient(patientId);
        if (!patient) showError(QStringLiteral("Select a patient to edit.")); else showPatientDialog(patient);
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        const auto patientId = selectedPatientId(m_patientsTable);
        if (patientId.isEmpty()) { showError(QStringLiteral("Select a patient to delete.")); return; }
        QMessageBox confirmation(QMessageBox::Question, QStringLiteral("Delete patient"),
            QStringLiteral("Delete this patient and all associated appointments and queue entries?"),
            QMessageBox::Yes | QMessageBox::Cancel, this);
        applyLightDialogTheme(&confirmation);
        confirmation.setDefaultButton(QMessageBox::Cancel);
        if (confirmation.exec() != QMessageBox::Yes) return;
        QString error;
        if (!deletePatient(patientId, &error)) showError(error);
    });
    return page;
}

QWidget* MainWindow::makeAppointmentsPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12); layout->setSpacing(12);
    layout->addWidget(pageHeading(QStringLiteral("Appointments"),
        QStringLiteral("Schedule future appointments and manage Pending, Completed, and Cancelled states.")));
    auto* actions = new QHBoxLayout;
    auto* schedule = actionButton(QStringLiteral("Schedule appointment"), QStringLiteral("scheduleAppointmentButton"), page, true);
    auto* complete = actionButton(QStringLiteral("Mark completed"), QStringLiteral("completeAppointmentButton"), page);
    auto* cancel = actionButton(QStringLiteral("Cancel appointment"), QStringLiteral("cancelAppointmentButton"), page);
    actions->addWidget(schedule); actions->addWidget(complete); actions->addWidget(cancel); actions->addStretch();
    layout->addLayout(actions);
    m_appointmentsTable = makeTable({QStringLiteral("Appointment ID"), QStringLiteral("Patient"),
        QStringLiteral("Scheduled date/time"), QStringLiteral("Status"), QStringLiteral("Notes")}, page);
    m_appointmentsTable->setObjectName(QStringLiteral("appointmentsTable"));
    layout->addWidget(m_appointmentsTable, 1);
    connect(schedule, &QPushButton::clicked, this, [this] { showAppointmentDialog(); });
    connect(complete, &QPushButton::clicked, this, [this] {
        const auto id = selectedPatientId(m_appointmentsTable);
        if (id.isEmpty()) { showError(QStringLiteral("Select an appointment to complete.")); return; }
        QString error;
        if (!completeAppointment(id, QDateTime::currentDateTime(), &error)) showError(error);
    });
    connect(cancel, &QPushButton::clicked, this, [this] {
        const auto id = selectedPatientId(m_appointmentsTable);
        if (id.isEmpty()) { showError(QStringLiteral("Select an appointment to cancel.")); return; }
        QString error;
        if (!cancelAppointment(id, &error)) showError(error);
    });
    return page;
}

QWidget* MainWindow::makeQueuePage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12); layout->setSpacing(12);
    layout->addWidget(pageHeading(QStringLiteral("Waiting queue"),
        QStringLiteral("Queue order is strictly FIFO. Emergency status is visible and does not reorder the line.")));
    auto* actions = new QHBoxLayout;
    auto* patient = new QComboBox(page); patient->setObjectName(QStringLiteral("queuePatientCombo"));
    applyLightComboPopupTheme(patient, true);
    auto* enqueue = actionButton(QStringLiteral("Add to queue"), QStringLiteral("enqueuePatientButton"), page, true);
    auto* callNext = actionButton(QStringLiteral("Call next patient"), QStringLiteral("callNextPatientButton"), page);
    actions->addWidget(patient); actions->addWidget(enqueue); actions->addWidget(callNext); actions->addStretch();
    layout->addLayout(actions);
    m_queueTable = makeTable({QStringLiteral("Position"), QStringLiteral("Patient ID"), QStringLiteral("Name"),
        QStringLiteral("Category"), QStringLiteral("Priority")}, page);
    m_queueTable->setObjectName(QStringLiteral("queueTable"));
    layout->addWidget(m_queueTable, 1);
    connect(enqueue, &QPushButton::clicked, this, [this, patient] {
        if (patient->currentIndex() < 0) { showError(QStringLiteral("Register a patient before adding to the queue.")); return; }
        QString error;
        if (!enqueuePatient(patient->currentData().toString(), &error)) showError(error);
    });
    connect(callNext, &QPushButton::clicked, this, [this] {
        const auto called = callNextPatient();
        if (!called) showError(QStringLiteral("The waiting queue is empty."));
        else showSuccess(QStringLiteral("Called next: %1").arg(*called));
    });
    return page;
}

QWidget* MainWindow::makeSearchPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12); layout->setSpacing(11);
    layout->addWidget(pageHeading(QStringLiteral("Search & sort"),
        QStringLiteral("Explore the manually implemented search and sorting algorithms required by the assessment.")));

    auto* searchFrame = new QFrame(page);
    searchFrame->setObjectName(QStringLiteral("searchControlsFrame"));
    searchFrame->setStyleSheet(QStringLiteral(
        "QFrame#searchControlsFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 9px; }"));
    auto* searchLayout = new QVBoxLayout(searchFrame);
    searchLayout->setContentsMargins(18, 16, 18, 16);
    searchLayout->setSpacing(12);
    auto* searchForm = new QHBoxLayout;
    searchForm->setContentsMargins(0, 0, 0, 0);
    searchForm->setSpacing(12);
    m_searchField = new QComboBox(searchFrame); m_searchField->setObjectName(QStringLiteral("searchFieldCombo"));
    m_searchField->addItems({QStringLiteral("Patient ID"), QStringLiteral("Patient name"), QStringLiteral("Category")});
    applyLightComboPopupTheme(m_searchField, false, true);
    m_searchField->setMinimumWidth(142);
    m_searchAlgorithm = new QComboBox(searchFrame); m_searchAlgorithm->setObjectName(QStringLiteral("searchAlgorithmCombo"));
    applyLightComboPopupTheme(m_searchAlgorithm, false, true);
    m_searchAlgorithm->setMinimumWidth(158);
    m_searchText = new QLineEdit(searchFrame); m_searchText->setObjectName(QStringLiteral("searchQueryEdit"));
    m_searchText->setPlaceholderText(QStringLiteral("Enter search text…"));
    m_searchText->setMinimumWidth(180);
    m_searchText->setMinimumHeight(40);
    m_searchText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_searchCategory = new QComboBox(searchFrame); m_searchCategory->setObjectName(QStringLiteral("searchCategoryCombo"));
    m_searchCategory->addItems({QStringLiteral("General"), QStringLiteral("Emergency"), QStringLiteral("Follow-Up")});
    applyLightComboPopupTheme(m_searchCategory, false, true);
    m_searchCategory->setMinimumWidth(142);
    auto* find = actionButton(QStringLiteral("Search"), QStringLiteral("runSearchButton"), searchFrame, true);
    find->setMinimumSize(104, 40);
    find->setStyleSheet(QStringLiteral(
        "QPushButton#runSearchButton { color: #ffffff; background-color: #176b87; border: 1px solid #176b87;"
        " border-radius: 6px; padding: 7px 16px; font-weight: 600; }"
        "QPushButton#runSearchButton:hover { color: #ffffff; background-color: #12566d; border-color: #12566d; }"));
    searchForm->addWidget(new QLabel(QStringLiteral("Field"), searchFrame)); searchForm->addWidget(m_searchField);
    searchForm->addWidget(new QLabel(QStringLiteral("Algorithm"), searchFrame)); searchForm->addWidget(m_searchAlgorithm);
    searchForm->addWidget(m_searchText, 1); searchForm->addWidget(m_searchCategory); searchForm->addWidget(find);
    searchLayout->addLayout(searchForm);
    m_searchHelp = new QLabel(searchFrame); m_searchHelp->setObjectName(QStringLiteral("searchHelpLabel"));
    m_searchHelp->setWordWrap(true); m_searchHelp->setStyleSheet(QStringLiteral("color: #64748b; border: 0; padding: 2px;"));
    searchLayout->addWidget(m_searchHelp);
    layout->addWidget(searchFrame);

    auto* sortFrame = new QFrame(page);
    sortFrame->setObjectName(QStringLiteral("sortControlsFrame"));
    sortFrame->setStyleSheet(QStringLiteral(
        "QFrame#sortControlsFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 9px; }"));
    auto* sortLayout = new QHBoxLayout(sortFrame);
    sortLayout->setContentsMargins(18, 14, 18, 14);
    sortLayout->setSpacing(12);
    m_sortField = new QComboBox(sortFrame); m_sortField->setObjectName(QStringLiteral("sortFieldCombo"));
    m_sortField->addItems({QStringLiteral("Patient name"), QStringLiteral("Patient ID"), QStringLiteral("Age")});
    applyLightComboPopupTheme(m_sortField, false, true);
    m_sortField->setMinimumWidth(150);
    m_sortAlgorithm = new QComboBox(sortFrame); m_sortAlgorithm->setObjectName(QStringLiteral("sortAlgorithmCombo"));
    m_sortAlgorithm->addItems({QStringLiteral("Insertion Sort"), QStringLiteral("Selection Sort")});
    applyLightComboPopupTheme(m_sortAlgorithm, false, true);
    m_sortAlgorithm->setMinimumWidth(170);
    auto* ascending = new QCheckBox(QStringLiteral("Ascending"), sortFrame); ascending->setObjectName(QStringLiteral("sortAscendingCheck")); ascending->setChecked(true);
    ascending->setMinimumHeight(40);
    auto* sort = actionButton(QStringLiteral("Sort all patients"), QStringLiteral("runSortButton"), sortFrame);
    sort->setMinimumSize(156, 40);
    sortLayout->addWidget(new QLabel(QStringLiteral("Sort by"), sortFrame)); sortLayout->addWidget(m_sortField);
    sortLayout->addWidget(m_sortAlgorithm); sortLayout->addWidget(ascending); sortLayout->addWidget(sort); sortLayout->addStretch();
    layout->addWidget(sortFrame);

    m_resultsTable = makeTable({QStringLiteral("Patient ID"), QStringLiteral("Full name"), QStringLiteral("Age"),
        QStringLiteral("Category"), QStringLiteral("Priority"), QStringLiteral("Phone"), QStringLiteral("Email")}, page);
    m_resultsTable->setObjectName(QStringLiteral("searchResultsTable"));
    layout->addWidget(m_resultsTable, 1);
    connect(m_searchField, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { updateSearchControls(); });
    connect(find, &QPushButton::clicked, this, [this] { searchPatients(); });
    connect(m_searchText, &QLineEdit::returnPressed, this, [this] { searchPatients(); });
    connect(sort, &QPushButton::clicked, this, [this] { sortPatients(); });
    updateSearchControls();
    return page;
}

QWidget* MainWindow::makeReportsPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12); layout->setSpacing(12);
    layout->addWidget(pageHeading(QStringLiteral("Reports"),
        QStringLiteral("Reports are calculated from the current clinic records and appointment history.")));
    auto* controls = new QHBoxLayout;
    m_reportType = new QComboBox(page); m_reportType->setObjectName(QStringLiteral("reportTypeCombo"));
    m_reportType->addItems({QStringLiteral("Most Frequently Attended Patients"),
        QStringLiteral("Patients with Pending Appointments"), QStringLiteral("Patient Category Distribution"),
        QStringLiteral("Daily Appointment Summary")});
    applyLightComboPopupTheme(m_reportType);
    m_reportDate = new QDateEdit(QDate::currentDate(), page); m_reportDate->setObjectName(QStringLiteral("reportDateEdit"));
    m_reportDate->setCalendarPopup(true); m_reportDate->setDisplayFormat(QStringLiteral("dd MMM yyyy"));
    applyLightCalendarTheme(m_reportDate->calendarWidget());
    auto* generate = actionButton(QStringLiteral("Generate report"), QStringLiteral("generateReportButton"), page, true);
    controls->addWidget(m_reportType, 1); controls->addWidget(m_reportDate); controls->addWidget(generate);
    layout->addLayout(controls);
    m_reportSummary = new QLabel(page); m_reportSummary->setObjectName(QStringLiteral("reportSummaryLabel"));
    m_reportSummary->setWordWrap(true); m_reportSummary->setStyleSheet(QStringLiteral("font-weight: 600; color: #31516b;"));
    layout->addWidget(m_reportSummary);
    m_reportsTable = makeTable({QStringLiteral("Report detail"), QStringLiteral("Value")}, page);
    m_reportsTable->setObjectName(QStringLiteral("reportsTable"));
    layout->addWidget(m_reportsTable, 1);
    connect(generate, &QPushButton::clicked, this, [this] { refreshReport(); });
    connect(m_reportType, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { updateReportControls(); refreshReport(); });
    updateReportControls();
    return page;
}

void MainWindow::navigateTo(Page page)
{
    m_pages->setCurrentIndex(static_cast<int>(page));
    if (page == DashboardPage) refreshDashboard();
    else if (page == PatientsPage) refreshPatients();
    else if (page == AppointmentsPage) refreshAppointments();
    else if (page == QueuePage) refreshQueue();
    else if (page == ReportsPage) refreshReport();
}

void MainWindow::refreshAll()
{
    refreshDashboard(); refreshPatients(); refreshAppointments(); refreshQueue();
    if (m_resultsTable) m_resultsTable->setRowCount(0);
    if (m_reportsTable) refreshReport();
}

void MainWindow::refreshDashboard()
{
    if (!m_patientCount) return;
    std::size_t pending = 0;
    for (const auto& appointment : m_database.appointments())
        if (appointment.status() == models::AppointmentStatus::Pending) ++pending;
    m_patientCount->setText(QString::number(m_database.patients().size()));
    m_appointmentCount->setText(QString::number(m_database.appointments().size()));
    m_pendingCount->setText(QString::number(pending));
    m_queueCount->setText(QString::number(m_database.waitingQueue().size()));
}

void MainWindow::refreshPatients()
{
    if (!m_patientsTable) return;
    m_patientsTable->setRowCount(0);
    for (const auto& patient : m_database.patients()) {
        const int row = m_patientsTable->rowCount(); m_patientsTable->insertRow(row);
        m_patientsTable->setItem(row, 0, cell(patient->patientId(), patient->patientId()));
        m_patientsTable->setItem(row, 1, cell(patient->fullName()));
        m_patientsTable->setItem(row, 2, cell(QString::number(patient->age())));
        m_patientsTable->setItem(row, 3, cell(categoryString(patient->category())));
        m_patientsTable->setItem(row, 4, cell(patient->priorityLabel()));
        m_patientsTable->setItem(row, 5, cell(patient->contactInformation().phoneNumber));
        m_patientsTable->setItem(row, 6, cell(patient->contactInformation().email));
        if (patient->category() == models::PatientCategory::Emergency) {
            for (int column = 0; column < m_patientsTable->columnCount(); ++column) {
                m_patientsTable->item(row, column)->setBackground(QColor(QStringLiteral("#fff0ee")));
                if (column == 3 || column == 4) m_patientsTable->item(row, column)->setForeground(QColor(QStringLiteral("#a43127")));
            }
            m_patientsTable->item(row, 4)->setText(QStringLiteral("⚠ High Priority"));
        }
    }
}

void MainWindow::refreshAppointments()
{
    if (!m_appointmentsTable) return;
    m_appointmentsTable->setRowCount(0);
    for (const auto& appointment : m_database.appointments()) {
        const auto patient = m_database.findPatient(appointment.patientId());
        const int row = m_appointmentsTable->rowCount(); m_appointmentsTable->insertRow(row);
        m_appointmentsTable->setItem(row, 0, cell(appointment.appointmentId(), appointment.appointmentId()));
        m_appointmentsTable->setItem(row, 1, cell(patient ? QStringLiteral("%1 (%2)").arg(patient->fullName(), patient->patientId()) : appointment.patientId()));
        m_appointmentsTable->setItem(row, 2, cell(appointment.scheduledAt().toLocalTime().toString(QStringLiteral("dd MMM yyyy  HH:mm"))));
        m_appointmentsTable->setItem(row, 3, cell(appointment.statusLabel()));
        m_appointmentsTable->setItem(row, 4, cell(appointment.notes()));
        m_appointmentsTable->item(row, 3)->setBackground(QColor(statusColor(appointment.status())));
        m_appointmentsTable->item(row, 3)->setFont(QFont(QString(), -1, QFont::DemiBold));
    }
}

void MainWindow::refreshQueue()
{
    if (!m_queueTable) return;
    m_queueTable->setRowCount(0);
    const auto ids = m_database.waitingQueue();
    int position = 1;
    for (const auto& id : ids) {
        const auto patient = m_database.findPatient(id);
        if (!patient) continue;
        const int row = m_queueTable->rowCount(); m_queueTable->insertRow(row);
        m_queueTable->setItem(row, 0, cell(QString::number(position++)));
        m_queueTable->setItem(row, 1, cell(id, id));
        m_queueTable->setItem(row, 2, cell(patient->fullName()));
        m_queueTable->setItem(row, 3, cell(categoryString(patient->category())));
        m_queueTable->setItem(row, 4, cell(patient->priorityLabel()));
        if (patient->category() == models::PatientCategory::Emergency) {
            for (int column = 0; column < m_queueTable->columnCount(); ++column)
                m_queueTable->item(row, column)->setBackground(QColor(QStringLiteral("#fff0ee")));
            m_queueTable->item(row, 4)->setText(QStringLiteral("⚠ High Priority"));
            m_queueTable->item(row, 4)->setForeground(QColor(QStringLiteral("#a43127")));
        }
    }
    auto* combo = findChild<QComboBox*>(QStringLiteral("queuePatientCombo"));
    if (combo) {
        const auto previouslySelected = combo->currentData().toString();
        combo->clear();
        for (const auto& patient : m_database.patients())
            combo->addItem(QStringLiteral("%1 · %2 (%3)").arg(patient->patientId(), patient->fullName(), patient->priorityLabel()), patient->patientId());
        const int priorIndex = combo->findData(previouslySelected);
        if (priorIndex >= 0) combo->setCurrentIndex(priorIndex);
        applyLightComboPopupTheme(combo, true);
    }
}

void MainWindow::refreshSearchResults(const std::vector<data::ClinicDatabase::PatientPointer>& records)
{
    if (!m_resultsTable) return;
    m_resultsTable->setRowCount(0);
    for (const auto& patient : records) {
        const int row = m_resultsTable->rowCount(); m_resultsTable->insertRow(row);
        m_resultsTable->setItem(row, 0, cell(patient->patientId(), patient->patientId()));
        m_resultsTable->setItem(row, 1, cell(patient->fullName()));
        m_resultsTable->setItem(row, 2, cell(QString::number(patient->age())));
        m_resultsTable->setItem(row, 3, cell(categoryString(patient->category())));
        m_resultsTable->setItem(row, 4, cell(patient->priorityLabel()));
        m_resultsTable->setItem(row, 5, cell(patient->contactInformation().phoneNumber));
        m_resultsTable->setItem(row, 6, cell(patient->contactInformation().email));
        if (patient->category() == models::PatientCategory::Emergency)
            for (int column = 0; column < m_resultsTable->columnCount(); ++column)
                m_resultsTable->item(row, column)->setBackground(QColor(QStringLiteral("#fff0ee")));
    }
}

void MainWindow::refreshReport()
{
    if (!m_reportsTable || !m_reportType) return;
    m_reportsTable->setRowCount(0);
    auto addRow = [this](const QString& label, const QString& value) {
        const int row = m_reportsTable->rowCount(); m_reportsTable->insertRow(row);
        m_reportsTable->setItem(row, 0, cell(label)); m_reportsTable->setItem(row, 1, cell(value));
    };
    switch (m_reportType->currentIndex()) {
    case 0: {
        const auto rows = controllers::ClinicReports::mostFrequentlyAttended(m_database);
        m_reportSummary->setText(QStringLiteral("Patients with completed appointments, ranked by completed visits."));
        for (const auto& row : rows) addRow(QStringLiteral("%1 · %2").arg(row.patient->patientId(), row.patient->fullName()), QString::number(row.completedAppointments));
        if (rows.empty()) addRow(QStringLiteral("No completed appointments yet"), QStringLiteral("0"));
        break;
    }
    case 1: {
        const auto rows = controllers::ClinicReports::patientsWithPendingAppointments(m_database);
        m_reportSummary->setText(QStringLiteral("Patients who currently have at least one Pending appointment."));
        for (const auto& row : rows) addRow(QStringLiteral("%1 · %2").arg(row.patient->patientId(), row.patient->fullName()), QString::number(row.pendingAppointments));
        if (rows.empty()) addRow(QStringLiteral("No pending appointments"), QStringLiteral("0"));
        break;
    }
    case 2: {
        const auto report = controllers::ClinicReports::patientCategoryDistribution(m_database);
        m_reportSummary->setText(QStringLiteral("Registered patient counts by category (%1 total)." ).arg(report.total()));
        addRow(QStringLiteral("General"), QString::number(report.count(models::PatientCategory::General)));
        addRow(QStringLiteral("Emergency"), QString::number(report.count(models::PatientCategory::Emergency)));
        addRow(QStringLiteral("Follow-Up"), QString::number(report.count(models::PatientCategory::FollowUp)));
        break;
    }
    case 3: {
        const auto report = controllers::ClinicReports::dailyAppointmentSummary(m_database, m_reportDate->date());
        m_reportSummary->setText(QStringLiteral("Appointments scheduled on %1 · %2 total.")
                                 .arg(report.date.toString(QStringLiteral("dddd, d MMMM yyyy"))).arg(report.total));
        addRow(QStringLiteral("Pending"), QString::number(report.pending));
        addRow(QStringLiteral("Completed"), QString::number(report.completed));
        addRow(QStringLiteral("Cancelled"), QString::number(report.cancelled));
        addRow(QStringLiteral("Total"), QString::number(report.total));
        break;
    }
    default: break;
    }
}

void MainWindow::showPatientDialog(data::ClinicDatabase::PatientPointer patient)
{
    const bool editing = static_cast<bool>(patient);
    QDialog dialog(this);
    applyLightDialogTheme(&dialog);
    dialog.setWindowTitle(editing ? QStringLiteral("Edit patient") : QStringLiteral("Register patient"));
    dialog.setMinimumWidth(430);
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* idLabel = new QLabel(editing ? patient->patientId() : m_clinicManager.nextPatientId(), &dialog);
    idLabel->setObjectName(QStringLiteral("patientIdPreview"));
    auto* name = new QLineEdit(editing ? patient->fullName() : QString(), &dialog);
    name->setObjectName(QStringLiteral("patientNameEdit"));
    auto* age = new QSpinBox(&dialog); age->setObjectName(QStringLiteral("patientAgeSpin")); age->setRange(0, 120);
    if (editing) age->setValue(patient->age()); else age->setValue(18);
    auto* category = new QComboBox(&dialog); category->setObjectName(QStringLiteral("patientCategoryCombo"));
    category->addItems({QStringLiteral("General"), QStringLiteral("Emergency"), QStringLiteral("Follow-Up")});
    applyLightComboPopupTheme(category);
    if (editing) category->setCurrentIndex(static_cast<int>(patient->category()));
    category->setEnabled(!editing);
    auto* phone = new QLineEdit(editing ? patient->contactInformation().phoneNumber : QString(), &dialog);
    phone->setObjectName(QStringLiteral("patientPhoneEdit"));
    auto* email = new QLineEdit(editing ? patient->contactInformation().email : QString(), &dialog);
    email->setObjectName(QStringLiteral("patientEmailEdit"));
    auto* reason = new QLineEdit(&dialog); reason->setObjectName(QStringLiteral("followUpReasonEdit"));
    if (editing && patient->category() == models::PatientCategory::FollowUp) {
        const auto* followUp = dynamic_cast<const models::FollowUpPatient*>(patient.get());
        if (followUp) reason->setText(followUp->followUpReason());
    }
    auto* reasonLabel = new QLabel(QStringLiteral("Follow-up reason"), &dialog);
    form->addRow(QStringLiteral("Patient ID"), idLabel);
    form->addRow(QStringLiteral("Full name"), name);
    form->addRow(QStringLiteral("Age"), age);
    form->addRow(QStringLiteral("Category"), category);
    form->addRow(QStringLiteral("Phone number"), phone);
    form->addRow(QStringLiteral("Email (optional)"), email);
    form->addRow(reasonLabel, reason);
    auto updateReason = [category, reasonLabel, reason] {
        const bool visible = category->currentIndex() == static_cast<int>(models::PatientCategory::FollowUp);
        reasonLabel->setVisible(visible); reason->setVisible(visible);
    };
    connect(category, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [updateReason] { updateReason(); });
    updateReason();
    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    QString error;
    const bool saved = editing
        ? editPatient(patient->patientId(), name->text(), age->value(), phone->text(), email->text(), reason->text(), &error)
        : registerPatient(category->currentText(), name->text(), age->value(), phone->text(), email->text(), reason->text(), &error);
    if (!saved) showError(error);
}

void MainWindow::showAppointmentDialog()
{
    if (m_database.patients().empty()) { showError(QStringLiteral("Register at least one patient before scheduling an appointment.")); return; }
    QDialog dialog(this); applyLightDialogTheme(&dialog);
    dialog.setWindowTitle(QStringLiteral("Schedule appointment"));
    auto* layout = new QVBoxLayout(&dialog); auto* form = new QFormLayout;
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* id = new QLabel(m_appointmentController.nextAppointmentId(), &dialog); id->setObjectName(QStringLiteral("appointmentIdPreview"));
    auto* patient = new QComboBox(&dialog); patient->setObjectName(QStringLiteral("appointmentPatientCombo"));
    patient->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    for (const auto& record : m_database.patients()) {
        const QString display = QStringLiteral("%1 · %2 (%3)")
                                    .arg(record->patientId(), record->fullName(), record->priorityLabel());
        patient->addItem(display, record->patientId());
    }
    applyLightComboPopupTheme(patient, true);
    const int patientControlWidth = patient->minimumWidth();
    patient->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* when = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), &dialog);
    when->setObjectName(QStringLiteral("appointmentDateTimeEdit")); when->setCalendarPopup(true);
    when->setDisplayFormat(QStringLiteral("ddd, d MMM yyyy  HH:mm")); when->setMinimumDateTime(QDateTime::currentDateTime().addSecs(60));
    when->setMinimumWidth(360);
    when->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    applyLightCalendarTheme(when->calendarWidget());
    auto* notes = new QLineEdit(&dialog); notes->setObjectName(QStringLiteral("appointmentNotesEdit"));
    notes->setMinimumWidth(360);
    notes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    form->addRow(QStringLiteral("Appointment ID"), id); form->addRow(QStringLiteral("Patient"), patient);
    form->addRow(QStringLiteral("Date and time"), when); form->addRow(QStringLiteral("Notes"), notes);
    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->setOrientation(Qt::Horizontal);
    layout->addWidget(buttons); connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.setMinimumWidth(std::max(680, patientControlWidth + 190));
    if (dialog.exec() != QDialog::Accepted) return;
    QString error;
    if (!scheduleAppointment(patient->currentData().toString(), when->dateTime(), notes->text(), &error)) showError(error);
}

bool MainWindow::registerPatient(QString category, QString fullName, int age, QString phone,
                                 QString email, QString followUpReason, QString* error)
{
    try {
        const QString id = m_clinicManager.nextPatientId();
        const models::ContactInformation contact{std::move(phone), std::move(email)};
        std::shared_ptr<const models::PatientRecord> patient;
        if (category == QStringLiteral("General"))
            patient = std::make_shared<models::GeneralPatient>(id, std::move(fullName), age, contact);
        else if (category == QStringLiteral("Emergency"))
            patient = std::make_shared<models::EmergencyPatient>(id, std::move(fullName), age, contact);
        else if (category == QStringLiteral("Follow-Up") || category == QStringLiteral("FollowUp"))
            patient = std::make_shared<models::FollowUpPatient>(id, std::move(fullName), age, contact, std::move(followUpReason));
        else throw std::invalid_argument("Select a valid patient category.");
        m_clinicManager.registerPatient(std::move(patient));
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Patient registered with ID %1.").arg(id));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Patient was not registered: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::editPatient(const QString& patientId, QString fullName, int age, QString phone,
                             QString email, QString followUpReason, QString* error)
{
    try {
        const auto patient = m_database.requirePatient(patientId);
        std::optional<QString> reason;
        if (patient->category() == models::PatientCategory::FollowUp) reason = std::move(followUpReason);
        m_clinicManager.editPatient(patientId, std::move(fullName), age,
                                    {std::move(phone), std::move(email)}, std::move(reason));
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Patient %1 updated.").arg(patientId));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Patient update failed: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::deletePatient(const QString& patientId, QString* error)
{
    try {
        m_clinicManager.deletePatient(patientId);
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Patient %1 and related records deleted.").arg(patientId));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Patient deletion failed: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::scheduleAppointment(QString patientId, QDateTime when, QString notes, QString* error)
{
    try {
        const QString id = m_appointmentController.nextAppointmentId();
        m_appointmentController.scheduleAppointment(id, patientId, std::move(when), std::move(notes));
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Appointment %1 scheduled.").arg(id));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Appointment scheduling failed: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::cancelAppointment(const QString& appointmentId, QString* error)
{
    try {
        m_appointmentController.cancelAppointment(appointmentId);
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Appointment %1 cancelled.").arg(appointmentId));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Appointment cancellation failed: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::completeAppointment(const QString& appointmentId, QDateTime now, QString* error)
{
    try {
        m_appointmentController.completeAppointment(appointmentId, std::move(now));
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Appointment %1 marked Completed.").arg(appointmentId));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Appointment completion failed: %1").arg(message), 7000);
        return false;
    }
}

bool MainWindow::enqueuePatient(const QString& patientId, QString* error)
{
    try {
        m_clinicManager.enqueuePatient(patientId);
        setDirty(); refreshAll(); showSuccess(QStringLiteral("Patient %1 added to the end of the queue.").arg(patientId));
        return true;
    } catch (const std::exception& exception) {
        const QString message = QString::fromUtf8(exception.what()); if (error) *error = message;
        statusBar()->showMessage(QStringLiteral("Could not update the queue: %1").arg(message), 7000);
        return false;
    }
}

std::optional<QString> MainWindow::callNextPatient()
{
    const auto patientId = m_clinicManager.callNextPatient();
    if (patientId) { setDirty(); refreshAll(); }
    return patientId;
}

bool MainWindow::undoLastOperation()
{
    if (!m_clinicManager.undoLastAdministrativeOperation()) return false;
    setDirty(); refreshAll(); showSuccess(QStringLiteral("Last administrative operation undone."));
    return true;
}

persistence::PersistenceResult MainWindow::saveToFile(const QString& path)
{
    const auto result = persistence::FileHandler::save(m_database, path);
    if (result.success) { m_currentFilePath = path; m_dirty = false; updateWindowTitle(); }
    statusBar()->showMessage(result.message, 6000);
    return result;
}

persistence::PersistenceResult MainWindow::loadFromFile(const QString& path)
{
    const auto result = persistence::FileHandler::load(m_database, path);
    if (result.success) { m_currentFilePath = path; m_dirty = false; updateWindowTitle(); refreshAll(); }
    statusBar()->showMessage(result.message, 7000);
    return result;
}

void MainWindow::newDataset()
{
    data::ClinicDatabase empty;
    m_database.replaceWithValidatedState(std::move(empty));
    m_currentFilePath.clear(); m_dirty = false; updateWindowTitle(); refreshAll();
}

void MainWindow::showError(const QString& message)
{
    statusBar()->showMessage(message, 7000);
    QMessageBox alert(QMessageBox::Warning, QStringLiteral("SCAPMS"), message, QMessageBox::Ok, this);
    applyLightDialogTheme(&alert);
    alert.exec();
}

void MainWindow::showSuccess(const QString& message)
{
    statusBar()->showMessage(message, 5000);
}

bool MainWindow::confirmDiscardOrSave(const QString& operation)
{
    if (!m_dirty) return true;
    QMessageBox box(QMessageBox::Warning, QStringLiteral("Unsaved clinic changes"),
        QStringLiteral("Save your changes before you %1?").arg(operation), QMessageBox::NoButton, this);
    auto* save = box.addButton(QStringLiteral("Save"), QMessageBox::AcceptRole);
    auto* discard = box.addButton(QStringLiteral("Discard"), QMessageBox::DestructiveRole);
    auto* cancel = box.addButton(QMessageBox::Cancel);
    applyLightDialogTheme(&box);
    box.setDefaultButton(qobject_cast<QPushButton*>(save));
    box.exec();
    if (box.clickedButton() == save) return saveUsingDialog();
    return box.clickedButton() == discard && box.clickedButton() != cancel;
}

bool MainWindow::saveUsingDialog()
{
    QString path = m_currentFilePath;
    if (path.isEmpty()) path = QFileDialog::getSaveFileName(this, QStringLiteral("Save clinic data"),
                                                           QString(), QStringLiteral("SCAPMS JSON (*.json)"));
    if (path.isEmpty()) return false;
    const auto result = saveToFile(path);
    if (!result.success) showError(result.message);
    return result.success;
}

void MainWindow::loadUsingDialog()
{
    if (!confirmDiscardOrSave(QStringLiteral("load another dataset"))) return;
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load clinic data"),
                                                       QString(), QStringLiteral("SCAPMS JSON (*.json);;All files (*)"));
    if (path.isEmpty()) return;
    const auto result = loadFromFile(path);
    if (!result.success) showError(result.message);
}

void MainWindow::searchPatients()
{
    QString error;
    try {
        const auto records = m_clinicManager.listPatients();
        std::vector<data::ClinicDatabase::PatientPointer> matches;
        if (m_searchField->currentIndex() == 0) {
            if (m_searchAlgorithm->currentData().toString() == QStringLiteral("binary")) {
                const auto sorted = controllers::PatientAlgorithms::sort(records, controllers::PatientSortField::Id,
                    controllers::SortAlgorithm::Insertion, true);
                const auto result = controllers::PatientAlgorithms::binarySearchById(sorted, m_searchText->text());
                if (result) matches.push_back(result);
            } else {
                const auto result = controllers::PatientAlgorithms::linearSearchById(records, m_searchText->text());
                if (result) matches.push_back(result);
            }
        } else if (m_searchField->currentIndex() == 1) {
            matches = controllers::PatientAlgorithms::recursiveSearchByName(records, m_searchText->text());
        } else {
            const auto category = static_cast<models::PatientCategory>(m_searchCategory->currentIndex());
            matches = controllers::PatientAlgorithms::searchByCategory(records, category);
        }
        refreshSearchResults(matches);
        showSuccess(QStringLiteral("Search complete · %1 result(s). Use the visible algorithm selector to see the method used.").arg(matches.size()));
    } catch (const std::exception& exception) {
        error = QString::fromUtf8(exception.what());
        m_resultsTable->setRowCount(0);
        showError(error);
    }
}

void MainWindow::sortPatients()
{
    try {
        const auto field = static_cast<controllers::PatientSortField>(m_sortField->currentIndex());
        const auto algorithm = m_sortAlgorithm->currentIndex() == 0
            ? controllers::SortAlgorithm::Insertion : controllers::SortAlgorithm::Selection;
        auto* ascending = findChild<QCheckBox*>(QStringLiteral("sortAscendingCheck"));
        const auto sorted = controllers::PatientAlgorithms::sort(m_clinicManager.listPatients(), field,
            algorithm, ascending ? ascending->isChecked() : true);
        refreshSearchResults(sorted.records());
        showSuccess(QStringLiteral("Sorted %1 patients using %2.").arg(sorted.records().size()).arg(m_sortAlgorithm->currentText()));
    } catch (const std::exception& exception) { showError(QString::fromUtf8(exception.what())); }
}

void MainWindow::updateSearchControls()
{
    if (!m_searchField || !m_searchAlgorithm) return;
    m_searchAlgorithm->clear();
    if (m_searchField->currentIndex() == 0) {
        m_searchAlgorithm->addItem(QStringLiteral("Linear Search"), QStringLiteral("linear"));
        m_searchAlgorithm->addItem(QStringLiteral("Binary Search"), QStringLiteral("binary"));
        m_searchHelp->setText(QStringLiteral("ID: Linear Search scans records directly. Binary Search uses a manually sorted ascending ID copy before searching."));
        m_searchText->show(); m_searchCategory->hide();
    } else if (m_searchField->currentIndex() == 1) {
        m_searchAlgorithm->addItem(QStringLiteral("Recursive Search"), QStringLiteral("recursive"));
        m_searchHelp->setText(QStringLiteral("Name: Recursive Search checks each patient name case-insensitively and returns partial matches."));
        m_searchText->show(); m_searchCategory->hide();
    } else {
        m_searchAlgorithm->addItem(QStringLiteral("Linear Category Search"), QStringLiteral("category"));
        m_searchHelp->setText(QStringLiteral("Category: Linear Search checks each patient’s category. Emergency priority never changes queue order."));
        m_searchText->hide(); m_searchCategory->show();
    }
    applyLightComboPopupTheme(m_searchAlgorithm, false, true);
}

void MainWindow::updateReportControls()
{
    if (m_reportDate) m_reportDate->setVisible(m_reportType && m_reportType->currentIndex() == 3);
}

QString MainWindow::selectedPatientId(const QTableWidget* table) const
{
    if (!table || table->currentRow() < 0 || !table->item(table->currentRow(), 0)) return {};
    return table->item(table->currentRow(), 0)->data(Qt::UserRole).toString();
}

void MainWindow::setDirty()
{
    m_dirty = true;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString caption = QStringLiteral("SCAPMS | Smart Clinic Management");
    if (!m_currentFilePath.isEmpty()) caption += QStringLiteral(" — %1").arg(QFileInfo(m_currentFilePath).fileName());
    if (m_dirty) caption += QStringLiteral(" *");
    setWindowTitle(caption);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmDiscardOrSave(QStringLiteral("exit SCAPMS"))) event->accept();
    else event->ignore();
}

} // namespace scapms::gui
