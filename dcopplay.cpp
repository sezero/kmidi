#include <kapp.h>
#include <dcopclient.h>
#include <qfileinfo.h>

int main( int argc, char** argv )
{
    KApplication app( argc, argv, "KmidiClient", false );

    if (argc != 2) return 1;

    kapp->dcopClient()->attach();

    QFileInfo file;
    QString filename = argv[1];
    file.setFile(filename);

    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << file.absFilePath();

    if (kapp->dcopClient()->send( "kmidi", "kmidi", "play(QString)", params))
      debug("ok");
    else
      debug("ko");

}
