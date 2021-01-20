#ifndef NETIF_STAT_VIEW_WIDGET_H
#define NETIF_STAT_VIEW_WIDGET_H

#include <QWidget>
#include <DCommandLinkButton>
#include <vector>
#include "system/device_db.h"
#include <QScrollArea>
DWIDGET_USE_NAMESPACE

class ChartViewWidget;
class NetifInfoModel;
class QGridLayout;
class NetifItemViewWidget;

using namespace std;
using namespace core::system;

class NetifStatViewWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit NetifStatViewWidget(QWidget *parent = nullptr);
    void updateWidgetGeometry();
private:
    void showItem1();
    void showItem2();
    void showItemLg2(const unsigned long count);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
signals:

public slots:
    void fontChanged(const QFont &font);

    void onModelUpdate();
private:
    QFont m_font;
    NetifInfoDB *m_info;
    QWidget *m_centralWidget;
    QGridLayout  *m_graphicsLayout;
    vector<NetifItemViewWidget*> m_lists;
};

#endif // NETIF_STAT_VIEW_WIDGET_H
