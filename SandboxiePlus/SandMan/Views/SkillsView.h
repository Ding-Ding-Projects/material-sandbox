#pragma once

#include "LocalMemoryRepository.h"
#include <QRegularExpression>
#include <QWidget>

class CM3SearchField;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

class CSkillsView final : public QWidget
{
    Q_OBJECT
public:
    explicit CSkillsView(const QString& repositoryRoot = QString(), QWidget* parent = nullptr);
    void setRepositoryRoot(const QString& root);

public slots:
    void refresh();

private slots:
    void applyFilter(QString query, bool regexMode, QRegularExpression expression,
                     QString flags, bool valid, QString error);
    void openSelected();
    void copyReinstallInstruction();

private:
    QString selectedRelativePath() const;
    CLocalMemoryRepository m_repository;
    CM3SearchField* m_search;
    QLabel* m_state;
    QTreeWidget* m_tree;
};
