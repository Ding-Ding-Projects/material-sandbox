#pragma once

#include <QRegularExpression>
#include <QString>
#include <QWidget>

class CRegexBuilderDialog;
class QEvent;
class QLineEdit;
class QPaintEvent;
class QPushButton;

class CM3SearchField final : public QWidget
{
    Q_OBJECT

public:
    enum HeightVariant {
        Page = 56,
        Control = 48,
        Menu = 40
    };
    Q_ENUM(HeightVariant)

    explicit CM3SearchField(QWidget* parent = nullptr);

    QLineEdit* lineEdit() const;
    QString query() const;
    QString pattern() const;
    QString flags() const;
    bool regexMode() const;
    QRegularExpression expression() const;
    bool isValid() const;
    QString error() const;
    HeightVariant heightVariant() const;

    void setQuery(const QString& query);
    void setState(const QString& query, const QString& pattern, const QString& flags, bool regexMode);
    void setPlaceholderText(const QString& placeholder);
    void setAccessibleName(const QString& name);
    void setAccessibleDescription(const QString& description);
    void setHeightVariant(HeightVariant variant);

signals:
    void searchChanged(QString query,
                       bool regexMode,
                       QRegularExpression expression,
                       QString flags,
                       bool valid,
                       QString error);

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTextEdited(const QString& text);
    void clearSearch();
    void openRegexBuilder();
    void applyRegexPattern(const QString& pattern, const QString& flags);
    void keepPlainText(const QString& text);

private:
    void ensureRegexBuilder();
    void rebuildExpression(bool notify);
    void updateAccessibleNames();
    void updateControlState();
    void returnFocusToEditor();
    static QRegularExpression compileRegex(const QString& pattern, const QString& flags, QString* error);

    QLineEdit* m_lineEdit;
    QPushButton* m_clearButton;
    QPushButton* m_regexButton;
    CRegexBuilderDialog* m_regexBuilder;
    QString m_query;
    QString m_pattern;
    QString m_flags;
    QRegularExpression m_expression;
    QString m_error;
    QString m_accessibleDescription;
    HeightVariant m_heightVariant;
    bool m_regexMode;
    bool m_valid;
};
