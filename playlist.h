/*
   kModPlayer - Qt-Based Mod Player
   
   $Id$

   Copyright 1996 Bernd Johannes Wuebben math.cornell.edu

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef PLAY_LIST_H
#define PLAY_LIST_H

#include <qdatetime.h> 
#include <qlineedit.h>
//#include <qdialog.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qstrlist.h> 
#include <qlistbox.h> 
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qfile.h>
#include <qtextstream.h> 
#include <qdir.h>
#include <qstrlist.h>
#include <qpainter.h>
#include <qapplication.h>    
#include <qpopupmenu.h>
#include <qkeycode.h>
#include <kmenubar.h> 
//#include <qlineedit.h> 
#include <qframe.h> 
#include <ktmainwindow.h>

#include "kmidi.h"

class QSplitter;
class QFrame;

class MyListBoxItem : public QListBoxItem{

public:

  MyListBoxItem( const QString &s, const QPixmap p )
    : QListBoxItem(), pm(p)
    { setText( s ); }
  
protected:
  virtual void paint( QPainter * );
  virtual int height( const QListBox * ) const;
  virtual int width( const QListBox * ) const;
  virtual const QPixmap *pixmap() { return &pm; }
  
public:

  QString filename;
  QString size;
  QString month;
  QString day_and_time;
  QPixmap pm;
};


class PlaylistEdit : public KTMainWindow {

Q_OBJECT

public:
       PlaylistEdit(const char *name=0, QStrList *playlist = 0,
	int *current_playlist_ptr=0, QStrList *listplaylists = 0);
       ~PlaylistEdit();

//private:
// void resizeEvent(QResizeEvent *e);

private slots:

  void editNewPlaylist();
  void newPlaylist();
//  void help();
  void savePlaylistbyName(const QString &name, bool truncate);
  void removeEntry();
  void addEntry();
  void setFilter();
  void checkList();
  void clearPlist();
  void select_all();
  void local_file_selected(int index);
  void loadPlaylist(const QString &name);
  void readPlaylist(int index);
  void selectPlaylist(int index);
  void saveIt();
  void removeIt();
  void appendIt();
  void invokeWhatsThis();

protected:
    void parse_fileinfo(QFileInfo*, MyListBoxItem*);
    void set_local_dir(const QString &dir);
    void redoplist();
    void redoDisplay();
    //void closeEvent( QCloseEvent *e );  
    bool queryClose();  

public:
    QString current_playlist;
    void redoLists();

private:

    bool starting_up;  
    bool showmidisonly;  
    QDir cur_local_dir;
    QList<QFileInfo> cur_local_fileinfo;
    QListBox *local_list;
    QSplitter *hpanner, *vpanner;
    QLabel *statusbar;

    KMenuBar *menu;    
    QListBox* listbox;
    QListBox* plistbox;
    //QPushButton* filterButton;
    //QPushButton* addButton;
    //QPushButton* okButton;
    //QPushButton* removeButton;
    //QPushButton* cancelButton;
    QWhatsThis *what;
    QStrList*  songlist;
    QStrList*  listsonglist;
    QLineEdit* newEdit;
    int *playlist_ptr;
    QFrame* snpopup;
    //QPopupMenu *savenew;
    QPopupMenu *view;

};




#endif /* PLAY_LIST_H */
