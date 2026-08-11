#include "stdafx.h"
#include "SkillsView.h"
#include "../Windows/M3SearchField.h"

#include <QAbstractItemView>
#include <QDir>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

CSkillsView::CSkillsView(const QString& repositoryRoot, QWidget* parent)
    : QWidget(parent), m_repository(repositoryRoot), m_search(new CM3SearchField(this)),
      m_state(new QLabel(this)), m_tree(new QTreeWidget(this)), m_reader(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("skillsView"));
    setProperty("memoryOwned", true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Skills"), this);
    title->setProperty("m3Title", QStringLiteral("display-small"));
    auto* subtitle = new QLabel(tr("Owned local skills discovered from the configured repository."), this);
    subtitle->setWordWrap(true);
    subtitle->setProperty("m3Text", QStringLiteral("secondary"));
    root->addWidget(title);
    root->addWidget(subtitle);

    m_search->setPlaceholderText(tr("Search skills"));
    root->addWidget(m_search);
    m_state->setWordWrap(true);
    root->addWidget(m_state);

    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({tr("Skill"), tr("Scope"), tr("Install path"), tr("Status")});
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setAlternatingRowColors(false);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setAccessibleName(tr("Installed skills"));
    root->addWidget(m_tree, 1);

    m_reader->setObjectName(QStringLiteral("skillDocumentReader"));
    m_reader->setReadOnly(true);
    m_reader->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_reader->setProperty("m3Monospace", true);
    m_reader->setAccessibleName(tr("Selected skill documentation"));
    m_reader->setMinimumHeight(160);
    root->addWidget(m_reader, 1);

    auto* actions = new QHBoxLayout;
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setProperty("m3", QStringLiteral("tonal"));
    auto* openButton = new QPushButton(tr("View SKILL.md here"), this);
    openButton->setObjectName(QStringLiteral("skillViewHereButton"));
    auto* reinstallButton = new QPushButton(tr("Copy reinstall command"), this);
    reinstallButton->setProperty("m3", QStringLiteral("filled"));
    actions->addWidget(refreshButton);
    actions->addStretch(1);
    actions->addWidget(openButton);
    actions->addWidget(reinstallButton);
    root->addLayout(actions);

    connect(m_search, &CM3SearchField::searchChanged, this, &CSkillsView::applyFilter);
    connect(refreshButton, &QPushButton::clicked, this, &CSkillsView::refresh);
    connect(openButton, &QPushButton::clicked, this, &CSkillsView::openSelected);
    connect(reinstallButton, &QPushButton::clicked, this, &CSkillsView::copyReinstallInstruction);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int) { openSelected(); });
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this] { openSelected(); });
    refresh();
}

void CSkillsView::setRepositoryRoot(const QString& root)
{
    m_repository.setRoot(root);
    refresh();
}

void CSkillsView::refresh()
{
    m_tree->clear();
    m_reader->clear();
    const auto skills = m_repository.list(QStringLiteral("skills"));
    if (!skills.ok()) {
        m_state->setText(tr("Skills unavailable: %1").arg(skills.message));
        return;
    }
    int count = 0;
    for (const auto& entry : skills.entries) {
        if (!entry.directory)
            continue;
        const QString relative = entry.relativePath + QStringLiteral("/SKILL.md");
        const auto document = m_repository.readText(relative);
        if (!document.ok())
            continue;
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, entry.name);
        item->setText(1, tr("Owned"));
        item->setText(2, QDir::toNativeSeparators(document.absolutePath));
        item->setText(3, tr("Installed"));
        item->setData(0, Qt::UserRole, relative);
        ++count;
    }
    m_state->setText(tr("%1 local skill(s) · read-only inventory").arg(count));
    if (m_tree->topLevelItemCount() > 0)
        m_tree->setCurrentItem(m_tree->topLevelItem(0));
}

void CSkillsView::applyFilter(QString query, bool, QRegularExpression expression,
                              QString, bool valid, QString)
{
    const bool active = !query.isEmpty() && valid;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_tree->topLevelItem(i);
        const QString haystack = item->text(0) + QLatin1Char('\n') + item->text(1) + QLatin1Char('\n') + item->text(2);
        item->setHidden(active && !expression.match(haystack).hasMatch());
    }
}

QString CSkillsView::selectedRelativePath() const
{
    return m_tree->currentItem() ? m_tree->currentItem()->data(0, Qt::UserRole).toString() : QString();
}

void CSkillsView::openSelected()
{
    const QString relative = selectedRelativePath();
    const auto document = relative.isEmpty() ? CLocalMemoryRepository::TextResult()
                                             : m_repository.readText(relative);
    m_reader->setPlainText(document.ok() ? document.text : document.message);
    if (!relative.isEmpty())
        m_reader->setFocus(Qt::OtherFocusReason);
}

void CSkillsView::copyReinstallInstruction()
{
    const QString relative = selectedRelativePath();
    if (relative.isEmpty())
        return;
    const QString skillName = QFileInfo(QFileInfo(relative).path()).fileName();
    QApplication::clipboard()->setText(
        tr("Use the repository's documented installer to reinstall skill '%1'. SandMan does not execute it.")
            .arg(skillName));
}
