#ifndef DATACACHE_H
#define DATACACHE_H

#include <QDateTime>
#include <QObject>
#include <QVariant>

class DataPoint
{
public:
    QString sTag;
    QVariant Value;
    QDateTime Timestamp;

    DataPoint( const QString & sTag = "",
               const QVariant & Value = QVariant(),
               const QDateTime & Timestamp = QDateTime::currentDateTimeUtc() )
    {
        this->sTag = sTag;
        this->Value = Value;
        this->Timestamp = Timestamp;
    }

    DataPoint( const DataPoint & Original )
    {
        this->sTag = Original.sTag;
        this->Value = Original.Value;
        this->Timestamp = Original.Timestamp;
    }

    bool isValid()
    {
        return ( !sTag.isEmpty() ) && ( Value.isValid() ) && ( Timestamp.isValid() );
    }
};

class DataSubscriber
{
public:
    virtual ~DataSubscriber() = default;
    virtual void handleData( const DataPoint & Data ) = 0;
};

class DataStore : public QObject
{
    Q_OBJECT

public:
    static DataStore * instance();

    static void publish( const DataPoint & Data );
    static void subscribe( const QString & sTag, DataSubscriber * pSubscriber );
    static void unsubscribe( const QString & sTag, DataSubscriber * pSubscriber );
    static void unsubscribeAll( DataSubscriber * pSubscriber );

    static DataPoint getDataPoint( const QString & sTag );

signals:
    void newDataPoint( const DataPoint & Data );

private:
    explicit DataStore( QObject * pParent = nullptr );

    QMultiHash<QString, DataSubscriber *> Subscribers;
    QHash<QString, DataPoint> DataModel;
};

#endif // DATACACHE_H
