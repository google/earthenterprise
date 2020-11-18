#ifndef SPACER_H
#define SPACER_H

#include <Qt/qlayoutitem.h>

class spacer : public QSpacerItem
{
public:
    spacer() = default;

    void setObjectName(const QString& oName) {}
    void setOrientation(const Qt::Orientation& orient) {}
    void setSizeType(const QSizePolicy& policy) {}
    void setSizeHint(const QSize& sizeHint) {}
};

#endif // SPACER_H
