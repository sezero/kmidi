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

#ifndef __KMIDIFILEDLG_H__
#define __KMIDIFILEDLG_H__

#include <kfiledialog.h>


class KMidiFileDlg : public KFileDialog
{
  Q_OBJECT
public:
  KMidiFileDlg(const QString& dirName, const QString& filter= 0,
		QWidget *parent= 0, const char *name= 0,
		bool modal = false );

  static void getOpenDialog(const QString& dir= QString::null,
			    const QString& filter= QString::null,
			    QWidget *parent= 0, const char *name= 0);

protected:
  QString hlfile;

protected slots:
//    void slotOk(void);
    void accept(void);
    void closeFbox(void);
    void playFile(const QString& fname);
    void noteFile(const QString& fname);

};

#endif //__KICONFILEDLG_H__



