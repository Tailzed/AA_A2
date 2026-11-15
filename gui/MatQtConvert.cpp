#include "gui/MatQtConvert.hpp"

#include <vector>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <opencv2/imgcodecs.hpp>

QImage matToQImage(const cv::Mat& m) {
    if (m.empty() || m.depth() != CV_8U) return {};
    switch (m.channels()) {
        case 1:
            return QImage(m.data, m.cols, m.rows, static_cast<qsizetype>(m.step), QImage::Format_Grayscale8).copy();
        case 3:
            return QImage(m.data, m.cols, m.rows, static_cast<qsizetype>(m.step), QImage::Format_BGR888).copy();
        case 4:
            return QImage(m.data, m.cols, m.rows, static_cast<qsizetype>(m.step), QImage::Format_ARGB32).copy();
        default:
            return {};
    }
}

cv::Mat loadImageFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) return {};
    const std::vector<uchar> buf(bytes.begin(), bytes.end());
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

bool isSupportedImagePath(const QString& path) {
    static const QStringList exts{"png", "jpg", "jpeg", "bmp"};
    return exts.contains(QFileInfo(path).suffix().toLower());
}
