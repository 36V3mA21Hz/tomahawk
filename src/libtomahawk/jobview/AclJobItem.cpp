/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AclJobItem.h"

#include "JobStatusModel.h"
#include "utils/TomahawkUtils.h"

#include <QPixmap>
#include <QPainter>
#include <QListView>
#include <QApplication>


#define ROW_HEIGHT 40
#define ICON_PADDING 1
#define PADDING 2
AclJobDelegate::AclJobDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
    , m_parentView( qobject_cast< QListView* >( parent ) )
{
    Q_ASSERT( m_parentView );
}


void
AclJobDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QFontMetrics fm( opt.font );

    opt.state &= ~QStyle::State_MouseOver;
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

//     painter->drawLine( opt.rect.topLeft(), opt.rect.topRight() );

    painter->setRenderHint( QPainter::Antialiasing );
    //QRect iconRect( ICON_PADDING, ICON_PADDING + opt.rect.y(), ROW_HEIGHT - 2*ICON_PADDING, ROW_HEIGHT - 2*ICON_PADDING );
    //QPixmap p = index.data( Qt::DecorationRole ).value< QPixmap >();
    //p = p.scaledToHeight( iconRect.height(), Qt::SmoothTransformation );
    //painter->drawPixmap( iconRect, p );

    // draw right column if there is one
    AclJobItem* item = index.data( JobStatusModel::JobDataRole ).value< AclJobItem* >();
    const QString text = QString( tr( "Allow %1 to\nconnect and stream from you?" ) ).arg( item->username() );
    const int w = fm.width( text );
    const QRect rRect( opt.rect.left() + PADDING, opt.rect.y() - PADDING, opt.rect.width() - 2*PADDING, opt.rect.height() - 2*PADDING );
    painter->drawText( rRect, Qt::AlignCenter, text );

    /*
    rightEdge = rRect.left();

    const int mainW = rightEdge - 3*PADDING - iconRect.right();
    QString mainText = index.data( Qt::DisplayRole ).toString();
    QTextOption to( Qt::AlignLeft | Qt::AlignVCenter );
    if ( !allowMultiLine )
        mainText = fm.elidedText( mainText, Qt::ElideRight, mainW  );
    else
        to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );
    painter->drawText( QRect( iconRect.right() + 2*PADDING, PADDING + opt.rect.y(), mainW, opt.rect.height() - 2*PADDING ), mainText, to );
    */
}

QSize
AclJobDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    return QSize( QStyledItemDelegate::sizeHint ( option, index ).width(), ROW_HEIGHT );
}



AclJobItem::AclJobItem( ACLRegistry::User user, const QString &username )
    : m_delegate( 0 )
    , m_user( user )
    , m_username( username )
{
}


AclJobItem::~AclJobItem()
{
    if ( m_delegate )
        delete m_delegate;
}


void
AclJobItem::createDelegate( QObject* parent )
{
    if ( m_delegate )
        return;

    m_delegate = new AclJobDelegate( parent );
}


void
AclJobItem::done()
{
    emit finished();
}

