# libBCONNetwork

Library containing common networking functionality for the BCON RFID Arcade project.

## Contents

### /

- Qt project file for the library.

### build

- Build scrips to automate platform-dependent creation of the library.

### lib

- Output directory for the final build products. Subdirectories will be one of _Linux_, _macOS_, and/or _Windows_ depending on the executed build script. Build products are _not_ checked into the repository.

### src

- All header and source files for the library.

## Prerequisites

- [Qt Open Source](https://www.qt.io/download) (download the installer for your platform)
	- Version 5.12.0 or later preferred

## Backend API

The main header file `BCONNetwork.h` contains all supported backend data requests. Depending on the request, one or many parameters may be required (i.e. updating some piece of information). Translation of each of these requests into compliant HTTP requests is handled by the library. JSON replies from the server are fed directly into the DataStore.

The constructor for the main library object has the following prototype:

```c++
BCONNetwork( const QString & sServerRootAddress = "http://localhost:3000", const bool & bUseNFC = true );
```

Thus, the two optional parameters specify the server address and whether or not NFC should be used. There should be no final slash at the end of the server address. The default values suggest a server running locally on a debug port along with active use of the NFC functionality.

## DataStore

The DataStore, as its name implies, is the centralized data model for the network. It offers a simple publish-subscribe mechanism for advertising data throughout the system while remaining lightweight. It handles JSON replies from the server, breaks them down, and publishes each piece of data out in the form of a `DataPoint` to each registered `DataSubscriber`.

### DataPoint

This class defines the information associated with each piece of data in the store:

```c++
class DataPoint
{
public:
    QString sTag; // The flattened tag from the JSON object (i.e. Card.UID)
    QVariant Value; // The data, which can be converted to many specific types.
    QDateTime Timestamp; // The recorded timestamp upon receipt
    ...
};
```

### DataSubscriber

Each class who wishes to subscribe to data points must subclass `DataSubscriber` and implement, at a minimum, a single function which is used as a callback by the store when publishing data points. The constructed `DataPoint` is passed as a parameter to drive specific behavior within the system.

## NFCManager

The NFC reader/writer supported by the library is the [ACS ACR122U](https://www.acs.com.hk/en/products/3/acr122u-usb-nfc-reader/). The `NFCManager` class, if elected to be used in the construction of the library, handles all interfacing with this device. The initialization function `NFCManagerInit()` 

## Building & Linking

From the _build_ directory, execute the build script for your respective platform. Ensure that Qt is present in your path before proceeding (i.e. `which qmake` should return something like `~/Qt/5.12.0/clang_64/bin` on macOS). Upon success, the library will be created in `libs/<platform>`.

Integrating the library into another Qt project can be taken of in Qt Creator:

1. Right-click the project and select the _Add Library..._ context menu option.
2. Select _External Library_.
3. Browse to the library and header file directory locations.
4. Uncheck all platforms except the current one the library was built for.
5. Click through the remaining screens to have the library dependencies automatically added to the project file.
6. Re-run qmake and rebuild the project to force the new library linkage.