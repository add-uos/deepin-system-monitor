/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CHART_VIEW_WIDGET_H
#define CHART_VIEW_WIDGET_H

#include <QWidget>

class ChartViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartViewWidget(QWidget *parent = nullptr);

public:
    void setData1Color(const QColor &color);
    void addData1(qreal data);

    void setData2Color(const QColor &color);
    void addData2(qreal data);

    void setAxisTitle(const QString &text);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);

private slots:
    void changeFont(const QFont &font);

private:
    void drawBackPixmap();
    void drawData1(QPainter *painter);
    void drawData2(QPainter *painter);

    void getPainterPathByData(const QList<qreal> &listData, QPainterPath &path);

private:
    int gridSize = 10;

    QString m_axisTitle = {"100%"};

    QFont m_textfont;
    QPixmap m_backPixmap;

    QRect m_chartRect;
    QColor m_data1Color = {"#00C5C0"};
    QColor m_data2Color = {"#FEDF19"};

    QList<qreal> m_listData1;
    QList<qreal> m_listData2;
};

#endif // CHART_VIEW_WIDGET_H