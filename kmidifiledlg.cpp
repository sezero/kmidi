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
#include "kmidifiledlg.h"
#include "kmidi.h"

extern KMidi *kmidi;

KMidiFileDlg::KMidiFileDlg(const QString& dirName, const QString& filter,
				 QWidget *parent, const char *name, 
				 bool modal )
    : KFileDialog(dirName, filter, parent, name, modal)
{
//Can't catch this signal, for some reason.
 //connect(this,SIGNAL(fileSelected(const QString&)),this,SLOT(playFile(const QString&)));
 //connect(this,SIGNAL(fileSelected(const QString&)),SLOT(playFile(const QString&)));
}

void KMidiFileDlg::playFile(const QString& fname)
{
    fprintf(stderr, "You clicked on %s\n", fname.ascii());
}

void KMidiFileDlg::slotOk()
{

    if ( locationEdit->currentText().stripWhiteSpace().isEmpty() )
        return;

    QString filename = locationEdit->currentText();

    KURL u( filename );
    QString thePath = u.path();

    if ( u.isLocalFile() ) {
      if ( QFileInfo(thePath).isDir() ) {
	setURL( QDir::cleanDirPath( thePath ) );
	return;
      }
      if (!thePath.isNull())
      {
	kmidi->playlist->insert(0, thePath);
	kmidi->restartPlaybox();
      }
    }

}

void KMidiFileDlg::getOpenDialog(const QString& dir, const QString& filter,
				     QWidget *parent, const char *name)
{
    static KMidiFileDlg *dlg = 0;

    if (!dlg) {
	dlg= new KMidiFileDlg(dir, filter, parent, name, false);
	dlg->setCaption(i18n("Open"));
//This doesn't work, either.
// connect(dlg,SIGNAL(fileSelected(const QString&)),dlg,SLOT(playFile(const QString&)));
// connect(dlg,SIGNAL(fileHighlighted(const QString&)),dlg,SLOT(playFile(const QString&)));
    }

    dlg->show();
}

#include "kmidifiledlg.moc"
