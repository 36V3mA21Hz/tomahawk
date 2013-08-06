/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "HatchetAccount.h"
#include <QHostInfo>

#include "HatchetAccountConfig.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "sip/HatchetSip.h"
#include "utils/TomahawkUtils.h"
#include "utils/NetworkAccessManager.h"

#include <QtPlugin>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Tomahawk;
using namespace Accounts;

static QPixmap* s_icon = 0;
HatchetAccount* HatchetAccount::s_instance  = 0;

const QString c_loginServer("https://auth.hatchet.is/v1");
const QString c_accessTokenServer("https://auth.hatchet.is/v1");

HatchetAccountFactory::HatchetAccountFactory()
{
#ifndef ENABLE_HEADLESS
    if ( s_icon == 0 )
        s_icon = new QPixmap( ":/hatchet-account/hatchet-icon-512x512.png" );
#endif
}


HatchetAccountFactory::~HatchetAccountFactory()
{

}


QPixmap
HatchetAccountFactory::icon() const
{
    return *s_icon;
}


Account*
HatchetAccountFactory::createAccount( const QString& pluginId )
{
    return new HatchetAccount( pluginId.isEmpty() ? generateId( factoryId() ) : pluginId );
}


// Hatchet account

HatchetAccount::HatchetAccount( const QString& accountId )
    : Account( accountId )
    , m_publicKey( nullptr )
{
    s_instance = this;

    QFile pemFile( ":/hatchet-account/mandella.pem" );
    pemFile.open( QIODevice::ReadOnly );
    tDebug() << Q_FUNC_INFO << "certs/mandella.pem: " << pemFile.readAll();
    pemFile.close();
    pemFile.open( QIODevice::ReadOnly );
    QCA::ConvertResult conversionResult;
    QCA::PublicKey publicKey = QCA::PublicKey::fromPEM(pemFile.readAll(), &conversionResult);
    if ( QCA::ConvertGood != conversionResult )
    {
        tLog() << Q_FUNC_INFO << "INVALID PUBKEY READ";
        return;
    }
    m_publicKey = new QCA::PublicKey( publicKey );
}


HatchetAccount::~HatchetAccount()
{

}


HatchetAccount*
HatchetAccount::instance()
{
    return s_instance;
}


AccountConfigWidget*
HatchetAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
        m_configWidget = QWeakPointer<HatchetAccountConfig>( new HatchetAccountConfig( this ) );

    return m_configWidget.data();
}


void
HatchetAccount::authenticate()
{
    if ( connectionState() == Connected )
        return;

    if ( !authToken().isEmpty() )
    {
        qDebug() << "Have saved credentials with auth token:" << authToken();
        if ( sipPlugin() )
            sipPlugin()->connectPlugin();
    }
    else if ( !username().isEmpty() )
    {
        // Need to re-prompt for password, since we don't save it!
    }
}


void
HatchetAccount::deauthenticate()
{
    if ( !m_tomahawkSipPlugin.isNull() )
        sipPlugin()->disconnectPlugin();
    emit deauthenticated();
}


void
HatchetAccount::setConnectionState( Account::ConnectionState connectionState )
{
    m_state = connectionState;

    emit connectionStateChanged( connectionState );
}


Account::ConnectionState
HatchetAccount::connectionState() const
{
    return m_state;
}


SipPlugin*
HatchetAccount::sipPlugin()
{
    if ( m_tomahawkSipPlugin.isNull() )
    {
        tLog() << Q_FUNC_INFO;
        m_tomahawkSipPlugin = QWeakPointer< HatchetSipPlugin >( new HatchetSipPlugin( this ) );
        connect( m_tomahawkSipPlugin.data(), SIGNAL( authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service, QString ) ),
                 this, SLOT( authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service, QString ) ) );

        return m_tomahawkSipPlugin.data();
    }
    return m_tomahawkSipPlugin.data();
}


QPixmap
HatchetAccount::icon() const
{
    return *s_icon;
}


bool
HatchetAccount::isAuthenticated() const
{
    return credentials().contains( "authtoken" );
}


QString
HatchetAccount::username() const
{
    return credentials().value( "username" ).toString();
}


QByteArray
HatchetAccount::authToken() const
{
    return credentials().value( "authtoken" ).toByteArray();
}


uint
HatchetAccount::authTokenExpiration() const
{
    bool ok;
    return credentials().value( "expiration" ).toUInt( &ok );
}


void
HatchetAccount::loginWithPassword( const QString& username, const QString& password, const QString &otp )
{
    if ( username.isEmpty() || password.isEmpty() || !m_publicKey )
    {
        tLog() << "No tomahawk account username or pw or public key, not logging in";
        return;
    }

    m_uuid = QUuid::createUuid().toString();
    QCA::SecureArray sa( m_uuid.toLatin1() );
    QCA::SecureArray result = m_publicKey->encrypt( sa, QCA::EME_PKCS1_OAEP );

    QVariantMap params;
    params[ "password" ] = password;
    params[ "username" ] = username;
    if ( !otp.isEmpty() )
        params[ "otp" ] = otp;
    params[ "client" ] = "Tomahawk (" + QHostInfo::localHostName() + ")";
    params[ "nonce" ] = QString( result.toByteArray().toBase64() );

    QJson::Serializer s;
    const QByteArray msgJson = s.serialize( params );

    QNetworkRequest req( QUrl( c_loginServer + "/auth/credentials") );
    req.setHeader( QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8" );
    QNetworkReply* reply = Tomahawk::Utils::nam()->post( req, msgJson );

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onPasswordLoginFinished( QNetworkReply*, const QString& ) ), reply, username );
}


