#pragma once

#include "LocalMemoryRepository.h"
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QTableWidget;

class COperationsView final : public QWidget
{
    Q_OBJECT
public:
    explicit COperationsView(const QString& repositoryRoot = QString(), QWidget* parent = nullptr);
    void setRepositoryRoot(const QString& root);

public slots:
    void refresh();

private:
    CLocalMemoryRepository m_repository;
    QLabel* m_banner;
    QLabel* m_repositoryState;
    QLabel* m_releaseState;
    QLabel* m_evidenceState;
    QTableWidget* m_sources;
    QPlainTextEdit* m_preview;
};
