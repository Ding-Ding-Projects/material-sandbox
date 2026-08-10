#pragma once

#include "LocalMemoryRepository.h"
#include <QRegularExpression>
#include <QWidget>

class CM3SearchField;
class QLabel;
class QListWidget;
class QPlainTextEdit;

class CMemoryInventoryView final : public QWidget
{
    Q_OBJECT
public:
    explicit CMemoryInventoryView(const QString& repositoryRoot = QString(), QWidget* parent = nullptr);
    void setRepositoryRoot(const QString& root);

public slots:
    void refresh();

private slots:
    void showCurrentFile();
    void applyFilter(QString query, bool regexMode, QRegularExpression expression,
                     QString flags, bool valid, QString error);
    void openCurrentFile();
    void exportCurrentFile();

private:
    void addIfPresent(const QString& relativePath);
    QString currentRelativePath() const;
    CLocalMemoryRepository m_repository;
    CM3SearchField* m_search;
    QLabel* m_state;
    QListWidget* m_files;
    QPlainTextEdit* m_reader;
};
