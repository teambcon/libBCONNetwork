#include <QCoreApplication>
#include <QDebug>

#include "datastore.h"
#include "nfcmanager.h"
/*--------------------------------------------------------------------------------------------------------------------*/

static NFCManager *pInstance = nullptr;
/*--------------------------------------------------------------------------------------------------------------------*/

NFCManager::NFCManager( QObject * pParent ) : QObject( pParent )
{
    hContext = 0;
    pcReaderName = nullptr;
    pWorkerThread = nullptr;
    sCurrentId = "";

    /* Wire up the signal to catch exit events. */
    connect( QCoreApplication::instance(), SIGNAL( aboutToQuit() ), this, SLOT( cleanupBeforeQuit() ) );
}
/*--------------------------------------------------------------------------------------------------------------------*/

NFCManager * NFCManager::instance()
{
    if ( nullptr == pInstance )
    {
        pInstance = new NFCManager();
    }

    return pInstance;
}
/*--------------------------------------------------------------------------------------------------------------------*/

QString NFCManager::getCurrentId()
{
    return sCurrentId;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCManager::readCard()
{
    if ( nullptr != pWorkerThread )
    {
//        pWorkerThread->setNextOperation( NFCWorker::eOperation_Read );
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool NFCManager::nfcManagerInit()
{
    bool bReturn = false;

    /* Create the card context. */
    if ( SCARD_S_SUCCESS == SCardEstablishContext( SCARD_SCOPE_SYSTEM, nullptr, nullptr, &hContext ) )
    {
        /* Attempt to get the reader name. */
        if ( getReaderName() )
        {
            /* Start the worker thread. */
            bReturn = startWorker();
        }
        else
        {
            /* Failed to find the reader. */
            bReturn = false;
        }
    }
    else
    {
        /* Failed to create the context. */
        bReturn = false;
    }

    return bReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool NFCManager::getReaderName()
{
    unsigned int uiLength;
    bool bReturn = false;

    /* If necessary, free previously-allocated memory. */
    if ( nullptr != pcReaderName )
    {
        free( pcReaderName );
        pcReaderName = nullptr;
    }

    /* Make the first request for the reader list with a null buffer to get the number of bytes to allocate. */
    if ( SCARD_S_SUCCESS == SCardListReaders( hContext, nullptr, nullptr, &uiLength ) )
    {
        /* Allocate space for the string with the returned buffer length. */
        pcReaderName = static_cast<char *>( malloc( uiLength * sizeof( char ) ) );

        /* Ensure the allocation was successful. */
        if ( nullptr != pcReaderName )
        {
            /* Now pass in the buffer to get the name. */
            *pcReaderName = '\0';
            if ( SCARD_S_SUCCESS == SCardListReaders( hContext, nullptr, pcReaderName, &uiLength ) )
            {
                qDebug() << "Detected reader:" << pcReaderName;
                bReturn = true;
            }
            else
            {
                /* Failed to get name. */
                qDebug() << "NFCManager::bGetReader failed to get the reader name.";
                bReturn = false;
            }
        }
        else
        {
            /* Out of memory. */
            qDebug() << "NFCManager::bGetReader ran out of memory!";
            bReturn = false;
        }
    }
    else
    {
        /* No readers detected. */
        qDebug() << "NFCManager::bGetReader failed to detect reader.";
        bReturn = false;
    }

    return bReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool NFCManager::startWorker()
{
    /* Kill any existing threads. */
    if ( nullptr != pWorkerThread )
    {
        pWorkerThread->terminate();
        pWorkerThread->wait();
        pWorkerThread = nullptr;
    }

    /* Set up the thread to offload polling for events. */
    pWorkerThread = new NFCWorker();
    pWorkerThread->setContext( hContext );
    pWorkerThread->setReaderName( pcReaderName );
    connect( pWorkerThread, SIGNAL( finished() ), pWorkerThread, SLOT( deleteLater() ) );

    /* Wire up relevant signals. */
    connect( pWorkerThread, SIGNAL( cardInserted() ), this, SLOT( on_cardInserted() ) );
    connect( pWorkerThread, SIGNAL( cardRemoved() ), this, SLOT( on_cardRemoved() ) );
    connect( pWorkerThread, SIGNAL( cardRead( const QString & ) ), this, SLOT( on_cardRead( const QString & ) ) );

    /* Start watching for events. */
    pWorkerThread->start();

    return pWorkerThread->isRunning();
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCManager::on_cardInserted()
{
    /* Pass the signal out. */
    emit cardInserted();
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCManager::on_cardRemoved()
{
    emit cardRemoved();
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCManager::on_cardRead( const QString & sId )
{
    /* Publish a data point with the card UID. */
    DataStore::publish( DataPoint( "Card.UID", QVariant( sId ) ) );

    emit cardRead( sId );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCManager::cleanupBeforeQuit()
{
    pWorkerThread->terminate();
    pWorkerThread->wait();
    ( void )SCardReleaseContext( hContext );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCWorker::run()
{
    unsigned int uiStatusReturnCode = SCARD_S_SUCCESS;
    bActive = true;

    /* Allocate the state struct. */
    pxReaderState = static_cast<SCARD_READERSTATE *>( calloc( 1, sizeof( *pxReaderState ) ) );
    if ( nullptr != pxReaderState )
    {
        /* Set the initial state to unknown. */
        pxReaderState->szReader = pcReaderName;
        pxReaderState->dwCurrentState = SCARD_STATE_UNAWARE;
    }
    else
    {
        /* Out of memory. */
        qDebug() << "NFCWorker::run ran out of memory!";
        exit( -1 );
    }

    /* Wait for events. */
    while ( bActive )
    {
        /* Nothing requested, just wait. */
        uiStatusReturnCode = static_cast<unsigned int>( SCardGetStatusChange( hContext, EVENT_TIMEOUT_MS,
                                                                              pxReaderState, 1 ) );

        /* Examine the return code. */
        if ( SCARD_S_SUCCESS == uiStatusReturnCode )
        {
            /* Ensure there is actually a change to process. */
            if ( ( pxReaderState->dwCurrentState == pxReaderState->dwEventState )
                 || ( !( pxReaderState->dwEventState & SCARD_STATE_CHANGED ) ) )
            {
                return;
            }

            /* Update the current state to be the new one. */
            pxReaderState->dwCurrentState = pxReaderState->dwEventState;

            if ( pxReaderState->dwEventState & SCARD_STATE_EMPTY )
            {
                emit cardRemoved();
            }

            if ( pxReaderState->dwEventState & SCARD_STATE_PRESENT )
            {
                emit cardInserted();

                if ( !readId() )
                {
                    qDebug() << "NFCWorker::run failed to read ID from card!";
                }
            }
        }
        else if ( SCARD_E_TIMEOUT != uiStatusReturnCode )
        {
            /* Unexpected return code, stop. */
            qDebug() << "NFCWorker::run got unexpected return code:" << uiStatusReturnCode;
        }
        else
        {
            /* Timeout, wait for another potential event. */
        }
    }

    /* Clean up. */
    free( pxReaderState );
    pxReaderState = nullptr;
    free( pcReaderName );
    pcReaderName = nullptr;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCWorker::setContext( const SCARDCONTEXT & hContext )
{
    this->hContext = hContext;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCWorker::setReaderName( const char * pcReaderName )
{
    if ( nullptr != this->pcReaderName )
    {
        free( this->pcReaderName );
        this->pcReaderName = nullptr;
    }

    this->pcReaderName = static_cast<char *>( malloc( strlen( pcReaderName ) * sizeof( char ) ) );

    if ( nullptr != pcReaderName )
    {
        strcpy( this->pcReaderName, pcReaderName );
    }
    else
    {
        qDebug() << "NFCWorker::setReaderName ran out of memory!";
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

void NFCWorker::terminate()
{
    /* Signal to the loop that it should break. */
    bActive = false;
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool NFCWorker::readId()
{
    unsigned int uiActiveProtocol = 0;
    SCARDHANDLE hCard;
    unsigned char Command[] = { 0xff, 0xCA, 0x00, 0x00, 0x00 };
    unsigned char RxBuffer[ 32 ];
    unsigned int uiRxLength = 32;
    SCARD_IO_REQUEST ioRequest;
    unsigned long ulRawId = 0;
    bool bReturn = false;

    /* Connect to the card. */
    if ( SCARD_S_SUCCESS == SCardConnect( hContext, pcReaderName,
                                          SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                                          &hCard, &uiActiveProtocol ) )
    {
        /* Set up the IO request. */
        ioRequest.dwProtocol = uiActiveProtocol;
        ioRequest.cbPciLength = sizeof(ioRequest);

        /* Send the command to the reader. */
        if ( SCARD_S_SUCCESS == SCardTransmit( hCard, &ioRequest, Command, 5, nullptr, RxBuffer, &uiRxLength ) )
        {
            /* Ensure the response is valid. */
            if ( 2 <= uiRxLength )
            {
                /* Check for an error. */
                if ( ( RxBuffer[ uiRxLength - 2 ] != 0x90 ) || ( RxBuffer[ uiRxLength - 1 ] != 0x00 ) )
                {
                    /* Reported error. */
                    bReturn = false;
                }
                else if ( 2 < uiRxLength )
                {
                    /* Extract the UID from the buffer. */
                    for ( unsigned int i = 0; i != ( uiRxLength - 2 ); i++ )
                    {
                        ulRawId <<= 8;
                        ulRawId |= static_cast<unsigned long>( RxBuffer[ i ] );
                    }

                    /* Announce the UID as a string. */
                    emit cardRead( QString::number( ulRawId, 16 ) );
                    bReturn = true;
                }
            }
            else
            {
                /* Bad response. */
                bReturn = false;
            }
        }
        else
        {
            /* Failed to send the command. */
            bReturn = false;
        }

        ( void )SCardDisconnect( hCard, SCARD_LEAVE_CARD );
    }
    else
    {
        /* Failed to connect. */
        bReturn = false;
    }

    return bReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/
