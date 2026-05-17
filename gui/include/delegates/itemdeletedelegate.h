#include <QStyledItemDelegate>

class QPainter;
class QEvent;
class QAbstractItemModel;
class DeleteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit DeleteItemDelegate(QObject* parent = nullptr);

    QRect trashRect(const QStyleOptionViewItem& option) const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;
};