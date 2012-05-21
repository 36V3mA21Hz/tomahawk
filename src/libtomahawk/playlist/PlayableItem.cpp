/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "PlayableItem.h"

#include "utils/TomahawkUtils.h"
#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlayableItem::~PlayableItem()
{
    // Don't use qDeleteAll here! The children will remove themselves
    // from the list when they get deleted and the qDeleteAll iterator
    // will fail badly!
    for ( int i = children.count() - 1; i >= 0; i-- )
        delete children.at( i );

    if ( m_parent && index.isValid() )
    {
        m_parent->children.removeAt( index.row() );
    }
}


PlayableItem::PlayableItem( PlayableItem* parent, QAbstractItemModel* model )
{
    m_parent = parent;
    this->model = model;
    childCount = 0;
    m_fetchingMore = false;
    m_isPlaying = false;

    if ( m_parent )
    {
        m_parent->children.append( this );
    }
}


PlayableItem::PlayableItem( const Tomahawk::album_ptr& album, PlayableItem* parent, int row )
    : QObject( parent )
    , m_album( album )
{
    m_parent = parent;
    m_fetchingMore = false;
    m_isPlaying = false;

    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }

    connect( album.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::artist_ptr& artist, PlayableItem* parent, int row )
    : QObject( parent )
    , m_artist( artist )
{
    m_parent = parent;
    m_fetchingMore = false;
    m_isPlaying = false;

    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }

    connect( artist.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::result_ptr& result, PlayableItem* parent, int row )
    : QObject( parent )
    , m_result( result )
{
    m_parent = parent;
    m_fetchingMore = false;
    m_isPlaying = false;

    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }
}


PlayableItem::PlayableItem( const Tomahawk::query_ptr& query, PlayableItem* parent, int row )
    : QObject( parent )
    , m_query( query )
{
    m_parent = parent;
    m_fetchingMore = false;
    m_isPlaying = false;

    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }

    onResultsChanged();

    connect( query.data(), SIGNAL( socialActionsLoaded() ),
                           SIGNAL( dataChanged() ) );

    connect( query.data(), SIGNAL( updated() ),
                           SIGNAL( dataChanged() ) );

    connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                             SLOT( onResultsChanged() ) );

    connect( query.data(), SIGNAL( resultsRemoved( Tomahawk::result_ptr ) ),
                             SLOT( onResultsChanged() ) );

    connect( query.data(), SIGNAL( resultsChanged() ),
                             SLOT( onResultsChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::plentry_ptr& entry, PlayableItem* parent, int row )
    : QObject( parent )
    , m_entry( entry )
{
    m_parent = parent;
    m_fetchingMore = false;
    m_isPlaying = false;
    m_query = entry->query();

    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }

    onResultsChanged();

    connect( m_query.data(), SIGNAL( socialActionsLoaded() ),
                             SIGNAL( dataChanged() ) );

    connect( m_query.data(), SIGNAL( updated() ),
                             SIGNAL( dataChanged() ) );

    connect( m_query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                               SLOT( onResultsChanged() ) );

    connect( m_query.data(), SIGNAL( resultsRemoved( Tomahawk::result_ptr ) ),
                               SLOT( onResultsChanged() ) );

    connect( m_query.data(), SIGNAL( resultsChanged() ),
                               SLOT( onResultsChanged() ) );
}


void
PlayableItem::onResultsChanged()
{
    if ( !m_query->results().isEmpty() )
        m_result = m_query->results().first();
    else
        m_result = result_ptr();

    emit dataChanged();
}


QString
PlayableItem::name() const
{
    if ( !m_artist.isNull() )
    {
        return m_artist->name();
    }
    else if ( !m_album.isNull() )
    {
        return m_album->name();
    }
    else if ( !m_result.isNull() )
    {
        return m_result->track();
    }
    else if ( !m_query.isNull() )
    {
        return m_query->track();
    }

    Q_ASSERT( false );
    return QString();
}


QString
PlayableItem::artistName() const
{
    if ( !m_result.isNull() )
    {
        return m_result->artist()->name();
    }
    else if ( !m_query.isNull() )
    {
        return m_query->artist();
    }

    return QString();
}


QString
PlayableItem::albumName() const
{
    if ( !m_result.isNull() && !m_result->album().isNull() )
    {
        return m_result->album()->name();
    }
    else if ( !m_query.isNull() )
    {
        return m_query->album();
    }

    return QString();
}


const Tomahawk::result_ptr&
PlayableItem::result() const
{
    if ( m_result.isNull() && !m_query.isNull() )
    {
        if ( m_query->numResults() )
            return m_query->results().first();
    }

    return m_result;
}
