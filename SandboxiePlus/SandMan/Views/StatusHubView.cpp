#include "stdafx.h"
#include "StatusHubView.h"

#include <QDir>
#include <QSet>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

CStatusHubView::CStatusHubView(const QString& repositoryRoot, QWidget* parent)
    : QWidget(parent), m_repository(repositoryRoot), m_state(new QLabel(this)),
      m_sessions(new QListWidget(this)), m_evidence(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("statusHubView"));
    setProperty("memoryOwned", true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Status"), this);
    title->setProperty("m3Title", QStringLiteral("display-small"));
    auto* subtitle = new QLabel(tr("Local status-hub projections. Replies are not simulated or delivered by SandMan."), this);
    subtitle->setWordWrap(true);
    subtitle->setProperty("m3Text", QStringLiteral("secondary"));
    root->addWidget(title);
    root->addWidget(subtitle);
    m_state->setWordWrap(true);
    root->addWidget(m_state);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    m_sessions->setMinimumWidth(300);
    m_sessions->setAccessibleName(tr("Local status sessions"));
    m_evidence->setReadOnly(true);
    m_evidence->setProperty("m3Monospace", true);
    m_evidence->setAccessibleName(tr("Session evidence"));
    splitter->addWidget(m_sessions);
    splitter->addWidget(m_evidence);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);

    auto* actions = new QHBoxLayout;
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setProperty("m3", QStringLiteral("tonal"));
    auto* replyButton = new QPushButton(tr("Copy reply instructions"), this);
    auto* revealButton = new QPushButton(tr("View card here"), this);
    revealButton->setObjectName(QStringLiteral("statusViewHereButton"));
    revealButton->setProperty("m3", QStringLiteral("filled"));
    actions->addWidget(refreshButton);
    actions->addStretch(1);
    actions->addWidget(replyButton);
    actions->addWidget(revealButton);
    root->addLayout(actions);

    connect(refreshButton, &QPushButton::clicked, this, &CStatusHubView::refresh);
    connect(replyButton, &QPushButton::clicked, this, &CStatusHubView::copyReplyInstructions);
    connect(revealButton, &QPushButton::clicked, this, &CStatusHubView::revealCurrent);
    connect(m_sessions, &QListWidget::currentItemChanged, this, [this] { showCurrent(); });
    connect(m_sessions, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { revealCurrent(); });
    refresh();
}

void CStatusHubView::setRepositoryRoot(const QString& root)
{
    m_repository.setRoot(root);
    refresh();
}

void CStatusHubView::addStatusEntry(const CLocalMemoryRepository::Entry& entry)
{
    if (entry.directory)
        return;
    auto* item = new QListWidgetItem(entry.name, m_sessions);
    item->setData(Qt::UserRole, entry.relativePath);
    item->setToolTip(QDir::toNativeSeparators(entry.absolutePath));
    item->setSizeHint(QSize(0, 56));
}

void CStatusHubView::refresh()
{
    m_sessions->clear();
    m_evidence->clear();
    const QStringList roots = {
        QStringLiteral("apps/status-hub/status"),
        QStringLiteral("apps/status-hub/data"),
        QStringLiteral("apps/status-hub")
    };
    QSet<QString> seen;
    for (const QString& relativeRoot : roots) {
        const auto listing = m_repository.list(relativeRoot,
            {QStringLiteral("*.json"), QStringLiteral("*.md"), QStringLiteral("*.txt")}, 2000);
        if (!listing.ok())
            continue;
        for (const auto& entry : listing.entries) {
            if (!entry.directory && !seen.contains(entry.absolutePath)) {
                seen.insert(entry.absolutePath);
                addStatusEntry(entry);
            }
        }
        if (!seen.isEmpty())
            break;
    }
    m_state->setText(m_repository.isConfigured()
        ? tr("%1 local status projection(s) · no live delivery connection").arg(m_sessions->count())
        : tr("Status unavailable: %1").arg(CLocalMemoryRepository::errorText(CLocalMemoryRepository::Unconfigured)));
    if (m_sessions->count() > 0)
        m_sessions->setCurrentRow(0);
    else
        m_evidence->setPlainText(tr("No local status-hub projection was found. SandMan does not fabricate sessions."));
}

QString CStatusHubView::currentRelativePath() const
{
    return m_sessions->currentItem() ? m_sessions->currentItem()->data(Qt::UserRole).toString() : QString();
}

void CStatusHubView::showCurrent()
{
    const QString relative = currentRelativePath();
    const auto result = relative.isEmpty() ? CLocalMemoryRepository::TextResult() : m_repository.readText(relative);
    m_evidence->setPlainText(result.ok() ? result.text : result.message);
}

void CStatusHubView::revealCurrent()
{
    showCurrent();
    m_evidence->setFocus(Qt::OtherFocusReason);
}

void CStatusHubView::copyReplyInstructions()
{
    if (currentRelativePath().isEmpty())
        return;
    QApplication::clipboard()->setText(
        tr("Reply through the documented status-hub client for this local projection. SandMan does not deliver replies."));
}
