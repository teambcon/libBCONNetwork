#include "datastore.h"
/*--------------------------------------------------------------------------------------------------------------------*/

static DataStore *pInstance = nullptr;
/*--------------------------------------------------------------------------------------------------------------------*/

DataStore::DataStore( QObject * pParent ) : QObject( pParent )
{
    /* Nothing to do. */
}
/*--------------------------------------------------------------------------------------------------------------------*/

DataStore * DataStore::instance()
{
    if ( nullptr == pInstance )
    {
        pInstance = new DataStore();
    }

    return pInstance;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void DataStore::publish( const DataPoint & Data )
{
    if ( nullptr == pInstance )
    {
        return;
    }

    /* Update the data model. */
    pInstance->DataModel.insert( Data.sTag.toLower(), Data );

    /* Emit a signal for anyone interested. */
    emit pInstance->newDataPoint( Data );

    /* Inform all of the subscribers. */
    for ( DataSubscriber * const pSubscriber : pInstance->Subscribers.values( Data.sTag.toLower() ) )
    {
        pSubscriber->handleData( Data );
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

void DataStore::subscribe( const QString & sTag, DataSubscriber * pSubscriber )
{
    if ( nullptr == pInstance )
    {
        pInstance = new DataStore();
    }

    pInstance->Subscribers.insertMulti( sTag.toLower(), pSubscriber );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void DataStore::unsubscribe( const QString & sTag, DataSubscriber * pSubscriber )
{
    pInstance->Subscribers.remove( sTag.toLower(), pSubscriber );
}
/*--------------------------------------------------------------------------------------------------------------------*/

void DataStore::unsubscribeAll( DataSubscriber * pSubscriber )
{
    /* Remove all references to the subscriber. */
    for ( QMultiHash<QString, DataSubscriber *>::const_iterator Iterator = pInstance->Subscribers.begin();
          Iterator != pInstance->Subscribers.end();
          ++Iterator )
    {
        if ( Iterator.value() == pSubscriber )
        {
            pInstance->Subscribers.remove( Iterator.key(), pSubscriber );
        }
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

DataPoint DataStore::getDataPoint( const QString & sTag )
{
    if ( nullptr == pInstance )
    {
        return DataPoint();
    }

    return pInstance->DataModel.value( sTag.toLower(), DataPoint() );
}
/*--------------------------------------------------------------------------------------------------------------------*/
