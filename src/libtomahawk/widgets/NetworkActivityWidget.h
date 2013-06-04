/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef NETWORKACTIVITYWIDGET_H
#define NETWORKACTIVITYWIDGET_H

#include "ViewPage.h"

class AnimatedSpinner;
namespace Ui
{
    class NetworkActivityWidget;
}

class NetworkActivityWidget : public QWidget, public Tomahawk::ViewPage
{
    Q_OBJECT
public:
    NetworkActivityWidget(QWidget *parent = 0);
    ~NetworkActivityWidget();
    
    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "Network Activity" ); }
    virtual QString description() const { return QString(); }

    virtual bool jumpToCurrentTrack();

    void fetchData();
signals:
    
public slots:

private:
    void actualFetchData();

    Ui::NetworkActivityWidget* ui;
    Tomahawk::playlistinterface_ptr m_playlistInterface;
    QSharedPointer<AnimatedSpinner> m_spinner;
};

#endif // NETWORKACTIVITYWIDGET_H
