#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtMath>
#include <QNetworkReply>

#include "bconnetwork.h"
/*--------------------------------------------------------------------------------------------------------------------*/

BCONNetwork::BCONNetwork( const QString & sServerRootAddress, const bool & bUseNFC )
{
    pModel = DataStore::instance();
    pNFCManager = NFCManager::instance();
    pNetworkManager = new QNetworkAccessManager();
    connect( pNetworkManager, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( handleNetworkReply( QNetworkReply * ) ) );

    /* Set up the NFC manager if requested. */
    if ( ( bUseNFC ) && ( !pNFCManager->nfcManagerInit() ) )
    {
        qDebug() << "LibBCONNetwork::LibBCONNetwork: Failed to initialize the NFC manager!";
    }

    sServerAddress = sServerRootAddress;

    /* Output a warning if the address is invalid. */
    if ( !QUrl( sServerAddress ).isValid() )
    {
        qDebug() << "LibBCONNetwork::LibBCONNetwork: The passed server URL is invalid!";
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

BCONNetwork::~BCONNetwork()
{
    delete pNetworkManager;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::handleNetworkReply( QNetworkReply *pReply )
{
    if ( QNetworkReply::NoError == pReply->error() )
    {
        /* Process the request. */
        handleJSONPayload( pReply->readAll() );
    }
    else
    {
        /* Check the HTTP status code. */
        switch ( pReply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() )
        {
        case 400:
        case 500:
            /* Attempt to parse out the error detail. */
            handleJSONPayload( pReply->readAll() );
            break;

        default:
            qDebug() << "LibBCONNetwork::handleNetworkReply received error" << pReply->error() << "for" << pReply->url();
            break;
        }
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::handleJSONPayload( const QByteArray & Message )
{
    QJsonDocument Document = QJsonDocument::fromJson( QString( Message ).toUtf8() );

    if ( !Document.isNull() )
    {
        QList<DataPoint> Points = JSONUnpackObject( Document.object(),
                                                    Document.object().begin(),
                                                    QString(),
                                                    QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch(),
                                                                                    Qt::UTC ) );

        for ( const DataPoint & Point : Points )
        {
            DataStore::publish( Point );
        }
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

QList<DataPoint> BCONNetwork::JSONUnpackObject( const QJsonObject & ParentObject,
                                                   const QJsonObject::const_iterator & ParentIterator,
                                                   const QString & sParentKey,
                                                   const QDateTime & Timestamp )
{
    QJsonObject::const_iterator Iterator;
    QList<DataPoint> Points;
    QString sFlattenedKey = sParentKey;

    if ( !sParentKey.isEmpty() )
    {
        sFlattenedKey.append( "." );
        Points.append( DataPoint( sFlattenedKey + "^", QVariant(), Timestamp ) );
    }

    /* Process the document. */
    for ( Iterator = ParentIterator; Iterator != ParentObject.end(); ++Iterator )
    {
        Points.append( JSONValueToDataPoint( Iterator.value(), sFlattenedKey + Iterator.key(), Timestamp ) );
    }

    if ( !sParentKey.isEmpty() )
    {
        Points.append( DataPoint( sFlattenedKey + "$", QVariant(), Timestamp ) );
    }

    return Points;
}
/*--------------------------------------------------------------------------------------------------------------------*/

QList<DataPoint> BCONNetwork::JSONValueToDataPoint( const QJsonValue & Value,
                                                       const QString & sKey,
                                                       const QDateTime & Timestamp )
{
    DataPoint Data;
    QList<DataPoint> Points;

    Data.sTag = sKey;
    Data.Timestamp = Timestamp.isValid() ?
                Timestamp : QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch(), Qt::UTC );

    /* Examine the value type to determine what to do next. */
    switch ( Value.type() )
    {
    case QJsonValue::Array:
        Points.append( DataPoint( sKey + ".^", QVariant(), Timestamp ) );
        for ( int i = 0; i < Value.toArray().size(); i++ )
        {
            Points.append( JSONValueToDataPoint( Value.toArray()[ i ], sKey + "." + QString::number( i ), Timestamp ) );
        }
        Points.append( DataPoint( sKey + ".length", QVariant( Value.toArray().size() ), Timestamp ) );
        Points.append( DataPoint( sKey + ".$", QVariant(), Timestamp ) );
        break;

    case QJsonValue::Bool:
        Data.Value = QVariant( Value.toBool() );
        break;

    case QJsonValue::Double:
        /* Check if this is really an integer. */
        if ( qFuzzyCompare( Value.toDouble(), qFloor( Value.toDouble() ) ) )
        {
            Data.Value = QVariant( Value.toInt() );
        }
        else
        {
            Data.Value = QVariant( Value.toDouble() );
        }
        break;

    case QJsonValue::Object:
        /* Go deeper to break down the object. */
        Points.append( JSONUnpackObject( Value.toObject(), Value.toObject().begin(), sKey, Timestamp ) );
        break;

    case QJsonValue::String:
        Data.Value = QVariant( Value.toString() );
        break;

    default:
        /* Undefined type, ignore. */
        break;
    }

    if ( ( !Value.isArray() ) && ( !Value.isObject() ) )
    {
        Points.append( Data );
    }

    return Points;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::sendRequest( const QUrl & Destination,
                                  const QNetworkAccessManager::Operation & eRequestType,
                                  const QJsonObject & Body )
{
    QByteArray Data;
    QNetworkRequest Request;

    /* Ensure the URL is valid. */
    if ( Destination.isValid() )
    {
        /* Set common request properties. */
        Request.setUrl( Destination );
        Request.setRawHeader( "User-Agent", "BCON Network" );
        Request.setRawHeader( "X-Custom-User-Agent", "BCON Network" );

        if ( !Body.isEmpty() )
        {
            /* Convert the body to a byte array. */
            Data = QJsonDocument( Body ).toJson( QJsonDocument::Compact );

            /* Add the additional headers. */
            Request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
        }

        /* Examine the request type. */
        switch ( eRequestType )
        {
        case QNetworkAccessManager::GetOperation:
            pNetworkManager->get( Request );
            break;

        case QNetworkAccessManager::PostOperation:
            pNetworkManager->post( Request, Data );
            break;

        case QNetworkAccessManager::PutOperation:
            pNetworkManager->put( Request, Data );
            break;

        case QNetworkAccessManager::DeleteOperation:
            pNetworkManager->deleteResource( Request );
            break;

        default:
            /* Unsupported request. */
            break;
        }
    }
    else
    {
        /* Invalid URL. */
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::createGame( const QString & sName, const int & iTokenCost )
{
    /* Populate the body with the passed data. */
    QJsonObject Body
    {
        { "name", sName },
        { "tokenCost", iTokenCost }
    };

    sendRequest( QUrl( sServerAddress + "/games/create" ), QNetworkAccessManager::PostOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getGame( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/games/" + sId ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getAllGames()
{
    sendRequest( QUrl( sServerAddress + "/games" ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updateGameName( const QString & sId, const QString sNewName )
{
    QJsonObject Body
    {
        { "name", sNewName }
    };

    sendRequest( QUrl( sServerAddress + "/games/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updateGameTokenCost( const QString & sId, const int & iNewTokenCost )
{
    QJsonObject Body
    {
        { "tokenCost", iNewTokenCost }
    };

    sendRequest( QUrl( sServerAddress + "/games/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updateGameTopPlayer( const QString & sId, const QString & sPlayerId )
{
    QJsonObject Body
    {
        { "topPlayer", sPlayerId }
    };

    sendRequest( QUrl( sServerAddress + "/games/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::deleteGame( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/games/" + sId + "/delete" ), QNetworkAccessManager::DeleteOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::createPlayer( const QString & sId, const QString & sFirstName, const QString & sLastName, const QString & sScreenName )
{
    QJsonObject Body
    {
        { "playerId", sId },
        { "firstName", sFirstName },
        { "lastName", sLastName },
        { "screenName", sScreenName }
    };

    sendRequest( QUrl( sServerAddress + "/players/create" ), QNetworkAccessManager::PostOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getPlayer( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/players/" + sId ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getAllPlayers()
{
    sendRequest( QUrl( sServerAddress + "/players" ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePlayerId( const QString & sId, const QString & sNewId )
{
    QJsonObject Body
    {
        { "playerId", sNewId }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePlayerName( const QString & sId, const QString & sNewFirstName, const QString & sNewLastName )
{
    QJsonObject Body
    {
        { "firstName", sNewFirstName },
        { "lastName", sNewLastName }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePlayerScreenName( const QString & sId, const QString & sNewScreenName )
{
    QJsonObject Body
    {
        { "screenName", sNewScreenName }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePlayerTokens( const QString & sId, const int & iNewTokens )
{
    QJsonObject Body
    {
        { "tokens", iNewTokens }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePlayerTickets( const QString & sId, const int & iNewTickets )
{
    QJsonObject Body
    {
        { "tickets", iNewTickets }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::publishPlayerStats( const QString & sId, const QString & sGameId, const int & iTicketsEarned, const int & iHighScore )
{
    QJsonObject Body
    {
        { "gameId", sGameId },
        { "ticketsEarned", iTicketsEarned },
        { "highScore", iHighScore }
    };

    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/publishstats" ), QNetworkAccessManager::PostOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::deletePlayer( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/players/" + sId + "/delete" ), QNetworkAccessManager::DeleteOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::createPrize( const QString & sName, const int & iTicketCost, const int & iAvailableQuantity )
{
    QJsonObject Body
    {
        { "name", sName },
        { "ticketCost", iTicketCost },
        { "availableQuantity", iAvailableQuantity }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/create" ), QNetworkAccessManager::PostOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getPrize( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/prizes/" + sId ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::getAllPrizes()
{
    sendRequest( QUrl( sServerAddress + "/prizes" ), QNetworkAccessManager::GetOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePrizeName( const QString & sId, const QString & sNewName )
{
    QJsonObject Body
    {
        { "name", sNewName }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePrizeDescription( const QString & sId, const QString & sNewDescription )
{
    QJsonObject Body
    {
        { "description", sNewDescription }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePrizeTicketCost( const QString & sId, const int & iNewTicketCost )
{
    QJsonObject Body
    {
        { "ticketCost", iNewTicketCost }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::updatePrizeAvailableQuantity( const QString & sId, const int & iNewAvailableQuantity )
{
    QJsonObject Body
    {
        { "availableQuantity", iNewAvailableQuantity }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/" + sId + "/update" ), QNetworkAccessManager::PutOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::redeemPrize( const QString & sPrizeId, const QString & sPlayerId )
{
    QJsonObject Body
    {
        { "playerId", sPlayerId }
    };

    sendRequest( QUrl( sServerAddress + "/prizes/" + sPrizeId + "/redeem" ), QNetworkAccessManager::PostOperation, Body );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void BCONNetwork::deletePrize( const QString & sId )
{
    sendRequest( QUrl( sServerAddress + "/prizes/" + sId + "/delete" ), QNetworkAccessManager::DeleteOperation );
}
/*--------------------------------------------------------------------------------------------------------------------*/

