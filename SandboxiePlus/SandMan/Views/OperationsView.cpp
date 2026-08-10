#include "stdafx.h"
#include "OperationsView.h"

#include <QAbstractItemView>
#include <QFileInfo>
#include <QFrame>
#include <QDateTime>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QFrame* operationCard(const QString& title, QLabel** value, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setProperty("m3Card", true);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    auto* label = new QLabel(title, card);
    label->setProperty("m3Label", QStringLiteral("small"));
    *value = new QLabel(QStringLiteral("—"), card);
    (*value)->setWordWrap(true);
    layout->addWidget(label);
    layout->addWidget(*value);
    return card;
}
}

COperationsView::COperationsView(const QString& repositoryRoot, QWidget* parent)
    : QWidget(parent), m_repository(repositoryRoot), m_banner(new QLabel(this)),
      m_repositoryState(nullptr), m_releaseState(nullptr), m_evidenceState(nullptr),
      m_sources(new QTableWidget(this)), m_preview(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("operationsView"));
    setProperty("memoryOwned", true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Ops"), this);
    title->setProperty("m3Title", QStringLiteral("display-small"));
    auto* subtitle = new QLabel(tr("Local operations evidence only. No GitHub polling or background network access."), this);
    subtitle->setWordWrap(true);
    subtitle->setProperty("m3Text", QStringLiteral("secondary"));
    root->addWidget(title);
    root->addWidget(subtitle);

    m_banner->setWordWrap(true);
    m_banner->setVisible(false);
    m_banner->setProperty("m3Banner", QStringLiteral("error"));
    root->addWidget(m_banner);

    auto* cards = new QGridLayout;
    cards->addWidget(operationCard(tr("Repository"), &m_repositoryState, this), 0, 0);
    cards->addWidget(operationCard(tr("Release evidence"), &m_releaseState, this), 0, 1);
    cards->addWidget(operationCard(tr("Evidence freshness"), &m_evidenceState, this), 0, 2);
    root->addLayout(cards);

    m_sources->setColumnCount(3);
    m_sources->setHorizontalHeaderLabels({tr("Local source"), tr("State"), tr("Modified")});
    m_sources->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_sources->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_sources->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_sources->verticalHeader()->hide();
    m_sources->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sources->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(m_sources);

    m_preview->setReadOnly(true);
    m_preview->setMaximumBlockCount(5000);
    m_preview->setProperty("m3Monospace", true);
    root->addWidget(m_preview, 1);

    auto* refreshButton = new QPushButton(tr("Refresh local evidence"), this);
    refreshButton->setProperty("m3", QStringLiteral("tonal"));
    root->addWidget(refreshButton, 0, Qt::AlignLeft);
    connect(refreshButton, &QPushButton::clicked, this, &COperationsView::refresh);
    connect(m_sources, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) {
        if (row < 0) return;
        const QString relative = m_sources->item(row, 0)->data(Qt::UserRole).toString();
        const auto text = m_repository.readText(relative);
        m_preview->setPlainText(text.ok() ? text.text : text.message);
    });
    refresh();
}

void COperationsView::setRepositoryRoot(const QString& root)
{
    m_repository.setRoot(root);
    refresh();
}

void COperationsView::refresh()
{
    m_sources->setRowCount(0);
    m_preview->clear();
    const QStringList candidates = {
        QStringLiteral("README.md"),
        QStringLiteral("CHANGELOG.md"),
        QStringLiteral("docs/changelog.md"),
        QStringLiteral("docs/features/operations/README.md"),
        QStringLiteral("docs/features/operations/releases.md")
    };

    bool failingEvidence = false;
    bool unsignedEvidence = false;
    QDateTime newest;
    int found = 0;
    for (const QString& relative : candidates) {
        const auto text = m_repository.readText(relative);
        if (!text.ok())
            continue;
        const QFileInfo info(text.absolutePath);
        newest = newest.isValid() && newest > info.lastModified() ? newest : info.lastModified();
        failingEvidence = failingEvidence
            || text.text.contains(QStringLiteral("the hui is red"), Qt::CaseInsensitive)
            || text.text.contains(QStringLiteral("workflow failed"), Qt::CaseInsensitive);
        unsignedEvidence = unsignedEvidence
            || text.text.contains(QStringLiteral("unsigned"), Qt::CaseInsensitive);
        const int row = m_sources->rowCount();
        m_sources->insertRow(row);
        auto* source = new QTableWidgetItem(relative);
        source->setData(Qt::UserRole, relative);
        m_sources->setItem(row, 0, source);
        m_sources->setItem(row, 1, new QTableWidgetItem(tr("Local evidence")));
        m_sources->setItem(row, 2, new QTableWidgetItem(info.lastModified().toString(Qt::ISODate)));
        ++found;
    }

    m_repositoryState->setText(m_repository.isConfigured() ? tr("Available / read-only") : tr("Unavailable"));
    m_releaseState->setText(unsignedEvidence ? tr("unsigned") : tr("No local release status"));
    m_evidenceState->setText(newest.isValid() ? newest.toString(Qt::ISODate) : tr("No evidence files"));
    m_banner->setVisible(failingEvidence);
    if (failingEvidence)
        m_banner->setText(tr("the hui is red — local evidence records a failing run. Open the source below for details."));
    if (found == 0)
        m_preview->setPlainText(tr("No local operations evidence was found. This surface does not infer live GitHub state."));
    else
        m_sources->setCurrentCell(0, 0);
}
