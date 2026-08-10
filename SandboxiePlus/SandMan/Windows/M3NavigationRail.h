#pragma once

#include <QFrame>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QString>

class QButtonGroup;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

class CM3NavigationRail final : public QFrame
{
    Q_OBJECT

public:
    struct Destination {
        QString id;
        QString label;
        QIcon icon;
        QString group;
        bool memoryOwned = false;
        bool enabled = true;
    };

    explicit CM3NavigationRail(QWidget* parent = nullptr);

    static QList<Destination> contractDestinations();
    void setDestinations(const QList<Destination>& destinations);
    QList<Destination> destinations() const;
    QString currentDestination() const;
    void setCurrentDestination(const QString& id, bool emitSignal = false);
    void setDestinationEnabled(const QString& id, bool enabled);

signals:
    void destinationActivated(QString id);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void activateButton();

private:
    void clearRail();
    QList<QToolButton*> navigableButtons() const;
    void focusRelative(int delta);

    QScrollArea* m_scroller;
    QWidget* m_content;
    QVBoxLayout* m_layout;
    QButtonGroup* m_group;
    QList<Destination> m_destinations;
    QHash<QString, QToolButton*> m_buttons;
    QString m_currentId;
};
