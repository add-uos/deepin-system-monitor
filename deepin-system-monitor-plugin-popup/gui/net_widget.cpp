/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "net_widget.h"
#include "../common/utils.h"
#include "common/datacommon.h"
#include "datadealsingleton.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QBrush>
#include <QPaintEvent>
#include <QFontMetrics>

DWIDGET_USE_NAMESPACE

using namespace Utils;

NetWidget::NetWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &NetWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    setFixedSize(280, 203);


    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &NetWidget::changeFont);

    downloadSpeeds = new QList<double>();
    for (int i = 0; i <= pointsNumber; i++) {
        downloadSpeeds->append(0);
    }
    uploadSpeeds = new QList<double>();
    for (int i = 0; i <= pointsNumber; i++) {
        uploadSpeeds->append(0);
    }

    initConnection();

    installEventFilter(this);

}
NetWidget::~NetWidget() {
    delete downloadSpeeds;
    delete uploadSpeeds;
}

void NetWidget::initConnection()
{
    connect(&DataDealSingleton::getInstance(), &DataDealSingleton::sigDataUpdate, this, &NetWidget::updateStatus);
}


void NetWidget::getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue)
{
    qreal offsetX = 0;
    qreal distance = (this->width() - 20) * 1.0 / pointsNumber;
    int dataCount = listData->size();

    for (int i = 0;  i < dataCount - 1; i++) {
        QPointF sp = QPointF(offsetX, renderMaxHeight * listData->at(i) / maxVlaue);;
        QPointF ep = QPointF(offsetX + distance, renderMaxHeight * listData->at(i + 1) / maxVlaue);;

        offsetX += distance;

        QPointF c1 = QPointF((sp.x() + ep.x()) / 2.0, sp.y());
        QPointF c2 = QPointF((sp.x() + ep.x()) / 2.0, ep.y());
        path.cubicTo(c1, c2, ep);
    }
}

void NetWidget::updateStatus()
{
    QString netReceive, netTotalReceive, netSend, totalSend;
    if (!DataDealSingleton::getInstance().readNetInfo(netReceive, netTotalReceive, netSend, totalSend))
        return;

    QStringList strs = netReceive.split(" ");
    if (strs.size() == 2)
    {
        m_netReceive = strs.at(0);
        m_netReceiveUnit = strs.at(1);
    }
    else
        return;

    // Init download path.
    downloadSpeeds->append(m_netReceive.toDouble());

    if (downloadSpeeds->size() > pointsNumber + 1) {
        downloadSpeeds->pop_front();
    }
    double downloadMaxHeight = *std::max_element(downloadSpeeds->begin(), downloadSpeeds->end()) * 1.1;

    strs = netSend.split(" ");
    if (strs.size() == 2)
    {
        m_netSend = strs.at(0);
        m_netSendUnit = strs.at(1);
    }
    else
        return;

    // Init upload path.
    uploadSpeeds->append(m_netSend.toDouble());

    if (uploadSpeeds->size() > pointsNumber + 1) {
        uploadSpeeds->pop_front();
    }
    double uploadMaxHeight = *std::max_element(uploadSpeeds->begin(), uploadSpeeds->end()) * 1.1;

    double maxHeight = qMax(downloadMaxHeight, uploadMaxHeight);

    QPainterPath tmpDownloadpath;
    getPainterPathByData(downloadSpeeds, tmpDownloadpath, maxHeight);
    downloadPath = tmpDownloadpath;

    QPainterPath tmpUploadpath;
    getPainterPathByData(uploadSpeeds, tmpUploadpath, maxHeight);
    uploadPath = tmpUploadpath;


    //zong
    strs = netTotalReceive.split(" ");
    if (strs.size() == 2)
    {
        m_netTotalReceive = strs.at(0);
        m_netTotalReceiveUnit = strs.at(1);
    }
    else
        return;

    strs = totalSend.split(" ");
    if (strs.size() == 2)
    {
        m_totalSend = strs.at(0);
        m_totalSendUnit = strs.at(1);
    }
    else
        return;

    update();
}

void NetWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_titleTrans = Globals::TitleTransLight;
        m_contentTrans = Globals::contentTransLight;
        m_hoverTrans = Globals::hoverTransLight;
        m_icon = QIcon(QString(":/icons/icon_network_light.png"));
        break;
    case DApplicationHelper::DarkType:
        m_titleTrans = Globals::TitleTransDark;
        m_contentTrans = Globals::contentTransDark;
        m_hoverTrans = Globals::hoverTransDark;
