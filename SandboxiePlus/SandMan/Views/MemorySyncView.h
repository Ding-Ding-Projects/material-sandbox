#pragma once

#include "LocalMemoryRepository.h"
#include <QWidget>

class QLabel;
class QTableWidget;

class CMemorySyncView final : public QWidget
{
    Q_OBJECT
public:
    explicit CMemorySyncView(const QString& repositoryRoot = QString(), QWidget* parent = nullptr);
    void setRepositoryRoot(const QString& root);

public slots:
    void refresh();

private:
    void copyCommand(const QString& command);
    CLocalMemoryRepository m_repository;
    QLabel* m_rootLabel;
    QLabel* m_stateLabel;
    QLabel* m_targetCount;
    QLabel* m_contractCount;
    QLabel* m_refreshTime;
    QLabel* m_conflictBanner;
    QTableWidget* m_targets;
};
