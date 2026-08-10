#pragma once

#include "LocalMemoryRepository.h"
#include <QWidget>

class QLabel;
class QListWidget;
class QPlainTextEdit;

class CStatusHubView final : public QWidget
{
    Q_OBJECT
public:
    explicit CStatusHubView(const QString& repositoryRoot = QString(), QWidget* parent = nullptr);
    void setRepositoryRoot(const QString& root);

public slots:
    void refresh();

private slots:
    void showCurrent();
    void revealCurrent();
    void copyReplyInstructions();

private:
    QString currentRelativePath() const;
    void addStatusEntry(const CLocalMemoryRepository::Entry& entry);
    CLocalMemoryRepository m_repository;
    QLabel* m_state;
    QListWidget* m_sessions;
    QPlainTextEdit* m_evidence;
};
