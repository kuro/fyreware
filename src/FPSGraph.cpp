/*
 * Copyright 2010 Blanton Black
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file FPSGraph.cpp
 * @brief FPSGraph implementation
 */

#include "defs.h"

#include "FPSGraph.moc"

#include <QLinkedList>
#include <QGLContext>

struct FPSGraph::Private
{
    QSizeF size;
    int fpsMax;
    int sampleCount;
    qreal smoothedFps;
    QLinkedList<qreal> samples;
    qreal minSample;
    qreal maxSample;

    qreal backgroundColor[4];
    qreal meanColor[4];
    qreal heavyBoundsColor[4];
    qreal lightBoundsColor[4];
    qreal pointColor[4];


    Private (const QSizeF& size, qreal fpsMax, int sampleCount, FPSGraph* q) :
        size(size),
        fpsMax(fpsMax),
        sampleCount(sampleCount),
        minSample(0.0),
        maxSample(0.0)
    {
        Q_UNUSED(q);

        setColor(backgroundColor , 0   , 0   , 0   , 0.4);
        setColor(meanColor       , 0x1b, 0xec, 0xf7, 0.8);
        setColor(heavyBoundsColor, 0x52, 0x18, 0xfa, 1.0);
        setColor(lightBoundsColor, 0   , 0   , 0   , 0  );
        setColor(pointColor      , 0xff, 0x29, 0xa2, 1.0);
    }

    void setColor (qreal* out, qreal r, qreal g, qreal b, qreal a = 1.0)
    {
        out[0] = r;
        out[1] = g;
        out[2] = b;
        out[3] = a;
    }
    void setColor (qreal* out, int r, int g, int b, qreal a = 1.0)
    {
        out[0] = qreal(r)/255.0;
        out[1] = qreal(g)/255.0;
        out[2] = qreal(b)/255.0;
        out[3] = a;
    }
};

FPSGraph::FPSGraph (const QSizeF& size,
                    qreal fpsMax,
                    int sampleCount,
                    QObject* parent) :
    QObject(parent),
    d(new Private(size, fpsMax, sampleCount, this))
{
}

FPSGraph::~FPSGraph ()
{
}

void FPSGraph::addSample (qreal realDt, qreal smoothedDt)
{
    d->smoothedFps = 1.0 / smoothedDt;
    d->samples.prepend(1.0/realDt);
    if (d->samples.size() == d->sampleCount) {
        d->samples.removeLast();
    }

    /// @todo might find something more efficient
    qreal minSample, maxSample;
    minSample = maxSample = d->samples.first();
    qreal sample;
    QLinkedListIterator<qreal> it (d->samples);
    while (it.hasNext()) {
        sample = it.next();
        minSample = qMin(minSample, sample);
        maxSample = qMax(maxSample, sample);
    }
    expMovAvg(d->minSample, minSample, d->sampleCount * 0.66);
    expMovAvg(d->maxSample, maxSample, d->sampleCount * 0.66);
}

void FPSGraph::draw ()
{
    glDepthMask(GL_FALSE);
    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewport[0], viewport[2], viewport[1], viewport[3], -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslated(viewport[2] - d->size.width(),
                 viewport[3] - d->size.height(),
                 0.0);

    glScaled(d->size.width() / d->fpsMax,
             d->size.height() / d->sampleCount,
             1.0);

    // background
    glColor4dv(d->backgroundColor);
    glBegin(GL_QUADS);
    glVertex2d(      0.0,            0.0);
    glVertex2d(d->fpsMax,            0.0);
    glVertex2d(d->fpsMax, d->sampleCount);
    glVertex2d(      0.0, d->sampleCount);
    glEnd();

    // draw bounds
    glBegin(GL_QUADS);
    // left
    glColor4dv(d->heavyBoundsColor);
    glVertex2d(d->minSample, d->sampleCount);
    glVertex2d(d->minSample, 0.0);
    glColor4dv(d->lightBoundsColor);
    glVertex2d(d->smoothedFps, 0.0);
    glVertex2d(d->smoothedFps, d->sampleCount);
    // right
    glVertex2d(d->smoothedFps, d->sampleCount);
    glVertex2d(d->smoothedFps, 0.0);
    glColor4dv(d->heavyBoundsColor);
    glVertex2d(d->maxSample, 0.0);
    glVertex2d(d->maxSample, d->sampleCount);
    glEnd();

    // draw markers
    glBegin(GL_LINES);
    qreal gray = 1.0;
    qreal alpha;
    // 15..120 (with light between marks)
    for (qreal i = 15.0; i <= 120.0; i += 15.0) {
        alpha = 1.0 - qAbs(60.0 - i) / 60.0;
        //glColor4dv(i == 60.0 ? d->heavyMarkerColor : d->mediumMarkerColor);
        glColor4d(gray, gray, gray, alpha);
        glVertex2d(i,            0.0);
        glVertex2d(i, d->sampleCount);
        //glColor4dv(d->lightMarkerColor);
        glColor4d(gray, gray, gray, alpha * 0.1);
        glVertex2d(i-7.5,            0.0);
        glVertex2d(i-7.5, d->sampleCount);
    }
    glEnd();

    // draw points
    qreal sample;
    int y = 0;
    glColor4dv(d->pointColor);
    glBegin(GL_POINTS);
    QLinkedListIterator<qreal> it (d->samples);
    while (it.hasNext()) {
        sample = it.next();
        glVertex2d(sample, y);
        y++;
    }
    glEnd();

    // draw smoothed line
    glLineWidth(2.0);
    glColor4dv(d->meanColor);
    glBegin(GL_LINES);
    glVertex2d(d->smoothedFps, 0.0);
    glVertex2d(d->smoothedFps, d->sampleCount);
    glEnd();
    glLineWidth(1.0);

    glPopAttrib();
    glDepthMask(GL_TRUE);
}
