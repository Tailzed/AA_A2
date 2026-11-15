#include "gui/DropImageView.hpp"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

#include "gui/MatQtConvert.hpp"

namespace {
// First dropped local file with a supported image extension, or an empty string.
QString firstImagePath(const QMimeData* mime) {
    if (!mime || !mime->hasUrls()) return {};
    for (const QUrl& url : mime->urls()) {
        if (url.isLocalFile() && isSupportedImagePath(url.toLocalFile())) return url.toLocalFile();
    }
    return {};
}
}  // namespace

DropImageView::DropImageView(QWidget* parent) : QLabel(parent) {
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    setMinimumSize(360, 280);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);  // never grow with the pixmap
    setText("Drop an image here\n(PNG / JPG / BMP)\n\nor use \"Open image...\"");
    setStyleSheet("QLabel { border: 2px dashed #888; border-radius: 8px; color: #666; font-size: 15px; }");
}

void DropImageView::setImage(const QImage& image) {
    pixmap_ = QPixmap::fromImage(image);
    refresh();
}

void DropImageView::dragEnterEvent(QDragEnterEvent* event) {
    if (!firstImagePath(event->mimeData()).isEmpty()) event->acceptProposedAction();
    else event->ignore();
}

void DropImageView::dropEvent(QDropEvent* event) {
    const QString path = firstImagePath(event->mimeData());
    if (path.isEmpty()) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    emit imageDropped(path);
}

void DropImageView::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    refresh();
}

void DropImageView::refresh() {
    if (pixmap_.isNull()) return;
    setPixmap(pixmap_.scaled(contentsRect().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
