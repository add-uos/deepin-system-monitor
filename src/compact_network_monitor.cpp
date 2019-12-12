/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <DStyle>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtMath>

#include "compact_network_monitor.h"
#include "constant.h"
#include "process/system_monitor.h"
#include "smooth_curve_generator.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactNetworkMonitor::CompactNetworkMonitor(QWidget *parent)
    : QWidget(parent)
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);
    m_margin = margin;

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth - margin * 2);
    setFixedHeight(150);

    pointsNumber = int(statusBarMaxWidth / 5.4);

    downloadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        downloadSpeeds->append(0);
    }

    uploadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        uploadSpeeds->append(0);
    }

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactNetworkMonitor::changeTheme);
    changeTheme(dAppHelper->themeType());

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::networkStatInfoUpdated, this,
                &CompactNetworkMonitor::updateStatus);
    }

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactNetworkMonitor::changeFont);
}

CompactNetworkMonitor::~CompactNetworkMonitor()
{
    delete downloadSpeeds;
    delete uploadSpeeds;
}

void CompactNetworkMonitor::updateStatus(long tRecvBytes, long tSentBytes, float tRecvKbs,
                                         float tSentKbs)
{
    totalRecvBytes = tRecvBytes;
    totalSentBytes = tSentBytes;
    totalRecvKbs = tRecvKbs;
    totalSentKbs = tSentKbs;

    // Init download path.
    downloadSpeeds->append(totalRecvKbs);

    if (downloadSpeeds->size() > pointsNumber) {
        downloadSpeeds->pop_front();
    }

    double downloadMaxHeight = 0;
    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadSpeeds->at(i) > downloadMaxHeight) {
            downloadMaxHeight = downloadSpeeds->at(i);
        }
    }

    // Init upload path.
    uploadSpeeds->append(totalSentKbs);

    if (uploadSpeeds->size() > pointsNumber) {
        uploadSpeeds->pop_front();
    }

    QList<QPointF> uploadPoints;

    double uploadMaxHeight = 0;
    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadSpeeds->at(i) > uploadMaxHeight) {
            uploadMaxHeight = uploadSpeeds->at(i);
        }
    }

    qreal maxHeight = std::max(uploadMaxHeight, downloadMaxHeight);
    // top/bottom margin
    int modDownloadRenderMaxHeight = downloadRenderMaxHeight - 2;
    int modUploadRenderMaxHeight = uploadRenderMaxHeight - 2;

    QList<QPointF> downloadPoints;
    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadMaxHeight < modDownloadRenderMaxHeight) {
            downloadPoints.append(QPointF(i * 5, downloadSpeeds->at(i)));
        } else {
            qreal scale = downloadSpeeds->at(i) * modDownloadRenderMaxHeight / maxHeight;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            downloadPoints.append(QPointF(i * 5, scale));
        }
    }

    downloadPath = SmoothCurveGenerator::generateSmoothCurve(downloadPoints);

    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadMaxHeight < modUploadRenderMaxHeight) {
            uploadPoints.append(QPointF(i * 5, uploadSpeeds->at(i)));
        } else {
            qreal scale = uploadSpeeds->at(i) * modUploadRenderMaxHeight / maxHeight;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            uploadPoints.append(QPointF(i * 5, scale));
        }
    }

    uploadPath = SmoothCurveGenerator::generateSmoothCurve(uploadPoints);

    update();
}

void CompactNetworkMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int sectionSize = 6;
    int padleft = sectionSize * 2 + 2;
    QRect contentRect(padleft, 0, rect().x() + rect().width() - padleft, 1);

    // Draw network summary.
    QString recvTitle = DApplication::translate("Process.Graph.View", "Download");
    QString recvContent = formatBandwidth(totalRecvKbs);
    QString recvTotalTitle = DApplication::translate("Process.Graph.View", "Total Received");
    QString recvTotalContent = formatByteCount(totalRecvBytes);

    QString sentTitle = DApplication::translate("Process.Graph.View", "Upload");
    QString sentContent = formatBandwidth(totalSentKbs);
    QString sentTotalTitle = DApplication::translate("Process.Graph.View", "Total Sent");
    QString sentTotalContent = formatByteCount(totalSentBytes);

    QFontMetrics fmContent(m_contentFont);
    QFontMetrics fmSubContent(m_subContentFont);
    int cw1 = std::max(fmContent.size(Qt::TextSingleLine, recvTitle).width(),
                       fmContent.size(Qt::TextSingleLine, sentTitle).width());
    int cw2 = qCeil(contentRect.width() / 2.) - cw1;
    int cw3 = std::max(fmContent.size(Qt::TextSingleLine, recvTotalTitle).width(),
                       fmContent.size(Qt::TextSingleLine, recvTotalTitle).width());
    int cw4 = contentRect.width() - cw1 - cw2 - cw3;
    QRect crect11(contentRect.x(), contentRect.y(), cw1, fmContent.height());
    QRect crect12(crect11.x() + cw1 + 4, crect11.y(), cw2, crect11.height());
    QRect crect13(crect12.x() + cw2, crect11.y(), cw3, crect11.height());
    QRect crect14(crect13.x() + cw3 + 4, crect11.y(), cw4, crect11.height());
    QRect crect21(crect11.x(), crect11.y() + crect11.height(), cw1, crect11.height());
    QRect crect22(crect12.x(), crect21.y(), cw2, crect21.height());
    QRect crect23(crect13.x(), crect21.y(), cw3, crect21.height());
    QRect crect24(crect14.x(), crect21.y(), cw4, crect21.height());
    QRectF r1Ind(0, crect11.y() + qCeil((crect11.height() - sectionSize) / 2.), sectionSize,
                 sectionSize);
    QRectF r2Ind(0, crect21.y() + qCeil((crect21.height() - sectionSize) / 2.), sectionSize,
                 sectionSize);

    painter.setPen(textColor);
    painter.setFont(m_contentFont);
    painter.drawText(crect11, Qt::AlignLeft | Qt::AlignVCenter, recvTitle);
    painter.drawText(crect21, Qt::AlignLeft | Qt::AlignVCenter, sentTitle);
    painter.drawText(crect13, Qt::AlignLeft | Qt::AlignVCenter, recvTotalTitle);
    painter.drawText(crect23, Qt::AlignLeft | Qt::AlignVCenter, sentTotalTitle);

    painter.setPen(summaryColor);
    painter.setFont(m_subContentFont);
    painter.drawText(crect12, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(recvContent, Qt::ElideRight, cw2));
    painter.drawText(crect22, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(sentContent, Qt::ElideRight, cw2));
    painter.drawText(crect14, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(recvTotalContent, Qt::ElideRight, cw4));
    painter.drawText(crect24, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(sentTotalContent, Qt::ElideRight, cw4));

    QPainterPath path1, path2;
    path1.addEllipse(r1Ind);
    path2.addEllipse(r2Ind);

    painter.fillPath(path1, m_recvIndicatorColor);
    painter.fillPath(path2, m_sentIndicatorColor);

    // Draw background grid.
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen framePen;
    int penSize = 1;
    framePen.setColor(m_frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridX = rect().x() + penSize + 3;
    int gridY = rect().y() + crect22.y() + crect22.height() + m_margin;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = downloadRenderMaxHeight + uploadRenderMaxHeight + 4 * penSize;

    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setColor(m_frameColor);
    gridPen.setWidth(penSize);
    gridPen.setDashPattern(dashes);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX + gridSize + penSize < gridX + gridWidth) {
        gridLineX += gridSize + penSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY + gridSize + penSize < gridY + gridHeight) {
        gridLineY += gridSize + penSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    // for now, termporaryly comment out draw width change
    //    qreal devicePixelRatio = qApp->devicePixelRatio();
    qreal networkCurveWidth = 1.2;
    //    if (devicePixelRatio > 1) {
    //        networkCurveWidth = 2;
    //    }

    QBrush recvBrush(recvColor), sentBrush(sentColor);

    painter.translate(gridFrame.x() + 2, gridFrame.y() + gridFrame.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(recvBrush, networkCurveWidth));
    painter.drawPath(downloadPath);

    painter.translate(0, uploadWaveformsRenderOffsetY);
    painter.scale(1, -1);
    painter.setPen(QPen(sentBrush, networkCurveWidth));
    painter.drawPath(uploadPath);
}

void CompactNetworkMonitor::changeTheme(DGuiApplicationHelper::ColorType themeType)
{
    Q_UNUSED(themeType);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    textColor = palette.color(DPalette::TextTitle);
#else
    textColor = palette.color(DPalette::Text);
#endif

    summaryColor = palette.color(DPalette::TextTips);
    m_frameColor = palette.color(DPalette::FrameBorder);
}

void CompactNetworkMonitor::changeFont(const QFont &font)
{
    m_contentFont = font;
    m_contentFont.setWeight(QFont::Medium);
    m_contentFont.setPointSize(m_contentFont.pointSize() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSize(m_subContentFont.pointSize() - 1);
}
