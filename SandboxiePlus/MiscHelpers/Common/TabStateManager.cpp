#include "stdafx.h"
#include "TabStateManager.h"

#include "Settings.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFont>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSignalBlocker>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidgetAction>

CTabStateManager::CTabStateManager(QTabWidget* tabs, CSettings* settings, const QString& key, QObject* parent)
    : QObject(parent), m_tabs(tabs), m_settings(settings), m_key(key)
{
    if (!m_tabs)
        return;
    QTabBar* bar = m_tabs->tabBar();
    bar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(bar, &QTabBar::customContextMenuRequested, this, &CTabStateManager::showContextMenu);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        m_active = tabKey(index);
        save();
    });
    connect(bar, &QTabBar::tabMoved, this, [this](int, int) { save(); });
    bar->installEventFilter(this);
    auto anchor = [this]() {
        const int index = m_tabs ? m_tabs->currentIndex() : -1;
        return index >= 0 ? m_tabs->tabBar()->tabRect(index).center() : QPoint(1, 1);
    };
    auto addSearchShortcut = [this, anchor](const QKeySequence& sequence, SearchScope scope) {
        QShortcut* shortcut = new QShortcut(sequence, m_tabs);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated, this, [this, anchor, scope]() {
            showScopedTabSearch(scope, anchor());
        });
    };
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), SearchScope::CurrentStrip);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), SearchScope::CurrentGroup);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), SearchScope::GroupNames);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), SearchScope::MasterTabs);
    load();
    restoreOrder();
}

QString CTabStateManager::tabKey(int index) const
{
    return m_tabs && index >= 0 && index < m_tabs->count() ? tabKey(m_tabs->widget(index)) : QString();
}

QString CTabStateManager::tabKey(QWidget* page) const
{
    if (!page)
        return QString();
    const QString stable = page->objectName().trimmed();
    return stable.isEmpty() ? page->windowTitle().trimmed() : stable;
}

void CTabStateManager::load()
{
    if (!m_settings)
        return;
    const QByteArray data = m_settings->GetBlob(m_key);
    if (data.isEmpty())
        return;
    QJsonParseError error;
    const QJsonObject root = QJsonDocument::fromJson(data, &error).object();
    const int schema = root.value(QStringLiteral("schema")).toInt();
    if (error.error != QJsonParseError::NoError || (schema != 1 && schema != 2))
        return;
    for (const QJsonValue& value : root.value(QStringLiteral("pinned")).toArray())
        m_pinned.insert(value.toString());
    const QJsonObject groups = root.value(QStringLiteral("groups")).toObject();
    for (auto it = groups.begin(); it != groups.end(); ++it)
        m_groups.insert(it.key(), it.value().toString());
    if (schema >= 2)
        m_active = root.value(QStringLiteral("active")).toString();

    // Per-tab appearance is a real element target: restore only the bounded
    // QFont attributes that Qt widgets can apply without inventing layout
    // semantics such as line-height or baseline offsets.
    const QJsonObject appearance = root.value(QStringLiteral("appearance")).toObject();
    for (auto it = appearance.begin(); it != appearance.end(); ++it) {
        m_appearanceOverrides.insert(it.key());
        QWidget* page = nullptr;
        for (int i = 0; i < m_tabs->count(); ++i)
            if (tabKey(i) == it.key()) { page = m_tabs->widget(i); break; }
        if (!page || !it.value().isObject())
            continue;
        const QJsonObject value = it.value().toObject();
        QFont font = page->font();
        if (value.contains(QStringLiteral("family"))) font.setFamily(value.value(QStringLiteral("family")).toString());
        if (value.contains(QStringLiteral("pointSize"))) font.setPointSize(value.value(QStringLiteral("pointSize")).toInt());
        if (value.contains(QStringLiteral("weight"))) font.setWeight(static_cast<QFont::Weight>(value.value(QStringLiteral("weight")).toInt()));
        if (value.contains(QStringLiteral("style"))) font.setStyle(static_cast<QFont::Style>(value.value(QStringLiteral("style")).toInt()));
        if (value.contains(QStringLiteral("underline"))) font.setUnderline(value.value(QStringLiteral("underline")).toBool());
        if (value.contains(QStringLiteral("strikeOut"))) font.setStrikeOut(value.value(QStringLiteral("strikeOut")).toBool());
        if (value.contains(QStringLiteral("overline"))) font.setOverline(value.value(QStringLiteral("overline")).toBool());
        if (value.contains(QStringLiteral("capitalization"))) font.setCapitalization(static_cast<QFont::Capitalization>(value.value(QStringLiteral("capitalization")).toInt()));
        if (value.contains(QStringLiteral("letterSpacing"))) font.setLetterSpacing(QFont::AbsoluteSpacing, value.value(QStringLiteral("letterSpacing")).toDouble());
        if (value.contains(QStringLiteral("wordSpacing"))) font.setWordSpacing(value.value(QStringLiteral("wordSpacing")).toDouble());
        page->setFont(font);
    }
}

