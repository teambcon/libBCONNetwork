#ifndef LIBBCONNETWORK_H
#define LIBBCONNETWORK_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>

#include "datastore.h"
#include "nfcmanager.h"

class BCONNetwork : public QObject
{
    Q_OBJECT

public:
    BCONNetwork( const QString & sServerRootAddress = "http://localhost:3000", const bool & bUseNFC = true );
    ~BCONNetwork();

public slots:
    /* Game backend requests. */
    void createGame( const QString & sName, const int & iTokenCost );
    void getGame( const QString & sId );
    void getAllGames();
    void updateGameName( const QString & sId, const QString sNewName );
    void updateGameTokenCost( const QString & sId, const int & iNewTokenCost );
    void updateGameTopPlayer( const QString & sId, const QString & sPlayerId );
    void deleteGame( const QString & sId );

    /* Player backend requests. */
    void createPlayer( const QString & sId, const QString & sFirstName, const QString & sLastName, const QString & sScreenName );
    void getPlayer( const QString & sId );
    void getAllPlayers();
    void updatePlayerId( const QString & sId, const QString & sNewId );
    void updatePlayerName( const QString & sId, const QString & sNewFirstName, const QString & sNewLastName );
    void updatePlayerScreenName( const QString & sId, const QString & sNewScreenName );
    void updatePlayerTokens( const QString & sId, const int & iNewTokens );
    void updatePlayerTickets( const QString & sId, const int & iNewTickets );
    void publishPlayerStats( const QString & sId, const QString & sGameId, const int & iTicketsEarned, const int & iHighScore );
    void deletePlayer( const QString & sId );

    /* Prize backend requests. */
    void createPrize( const QString & sName, const int & iTicketCost, const int & iAvailableQuantity );
    void getPrize( const QString & sId );
    void getAllPrizes();
    void updatePrizeName( const QString & sId, const QString & sNewName );
    void updatePrizeDescription( const QString & sId, const QString & sNewDescription );
    void updatePrizeTicketCost( const QString & sId, const int & iNewTicketCost );
    void updatePrizeAvailableQuantity( const QString & sId, const int & iNewAvailableQuantity );
    void redeemPrize( const QString & sPrizeId, const QString & sPlayerId );
    void deletePrize( const QString & sId );

private slots:
    void handleNetworkReply( QNetworkReply * pReply );

private:
    DataStore *pModel;
    NFCManager *pNFCManager;
    QNetworkAccessManager *pNetworkManager;
    QString sServerAddress;

    static void handleJSONPayload( const QByteArray & Payload );
    static QList<DataPoint> JSONUnpackObject( const QJsonObject & ParentObject, const QJsonObject::const_iterator & ParentIterator, const QString & sParentKey, const QDateTime & Timestamp );
    static QList<DataPoint> JSONValueToDataPoint( const QJsonValue & Value, const QString & sKey, const QDateTime & Timestamp );

    void sendRequest( const QUrl & Destination, const QNetworkAccessManager::Operation & eRequestType, const QJsonObject & Body = QJsonObject() );
};

#endif // LIBBCONNETWORK_H