void
HatchetAccount::fetchAccessTokens( const QString& type )
{
    if ( username().isEmpty() || authToken().isEmpty() )
    {
        tLog() << "No authToken, not logging in";
        return;
    }

    if ( authTokenExpiration() < ( QDateTime::currentMSecsSinceEpoch() / 1000 ) )
        tLog() << "Auth token has expired, but may still be valid on the server";

    tLog() << "Fetching access tokens";
    QNetworkRequest req( QUrl( c_accessTokenServer + "/tokens/" + type + "?authtoken=" + authToken() ) );

    QNetworkReply* reply = Tomahawk::Utils::nam()->get( req );

    connect( reply, SIGNAL( finished() ), this, SLOT( onFetchAccessTokensFinished() ) );
}


void
HatchetAccount::onPasswordLoginFinished( QNetworkReply* reply, const QString& username )
{
    Q_ASSERT( reply );
    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error finding status code from auth server";
        emit authError( "An error occurred getting the status code from the server", 0 );
        return;
    }
    const QVariantMap resp = parseReply( reply, ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error getting parsed reply from auth server";
        emit authError( "An error occurred reading the reply from the server", statusCode );
        return;
    }

    if ( !resp.value( "error" ).toString().isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Auth server returned an error";
        emit authError( resp.value( "error" ).toString(), statusCode );
        return;
    }

    const QString nonce = resp.value( "data" ).toMap().value( "nonce" ).toString();
    if ( nonce != m_uuid )
    {
        tLog() << Q_FUNC_INFO << "Auth server nonce value does not match!";
        emit authError( "The nonce value was incorrect. YOUR ACCOUNT MAY BE COMPROMISED.", statusCode );
        return;
    }

    const QByteArray authenticationToken = resp.value( "data" ).toMap().value( "token" ).toByteArray();
    uint expiration = resp.value( "data" ).toMap().value( "expiration" ).toUInt( &ok );

    QVariantHash creds = credentials();
    creds[ "username" ] = username;
    creds[ "authtoken" ] = authenticationToken;
    creds[ "expiration" ] = expiration;
    setCredentials( creds );
    syncConfig();

    if ( !authenticationToken.isEmpty() )
    {
        if ( sipPlugin() )
            sipPlugin()->connectPlugin();
    }
}


void
HatchetAccount::onFetchAccessTokensFinished()
{
    tLog() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );
    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error finding status code from auth server";
        emit authError( "An error occurred getting the status code from the server", 0 );
        return;
    }
    const QVariantMap resp = parseReply( reply, ok );
    if ( !ok || !resp.value( "error" ).toString().isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Auth server returned an error";
        if ( ok )
            emit authError( resp.value( "error" ).toString(), statusCode );
        deauthenticate();
        return;
    }

    QVariantHash creds = credentials();
    QStringList tokenTypesFound;

    tDebug() << Q_FUNC_INFO << "resp: " << resp;

    foreach( QVariant tokenVariant, resp[ "data" ].toMap()[ "tokens" ].toList() )
    {
        QVariantMap tokenMap = tokenVariant.toMap();
        QString tokenTypeName = tokenMap[ "type" ].toString() + "tokens";
        if ( !tokenTypesFound.contains( tokenTypeName ) )
        {
            creds[ tokenTypeName ] = QVariantList();
            tokenTypesFound.append( tokenTypeName );
        }
        creds[ tokenTypeName ] = creds[ tokenTypeName ].toList() << tokenMap;
    }

    tDebug() << Q_FUNC_INFO << "Creds: " << creds;

    setCredentials( creds );
    syncConfig();

    tLog() << Q_FUNC_INFO << "Access tokens fetched successfully";

    emit accessTokensFetched();
}


QString
HatchetAccount::authUrlForService( const Service &service ) const
{
    return m_extraAuthUrls.value( service, QString() );
}


void
HatchetAccount::authUrlDiscovered( Service service, const QString &authUrl )
{
    m_extraAuthUrls[ service ] = authUrl;
}


QVariantMap
HatchetAccount::parseReply( QNetworkReply* reply, bool& okRet ) const
{
    QVariantMap resp;

    reply->deleteLater();

    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( reply->error() != QNetworkReply::NoError && statusCode != 400 && statusCode != 500 )
    {
        tLog() << Q_FUNC_INFO << "Network error in command:" << reply->error() << reply->errorString();
        okRet = false;
        return resp;
    }

    QJson::Parser p;
    resp = p.parse( reply, &ok ).toMap();

    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error parsing JSON from server";
        okRet = false;
        return resp;
    }

    if ( !resp.value( "error", "" ).toString().isEmpty() )
    {
        tLog() << "Error from tomahawk server response, or in parsing from json:" << resp.value( "error" ).toString() << resp;
    }

    tDebug() << Q_FUNC_INFO << "Got keys" << resp.keys();
    tDebug() << Q_FUNC_INFO << "Got values" << resp.values();
    okRet = true;
    return resp;
}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::HatchetAccountFactory )