void CTabStateManager::save() const
{
    if (!m_settings || !m_tabs)
        return;
    QJsonObject root;
    root.insert(QStringLiteral("schema"), 2);
    QJsonArray order;
    for (int i = 0; i < m_tabs->count(); ++i)
        order.append(tabKey(i));
    root.insert(QStringLiteral("order"), order);
    QJsonArray pinned;
    for (const QString& name : m_pinned)
        pinned.append(name);
    root.insert(QStringLiteral("pinned"), pinned);
    QJsonObject groups;
    for (auto it = m_groups.cbegin(); it != m_groups.cend(); ++it)
        groups.insert(it.key(), it.value());
    root.insert(QStringLiteral("groups"), groups);
    root.insert(QStringLiteral("active"), m_active.isEmpty() ? tabKey(m_tabs->currentIndex()) : m_active);
    QJsonObject appearance;
    for (const QString& key : m_appearanceOverrides) {
        QWidget* page = nullptr;
        for (int i = 0; i < m_tabs->count(); ++i)
            if (tabKey(i) == key) { page = m_tabs->widget(i); break; }
        if (!page)
            continue;
        const QFont font = page->font();
        QJsonObject value;
        value.insert(QStringLiteral("family"), font.family());
        value.insert(QStringLiteral("pointSize"), font.pointSize());
        value.insert(QStringLiteral("weight"), static_cast<int>(font.weight()));
        value.insert(QStringLiteral("style"), static_cast<int>(font.style()));
        value.insert(QStringLiteral("underline"), font.underline());
        value.insert(QStringLiteral("strikeOut"), font.strikeOut());
        value.insert(QStringLiteral("overline"), font.overline());
        value.insert(QStringLiteral("capitalization"), static_cast<int>(font.capitalization()));
        value.insert(QStringLiteral("letterSpacing"), font.letterSpacing());
        value.insert(QStringLiteral("wordSpacing"), font.wordSpacing());
        appearance.insert(key, value);
    }
    root.insert(QStringLiteral("appearance"), appearance);
    m_settings->SetBlob(m_key, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void CTabStateManager::restoreOrder()
{
    if (!m_tabs || !m_settings)
        return;
    const QJsonDocument document = QJsonDocument::fromJson(m_settings->GetBlob(m_key));
    const QJsonObject root = document.object();
    const int schema = root.value(QStringLiteral("schema")).toInt();
    if (schema != 1 && schema != 2)
        return;
    const QJsonArray order = root.value(QStringLiteral("order")).toArray();
    QTabBar* bar = m_tabs->tabBar();
    for (int target = 0; target < order.size(); ++target) {
        const QString wanted = order.at(target).toString();
        int current = -1;
        for (int i = target; i < m_tabs->count(); ++i) if (tabKey(i) == wanted) { current = i; break; }
        if (current >= 0 && current != target)
            bar->moveTab(current, target);
    }
    if (!m_active.isEmpty()) {
        for (int i = 0; i < m_tabs->count(); ++i) {
            if (tabKey(i) == m_active) {
                m_tabs->setCurrentIndex(i);
                break;
            }
        }
    }
}

void CTabStateManager::showContextMenu(const QPoint& position)
{
    if (!m_tabs)
        return;
    const int index = m_tabs->tabBar()->tabAt(position);
    if (index < 0)
        return;
    const QString name = tabKey(index);
    QMenu menu(m_tabs);
    QWidgetAction* searchAction = new QWidgetAction(&menu);
    QLineEdit* filter = new QLineEdit(&menu);
    filter->setPlaceholderText(tr("Filter tab actions"));
    filter->setAccessibleName(tr("Tab action search"));
    searchAction->setDefaultWidget(filter);
    menu.addAction(searchAction);
    QAction* pin = menu.addAction(m_pinned.contains(name) ? tr("Unpin tab") : tr("Pin tab"));
    QAction* currentStripSearch = menu.addAction(tr("Search current tab strip…"));
    currentStripSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    QAction* currentGroupSearch = menu.addAction(tr("Search current tab group…"));
    currentGroupSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    currentGroupSearch->setEnabled(!m_groups.value(name).isEmpty());
    QAction* groupNameSearch = menu.addAction(tr("Search tab groups…"));
    groupNameSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    QAction* searchTabs = menu.addAction(tr("Search all open tabs…"));
    searchTabs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    QAction* group = menu.addAction(tr("Move… into group…"));
    QAction* clearGroup = menu.addAction(tr("Remove from group"));
    QAction* edit = menu.addAction(tr("Edit tab page typography…"));
    connect(filter, &QLineEdit::textChanged, &menu, [filter, pin, currentStripSearch, currentGroupSearch, groupNameSearch, searchTabs, group, clearGroup, edit](const QString& query) {
        for (QAction* action : { pin, currentStripSearch, currentGroupSearch, groupNameSearch, searchTabs, group, clearGroup, edit })
            action->setVisible(query.isEmpty() || action->text().contains(query, Qt::CaseInsensitive));
    });
    connect(currentStripSearch, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::CurrentStrip, position); });
    connect(currentGroupSearch, &QAction::triggered, this, [this, position, name]() { showScopedTabSearch(SearchScope::CurrentGroup, position, m_groups.value(name)); });
    connect(groupNameSearch, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::GroupNames, position); });
    connect(searchTabs, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::MasterTabs, position); });
    connect(pin, &QAction::triggered, &menu, [this, name]() {
        if (m_pinned.contains(name)) m_pinned.remove(name); else m_pinned.insert(name);
        save();
    });
    connect(group, &QAction::triggered, &menu, [this, name, position]() {
        showGroupPicker(name, position);
    });
    connect(clearGroup, &QAction::triggered, &menu, [this, name]() { m_groups.remove(name); save(); });
    connect(edit, &QAction::triggered, &menu, [this, name]() {
        QWidget* page = nullptr;
        for (int i = 0; i < m_tabs->count(); ++i)
            if (tabKey(i) == name) { page = m_tabs->widget(i); break; }
        if (!page)
            return;
        QDialog* editor = new QDialog(m_tabs, Qt::Tool | Qt::WindowStaysOnTopHint);
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->setWindowTitle(tr("Edit tab page typography"));
        QFormLayout* form = new QFormLayout(editor);
        QFontComboBox* font = new QFontComboBox(editor);
        font->setCurrentFont(page->font());
        font->setAccessibleName(tr("Tab font"));
        form->addRow(tr("Font"), font);
        QSpinBox* size = new QSpinBox(editor);
        size->setRange(8, 48);
        size->setValue(page->font().pointSize() > 0 ? page->font().pointSize() : 10);
        size->setAccessibleName(tr("Tab font size"));
        form->addRow(tr("Size"), size);
        QComboBox* weight = new QComboBox(editor);
        const QList<QPair<QString, int>> weights = {
            {tr("Thin"), QFont::Thin}, {tr("Extra light"), QFont::ExtraLight},
            {tr("Light"), QFont::Light}, {tr("Normal"), QFont::Normal},
            {tr("Medium"), QFont::Medium}, {tr("Demi bold"), QFont::DemiBold},
            {tr("Bold"), QFont::Bold}, {tr("Extra bold"), QFont::ExtraBold},
            {tr("Black"), QFont::Black}
        };
        for (const auto& option : weights)
            weight->addItem(option.first, option.second);
        const int weightIndex = weight->findData(page->font().weight());
        if (weightIndex >= 0)
            weight->setCurrentIndex(weightIndex);
        weight->setAccessibleName(tr("Tab font weight"));
        form->addRow(tr("Weight"), weight);
        QComboBox* style = new QComboBox(editor);
        style->addItem(tr("Normal"), QFont::StyleNormal);
        style->addItem(tr("Italic"), QFont::StyleItalic);
        style->addItem(tr("Oblique"), QFont::StyleOblique);
        const int styleIndex = style->findData(page->font().style());
        if (styleIndex >= 0)
            style->setCurrentIndex(styleIndex);
        style->setAccessibleName(tr("Tab font style"));
        form->addRow(tr("Style"), style);
        QCheckBox* underline = new QCheckBox(tr("Underline"), editor);
        underline->setChecked(page->font().underline());
        underline->setAccessibleName(tr("Tab underline"));
        QCheckBox* strikeOut = new QCheckBox(tr("Strikeout"), editor);
        strikeOut->setChecked(page->font().strikeOut());
        strikeOut->setAccessibleName(tr("Tab strikeout"));
        QCheckBox* overline = new QCheckBox(tr("Overline"), editor);
        overline->setChecked(page->font().overline());
        overline->setAccessibleName(tr("Tab overline"));
        QHBoxLayout* decorations = new QHBoxLayout();
        decorations->addWidget(underline);
        decorations->addWidget(strikeOut);
        decorations->addWidget(overline);
        decorations->addStretch();
        form->addRow(tr("Decorations"), decorations);
        QComboBox* capitalization = new QComboBox(editor);
        capitalization->addItem(tr("Mixed case"), QFont::MixedCase);
        capitalization->addItem(tr("Small caps"), QFont::SmallCaps);
        capitalization->addItem(tr("All uppercase"), QFont::AllUppercase);
        capitalization->addItem(tr("All lowercase"), QFont::AllLowercase);
        capitalization->addItem(tr("Capitalize words"), QFont::Capitalize);
        const int capitalizationIndex = capitalization->findData(page->font().capitalization());
        if (capitalizationIndex >= 0)
            capitalization->setCurrentIndex(capitalizationIndex);
        capitalization->setAccessibleName(tr("Tab capitalization"));
        form->addRow(tr("Capitalization"), capitalization);
        QDoubleSpinBox* letterSpacing = new QDoubleSpinBox(editor);
        QDoubleSpinBox* wordSpacing = new QDoubleSpinBox(editor);
        for (QDoubleSpinBox* spacing : {letterSpacing, wordSpacing}) {
            spacing->setRange(-20.0, 100.0);
            spacing->setDecimals(1);
            spacing->setSingleStep(0.5);
            spacing->setSuffix(tr(" px"));
        }
        letterSpacing->setValue(page->font().letterSpacing());
        wordSpacing->setValue(page->font().wordSpacing());
        letterSpacing->setAccessibleName(tr("Tab letter spacing"));
        wordSpacing->setAccessibleName(tr("Tab word spacing"));
        form->addRow(tr("Letter spacing"), letterSpacing);
        form->addRow(tr("Word spacing"), wordSpacing);
        QLabel* limitation = new QLabel(tr("This target applies to the tab page content, not the tab-bar label. It uses only QFont properties that Qt widgets can render consistently. Line-height, baseline offset, superscript, subscript, variable-font axes, underline variants, and text effects such as outline, shadow, and glow are not represented."), editor);
        limitation->setWordWrap(true);
        limitation->setProperty("secondary", true);
        form->addRow(tr("Not represented"), limitation);
        QPushButton* apply = new QPushButton(tr("Apply"), editor);
        form->addRow(QString(), apply);
        connect(apply, &QPushButton::clicked, editor, [this, page, font, size, weight, style, underline, strikeOut, overline, capitalization, letterSpacing, wordSpacing, editor, name]() {
            QFont value = font->currentFont();
            value.setPointSize(size->value());
            value.setWeight(static_cast<QFont::Weight>(weight->currentData().toInt()));
            value.setStyle(static_cast<QFont::Style>(style->currentData().toInt()));
            value.setUnderline(underline->isChecked());
            value.setStrikeOut(strikeOut->isChecked());
            value.setOverline(overline->isChecked());
            value.setCapitalization(static_cast<QFont::Capitalization>(capitalization->currentData().toInt()));
            value.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing->value());
            value.setWordSpacing(wordSpacing->value());
            page->setFont(value);
            m_appearanceOverrides.insert(name);
            save();
            editor->close();
        });
        const QPoint anchor = m_tabs->tabBar()->mapToGlobal(m_tabs->tabBar()->tabRect(m_tabs->indexOf(page)).bottomLeft());
        editor->move(anchor);
        editor->show();
    });
    menu.exec(m_tabs->tabBar()->mapToGlobal(position));
}

