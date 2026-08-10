#include "stdafx.h"
#include "MemorySyncView.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QFrame* statCard(const QString& title, QLabel** value, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setProperty("m3Card", true);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(4);
    auto* label = new QLabel(title, card);
    label->setProperty("m3Label", QStringLiteral("small"));
    *value = new QLabel(QStringLiteral("—"), card);
    (*value)->setProperty("m3Title", QStringLiteral("medium"));
    layout->addWidget(label);
    layout->addWidget(*value);
    return card;
}
}

CMemorySyncView::CMemorySyncView(const QString& repositoryRoot, QWidget* parent)
    : QWidget(parent), m_repository(repositoryRoot), m_rootLabel(new QLabel(this)),
      m_stateLabel(nullptr), m_targetCount(nullptr), m_contractCount(nullptr),
      m_refreshTime(nullptr), m_conflictBanner(new QLabel(this)), m_targets(new QTableWidget(this))
{
    setObjectName(QStringLiteral("memorySyncView"));
    setProperty("memoryOwned", true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Memory Sync"), this);
    title->setProperty("m3Title", QStringLiteral("display-small"));
    auto* subtitle = new QLabel(tr("Read-only status from a configured local agent-memory checkout."), this);
    subtitle->setWordWrap(true);
    subtitle->setProperty("m3Text", QStringLiteral("secondary"));
    root->addWidget(title);
    root->addWidget(subtitle);
    m_rootLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_rootLabel->setWordWrap(true);
    root->addWidget(m_rootLabel);

    auto* cards = new QGridLayout;
    cards->setHorizontalSpacing(12);
    cards->setVerticalSpacing(12);
    cards->addWidget(statCard(tr("Repository"), &m_stateLabel, this), 0, 0);
    cards->addWidget(statCard(tr("Targets"), &m_targetCount, this), 0, 1);
    cards->addWidget(statCard(tr("Contracts"), &m_contractCount, this), 0, 2);
    cards->addWidget(statCard(tr("Refreshed"), &m_refreshTime, this), 0, 3);
    root->addLayout(cards);

    m_conflictBanner->setObjectName(QStringLiteral("memorySyncConflictBanner"));
    m_conflictBanner->setProperty("m3Banner", QStringLiteral("warning"));
    m_conflictBanner->setWordWrap(true);
    m_conflictBanner->setVisible(false);
    root->addWidget(m_conflictBanner);

    m_targets->setColumnCount(3);
    m_targets->setHorizontalHeaderLabels({tr("Target"), tr("Path"), tr("Evidence")});
    m_targets->horizontalHeader()->setStretchLastSection(true);
    m_targets->verticalHeader()->hide();
    m_targets->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_targets->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_targets->setAlternatingRowColors(false);
    m_targets->setMinimumHeight(220);
    root->addWidget(m_targets, 1);

    auto* actions = new QHBoxLayout;
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setProperty("m3", QStringLiteral("tonal"));
    auto* statusButton = new QPushButton(tr("Copy status command"), this);
    auto* dryRunButton = new QPushButton(tr("Copy dry-run command"), this);
    auto* installButton = new QPushButton(tr("Copy install command"), this);
    installButton->setProperty("m3", QStringLiteral("filled"));
    actions->addWidget(refreshButton);
    actions->addStretch(1);
    actions->addWidget(statusButton);
    actions->addWidget(dryRunButton);
    actions->addWidget(installButton);
    root->addLayout(actions);

    connect(refreshButton, &QPushButton::clicked, this, &CMemorySyncView::refresh);
    connect(statusButton, &QPushButton::clicked, this, [this] { copyCommand(QStringLiteral("pwsh ./scripts/sync-agent-memory.ps1 status")); });
    connect(dryRunButton, &QPushButton::clicked, this, [this] { copyCommand(QStringLiteral("pwsh ./scripts/sync-agent-memory.ps1 install")); });
    connect(installButton, &QPushButton::clicked, this, [this] { copyCommand(QStringLiteral("pwsh ./scripts/sync-agent-memory.ps1 install -Yes")); });
    refresh();
}

void CMemorySyncView::setRepositoryRoot(const QString& root)
{
    m_repository.setRoot(root);
    refresh();
}

void CMemorySyncView::copyCommand(const QString& command)
{
    QApplication::clipboard()->setText(command);
}

void CMemorySyncView::refresh()
{
    m_targets->setRowCount(0);
    const QString canonical = m_repository.canonicalRoot();
    m_rootLabel->setText(canonical.isEmpty()
        ? tr("Repository: unavailable. Set SANDMAN_MEMORY_ROOT to a local checkout.")
        : tr("Repository: %1").arg(QDir::toNativeSeparators(canonical)));
    m_stateLabel->setText(canonical.isEmpty() ? tr("Unavailable") : tr("Local / read-only"));
    m_refreshTime->setText(QDateTime::currentDateTime().toString(Qt::ISODate));

    int contracts = 0;
    const auto contractList = m_repository.list(QStringLiteral("contracts"));
    if (contractList.ok())
        contracts = contractList.entries.size();
    m_contractCount->setText(QString::number(contracts));

    const auto readme = m_repository.readText(QStringLiteral("README.md"));
    QStringList targetLines;
    bool inTargets = false;
    if (readme.ok()) {
        const QStringList lines = readme.text.split(QLatin1Char('\n'));
        for (const QString& line : lines) {
            if (line.contains(QStringLiteral("Installed targets"), Qt::CaseInsensitive)) {
                inTargets = true;
                continue;
            }
            if (inTargets && line.startsWith(QLatin1Char('#')))
                break;
            if (inTargets && line.count(QLatin1Char('|')) >= 2 && !line.contains(QStringLiteral("---")))
                targetLines.append(line);
        }
    }

    for (const QString& line : targetLines) {
        QStringList cells = line.split(QLatin1Char('|'), Qt::SkipEmptyParts);
        for (QString& cell : cells) cell = cell.trimmed().remove(QLatin1Char('`'));
        if (cells.size() < 2 || cells.first().contains(QStringLiteral("runtime"), Qt::CaseInsensitive))
            continue;
        const int row = m_targets->rowCount();
        m_targets->insertRow(row);
        m_targets->setItem(row, 0, new QTableWidgetItem(cells.value(0)));
        m_targets->setItem(row, 1, new QTableWidgetItem(cells.value(1)));
        m_targets->setItem(row, 2, new QTableWidgetItem(tr("README contract")));
    }
    m_targetCount->setText(QString::number(m_targets->rowCount()));

    const bool hasConflictContract = readme.ok()
        && (readme.text.contains(QStringLiteral("exit code 2"), Qt::CaseInsensitive)
            || readme.text.contains(QStringLiteral("conflict"), Qt::CaseInsensitive));
    m_conflictBanner->setVisible(hasConflictContract);
    if (hasConflictContract)
        m_conflictBanner->setText(tr("Conflicts are fail-closed. The source contract reserves exit code 2; this view never runs the installer."));
}
