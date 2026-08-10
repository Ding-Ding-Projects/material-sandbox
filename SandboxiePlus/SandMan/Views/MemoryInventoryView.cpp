#include "stdafx.h"
#include "MemoryInventoryView.h"
#include "../Windows/M3SearchField.h"

#include <QDir>
#include <QSet>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>

CMemoryInventoryView::CMemoryInventoryView(const QString& repositoryRoot, QWidget* parent)
    : QWidget(parent), m_repository(repositoryRoot), m_search(new CM3SearchField(this)),
      m_state(new QLabel(this)), m_files(new QListWidget(this)), m_reader(new QPlainTextEdit(this))
{
    setObjectName(QStringLiteral("memoryInventoryView"));
    setProperty("memoryOwned", true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Memory"), this);
    title->setProperty("m3Title", QStringLiteral("display-small"));
    auto* subtitle = new QLabel(tr("Bounded, local, read-only access to documented memory files."), this);
    subtitle->setWordWrap(true);
    subtitle->setProperty("m3Text", QStringLiteral("secondary"));
    root->addWidget(title);
    root->addWidget(subtitle);
    m_search->setPlaceholderText(tr("Search memory files"));
    root->addWidget(m_search);
    m_state->setWordWrap(true);
    root->addWidget(m_state);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    m_files->setMinimumWidth(260);
    m_files->setAccessibleName(tr("Memory files"));
    m_reader->setReadOnly(true);
    m_reader->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_reader->setProperty("m3Monospace", true);
    m_reader->setAccessibleName(tr("Memory file reader"));
    splitter->addWidget(m_files);
    splitter->addWidget(m_reader);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);

    auto* actions = new QHBoxLayout;
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setProperty("m3", QStringLiteral("tonal"));
    auto* openButton = new QPushButton(tr("Open in editor"), this);
    auto* exportButton = new QPushButton(tr("Export…"), this);
    exportButton->setProperty("m3", QStringLiteral("filled"));
    actions->addWidget(refreshButton);
    actions->addStretch(1);
    actions->addWidget(openButton);
    actions->addWidget(exportButton);
    root->addLayout(actions);

    connect(m_search, &CM3SearchField::searchChanged, this, &CMemoryInventoryView::applyFilter);
    connect(m_files, &QListWidget::currentItemChanged, this, [this] { showCurrentFile(); });
    connect(m_files, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { openCurrentFile(); });
    connect(refreshButton, &QPushButton::clicked, this, &CMemoryInventoryView::refresh);
    connect(openButton, &QPushButton::clicked, this, &CMemoryInventoryView::openCurrentFile);
    connect(exportButton, &QPushButton::clicked, this, &CMemoryInventoryView::exportCurrentFile);
    refresh();
}

void CMemoryInventoryView::setRepositoryRoot(const QString& root)
{
    m_repository.setRoot(root);
    refresh();
}

void CMemoryInventoryView::addIfPresent(const QString& relativePath)
{
    CLocalMemoryRepository::Error error = CLocalMemoryRepository::NoError;
    const QString path = m_repository.safeExistingPath(relativePath, &error);
    if (path.isEmpty() || !QFileInfo(path).isFile())
        return;
    auto* item = new QListWidgetItem(QFileInfo(relativePath).fileName(), m_files);
    item->setData(Qt::UserRole, relativePath);
    item->setToolTip(QDir::toNativeSeparators(path));
    item->setSizeHint(QSize(0, 48));
}

void CMemoryInventoryView::refresh()
{
    m_files->clear();
    m_reader->clear();
    const QStringList candidates = {
        QStringLiteral("SHARED_INSTRUCTIONS.md"),
        QStringLiteral("HOST_INVENTORY.md"),
        QStringLiteral("PERSONAL_VOCABULARY.json"),
        QStringLiteral("CLAUDE.md"),
        QStringLiteral("memory/SHARED_INSTRUCTIONS.md"),
        QStringLiteral("memory/HOST_INVENTORY.md"),
        QStringLiteral("memory/PERSONAL_VOCABULARY.json"),
        QStringLiteral("memory/CLAUDE.md")
    };
    QSet<QString> names;
    for (const QString& candidate : candidates) {
        const QString key = QFileInfo(candidate).fileName();
        if (!names.contains(key)) {
            const int before = m_files->count();
            addIfPresent(candidate);
            if (m_files->count() > before) names.insert(key);
        }
    }

    const QStringList projectRoots = {QStringLiteral("projects"), QStringLiteral("memory/projects")};
    for (const QString& projectRoot : projectRoots) {
        const auto projects = m_repository.list(projectRoot, {QStringLiteral("*.md")});
        if (!projects.ok())
            continue;
        for (const auto& entry : projects.entries) {
            if (!entry.directory) {
                auto* item = new QListWidgetItem(tr("Project · %1").arg(entry.name), m_files);
                item->setData(Qt::UserRole, entry.relativePath);
                item->setToolTip(QDir::toNativeSeparators(entry.absolutePath));
                item->setSizeHint(QSize(0, 48));
            }
        }
    }
    m_state->setText(m_repository.isConfigured()
        ? tr("%1 bounded local file(s) · maximum displayed file size 1 MiB").arg(m_files->count())
        : tr("Memory unavailable: %1").arg(CLocalMemoryRepository::errorText(CLocalMemoryRepository::Unconfigured)));
    if (m_files->count() > 0)
        m_files->setCurrentRow(0);
}

QString CMemoryInventoryView::currentRelativePath() const
{
    return m_files->currentItem() ? m_files->currentItem()->data(Qt::UserRole).toString() : QString();
}

void CMemoryInventoryView::showCurrentFile()
{
    const QString relative = currentRelativePath();
    if (relative.isEmpty()) {
        m_reader->clear();
        return;
    }
    const auto result = m_repository.readText(relative);
    m_reader->setPlainText(result.ok() ? result.text : result.message);
}

void CMemoryInventoryView::applyFilter(QString query, bool, QRegularExpression expression,
                                       QString, bool valid, QString)
{
    const bool active = !query.isEmpty() && valid;
    for (int i = 0; i < m_files->count(); ++i) {
        QListWidgetItem* item = m_files->item(i);
        const QString haystack = item->text() + QLatin1Char('\n') + item->data(Qt::UserRole).toString();
        item->setHidden(active && !expression.match(haystack).hasMatch());
    }
}

void CMemoryInventoryView::openCurrentFile()
{
    const QString relative = currentRelativePath();
    CLocalMemoryRepository::Error error = CLocalMemoryRepository::NoError;
    const QString path = relative.isEmpty() ? QString() : m_repository.safeExistingPath(relative, &error);
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void CMemoryInventoryView::exportCurrentFile()
{
    const QString relative = currentRelativePath();
    const auto result = relative.isEmpty() ? CLocalMemoryRepository::TextResult() : m_repository.readText(relative);
    if (!result.ok())
        return;
    const QString destination = QFileDialog::getSaveFileName(this, tr("Export memory file"), QFileInfo(relative).fileName());
    if (destination.isEmpty())
        return;
    QSaveFile output(destination);
    if (!output.open(QIODevice::WriteOnly) || output.write(result.text.toUtf8()) < 0 || !output.commit())
        QMessageBox::warning(this, tr("Export failed"), tr("The selected file could not be written."));
}
