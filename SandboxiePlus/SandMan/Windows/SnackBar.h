#pragma once

#include <QFrame>
#include <QQueue>
#include <QString>
#include <functional>

class QLabel;
class QPushButton;
class QTimer;

class CSnackBar final : public QFrame
{
    Q_OBJECT

public:
    explicit CSnackBar(QWidget* host);

    void showMessage(const QString& text,
                     const QString& actionText = QString(),
                     const std::function<void()>& action = std::function<void()>(),
                     int timeoutMs = 4000);
    void clear();

signals:
    void dismissed(QString text);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void showNext();
    void invokeAction();
    void dismissCurrent();

private:
    struct Message {
        QString text;
        QString actionText;
        std::function<void()> action;
        int timeoutMs = 4000;
    };

    void positionOnHost();

    QWidget* m_host;
    QLabel* m_label;
    QPushButton* m_actionButton;
    QPushButton* m_dismissButton;
    QTimer* m_timer;
    QQueue<Message> m_queue;
    Message m_current;
};
