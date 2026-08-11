#include "../Windows/M3PageNavigationHost.h"
#include "../Windows/M3Menu.h"
#include "../Windows/M3SearchField.h"
#include "../Windows/RegexBuilderDialog.h"
#include "../Windows/M3DialogHost.h"
#include "../../MiscHelpers/Common/Settings.h"
#include "../../MiscHelpers/Common/MaterialTheme.h"
#include <QtWidgets>
#include "../../MiscHelpers/Common/SettingsWidgets.h"
#include "../../MiscHelpers/Common/TabStateManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QShortcut>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtTest>

#include <functional>

namespace {

class PortableSettingsFixture
{
public:
    explicit PortableSettingsFixture(const QString& appName)
    {
        QVERIFY(m_directory.isValid());
        QFile portableMarker(m_directory.filePath(appName + QStringLiteral(".ini")));
        QVERIFY(portableMarker.open(QIODevice::WriteOnly));
        portableMarker.close();
        settings = new CSettings(m_directory.path(), appName, QStringLiteral("SandboxieTests"));
    }

    ~PortableSettingsFixture()
    {
        delete settings;
    }

    CSettings* settings = nullptr;

private:
    QTemporaryDir m_directory;
};

struct TreeContainer
{
    QWidget* widget;
    QStackedLayout* pages;
    QTreeWidget* titles;
    QList<QWidget*> pageWidgets;
};

class TestConfigDialog final : public CConfigDialog
{
public:
    using CConfigDialog::CConfigDialog;

