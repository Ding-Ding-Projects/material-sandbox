#include "../Windows/M3PageNavigationHost.h"

#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QEventLoop>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtTest>

#include <functional>

namespace {

struct TreeContainer
{
    QWidget* widget;
    QStackedLayout* pages;
    QTreeWidget* titles;
    QList<QWidget*> pageWidgets;
};

QTabWidget* makeTabs(QWidget* parent,
                     const QString& prefix,
                     int count,
                     QList<QWidget*>* pages)
{
    auto* tabs = new QTabWidget(parent);
    for (int index = 0; index < count; ++index) {
        auto* page = new QWidget(tabs);
        page->setObjectName(QStringLiteral("%1Page%2").arg(prefix).arg(index));
        page->setAccessibleName(QStringLiteral("%1 page %2").arg(prefix).arg(index + 1));
        auto* layout = new QVBoxLayout(page);
        layout->addWidget(new QLabel(QStringLiteral("%1 content %2").arg(prefix).arg(index + 1), page));
        tabs->addTab(page, QStringLiteral("%1 %2").arg(prefix).arg(index + 1));
        pages->append(page);
    }
    return tabs;
}

QTabWidget* replaceTabsLikeSettings(QTabWidget* legacy,
                                    QList<QWidget*>* finalPages)
{
    const int currentIndex = legacy->currentIndex();
    auto* replacement = new QTabWidget(legacy->parentWidget());
    while (legacy->count() > 0) {
        QWidget* page = legacy->widget(0);
        const QString title = legacy->tabText(0);
        const QIcon icon = legacy->tabIcon(0);
        legacy->removeTab(0);
        replacement->addTab(page, icon, title);
        finalPages->append(page);
    }
    replacement->setCurrentIndex(currentIndex);
    QLayoutItem* replaced = legacy->parentWidget()->layout()->replaceWidget(legacy, replacement);
    delete replaced;
    return replacement;
}

TreeContainer convertToTreeLikeConfigDialog(QTabWidget* tabs)
{
    QWidget* currentPage = tabs->currentWidget();
    auto* container = new QWidget(tabs->window());
    auto* layout = new QGridLayout(container);
    auto* titles = new QTreeWidget(container);
    titles->setHeaderHidden(true);
    auto* pages = new QStackedLayout();
    layout->addWidget(titles, 0, 0);
    layout->addLayout(pages, 0, 1);

    TreeContainer result{container, pages, titles, {}};
    auto* group = new QTreeWidgetItem(QStringList(QStringLiteral("Grouped pages")));
    titles->addTopLevelItem(group);
    while (tabs->count() > 0) {
        QWidget* page = tabs->widget(0);
        const QString title = tabs->tabText(0);
        const QIcon icon = tabs->tabIcon(0);
        tabs->removeTab(0);
        const int index = pages->addWidget(page);
        auto* item = new QTreeWidgetItem(QStringList(title));
        item->setData(0, Qt::UserRole, index);
        item->setIcon(0, icon);
        if (index < 2) {
            if (index == 0)
                group->setData(0, Qt::UserRole, index); // Matches ConvertToTree's parent alias.
            group->addChild(item);
        }
        else {
            titles->addTopLevelItem(item);
        }
        result.pageWidgets.append(page);
    }
    const int currentIndex = pages->indexOf(currentPage);
    if (currentIndex >= 0)
        pages->setCurrentIndex(currentIndex);
    return result;
}

void processDeferredDeletes()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void verifyVisibleNavigation(CM3PageNavigationHost* host,
                             const QList<QWidget*>& expectedPages,
                             const std::function<QWidget*()>& sourceCurrentPage)
{
    auto* navigation = host->findChild<QListWidget*>(QStringLiteral("m3PageNavigationList"));
    QVERIFY(navigation);
    QVERIFY(!host->isHidden());
    QVERIFY(!navigation->isHidden());
    QCOMPARE(host->pageCount(), expectedPages.size());
    QCOMPARE(navigation->count(), expectedPages.size());

    for (int index = 0; index < expectedPages.size(); ++index) {
        navigation->setCurrentRow(index);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(host->currentIndex(), index);
        QCOMPARE(host->currentPage(), expectedPages.at(index));
        QCOMPARE(sourceCurrentPage(), expectedPages.at(index));
        QCOMPARE(navigation->currentRow(), index);
    }
}

} // namespace

class M3PageNavigationHostTests : public QObject
{
    Q_OBJECT

private slots:
    void settingsNormalRebindSurvivesDeferredDelete();
    void settingsOptionTreeRebindSurvivesDeferredDeletes();
    void optionsNormalRefreshSurvivesDeferredDelete();
    void optionsOptionTreeRebindSurvivesDeferredDelete();
};

void M3PageNavigationHostTests::settingsNormalRebindSurvivesDeferredDelete()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    QList<QWidget*> originalPages;
    QTabWidget* legacy = makeTabs(&dialog, QStringLiteral("Settings"), 5, &originalPages);
    legacy->setCurrentIndex(3);
    root->addWidget(legacy);
    CM3PageNavigationHost* host = CM3PageNavigationHost::adapt(&dialog, legacy);
    QVERIFY(host);

    QList<QWidget*> finalPages;
    QTabWidget* replacement = replaceTabsLikeSettings(legacy, &finalPages);
    QPointer<QTabWidget> retiredTabs = legacy;
    legacy->deleteLater();
    host->rebind(replacement);

    QCOMPARE(host->currentIndex(), 3);
    verifyVisibleNavigation(host, finalPages, [replacement] { return replacement->currentWidget(); });
    processDeferredDeletes();
    QVERIFY(retiredTabs.isNull());
    verifyVisibleNavigation(host, finalPages, [replacement] { return replacement->currentWidget(); });
    QCOMPARE(replacement->currentWidget(), host->currentPage());
}

