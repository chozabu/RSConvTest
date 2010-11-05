/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the example classes of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <iostream>

#include <math.h>

#include "edge.h"
#include "node.h"
#include "graphwidget.h"

Node::Node(const std::string& node_string,GraphWidget::NodeType type,GraphWidget::AuthType auth,GraphWidget *graphWidget)
    : graph(graphWidget),_desc_string(node_string),_type(type),_auth(auth)
{
    setFlag(ItemIsMovable);
#if QT_VERSION >= 0x040600
    setFlag(ItemSendsGeometryChanges);
#endif
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
	 mDeterminedBB = false ;
	 mBBWidth = 0 ;

	 _speedx=_speedy=0;
	 _steps=0;
}

void Node::addEdge(Edge *edge)
{
    edgeList << edge;
    edge->adjust();
}

const QList<Edge *>& Node::edges() const
{
    return edgeList;
}

static double interpolate(const double *map,int W,int H,float x,float y) 
{
	if(x>W-2) x=W-2 ;
	if(y>H-2) y=H-2 ;
	if(x<0  ) x=0   ;
	if(y<0  ) y=0   ;

	int i=(int)floor(x) ;
	int j=(int)floor(y) ;
	double di = x-i ;
	double dj = y-j ;

	return (1-di)*( (1-dj)*map[2*(i+W*j)] + dj*map[2*(i+W*(j+1))])
				+di *( (1-dj)*map[2*(i+1+W*j)] + dj*map[2*(i+1+W*(j+1))]) ;
}

void Node::calculateForces(const double *map,int width,int height,int W,int H,float x,float y,float speedf)
{
	if (!scene() || scene()->mouseGrabberItem() == this) 
	{
		newPos = pos();
		return;
	}


	// Sum up all forces pushing this item away
	qreal xforce = 0;
	qreal yforce = 0;

	float dei=0.0f ;
	float dej=0.0f ;

	static float *e = NULL ;
	static const int KS = 5 ;

	if(e == NULL)
	{
		e = new float[(2*KS+1)*(2*KS+1)] ;

		for(int i=-KS;i<=KS;++i)
			for(int j=-KS;j<=KS;++j)
				e[i+KS+(2*KS+1)*(j+KS)] = exp( -(i*i+j*j)/30.0 ) ;	// can be precomputed
	}

	for(int i=-KS;i<=KS;++i)
		for(int j=-KS;j<=KS;++j)
		{
			int X = std::min(W-1,std::max(0,(int)rint(x))) ;
			int Y = std::min(H-1,std::max(0,(int)rint(y))) ;

			float val = map[2*((i+X+W)%W + W*((j+Y+H)%H))] ;

			dei += i * e[i+KS+(2*KS+1)*(j+KS)] * val ;
			dej += j * e[i+KS+(2*KS+1)*(j+KS)] * val ;
		}

	xforce = REPULSION_FACTOR * dei/25.0;
	yforce = REPULSION_FACTOR * dej/25.0;

	// Now subtract all forces pulling items together
	double weight = (n_edges() + 1) ;

	foreach (Edge *edge, edgeList) {
		QPointF pos;
		double w2 ;	// This factor makes the edge length depend on connectivity, so clusters of friends tend to stay in the
						// same location.
						//
		if (edge->sourceNode() == this)
		{
			pos = mapFromItem(edge->destNode(), 0, 0);
			w2 = sqrtf(std::min(n_edges(),edge->destNode()->n_edges())) ;
		}
		else
		{
			pos = mapFromItem(edge->sourceNode(), 0, 0);
			w2 = sqrtf(std::min(n_edges(),edge->sourceNode()->n_edges())) ;
		}

		float dist = sqrtf(pos.x()*pos.x() + pos.y()*pos.y()) ;
		float val = dist - graph->edgeLength() * w2 ;

		xforce += 0.01*pos.x() * val / weight;
		yforce += 0.01*pos.y() * val / weight;
	}

	xforce -= FRICTION_FACTOR * _speedx ;
	yforce -= FRICTION_FACTOR * _speedy ;

	// This term drags nodes away from the sides.
	//
	if(x < 15) xforce += 100.0/(x+0.1) ;
	if(y < 15) yforce += 100.0/(y+0.1) ;
	if(x > width-15) xforce -= 100.0/(width-x+0.1) ;
	if(y > height-15) yforce -= 100.0/(height-y+0.1) ;

	// now time filter:

	_speedx += xforce / MASS_FACTOR;
	_speedy += yforce / MASS_FACTOR;

	if(_speedx > 10) _speedx = 10.0f ;
	if(_speedy > 10) _speedy = 10.0f ;
	if(_speedx <-10) _speedx =-10.0f ;
	if(_speedy <-10) _speedy =-10.0f ;

	QRectF sceneRect = scene()->sceneRect();
	newPos = pos() + QPointF(_speedx, _speedy);
	newPos.setX(qMin(qMax(newPos.x(), sceneRect.left() + 10), sceneRect.right() - 10));
	newPos.setY(qMin(qMax(newPos.y(), sceneRect.top() + 10), sceneRect.bottom() - 10));
}

bool Node::advance()
{
	if(_type == GraphWidget::ELASTIC_NODE_TYPE_OWN)
		return false;

	float f = std::max(fabs(newPos.x() - pos().x()), fabs(newPos.y() - pos().y())) ;

    setPos(newPos);
    return f > 0.5;
}

QRectF Node::boundingRect() const
{
	    qreal adjust = 2;
    /* add in the size of the text */
    qreal realwidth = 40;
    if (mDeterminedBB)
    {
	realwidth = mBBWidth + adjust;
    }
    if (realwidth < 23 + adjust)
    {
    	realwidth = 23 + adjust;
    }

    return QRectF(-10 - adjust, -10 - adjust,
                  realwidth, 23 + adjust);
}

QPainterPath Node::shape() const
{
    QPainterPath path;
    path.addEllipse(-10, -10, 20, 20);
    return path;
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	static QColor type_color[4] = { QColor(Qt::yellow), QColor(Qt::green), QColor(Qt::cyan), QColor(Qt::black) } ;
	static QColor auth_color[3] = { QColor(Qt::darkYellow), QColor(Qt::darkGreen), QColor(Qt::darkBlue) } ;

	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::darkGray);
	painter->drawEllipse(-7, -7, 20, 20);

	QColor col0(type_color[_type]) ;
	QColor col1(auth_color[_auth]) ;

	QRadialGradient gradient(-3, -3, 10);
	if (option->state & QStyle::State_Sunken) 
	{
		gradient.setCenter(3, 3);
		gradient.setFocalPoint(3, 3);
		gradient.setColorAt(1, col0.light(120));
		gradient.setColorAt(0, col1.light(120));
	} 
	else 
	{
		gradient.setColorAt(0, col0);
		gradient.setColorAt(1, col1);
	}
	painter->setBrush(gradient);
	painter->setPen(QPen(Qt::black, 0));
	painter->drawEllipse(-10, -10, 20, 20);
	painter->drawText(-10, 0, QString::fromStdString(_desc_string));

	if (!mDeterminedBB)
	{
		QRect textBox = painter->boundingRect(-10, 0, 400, 20, 0, QString::fromStdString(_desc_string));
		mBBWidth = textBox.width();
		mDeterminedBB = true;
	}
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionHasChanged:
        foreach (Edge *edge, edgeList)
            edge->adjust();
        graph->itemMoved();
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mousePressEvent(event);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}