//        m_icon = QIcon(iconPathFromQrc("dark/icon_network_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);
}

void NetWidget::paintEvent(QPaintEvent *e)
{
    setFixedWidth(280);
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //裁剪绘制区域
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    if (m_isHover) {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, m_hoverTrans)));
    } else {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 0)));
    }

    //标题栏背景
    QRect titleRect(rect().x(), rect().y(), 280, 36);
    painter.fillRect(titleRect, QBrush(QColor(255, 255, 255, m_titleTrans)));
    QRect contentRect(rect().x(), rect().y()+36, 280, 167);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,m_contentTrans)));

    //标题
    painter.setFont(m_sectionFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitleTxt = fmTitle.width("网络");
    int heightTitleTxt = fmTitle.descent()+fmTitle.ascent();
    QRect netTitleRect(titleRect.x(), titleRect.y(), widthTitleTxt, heightTitleTxt);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,"网络");

    //图标
    int iconSize = 20;
    QRect iconRect(titleRect.x()+titleRect.width()/2-netTitleRect.width() -5, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    //分隔符
    painter.setFont(m_contentFont);
    QRect separatorRect1(contentRect.x()+10, contentRect.y()+10, 1, 38);
    painter.fillRect(separatorRect1, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect2(contentRect.x()+10+87*1, contentRect.y()+10, 1, 38);
    painter.fillRect(separatorRect2, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect3(contentRect.x()+10+87*2, contentRect.y()+10, 1, 38);
    painter.fillRect(separatorRect3, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect4(contentRect.x()+10+87*3, contentRect.y()+10, 1, 38);
    painter.fillRect(separatorRect4, QBrush(QColor(0, 0, 0,20)));

    int bulletSize = 6;
    int padleft = 36;
    auto spacing = 20;
    auto margin = 10;

    // Draw network summary.
    QString recvTitle = DApplication::translate("Process.Graph.View", "正在接受");//Download
    QString recvTotalTitle = DApplication::translate("Process.Graph.View", "总计接受");//Total Received"
    QString sentTitle = DApplication::translate("Process.Graph.View", "正在发送");//Upload
    QString sentTotalTitle = DApplication::translate("Process.Graph.View", "总计发送");//otal Sent

    QFontMetrics fmContent(m_contentFont);
    QFontMetrics fmContentUnit(m_contentUnitFont);
    QFontMetrics fmSubContent(m_subContentFont);
    QRect contentNetRect(padleft, separatorRect1.y() + separatorRect1.height(),
                      rect().x() + rect().width() - padleft, 1);

    //正在接受
    QRect recv1Rect(contentNetRect.x(), contentNetRect.y(),
                          fmContent.size(Qt::TextSingleLine, m_netReceive).width(), fmContent.height());
    //正在接受单位
    QRect recv2UnitRect(recv1Rect.x()+recv1Rect.width(), contentNetRect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_netReceiveUnit).width(), fmContent.height());
    //"总计接受"的值
    QRect totalReceiveRect(qCeil((contentNetRect.width() + spacing) / 2) + spacing, recv1Rect.y(),
                  contentNetRect.width() - recv1Rect.width() - spacing, recv1Rect.height());
    QRect Receive1Rect(totalReceiveRect.x(), totalReceiveRect.y(),
                          fmContent.size(Qt::TextSingleLine, m_netTotalReceive).width(), fmContent.height());
    QRect Receive2Rect(Receive1Rect.x()+Receive1Rect.width(), Receive1Rect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_netTotalReceiveUnit).width(), fmContent.height());

    //正在接受标题
    QRect recvTitleRect(contentNetRect.x(), recv1Rect.y() + recv1Rect.height(),
                        fmContent.size(Qt::TextSingleLine, recvTitle).width(), fmSubContent.height());
    //"总计接受"标题
    QRect totalReceiveTitleRect(totalReceiveRect.x(), recvTitleRect.y(), totalReceiveRect.width(), recvTitleRect.height());


    //"正在发送"的值
    QRect Sent1Rect(recvTitleRect.x(), recvTitleRect.y() + recvTitleRect.height()+10,
                          fmContent.size(Qt::TextSingleLine, m_netSend).width(), fmContent.height());
    QRect Sent2Rect(Sent1Rect.x()+Sent1Rect.width(), Sent1Rect.y() + 8,
                              fmContentUnit.size(Qt::TextSingleLine, m_netSendUnit).width(), fmContent.height());
    //"总计发送"的值
    QRect crect32(totalReceiveRect.x(), Sent1Rect.y(), totalReceiveRect.width(), recv1Rect.height());
    QRect SentTotal1Rect(crect32.x(), crect32.y(),
                          fmContent.size(Qt::TextSingleLine, m_totalSend).width(), fmContent.height());
    QRect SentTotal2Rect(SentTotal1Rect.x()+SentTotal1Rect.width(), SentTotal1Rect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_totalSendUnit).width(), fmContent.height());


    //正在发送标题
    QRect sentTitleRect(contentNetRect.x(), Sent1Rect.y() + Sent1Rect.height(),
                  fmContent.size(Qt::TextSingleLine, recvTitle).width(), fmSubContent.height());
    QRect sentTotalTitleRect(totalReceiveRect.x(), sentTitleRect.y(), totalReceiveRect.width(), recvTitleRect.height());
    QRectF r1Ind(contentNetRect.x() - margin, recvTitleRect.y() + qCeil((recvTitleRect.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);
    QRectF r2Ind(contentNetRect.x() - margin, sentTitleRect.y() + qCeil((sentTitleRect.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);

    //正在接受
//    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(recv1Rect, Qt::AlignLeft | Qt::AlignHCenter,m_netReceive);
    //正在接受的单位
//    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(recv2UnitRect, Qt::AlignLeft | Qt::AlignHCenter,m_netReceiveUnit);

    //总计接受
//    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(Receive1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_netTotalReceive);
    //总计接受的单位
//    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(Receive2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_netTotalReceiveUnit);

    //正在发送
//    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(Sent1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_netSend);
    //正在发送的单位
//    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(Sent2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_netSendUnit);

    //总发送
//    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(SentTotal1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_totalSend);
    //总的单位
//    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(SentTotal2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_totalSendUnit);


    //标题
//    painter.setPen(summaryColor);
    painter.setFont(m_subContentFont);
    painter.drawText(recvTitleRect, Qt::AlignLeft | Qt::AlignVCenter,recvTitle);
    painter.drawText(sentTitleRect, Qt::AlignLeft | Qt::AlignVCenter, sentTitle);
    painter.drawText(totalReceiveTitleRect, Qt::AlignLeft | Qt::AlignVCenter, recvTotalTitle);
    painter.drawText(sentTotalTitleRect, Qt::AlignLeft | Qt::AlignVCenter,sentTotalTitle);

    QPainterPath path1, path2;
    path1.addEllipse(r1Ind);
    path2.addEllipse(r2Ind);

    QColor m_recvIndicatorColor {"#E14300"};
    QColor m_sentIndicatorColor {"#004EEF"};
    painter.fillPath(path1, m_recvIndicatorColor);
    painter.fillPath(path2, m_sentIndicatorColor);

    //走势图
    QPainterPath framePath;
    QRect chartRect(separatorRect1.x(), separatorRect1.y(), contentRect.width()-20, 38);
//    framePath.addRect(chartRect);

    QColor recvColor {"#E14300"};
    QBrush recvBrush(recvColor);
    QColor sentColor {"#004EEF"};
    QBrush sentBrush(sentColor);
    qreal networkCurveWidth = 1.2;

    painter.setRenderHint(QPainter::Antialiasing, true);//反锯齿
    QPainterPath clip;
    clip.addRect(chartRect);
    painter.setClipPath(clip);
    painter.translate(chartRect.x() + 2, chartRect.y() + chartRect.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(recvBrush, networkCurveWidth));
    painter.drawPath(downloadPath);

    painter.translate(0, -5);
    painter.scale(1, -1);
    painter.setPen(QPen(sentBrush, networkCurveWidth));
    painter.drawPath(uploadPath);
}

bool NetWidget::eventFilter(QObject *target, QEvent *event)
{
    if (target == this) {
        if (event->type() == QEvent::Enter) {
            m_isHover = true;
            update();
            return true;
        }
        if (event->type() == QEvent::Leave) {
            m_isHover = false;
            update();
            return true;
        }
    }
    return QWidget::eventFilter(target, event);
}

void NetWidget::changeFont(const QFont &font)
{
    m_sectionFont = font;
    m_sectionFont.setWeight(QFont::DemiBold);
    m_sectionFont.setPointSizeF(m_sectionFont.pointSizeF());

    m_contentFont = font;
    m_contentFont.setWeight(QFont::Normal);
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF()+5);

    m_contentUnitFont = font;
    m_contentUnitFont.setWeight(QFont::Normal);
    m_contentUnitFont.setPointSizeF(m_contentUnitFont.pointSizeF());

    m_subContentFont = font;
    m_subContentFont.setWeight(QFont::ExtraLight);
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF()-2 );
}
