/*
    KDE Icon Editor - a small graphics drawing program for the KDE

    Copyright (C) 1998 Thomas Tanghus (tanghus@kde.org)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <qfileinfo.h>
#include <qdir.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <kdiroperator.h>
#include "kmidifiledlg.h"
#include "kmidi.h"

KMidiFileDlg::KMidiFileDlg(const QString& dirName, const QString& filter,
				 QWidget *parent, const char *name,
				 bool modal )
    : KFileDialog(dirName, filter, parent, name, modal)
{
/*    connect(this,SIGNAL(fileSelected(const QString&)),
	    SLOT(playFile(const QString&)));
*/
    connect(this,SIGNAL(fileHighlighted(const QString&)),
	    SLOT(noteFile(const QString&)));
    connect(this,SIGNAL(cancelClicked()),
	    SLOT(closeFbox()));
    //setButtonOKText(i18n("Play"));
    okButton()->setText(i18n("Play"));
    //setButtonCancelText(i18n("Close"));
    cancelButton()->setText(i18n("Close"));
}

void KMidiFileDlg::playFile(const QString& fname)
{
/*    fprintf(stderr, "You clicked on %s\n", fname.ascii()); */

    KURL u( fname );
    QString thePath = u.path();

    if ( u.isLocalFile() ) {
      if ( QFileInfo(thePath).isDir() ) {
	setURL( QDir::cleanDirPath( thePath ) );
	return;
      }
      if (!thePath.isNull())
      {
	kmidi->restartPlayboxWith(thePath);
      }
    }
}

void KMidiFileDlg::noteFile(const QString& fname)
{
//    fprintf(stderr, "You selected %s\n", fname.ascii());

    hlfile = fname;
}

void KMidiFileDlg::accept()
{
    if ( locationEdit->currentText().stripWhiteSpace().isEmpty() )
        return;

    playFile(hlfile);
}
#define ConfigGroup QString::fromLatin1("KFileDialog Settings")

void KMidiFileDlg::closeFbox()
{
/* this doesn't appear to work */
    *lastDirectory = ops->url();
    KSimpleConfig *c = new KSimpleConfig(QString::fromLatin1("kdeglobals"),
                                         false);
    saveConfig( c, ConfigGroup );
    saveRecentFiles( KGlobal::config() );
    delete c;
}

void KMidiFileDlg::getOpenDialog(const QString& dir, const QString& filter,
				     QWidget *parent, const char *name)
{
    static KMidiFileDlg *dlg = 0;

    if (!dlg) {
	dlg= new KMidiFileDlg(dir, filter, parent, name, false);
	dlg->setCaption(i18n("Open"));
    }

    dlg->show();
}

#include "kmidifiledlg.moc"