void M3PageNavigationHostTests::settingsOptionTreeRebindSurvivesDeferredDeletes()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    QList<QWidget*> originalPages;
    QTabWidget* designerTabs = makeTabs(&dialog, QStringLiteral("Settings tree"), 6, &originalPages);
    designerTabs->setCurrentIndex(4);
    root->addWidget(designerTabs);
    CM3PageNavigationHost* host = CM3PageNavigationHost::adapt(&dialog, designerTabs);
    QVERIFY(host);

    QList<QWidget*> replacementPages;
    QTabWidget* finalTabs = replaceTabsLikeSettings(designerTabs, &replacementPages);
    QPointer<QTabWidget> retiredDesignerTabs = designerTabs;
    designerTabs->deleteLater();
    host->rebind(finalTabs);
    QCOMPARE(host->currentIndex(), 4);
    verifyVisibleNavigation(host, replacementPages, [finalTabs] { return finalTabs->currentWidget(); });
    host->setCurrentIndex(4);
    QCOMPARE(finalTabs->currentWidget(), replacementPages.at(4));

    TreeContainer tree = convertToTreeLikeConfigDialog(finalTabs);
    tree.titles->hide();
    QPointer<QTabWidget> retiredFinalTabs = finalTabs;
    host->rebind(tree.widget, tree.pages, tree.titles);
    QCOMPARE(finalTabs->parentWidget()->layout()->indexOf(finalTabs), -1);
    QCOMPARE(host->currentIndex(), 4);
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    auto* treeNavigation = host->findChild<QListWidget*>(QStringLiteral("m3PageNavigationList"));
    QVERIFY(treeNavigation->item(0)->text().contains(QStringLiteral("Grouped pages")));
    finalTabs->deleteLater();

    processDeferredDeletes();
    QVERIFY(retiredDesignerTabs.isNull());
    QVERIFY(retiredFinalTabs.isNull());
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    QCOMPARE(tree.pages->currentWidget(), host->currentPage());
}

void M3PageNavigationHostTests::optionsNormalRefreshSurvivesDeferredDelete()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    QList<QWidget*> pages;
    QTabWidget* tabs = makeTabs(&dialog, QStringLiteral("Options"), 4, &pages);
    tabs->setCurrentIndex(2);
    root->addWidget(tabs);
    CM3PageNavigationHost* host = CM3PageNavigationHost::adapt(&dialog, tabs);
    QVERIFY(host);

    QWidget* retiredPage = pages.takeAt(1);
    tabs->removeTab(1);
    QPointer<QWidget> retiredPageGuard = retiredPage;
    retiredPage->deleteLater();
    auto* addedPage = new QWidget(tabs);
    addedPage->setObjectName(QStringLiteral("OptionsAddedPage"));
    addedPage->setAccessibleName(QStringLiteral("Added options page"));
    tabs->insertTab(1, addedPage, QStringLiteral("Options added"));
    pages.insert(1, addedPage);
    tabs->setTabText(2, QStringLiteral("Options renamed"));
    host->rebind(tabs);
    host->rebind(tabs); // Rebinding the same source must not duplicate callbacks.

    auto* navigation = host->findChild<QListWidget*>(QStringLiteral("m3PageNavigationList"));
    QSignalSpy pageChanges(host, &CM3PageNavigationHost::currentPageChanged);
    navigation->setCurrentRow(0);
    QCOMPARE(pageChanges.count(), 1);
    QCOMPARE(navigation->item(2)->text(), QStringLiteral("Options renamed"));
    verifyVisibleNavigation(host, pages, [tabs] { return tabs->currentWidget(); });

    processDeferredDeletes();
    QVERIFY(retiredPageGuard.isNull());
    verifyVisibleNavigation(host, pages, [tabs] { return tabs->currentWidget(); });
    QCOMPARE(tabs->currentWidget(), host->currentPage());
}

void M3PageNavigationHostTests::optionsOptionTreeRebindSurvivesDeferredDelete()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    QList<QWidget*> originalPages;
    QTabWidget* tabs = makeTabs(&dialog, QStringLiteral("Options tree"), 7, &originalPages);
    tabs->setCurrentIndex(5);
    root->addWidget(tabs);
    CM3PageNavigationHost* host = CM3PageNavigationHost::adapt(&dialog, tabs);
    QVERIFY(host);

    TreeContainer tree = convertToTreeLikeConfigDialog(tabs);
    tree.titles->hide();
    QPointer<QTabWidget> retiredTabs = tabs;
    host->rebind(tree.widget, tree.pages, tree.titles);
    QCOMPARE(host->currentIndex(), 5);
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    tabs->deleteLater();

    processDeferredDeletes();
    QVERIFY(retiredTabs.isNull());
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    QCOMPARE(tree.pages->currentWidget(), host->currentPage());
}

QTEST_MAIN(M3PageNavigationHostTests)
#include "M3PageNavigationHostTests.moc"
