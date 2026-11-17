#pragma once
#include <QLabel>
#include <QPixmap>

// Shows the loaded image (scaled to fit) and accepts image files dropped onto it.
class DropImageView : public QLabel {
    Q_OBJECT
    public:
        explicit DropImageView(QWidget* parent = nullptr);
        void setImage(const QImage& image);

    signals:
        void imageDropped(const QString& path);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void refresh();
        QPixmap pixmap_;
};