void CTabStateManager::showGroupPicker(const QString& tabName, const QPoint& position)
{
    if (!m_tabs)
        return;

    QDialog* dialog = new QDialog(m_tabs, Qt::Tool | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Move tab into group"));
    dialog->setMinimumSize(420, 340);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* description = new QLabel(tr("Choose an existing group or create a new one. The group stays collapsed when it was collapsed before this move."), dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    QHBoxLayout* searchRow = new QHBoxLayout();
    QLineEdit* search = new QLineEdit(dialog);
    search->setPlaceholderText(tr("Search groups"));
    search->setAccessibleName(tr("Group search"));
    QCheckBox* regex = new QCheckBox(tr("Regex"), dialog);
    regex->setToolTip(tr("Use the regex builder for this group search"));
    regex->setAccessibleName(tr("Enable regular expression group search"));
    searchRow->addWidget(search, 1);
    searchRow->addWidget(regex);
    layout->addLayout(searchRow);

    QGroupBox* builder = new QGroupBox(tr("Regex builder"), dialog);
    QFormLayout* builderLayout = new QFormLayout(builder);
    QLineEdit* pattern = new QLineEdit(dialog);
    pattern->setPlaceholderText(tr("Raw pattern (for example: ^Work)"));
    pattern->setAccessibleName(tr("Group regex pattern"));
    pattern->setEnabled(false);
    QLabel* validation = new QLabel(tr("Plain-text search is active."), dialog);
    validation->setWordWrap(true);
    validation->setAccessibleName(tr("Group regex validation"));
    builderLayout->addRow(tr("Pattern"), pattern);
    builderLayout->addRow(tr("Validation"), validation);
    layout->addWidget(builder);

    QListWidget* groups = new QListWidget(dialog);
    groups->setAccessibleName(tr("Available tab groups"));
    groups->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(groups, 1);

    QHBoxLayout* actions = new QHBoxLayout();
    QPushButton* create = new QPushButton(tr("Create new group…"), dialog);
    create->setAccessibleName(tr("Create a new tab group"));
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Move"));
    actions->addWidget(create);
    actions->addStretch(1);
    actions->addWidget(buttons);
    layout->addLayout(actions);

    auto refresh = [this, search, regex, pattern, validation, groups]() {
        const QString needle = (regex->isChecked() ? pattern->text() : search->text()).left(4096);
        QRegularExpression expression;
        if (regex->isChecked()) {
            expression = QRegularExpression(needle);
            if (!expression.isValid()) {
                validation->setText(tr("Invalid pattern: %1").arg(expression.errorString()));
                groups->clear();
                return;
            }
            validation->setText(tr("Valid regex; results update as you type."));
        } else {
            validation->setText(tr("Plain-text search is active."));
        }
        QSet<QString> names;
        for (auto it = m_groups.cbegin(); it != m_groups.cend(); ++it)
            if (!it.value().isEmpty())
                names.insert(it.value());
        QStringList sorted = names.values();
        sorted.sort(Qt::CaseInsensitive);
        groups->clear();
        for (const QString& name : sorted) {
            const bool match = needle.isEmpty() || (regex->isChecked() ? expression.match(name).hasMatch() : name.contains(needle, Qt::CaseInsensitive));
            if (!match)
                continue;
            int members = 0;
            for (auto it = m_groups.cbegin(); it != m_groups.cend(); ++it)
                if (it.value() == name)
                    ++members;
            QListWidgetItem* item = new QListWidgetItem(tr("%1  · %2 tabs").arg(name).arg(members), groups);
            item->setData(Qt::UserRole, name);
            item->setToolTip(tr("Group %1 contains %2 tab(s)").arg(name).arg(members));
            item->setBackground(QColor::fromHsv(static_cast<int>(qHash(name) % 360u), 35, 245));
        }
        if (groups->count() > 0)
            groups->setCurrentRow(0);
    };
    connect(search, &QLineEdit::textChanged, dialog, [pattern, regex, refresh](const QString& value) {
        if (!regex->isChecked()) {
            QSignalBlocker blocker(pattern);
            pattern->setText(value);
        }
        refresh();
    });
    connect(pattern, &QLineEdit::textChanged, dialog, [refresh](const QString&) { refresh(); });
    connect(regex, &QCheckBox::toggled, dialog, [search, pattern, refresh](bool enabled) {
        QSignalBlocker blocker(pattern);
        pattern->setText(search->text());
        pattern->setEnabled(enabled);
        refresh();
    });
    connect(create, &QPushButton::clicked, dialog, [this, tabName, dialog, search, refresh]() {
        const QString value = search->text().trimmed();
        if (value.isEmpty()) {
            search->setFocus();
            return;
        }
        m_groups.insert(tabName, value);
        save();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, tabName, groups, dialog]() {
        QListWidgetItem* item = groups->currentItem();
        const QString value = item ? item->data(Qt::UserRole).toString().trimmed() : QString();
        if (value.isEmpty())
            return;
        m_groups.insert(tabName, value);
        save();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    search->setFocus();
    refresh();
    const int anchorIndex = m_tabs->tabBar()->tabAt(position);
    const QRect anchorRect = anchorIndex >= 0 ? m_tabs->tabBar()->tabRect(anchorIndex) : QRect(QPoint(0, 0), QSize(1, 1));
    dialog->move(m_tabs->tabBar()->mapToGlobal(anchorRect.bottomLeft()));
    dialog->show();
}

void CTabStateManager::showTabSearch(const QPoint& position)
{
    showScopedTabSearch(SearchScope::MasterTabs, position);
}

void CTabStateManager::showScopedTabSearch(SearchScope scope, const QPoint& position, const QString& groupName)
{
    if (!m_tabs)
        return;

    QDialog* dialog = new QDialog(m_tabs, Qt::Tool | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    const bool groupNames = scope == SearchScope::GroupNames;
    const bool groupScoped = scope == SearchScope::CurrentGroup;
    const QString title = groupNames ? tr("Search tab groups") : groupScoped ? tr("Search current tab group") : scope == SearchScope::CurrentStrip ? tr("Search current tab strip") : tr("Search all open tabs");
    dialog->setWindowTitle(title);
    dialog->setMinimumSize(440, 360);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QHBoxLayout* searchRow = new QHBoxLayout();
    QLineEdit* query = new QLineEdit(dialog);
    query->setPlaceholderText(groupNames ? tr("Search group names") : groupScoped ? tr("Search tabs in group %1").arg(groupName) : scope == SearchScope::CurrentStrip ? tr("Search tabs in this strip") : tr("Search tab labels and groups"));
    query->setAccessibleName(title);
    QCheckBox* regex = new QCheckBox(tr("Regex"), dialog);
    regex->setToolTip(tr("Use the regex builder for this tab search"));
    regex->setAccessibleName(tr("Enable regular expression search"));
    QCheckBox* caseSensitive = new QCheckBox(tr("Case sensitive"), dialog);
    caseSensitive->setAccessibleName(tr("Case sensitive tab search"));
    searchRow->addWidget(query, 1);
    searchRow->addWidget(regex);
    searchRow->addWidget(caseSensitive);
    layout->addLayout(searchRow);

    QGroupBox* builder = new QGroupBox(tr("Regex builder"), dialog);
    QFormLayout* builderLayout = new QFormLayout(builder);
    QLineEdit* pattern = new QLineEdit(dialog);
    pattern->setPlaceholderText(tr("Raw pattern (for example: ^Settings)"));
    pattern->setAccessibleName(tr("Regex pattern"));
    pattern->setEnabled(false);
    QLineEdit* sample = new QLineEdit(dialog);
    sample->setPlaceholderText(tr("Sample text for capture preview"));
    sample->setAccessibleName(tr("Regex sample text"));
    QLabel* regexStatus = new QLabel(tr("Plain-text search is active."), dialog);
    regexStatus->setWordWrap(true);
    regexStatus->setAccessibleName(tr("Regex validation and capture preview"));
    builderLayout->addRow(tr("Pattern"), pattern);
    builderLayout->addRow(tr("Sample"), sample);
    builderLayout->addRow(tr("Validation"), regexStatus);
    layout->addWidget(builder);

    QListWidget* results = new QListWidget(dialog);
    results->setAccessibleName(groupNames ? tr("Tab group search results") : tr("Open tab search results"));
    results->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(results, 1);
    QLabel* count = new QLabel(dialog);
    count->setAccessibleName(tr("Matching tab count"));
    layout->addWidget(count);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    auto refresh = [this, query, regex, caseSensitive, pattern, sample, regexStatus, results, count, groupScoped, groupName, groupNames]() {
        const QString needle = query->text().left(4096);
        const bool useRegex = regex->isChecked();
        const Qt::CaseSensitivity sensitivity = caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegularExpression expression;
        if (useRegex) {
            QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
            if (!caseSensitive->isChecked())
                options |= QRegularExpression::CaseInsensitiveOption;
            expression = QRegularExpression(pattern->text().left(4096), options);
            if (!expression.isValid()) {
                regexStatus->setText(tr("Invalid pattern: %1").arg(expression.errorString()));
                results->clear();
                count->setText(tr("0 matching tabs"));
                return;
            }
        }
        regexStatus->setText(useRegex ? tr("Valid pattern. Capture preview: %1").arg(expression.match(sample->text()).capturedTexts().join(QStringLiteral(" · "))) : tr("Plain-text search is active."));
        results->clear();
        int matches = 0;
        QSet<QString> seenGroups;
        for (int i = 0; i < m_tabs->count(); ++i) {
            const QString key = tabKey(i);
            const QString group = m_groups.value(key);
            const QString label = m_tabs->tabText(i);
            if (groupScoped && group != groupName)
                continue;
            if (groupNames && group.isEmpty())
                continue;
            if (groupNames && seenGroups.contains(group))
                continue;
            if (groupNames)
                seenGroups.insert(group);
            const QString haystack = label + QStringLiteral(" ") + key + QStringLiteral(" ") + group;
            bool matched = needle.isEmpty();
            if (!needle.isEmpty())
                matched = useRegex ? expression.match(haystack).hasMatch() : haystack.contains(needle, sensitivity);
            if (!matched)
                continue;
            QString display = groupNames ? group : (label.isEmpty() ? key : label);
            if (!groupNames && !group.isEmpty())
                display += tr("  · group: %1").arg(group);
            if (!groupNames && m_pinned.contains(key))
                display += tr("  · pinned");
            QListWidgetItem* item = new QListWidgetItem(display, results);
            item->setData(Qt::UserRole, i);
            ++matches;
        }
        count->setText(tr("%1 matching tabs").arg(matches));
        if (results->count() > 0)
            results->setCurrentRow(0);
    };
    connect(query, &QLineEdit::textChanged, dialog, [pattern, regex, refresh](const QString& value) {
        if (!regex->isChecked()) {
            QSignalBlocker blocker(pattern);
            pattern->setText(value);
        }
        refresh();
    });
    connect(pattern, &QLineEdit::textChanged, dialog, [query, regex, refresh](const QString& value) {
        if (regex->isChecked()) {
            QSignalBlocker blocker(query);
            query->setText(value);
        }
        refresh();
    });
    connect(regex, &QCheckBox::toggled, dialog, [query, pattern, refresh](bool enabled) {
        QSignalBlocker blocker(pattern);
        pattern->setText(query->text());
        refresh();
        pattern->setEnabled(enabled);
    });
    connect(caseSensitive, &QCheckBox::toggled, dialog, [refresh](bool) { refresh(); });
    connect(sample, &QLineEdit::textChanged, dialog, [refresh](const QString&) { refresh(); });
    connect(results, &QListWidget::itemActivated, dialog, [this, dialog](QListWidgetItem* item) {
        const int index = item ? item->data(Qt::UserRole).toInt() : -1;
        if (index >= 0 && index < m_tabs->count()) {
            m_tabs->setCurrentIndex(index);
            m_active = tabKey(index);
            save();
            dialog->close();
        }
    });
    query->setFocus();
    refresh();
    const int anchorIndex = m_tabs->tabBar()->tabAt(position);
    const QRect anchorRect = anchorIndex >= 0 ? m_tabs->tabBar()->tabRect(anchorIndex) : QRect(QPoint(0, 0), QSize(1, 1));
    const QPoint anchor = m_tabs->tabBar()->mapToGlobal(anchorRect.bottomLeft());
    dialog->move(anchor);
    dialog->show();
}

bool CTabStateManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_tabs->tabBar() && event->type() == QEvent::MouseButtonRelease)
        save();
    return QObject::eventFilter(watched, event);
}
