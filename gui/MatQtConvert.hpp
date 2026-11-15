#pragma once
#include <QImage>
#include <QString>

#include <opencv2/core.hpp>

// cv::Mat (8-bit, 1/3/4 channels, BGR order) -> QImage. Always returns a deep copy,
// so the QImage stays valid after the Mat is destroyed. Null QImage on failure.
QImage matToQImage(const cv::Mat& m);

// Unicode-safe image loading (reads the file with Qt, decodes with OpenCV, so
// non-ASCII Windows paths work). Returns an empty Mat on failure. Result is BGR.
cv::Mat loadImageFile(const QString& path);

// Cheap extension check used by the drag-and-drop handlers.
bool isSupportedImagePath(const QString& path);
