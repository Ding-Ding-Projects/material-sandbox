#include "stdafx.h"
#include "DimSumSurprise.h"

#include "../SandMan.h"
#include "../../MiscHelpers/Common/Settings.h"
#include "../../MiscHelpers/Common/UserPresentationSettings.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QRandomGenerator>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QPixmap>
#include <QRegularExpression>
#include <QWindow>

extern QString g_PendingMessage;

namespace {

const QString kCatalogUrl = QStringLiteral(
    "https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json");
const QString kCacheRelativePath = QStringLiteral("dim-sum/catalog-cache.json");

struct Dish {
    QString english;
    QString cantonese;
    QString altEnglish;
    QString altCantonese;
    QString imagePath;
};

bool isPublicImageUrl(const QString& url)
{
    static const QRegularExpression pattern(QStringLiteral(
        "^https://github\\.com/Ding-Ding-Projects/dim-sum-photos/releases/download/"
        "catalog-v1(?:-part-[0-9]{3})?/[^/]+\\.png$"));
    return pattern.match(url).hasMatch();
}

bool isInside(const QString& basePath, const QString& candidatePath)
{
    const QString base = QDir::cleanPath(QFileInfo(basePath).absoluteFilePath()) + QLatin1Char('/');
    const QString candidate = QDir::cleanPath(QFileInfo(candidatePath).absoluteFilePath());
    return candidate.startsWith(base, Qt::CaseInsensitive);
}

QList<Dish> readCache()
{
    if (!theConf)
        return {};

    const QString cacheRoot = QDir(theConf->GetConfigDir()).filePath(QStringLiteral("dim-sum"));
    const QString cachePath = QDir(theConf->GetConfigDir()).filePath(kCacheRelativePath);
    QFile file(cachePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    if (file.size() > 1024 * 1024)
        return {};

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(file.read(1024 * 1024), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return {};
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("sourceUrl")).toString() != kCatalogUrl
        || root.value(QStringLiteral("catalogRevision")).toString().trimmed().isEmpty())
        return {};

    QList<Dish> dishes;
    const QJsonArray records = root.value(QStringLiteral("dishes")).toArray();
    for (const QJsonValue& value : records) {
        const QJsonObject record = value.toObject();
        const QJsonObject names = record.value(QStringLiteral("name")).toObject();
        const QJsonObject imageRecord = record.value(QStringLiteral("image")).toObject();
        const QJsonObject alt = imageRecord.value(QStringLiteral("alt")).toObject();
        const QString english = names.value(QStringLiteral("en")).toString().trimmed();
        const QString cantonese = names.value(QStringLiteral("zhHant")).toString().trimmed();
        const QString altEnglish = alt.value(QStringLiteral("en")).toString().trimmed();
        const QString altCantonese = alt.value(QStringLiteral("yue")).toString().trimmed();
        const QString catalogImagePath = imageRecord.value(QStringLiteral("path")).toString().trimmed();
        const QString relativeImage = imageRecord.value(QStringLiteral("localPath")).toString().trimmed();
        const QString imageUrl = imageRecord.value(QStringLiteral("url")).toString().trimmed();
        if (english.isEmpty() || cantonese.isEmpty() || altEnglish.isEmpty() || altCantonese.isEmpty()
            || catalogImagePath.isEmpty() || relativeImage.isEmpty() || !isPublicImageUrl(imageUrl))
            continue;

        const QString imagePath = QFileInfo(QDir(cacheRoot).filePath(relativeImage)).absoluteFilePath();
        const QFileInfo imageInfo(imagePath);
        if (!isInside(cacheRoot, imagePath) || !imageInfo.exists() || imageInfo.size() > 8 * 1024 * 1024)
            continue;
        QImage decodedImage(imagePath);
        if (decodedImage.isNull() || decodedImage.width() > 4096 || decodedImage.height() > 4096)
            continue;
        dishes.append({ english, cantonese, altEnglish, altCantonese, imagePath });
    }
    return dishes;
}

class DimSumToast final : public QWidget
{
public:
    DimSumToast(QWidget* parent, const Dish& dish)
        : QWidget(nullptr)
    {
        Q_UNUSED(parent);
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
            | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_DeleteOnClose);
        setFocusPolicy(Qt::NoFocus);
        setObjectName(QStringLiteral("MaterialDimSumToast"));
        setStyleSheet(QStringLiteral(
            "#MaterialDimSumToast { background: #fffbfe; color: #1d1b20; "
            "border: 1px solid #79747e; border-radius: 16px; }"
            "QLabel { padding: 4px; }"));

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(4);

        QLabel* image = new QLabel(this);
        image->setPixmap(QPixmap::fromImage(QImage(dish.imagePath)).scaled(
            180, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        image->setAlignment(Qt::AlignCenter);
        image->setAccessibleName(QStringLiteral("%1 · %2").arg(dish.altEnglish, dish.altCantonese));
        image->setToolTip(image->accessibleName());
        layout->addWidget(image);

        QLabel* title = new QLabel(UserPresentationSettings::formatMessage(
            theConf,
            QStringLiteral("A small dim-sum surprise: %1").arg(dish.english),
            QStringLiteral("有份點心小驚喜：%1").arg(dish.cantonese)), this);
        title->setWordWrap(true);
        title->setAlignment(Qt::AlignCenter);
        title->setAccessibleName(QStringLiteral("Dim-sum surprise: %1 · %2")
            .arg(dish.english, dish.cantonese));
        layout->addWidget(title);

        QLabel* source = new QLabel(tr("Photo source: public dim-sum catalog cache"), this);
        source->setWordWrap(true);
        source->setAlignment(Qt::AlignCenter);
        source->setToolTip(kCatalogUrl);
        source->setAccessibleName(QStringLiteral("Photo source: public dim-sum catalog"));
        layout->addWidget(source);

        adjustSize();
    }

    void showForStartup(QScreen* screen)
    {
        if (!screen)
            return;
        const QRect area = screen->availableGeometry();
        move(area.right() - width() - 24, area.bottom() - height() - 32);
        show();
        QTimer::singleShot(9000, this, &QWidget::close);
    }
};

} // namespace

namespace DimSumSurprise {

void schedule(CSandMan* parent)
{
    if (!parent)
        return;
    static bool scheduled = false;
    if (scheduled)
        return;
    scheduled = true;

    // One fresh draw per launch, with no preference or opt-out switch.
    QTimer::singleShot(3500, parent, [parent]() {
        if (!parent->property("DimSumSurpriseStartupReady").toBool()
            || !parent->isVisible() || !parent->isActiveWindow() || QApplication::activeModalWidget())
            return;
        if (QApplication::arguments().contains(QStringLiteral("-autorun"))
            || !g_PendingMessage.isEmpty()
            || UserPresentationSettings::schoolModeEnabled(theConf))
            return;
        if (QRandomGenerator::global()->bounded(10) != 0)
            return;

        const QList<Dish> dishes = readCache();
        if (dishes.isEmpty())
            return;
        const Dish& dish = dishes.at(QRandomGenerator::global()->bounded(dishes.size()));
        auto* toast = new DimSumToast(parent, dish);
        QScreen* screen = parent->windowHandle() ? parent->windowHandle()->screen() : nullptr;
        toast->showForStartup(screen);
    });
}

} // namespace DimSumSurprise