    QWidget* convertToTree(QTabWidget* tabs) { return ConvertToTree(tabs); }
    QStackedLayout* pageStack() const { return m_pStack; }
    QTreeWidget* pageTree() const { return m_pTree; }
    QWidget* lastOnTabPage() const { return m_lastOnTabPage; }
    int onTabCount() const { return m_onTabCount; }

protected:
    void OnTab(QWidget* page) override
    {
        m_lastOnTabPage = page;
        ++m_onTabCount;
    }

private:
    QWidget* m_lastOnTabPage = nullptr;
    int m_onTabCount = 0;
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

TreeContainer convertWithProductionConfigDialog(TestConfigDialog* dialog,
                                                QTabWidget* tabs)
{
    QWidget* currentPage = tabs->currentWidget();
    if (currentPage) {
        QGridLayout* pageLayout = qobject_cast<QGridLayout*>(currentPage->layout());
        QLayoutItem* firstItem = pageLayout ? pageLayout->itemAt(0) : nullptr;
        QTabWidget* childTabs = firstItem ? qobject_cast<QTabWidget*>(firstItem->widget()) : nullptr;
        if (childTabs && childTabs->currentWidget())
            currentPage = childTabs->currentWidget();
    }

    QWidget* container = dialog->convertToTree(tabs);
    QStackedLayout* pages = dialog->pageStack();
    QTreeWidget* titles = dialog->pageTree();
    TreeContainer result{container, pages, titles, {}};
    for (int index = 0; index < pages->count(); ++index)
        result.pageWidgets.append(pages->widget(index));
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

void showFixture(QDialog& dialog)
{
    dialog.resize(1100, 720);
    dialog.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY(dialog.isVisible());
}

QStringList shortcutSequences(CM3PageNavigationHost* host)
{
    QStringList sequences;
    const auto shortcuts = host->findChildren<QShortcut*>(QString(), Qt::FindDirectChildrenOnly);
    for (QShortcut* shortcut : shortcuts)
        sequences.append(shortcut->key().toString(QKeySequence::PortableText));
    sequences.sort();
    return sequences;
}

void verifyContextMenu(QListWidget* navigation)
{
    QVERIFY(navigation);
    QVERIFY(navigation->count() > 0);
    bool verified = false;
    QTimer::singleShot(0, [&verified]() {
        QMenu* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!popup) {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                popup = qobject_cast<QMenu*>(widget);
                if (popup && popup->isVisible())
                    break;
                popup = nullptr;
            }
        }
        if (!popup)
            return;
        QStringList actions;
        for (QAction* action : popup->actions())
            actions.append(action->text());
        verified = actions.contains(QStringLiteral("Pin tab"))
            && actions.contains(QStringLiteral("Search all open tabs…"));
        popup->close();
    });
    const QPoint position = navigation->visualItemRect(navigation->item(0)).center();
    QVERIFY(QMetaObject::invokeMethod(navigation,
                                      "customContextMenuRequested",
                                      Qt::DirectConnection,
                                      Q_ARG(QPoint, position)));
    QVERIFY(verified);
}

void verifyStateManager(CM3PageNavigationHost* host,
                        const QList<QWidget*>& expectedPages,
                        CSettings* settings,
                        const QString& key,
                        const std::function<int()>& sourceCurrentIndex,
                        const std::function<void(int)>& setSourceCurrentIndex,
                        const std::function<void()>& rebindManagedSource,
                        bool exerciseShortcutActions = false)
{
    auto* navigation = host->navigationList();
    QVERIFY(navigation);
    QCOMPARE(navigation->contextMenuPolicy(), Qt::CustomContextMenu);
    QCOMPARE(host->findChildren<CTabStateManager*>(QString(), Qt::FindDirectChildrenOnly).size(), 1);
    QCOMPARE(shortcutSequences(host),
             QStringList({QStringLiteral("Ctrl+Shift+G"),
                          QStringLiteral("Ctrl+Shift+N"),
                          QStringLiteral("Ctrl+Shift+O"),
                          QStringLiteral("Ctrl+Shift+T")}));

    QSet<QString> stableKeys;
    for (QWidget* page : expectedPages) {
        QVERIFY(page);
        const QString stableKey = page->property("tabStateManagerKey").toString();
        QVERIFY(!stableKey.isEmpty());
        QVERIFY(!stableKeys.contains(stableKey));
        stableKeys.insert(stableKey);
    }

    verifyContextMenu(navigation);

    if (exerciseShortcutActions) {
        const auto shortcuts = host->findChildren<QShortcut*>(QString(), Qt::FindDirectChildrenOnly);
        for (QShortcut* shortcut : shortcuts)
            QVERIFY(QMetaObject::invokeMethod(shortcut, "activated", Qt::DirectConnection));
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(host->findChildren<QDialog*>(QString(), Qt::FindDirectChildrenOnly).size(), 4);
    }

    const int savedIndex = (sourceCurrentIndex() + 1) % expectedPages.size();
    navigation->setCurrentRow(savedIndex);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCOMPARE(sourceCurrentIndex(), savedIndex);
    const QJsonObject persisted = QJsonDocument::fromJson(settings->GetBlob(key)).object();
    QCOMPARE(persisted.value(QStringLiteral("active")).toString(),
             expectedPages.at(savedIndex)->property("tabStateManagerKey").toString());

    host->releaseStateManager();
    QCOMPARE(host->findChildren<CTabStateManager*>(QString(), Qt::FindDirectChildrenOnly).size(), 0);
    QCOMPARE(shortcutSequences(host).size(), 0);
    QCOMPARE(host->findChildren<QDialog*>(QString(), Qt::FindDirectChildrenOnly).size(), 0);
    setSourceCurrentIndex((savedIndex + 1) % expectedPages.size());
    rebindManagedSource();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCOMPARE(sourceCurrentIndex(), savedIndex);
    QCOMPARE(shortcutSequences(host).size(), 4);

    rebindManagedSource();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCOMPARE(sourceCurrentIndex(), savedIndex);
    QCOMPARE(host->findChildren<CTabStateManager*>(QString(), Qt::FindDirectChildrenOnly).size(), 1);
    QCOMPARE(shortcutSequences(host).size(), 4);
}

void verifyVisibleNavigation(CM3PageNavigationHost* host,
                             const QList<QWidget*>& expectedPages,
                             const std::function<QWidget*()>& sourceCurrentPage)
{
    auto* navigation = host->findChild<QListWidget*>(QStringLiteral("m3PageNavigationList"));
    QVERIFY(navigation);
    QVERIFY(host->isVisible());
    QVERIFY(navigation->isVisible());
    QVERIFY(navigation->viewport()->isVisible());
    QCOMPARE(host->pageCount(), expectedPages.size());
    QCOMPARE(navigation->count(), expectedPages.size());

    for (int index = 0; index < expectedPages.size(); ++index) {
        navigation->setCurrentRow(index);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(host->currentIndex(), index);
        QCOMPARE(host->currentPage(), expectedPages.at(index));
        QCOMPARE(sourceCurrentPage(), expectedPages.at(index));
        QCOMPARE(navigation->currentRow(), index);
        QVERIFY(host->currentPage()->isVisible());
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
    void searchCapsuleStatesAreVisibleAndAccessible();
    void searchBuilderCancellationReturnsFocusToEditor();
    void nativeMenuBuilderCancellationPreservesFilterAndFocus();
    void dialogHostTransfersExistingLayoutWithoutCrash();
};

void M3PageNavigationHostTests::dialogHostTransfersExistingLayoutWithoutCrash()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    auto* content = new QLabel(QStringLiteral("dialog content"), &dialog);
    root->addWidget(content);

    M3DialogHost::Install(&dialog);
    showFixture(dialog);

    QCOMPARE(dialog.layout(), static_cast<QLayout*>(root));
    QWidget* title = root->menuBar();
    QVERIFY(title);
    QCOMPARE(title->objectName(), QStringLiteral("m3DialogTitle"));
    QCOMPARE(dialog.findChildren<QWidget*>(QStringLiteral("m3DialogTitle")).size(), 1);
    QVERIFY(dialog.findChildren<QLabel*>(QString(), Qt::FindChildrenRecursively).contains(content));
    QVERIFY(content->isVisible());
    M3DialogHost::Install(&dialog);
    QCOMPARE(root->menuBar(), title);
    dialog.close();

    M3DialogHost::InstallForApplication(qApp);
    QMessageBox message;
    message.setText(QStringLiteral("message content"));
    message.setStandardButtons(QMessageBox::Ok);
    M3DialogHost::Install(&message);
    QVERIFY(message.layout());
    QVERIFY(message.layout()->menuBar());
    QCOMPARE(message.layout()->menuBar()->objectName(), QStringLiteral("m3DialogTitle"));
    message.close();
}

void M3PageNavigationHostTests::settingsNormalRebindSurvivesDeferredDelete()
{
    PortableSettingsFixture settings(QStringLiteral("SettingsNormalLifecycle"));
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
    const QString stateKey = QStringLiteral("Tests/SettingsWindow/Tabs");
    host->rebind(replacement, settings.settings, stateKey);

    showFixture(dialog);
    QCOMPARE(host->currentIndex(), 3);
    verifyVisibleNavigation(host, finalPages, [replacement] { return replacement->currentWidget(); });
    processDeferredDeletes();
    QVERIFY(retiredTabs.isNull());
    verifyVisibleNavigation(host, finalPages, [replacement] { return replacement->currentWidget(); });
    QCOMPARE(replacement->currentWidget(), host->currentPage());
    verifyStateManager(host,
                       finalPages,
                       settings.settings,
                       stateKey,
                       [replacement] { return replacement->currentIndex(); },
                       [replacement](int index) { replacement->setCurrentIndex(index); },
                       [host, replacement, &settings, stateKey] {
                           host->rebind(replacement, settings.settings, stateKey);
                       },
                       true);
}

void M3PageNavigationHostTests::settingsOptionTreeRebindSurvivesDeferredDeletes()
{
    PortableSettingsFixture settings(QStringLiteral("SettingsTreeLifecycle"));
    TestConfigDialog dialog;
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
    host->rebind(finalTabs,
                 settings.settings,
                 QStringLiteral("Tests/SettingsWindow/Tabs"));
    showFixture(dialog);
    QCOMPARE(host->currentIndex(), 4);
    verifyVisibleNavigation(host, replacementPages, [finalTabs] { return finalTabs->currentWidget(); });
    host->setCurrentIndex(4);
    QCOMPARE(finalTabs->currentWidget(), replacementPages.at(4));

    host->releaseStateManager();
    host->rebind(finalTabs);
    TreeContainer tree = convertWithProductionConfigDialog(&dialog, finalTabs);
    tree.titles->hide();
    QPointer<QTabWidget> retiredFinalTabs = finalTabs;
    const QString treeStateKey = QStringLiteral("Tests/SettingsWindow/Tabs/Tree");
    host->rebind(tree.widget, tree.pages, tree.titles, settings.settings, treeStateKey);
    QCOMPARE(finalTabs->parentWidget()->layout()->indexOf(finalTabs), -1);
    QCOMPARE(host->currentIndex(), 4);
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    auto* treeNavigation = host->findChild<QListWidget*>(QStringLiteral("m3PageNavigationList"));
    QCOMPARE(treeNavigation->item(0)->text(), QStringLiteral("Settings tree 1"));
    finalTabs->deleteLater();

    processDeferredDeletes();
    QVERIFY(retiredDesignerTabs.isNull());
    QVERIFY(retiredFinalTabs.isNull());
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    QCOMPARE(tree.pages->currentWidget(), host->currentPage());
    verifyStateManager(host,
                       tree.pageWidgets,
                       settings.settings,
                       treeStateKey,
                       [pages = tree.pages] { return pages->currentIndex(); },
                       [pages = tree.pages](int index) { pages->setCurrentIndex(index); },
                       [host, tree, &settings, treeStateKey] {
                           host->rebind(tree.widget,
                                        tree.pages,
                                        tree.titles,
                                        settings.settings,
                                        treeStateKey);
                       });
}

void M3PageNavigationHostTests::optionsNormalRefreshSurvivesDeferredDelete()
{
    PortableSettingsFixture settings(QStringLiteral("OptionsNormalLifecycle"));
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
    const QString stateKey = QStringLiteral("Tests/OptionsWindow/Tabs");
    host->rebind(tabs, settings.settings, stateKey);
    host->rebind(tabs, settings.settings, stateKey); // Rebinding the same source must not duplicate callbacks.

    showFixture(dialog);
    auto* navigation = host->navigationList();
    QSignalSpy pageChanges(host, &CM3PageNavigationHost::currentPageChanged);
    navigation->setCurrentRow(0);
    QCOMPARE(pageChanges.count(), 1);
    QCOMPARE(navigation->item(2)->text(), QStringLiteral("Options renamed"));
    verifyVisibleNavigation(host, pages, [tabs] { return tabs->currentWidget(); });

    processDeferredDeletes();
    QVERIFY(retiredPageGuard.isNull());
    verifyVisibleNavigation(host, pages, [tabs] { return tabs->currentWidget(); });
    QCOMPARE(tabs->currentWidget(), host->currentPage());
    verifyStateManager(host,
                       pages,
                       settings.settings,
                       stateKey,
                       [tabs] { return tabs->currentIndex(); },
                       [tabs](int index) { tabs->setCurrentIndex(index); },
                       [host, tabs, &settings, stateKey] {
                           host->rebind(tabs, settings.settings, stateKey);
                       });
}

void M3PageNavigationHostTests::optionsOptionTreeRebindSurvivesDeferredDelete()
{
    PortableSettingsFixture settings(QStringLiteral("OptionsTreeLifecycle"));
    TestConfigDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    QList<QWidget*> originalPages;
    QTabWidget* tabs = makeTabs(&dialog, QStringLiteral("Options tree"), 7, &originalPages);
    tabs->setCurrentIndex(5);
    root->addWidget(tabs);
    CM3PageNavigationHost* host = CM3PageNavigationHost::adapt(&dialog, tabs);
    QVERIFY(host);

    host->rebind(tabs,
                 settings.settings,
                 QStringLiteral("Tests/OptionsWindow/Tabs"));
    host->releaseStateManager();
    host->rebind(tabs);
    TreeContainer tree = convertWithProductionConfigDialog(&dialog, tabs);
    tree.titles->hide();
    QPointer<QTabWidget> retiredTabs = tabs;
    const QString treeStateKey = QStringLiteral("Tests/OptionsWindow/Tabs/Tree");
    host->rebind(tree.widget, tree.pages, tree.titles, settings.settings, treeStateKey);
    showFixture(dialog);
    QCOMPARE(host->currentIndex(), 5);
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    tabs->deleteLater();

    processDeferredDeletes();
    QVERIFY(retiredTabs.isNull());
    verifyVisibleNavigation(host, tree.pageWidgets, [pages = tree.pages] { return pages->currentWidget(); });
    QCOMPARE(tree.pages->currentWidget(), host->currentPage());
    verifyStateManager(host,
                       tree.pageWidgets,
                       settings.settings,
                       treeStateKey,
                       [pages = tree.pages] { return pages->currentIndex(); },
                       [pages = tree.pages](int index) { pages->setCurrentIndex(index); },
                       [host, tree, &settings, treeStateKey] {
                           host->rebind(tree.widget,
                                        tree.pages,
                                        tree.titles,
                                        settings.settings,
                                        treeStateKey);
                       });
}

void M3PageNavigationHostTests::searchCapsuleStatesAreVisibleAndAccessible()
{
    MaterialTheme::Apply(qApp, false);

    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    auto* search = new CM3SearchField(&dialog);
    auto* focusParking = new QToolButton(&dialog);
    focusParking->setText(QStringLiteral("Park focus"));
    search->setAccessibleName(QStringLiteral("Sandbox search"));
    root->addWidget(search);
    root->addWidget(focusParking);
    showFixture(dialog);

    auto* clear = search->findChild<QToolButton*>(QStringLiteral("m3SearchClearButton"));
    auto* regex = search->findChild<QToolButton*>(QStringLiteral("m3RegexBuilderButton"));
    QVERIFY(clear);
    QVERIFY(regex);

    search->setState(QStringLiteral("a"), QStringLiteral("a"), QStringLiteral("i"), true);
    QVERIFY(search->isValid());
    focusParking->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(QApplication::focusWidget() == focusParking);
    const QImage validUnfocusedCapsule = search->grab().toImage();
    const QImage validRegexAction = regex->grab().toImage();
    search->focusEditor();
    QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
    QVERIFY(search->property("m3Focus").toBool());
    QVERIFY(validUnfocusedCapsule != search->grab().toImage());

    search->setState(QStringLiteral("("), QStringLiteral("("), QStringLiteral("i"), true);
    QVERIFY(!search->isValid());
    QVERIFY(!search->expression().isValid());
    QVERIFY(search->property("m3Invalid").toBool());
    QVERIFY(search->lineEdit()->property("m3Invalid").toBool());
    QVERIFY(clear->property("m3Invalid").toBool());
    QVERIFY(regex->property("m3Invalid").toBool());
    QVERIFY(search->accessibleDescription().contains(QStringLiteral("Invalid regular expression")));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY(validUnfocusedCapsule != search->grab().toImage());
    QVERIFY(validRegexAction != regex->grab().toImage());
    const QImage invalidRegexAction = regex->grab().toImage();
    regex->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(regex->property("m3Focus").toBool());
    QVERIFY(invalidRegexAction != regex->grab().toImage());

    search->setQuery(QStringLiteral("clear me"));
    QTest::mouseClick(clear, Qt::LeftButton);
    QTRY_VERIFY(search->query().isEmpty());
    QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
}

void M3PageNavigationHostTests::searchBuilderCancellationReturnsFocusToEditor()
{
    QDialog dialog;
    auto* root = new QVBoxLayout(&dialog);
    auto* search = new CM3SearchField(&dialog);
    root->addWidget(search);
    showFixture(dialog);

    auto* regex = search->findChild<QToolButton*>(QStringLiteral("m3RegexBuilderButton"));
    QVERIFY(regex);
    QTest::mouseClick(regex, Qt::LeftButton);
    auto* builder = search->findChild<CRegexBuilderDialog*>(QStringLiteral("regexBuilderDialog"));
    QTRY_VERIFY(builder && builder->isVisible());
    auto* cancel = builder->findChild<QPushButton*>(QStringLiteral("regexCancelButton"));
    QVERIFY(cancel);
    QTest::mouseClick(cancel, Qt::LeftButton);
    QTRY_VERIFY(!builder->isVisible());
    QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
    QCOMPARE(regex->isChecked(), search->regexMode());

    QTest::mouseClick(regex, Qt::LeftButton);
    QTRY_VERIFY(builder->isVisible());
    auto* pattern = builder->findChild<QLineEdit*>(QStringLiteral("regexPatternEdit"));
    QVERIFY(pattern);
    QTest::keyClick(pattern, Qt::Key_Escape);
    QTRY_VERIFY(!builder->isVisible());
    QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
    QCOMPARE(regex->isChecked(), search->regexMode());

    QTest::keyClick(search->lineEdit(), Qt::Key_X);
    QCOMPARE(search->query(), QStringLiteral("x"));
}

void M3PageNavigationHostTests::nativeMenuBuilderCancellationPreservesFilterAndFocus()
{
    CM3Menu::installGlobal(qApp);

    QMenu menu;
    menu.setTitle(QStringLiteral("Searchable actions"));
    QAction* alpha = menu.addAction(QStringLiteral("Alpha action"));
    QAction* beta = menu.addAction(QStringLiteral("Beta action"));
    menu.ensurePolished();
    menu.popup(QPoint(32, 32));
    QTRY_VERIFY(menu.isVisible());

    auto* search = menu.findChild<CM3SearchField*>(QStringLiteral("m3NativeMenuSearch"));
    QTRY_VERIFY(search);
    QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
    auto* regex = search->findChild<QToolButton*>(QStringLiteral("m3RegexBuilderButton"));
    QVERIFY(regex);

    search->setQuery(QStringLiteral("alpha"));
    QTRY_VERIFY(alpha->isVisible());
    QTRY_VERIFY(!beta->isVisible());

    const auto dismissBuilder = [&menu, search, regex, alpha, beta](bool withEscape) {
        bool sawNestedPopup = false;
        bool dismissed = false;
        bool deadline = false;
        QTimer dismissTimer;
        dismissTimer.setSingleShot(true);
        QTimer breakerTimer;
        breakerTimer.setSingleShot(true);
        connect(&dismissTimer, &QTimer::timeout, &menu, [&] {
            auto* builder = search->findChild<CRegexBuilderDialog*>(QStringLiteral("regexBuilderDialog"));
            if (!builder || !builder->isVisible())
                return;
            sawNestedPopup = menu.property("m3ChildDialogActive").toBool();
            if (withEscape) {
                auto* pattern = builder->findChild<QLineEdit*>(QStringLiteral("regexPatternEdit"));
                if (pattern) {
                    dismissed = true;
                    QTest::keyClick(pattern, Qt::Key_Escape);
                }
            } else {
                auto* cancel = builder->findChild<QPushButton*>(QStringLiteral("regexCancelButton"));
                if (cancel) {
                    dismissed = true;
                    QTest::mouseClick(cancel, Qt::LeftButton);
                }
            }
        });
        connect(&breakerTimer, &QTimer::timeout, &menu, [&] {
            deadline = true;
            if (auto* builder = search->findChild<CRegexBuilderDialog*>(QStringLiteral("regexBuilderDialog")))
                builder->reject();
        });
        dismissTimer.start(10);
        breakerTimer.start(1000);
        QTest::mouseClick(regex, Qt::LeftButton);
        dismissTimer.stop();
        breakerTimer.stop();
        QVERIFY2(dismissed && !deadline, "native menu builder cancellation did not complete through its requested control");
        QVERIFY(sawNestedPopup);
        QTRY_VERIFY(menu.isVisible());
        QTRY_VERIFY(!menu.property("m3ResumeMenuSearch").toBool());
        QTRY_VERIFY(QApplication::focusWidget() == search->lineEdit());
        QCOMPARE(regex->isChecked(), search->regexMode());
        QCOMPARE(search->query(), QStringLiteral("alpha"));
        QVERIFY(alpha->isVisible());
        QVERIFY(!beta->isVisible());
    };

    dismissBuilder(false);
    dismissBuilder(true);
    menu.close();
    QTRY_VERIFY(!menu.isVisible());
}

QTEST_MAIN(M3PageNavigationHostTests)
#include "M3PageNavigationHostTests.moc"
