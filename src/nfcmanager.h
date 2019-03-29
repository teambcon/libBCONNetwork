#ifndef NFCMANAGER_H
#define NFCMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#define POLLING_INTERVAL_MS  1000
#define EVENT_TIMEOUT_MS     1000

class NFCWorker : public QThread
{
    Q_OBJECT

public:
    void run() override;

    void setContext( const SCARDCONTEXT & hContext );
    void setReaderName( const char * pcReaderName );

signals:
    void cardInserted();
    void cardRemoved();
    void cardRead( QString sId );

public slots:
    void terminate();

private:
    bool bActive;
    SCARDCONTEXT hContext;
    char *pcReaderName;
    SCARD_READERSTATE *pxReaderState;

    bool readId();
};

class NFCManager : public QObject
{
    Q_OBJECT
public:
    static NFCManager * instance();

    bool nfcManagerInit();

    QString getCurrentId();

signals:
    void cardInserted();
    void cardRemoved();
    void cardRead( const QString & sId );

private slots:
    void on_cardInserted();
    void on_cardRemoved();
    void on_cardRead( const QString & sId );
    void cleanupBeforeQuit();

private:
    SCARDCONTEXT hContext;
    char *pcReaderName;
    NFCWorker *pWorkerThread;
    QString sCurrentId;

    explicit NFCManager( QObject * pParent = nullptr );
    bool getReaderName();
    bool startWorker();
};

#endif // NFCMANAGER_H
