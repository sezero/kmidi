/*
   kmidi

   $Id$

   Copyright 1997 Bernd Johannes Wuebben math.cornell.edu

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

#include <stdio.h>

#include <qsplitter.h>
#include <qtextstream.h>

#include <kconfig.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>

#include "playlist.h"
#include "playlist.moc"
#include "kmidi.h"
#include "midiapplication.h"

#define BORDER_WIDTH 2

int loc_max_namewidth=0;
int loc_max_monthwidth=0;

char fontrefstring1[]="012345678901234567890";
char fontrefstring2[]="01234567890";
char fontrefstring3[]="00  00:00";



PlaylistEdit::PlaylistEdit(const char *name, QStringList *playlist,
	int *current_playlist_ptr, QStringList *listplaylists)
  : KMainWindow(0, name){

  setCaption(i18n("Compose Play List"));

  starting_up = true;


  what = new QWhatsThis(this);

  QPopupMenu *file = new QPopupMenu;
  Q_CHECK_PTR( file );

  file->insertItem( i18n("OK"), this, SLOT(checkList()), 0, 40);
	file->setWhatsThis(40, i18n("Accept and use the new play list.") );
  file->insertItem( i18n("Save"), this, SLOT(saveIt()), 0, 41);
	file->setWhatsThis(41, i18n("Replace the contents of the<br>\n"
				"currently selected playlist<br>\n"
				"file with the new play list.") );
  file->insertItem( i18n("Save As..."), this, SLOT( newPlaylist() ), 0, 42);
	file->setWhatsThis(42, i18n("Make up a name for a<br>\n"
				"new playlist file and<br>\n"
				"save the current play list there.") );
  file->insertItem( i18n("Append"), this, SLOT(appendIt()), 0, 43);
	file->setWhatsThis(43, i18n("Add the midi files in the<br>\n"
				"current play list to the selected<br>\n"
				"playlist file.") );
  file->insertItem( i18n("Delete"), this, SLOT(removeIt()), 0, 44);
	file->setWhatsThis(44, i18n("Delete the currently selected<br>\n"
				"playlist file (it's contents<br>\n"
				"will be lost).") );
  file->insertSeparator();
  file->insertItem( i18n("&Quit"), this, SLOT(hide()), 0, 45);
	file->setWhatsThis(45, i18n("Discard the play list<br>\n"
				"and exit the play list editor.") );

  view = new QPopupMenu;
  Q_CHECK_PTR( view );
  view->insertItem( i18n("All Files"), this, SLOT(setFilter()), 0, 50 );
	view->setWhatsThis(50, i18n("Choose whether the file<br>\n"
				"list should show only uncompressed<br>\n"
				"midi files or all files.") );

  QPopupMenu *edit = new QPopupMenu;
  Q_CHECK_PTR( edit );
  edit->insertItem( i18n("Clear"), this, SLOT(clearPlist()), 0, 51 );
	edit->setWhatsThis(51, i18n("Discard the contents<br>\n"
				"of the current play list.") );
  edit->insertItem( i18n("Select All"), this, SLOT(select_all()), 0, 52 );
	edit->setWhatsThis(52, i18n("Append all the files<br>\n"
				"in the directory listing<br>\n"
				"to the play list.") );
  edit->insertItem( i18n("Add"), this, SLOT(addEntry()), 0, 53 );
	edit->setWhatsThis(53, i18n("Append the selected file<br>\n"
				"in the directory listing<br>\n"
				"file to the play list.") );
  edit->insertItem( i18n("Remove"), this, SLOT(removeEntry()), 0, 54 );
	edit->setWhatsThis(54, i18n("Remove the selected<br>\n"
				"file from the play list.") );

  menu = new KMenuBar( this );
  Q_CHECK_PTR( menu );
  menu->insertItem( i18n("&File"), file );
  menu->insertItem( i18n("&View"), view );
  menu->insertItem( i18n("&Edit"), edit );
  menu->insertSeparator();
  menu->insertItem( i18n("What's This?"),  this, SLOT(invokeWhatsThis()) );

#if 0
  QString aboutapp;
  aboutapp = i18n("KDE midi file player\n\n"
                     "A software synthesizer for playing\n"
                     "midi songs using Tuukka Toivonen's\n"
                     "TiMidity");

  QPopupMenu *about = helpMenu(aboutapp);
  menu->insertItem( i18n("About"), about);
#endif

  vpanner = new QSplitter(Vertical, this, "_panner");
  hpanner = new QSplitter(Horizontal, vpanner, "_panner");
  vpanner->setOpaqueResize( TRUE );
  hpanner->setOpaqueResize( TRUE );
  what->add(vpanner, i18n("You can drag this<br>\nbar up or down.") );
  what->add(hpanner, i18n("You can drag this<br>\nbar left or right.") );

  listbox = new QListBox(hpanner,"listbox",0);
  what->add(listbox, i18n("Here is the play<br>\nlist you are composing.<br>\n"
	"Double-click to delete<br>\n"
	"an entry."));
  hpanner->moveToLast(listbox);

  connect(listbox, SIGNAL(selected(int)), this, SLOT(removeEntry()));

  showmidisonly = TRUE;

  local_list = new QListBox(hpanner, "local_list",0);
  what->add(local_list, i18n("Here are the files<br>\nyou can add<br>\n"
			     "to the play list.<br>\n"
			     "Double-click on a file name<br>\n"
			     "to add it."));
  hpanner->moveToFirst(local_list);

  playlist_ptr = current_playlist_ptr;
  plistbox = new QListBox(vpanner,"listbox",0);
  connect(plistbox, SIGNAL(selected(int)), this, SLOT(readPlaylist(int)));
  connect(plistbox, SIGNAL(highlighted(int)), this, SLOT(selectPlaylist(int)));
  what->add(plistbox, i18n("<u>Playlist files:</u><br>\n"
			   "Click to select one.<br>\n"
			   "Doubleclick to append contents<br>\n"
			   "to the play list."));
  QValueList<int> size;
  size << 30 << 70;
  hpanner->setSizes(size);

  //local_list->insertItem("Local Directory", -1);
  connect(local_list, SIGNAL(selected(int )), this, SLOT(local_file_selected(int )));

  QString str;
  KConfig *config = thisapp->config();
  config->setGroup("KMidi");
  str = config->readEntry("Directory");
  if ( !str.isNull() ) set_local_dir(str);
  else set_local_dir(QString(""));

  this->resize(PLAYLIST_WIDTH,PLAYLIST_HEIGHT);

  songlist = playlist;
  listsonglist = listplaylists;

  redoLists();

  setCentralWidget(vpanner);
};


PlaylistEdit::~PlaylistEdit(){

};

//void PlaylistEdit::closeEvent( QCloseEvent *e ){
//    hide();
//    e->ignore();
//}
bool PlaylistEdit::queryClose(){
    hide();
    return false;
}

void PlaylistEdit::invokeWhatsThis(){
    QWhatsThis::enterWhatsThisMode();
}

void MyListBoxItem::paint( QPainter *p ){

  p->drawPixmap( 3, 0, pm );
  QFontMetrics fm = p->fontMetrics();


  int yPos;                       // vertical text position

  if ( pm.height() < fm.height() )
    yPos = fm.ascent() + fm.leading()/2;
  else
    yPos = pm.height()/2 - fm.height()/2 + fm.ascent();

  p->drawText( pm.width() + 5, yPos, filename );

}



int MyListBoxItem::height(const QListBox *lb ) const{

  return QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 1 );
}



int MyListBoxItem::width(const QListBox *lb ) const{

  QFontMetrics fm = lb->fontMetrics();

  QRect ref = fm.boundingRect(filename);
  int ref1 = ref.width();
  return 6 + pm.width() + ref1;

}	

void PlaylistEdit::redoDisplay() {

  redoplist();

  if ((int)plistbox->count() >= *playlist_ptr) {
    plistbox->setCurrentItem(*playlist_ptr);
    current_playlist = plistbox->text((uint) *playlist_ptr);
    current_playlist += ".plist";
  }
  else {
    current_playlist = "default.plist";
    *playlist_ptr = 0;
  }
  if (!listbox->count())
    loadPlaylist(current_playlist);
}

void PlaylistEdit::redoLists() {

  listbox->clear();
  listbox->insertStrList(songlist,-1);
  redoDisplay();
}

void PlaylistEdit::clearPlist() {

  listbox->clear();
}

void  PlaylistEdit::parse_fileinfo(QFileInfo* fi, MyListBoxItem* lbitem){

  lbitem->filename =fi->fileName();

  QString size;
  size.setNum(fi->size());
  lbitem->size = size;

  QDate date = fi->lastModified().date();

  QString monthname = date.monthName(date.month());
  lbitem->month = monthname;

  QTime time = fi->lastModified().time();

  lbitem->day_and_time.sprintf("%02d  %02d:%02d",date.day(),time.hour(),time.minute());

  QFontMetrics fm = local_list->fontMetrics();

  QRect ref1 = fm.boundingRect(fi->fileName());
  loc_max_namewidth  = QMAX(loc_max_namewidth,ref1.width());

  ref1 = fm.boundingRect(monthname);
  loc_max_monthwidth  = QMAX(loc_max_monthwidth,ref1.width());
}



void PlaylistEdit::select_all(){
  uint index;

  for (index = 0; index < local_list->count(); index++) {

    if( !cur_local_fileinfo.at((uint)index)->isDir())
      listbox->insertItem(cur_local_fileinfo.at((uint)index)->filePath(),-1);

  }
}

void PlaylistEdit::local_file_selected(int index){

  QString dir = local_list->text((uint) index);

  //  printf("File:%s\n",cur_local_fileinfo.at((uint)index)->filePath());

  if( cur_local_fileinfo.at((uint)index)->isDir()){
    set_local_dir(dir);
    return;
  }

  listbox->insertItem(cur_local_fileinfo.at((uint)index)->filePath(),-1);

}


void PlaylistEdit::redoplist()
{
    QString filenamestr;
    int index;

    plistbox->clear();

    for ( QStringList::Iterator it = listsonglist->begin();
          it != listsonglist->end();
          ++it )
    {
	filenamestr = *it;
	index = filenamestr.findRev('/',-1,TRUE);
	if(index != -1)
	    filenamestr = filenamestr.right(filenamestr.length() -index -1);
	filenamestr = filenamestr.replace(QRegExp("_"), " ");

	if(filenamestr.length() > 6){
	if(filenamestr.right(6) == QString(".plist"))
	    filenamestr = filenamestr.left(filenamestr.length()-6);
	}
	plistbox->insertItem(filenamestr,-1);

    }
}

void PlaylistEdit::saveIt()
{
  int i;
  if ( (i=plistbox->currentItem()) < 0) return;
  QString name = plistbox->text((uint) i);
  name += ".plist";
  savePlaylistbyName(name, true);

}

void PlaylistEdit::appendIt()
{
  int i;
  if ( (i=plistbox->currentItem()) < 0) return;
  QString name = plistbox->text((uint) i);
  name += ".plist";
  savePlaylistbyName(name, false);

}

void PlaylistEdit::removeIt()
{
  int i;
  if ( (i=plistbox->currentItem()) < 0) return;
  QString name = plistbox->text((uint) i);
  name += ".plist";
  QString path = locateLocal("appdata", name);
  QFile f(path);
  if (f.remove()) {
    listsonglist->remove(listsonglist->at(i));
    redoDisplay();
  }
}


void PlaylistEdit::readPlaylist(int index){

  *playlist_ptr = index;
  QString name = plistbox->text((uint) index);
  name += ".plist";
  //listbox->clear();
  loadPlaylist( name );

}

void PlaylistEdit::selectPlaylist(int index){

  *playlist_ptr = index;
  QString name = plistbox->text((uint) index);
  name += ".plist";
  current_playlist = name;

}


void PlaylistEdit::set_local_dir(const QString &dir){


  if (!dir.isEmpty()){
    if ( !cur_local_dir.setCurrent(dir)){
      QString str = i18n("Can not enter directory: %1\n").arg(dir);
      KMessageBox::sorry(this, str);
      return;
    }
  }

   cur_local_dir  = QDir::current();
   KConfig *config = thisapp->config();
   config->setGroup("KMidi");
   config->writeEntry("Directory", cur_local_dir.filePath("."));
   //config->sync();

   cur_local_dir.setSorting( cur_local_dir.sorting() | QDir::DirsFirst );

  qApp ->setOverrideCursor( waitCursor );
  local_list->clear();

  cur_local_fileinfo.clear();

  const QFileInfoList  *filist = cur_local_dir.entryInfoList();

  loc_max_namewidth = 0; // reset the fontmetric length of the longest filename
  loc_max_monthwidth = 0; // same for the month

  if ( filist ) {
    QFileInfoListIterator it( *filist );
    QFileInfo		 *fi = it.current();
    while ( fi && fi->isDir() ) {

      QString tmp = fi->fileName();

      if (tmp == ".."){

	MyListBoxItem* mylistboxitem1 =
	  new MyListBoxItem(fi->fileName(),kmidi->cdup_pixmap);

	parse_fileinfo(fi,mylistboxitem1);

	local_list->insertItem( mylistboxitem1, -1 );

	cur_local_fileinfo.append(fi);

      }else{
	if (tmp == "."){

	  // skip we don't want to see this.

	}else{
	  MyListBoxItem* mylistboxitem1 = new MyListBoxItem(fi->fileName()
							,kmidi->folder_pixmap);
	  parse_fileinfo(fi,mylistboxitem1);
	  local_list->insertItem( mylistboxitem1, -1 );
	  cur_local_fileinfo.append(fi);
	}
      }

      // printf("directory:%s\n",fi->fileName().data());

      fi = ++it;
    }
    for ( ; fi ; fi = ++it ) {
      QString displayline;
      char mbuff[5];
      QFile f(fi->filePath());
      if (showmidisonly) {
          if (!f.open( IO_ReadOnly )) continue;
          if (f.readBlock(mbuff, 4) != 4) {
		f.close();
		continue;
          }
          mbuff[4] = '\0';
          if (strcmp(mbuff, "MThd")) {
		f.close();
		continue;
          }
          f.close();
      }
      MyListBoxItem* mylistboxitem2 = new MyListBoxItem(fi->fileName()
							,kmidi->file_pixmap);
      parse_fileinfo(fi,mylistboxitem2);
      local_list->insertItem( mylistboxitem2, -1 );
      cur_local_fileinfo.append(fi);
      //printf("file:%s\n",fi->fileName().data());
    }
  } else {
    qApp->restoreOverrideCursor();
    KMessageBox::error(this, i18n("Cannot open or read directory."));
    qApp ->setOverrideCursor( waitCursor );
  }
  local_list->repaint();

  //updatePathBox( d.path() );

  qApp->restoreOverrideCursor();

  //  printf("Current Dir Path: %s\n",QDir::currentDirPath().data());
}

void PlaylistEdit::setFilter(){

	//showmidisonly = filterButton->isOn();
   if (showmidisonly) view->changeItem(0, "Midi files only");
   else view->changeItem(0, "All files");
   showmidisonly = !showmidisonly;
   set_local_dir(QString(""));
}

void PlaylistEdit::addEntry(){

  //  printf("File:%s\n",cur_local_fileinfo.at(local_list->currentItem())->filePath());

  if (local_list->currentItem() != -1)
    if(!cur_local_fileinfo.at(local_list->currentItem())->isDir()){
      listbox->insertItem(
	  cur_local_fileinfo.at(local_list->currentItem())->filePath(),-1);
  }
}




void PlaylistEdit::removeEntry(){

  if (listbox->currentItem() != -1){
    //    printf("removing:%s:\n",listbox->text(listbox->currentItem()));
    listbox->removeItem(listbox->currentItem());

  }
}




void PlaylistEdit::checkList(){

  songlist->clear();
  if (listbox->count()){
    for(int index = 0; index < (int)listbox->count(); index ++){
      //      printf("Appending:%s:\n",listbox->text(index));
      songlist->append( listbox->text(index));
    }
  }

  kmidi->acceptPlaylist();
  //savePlaylist();
  //accept();
  hide();
}


void PlaylistEdit::newPlaylist(){
	KLineEditDlg dlg(i18n("Enter name:"), QString::null, this);
	dlg.setCaption(i18n("New Playlist"));
	if (dlg.exec()) {
	  QString plf = dlg.text();
	  plf += ".plist";
          QString path = locateLocal("appdata", plf);
	  listsonglist->append(path);
          listsonglist->sort();
	  *playlist_ptr = listsonglist->findIndex(path);
	  savePlaylistbyName(plf, true);
	  redoDisplay();
	}
}


void PlaylistEdit::savePlaylistbyName(const QString &name, bool truncate)
{
  if(name.isEmpty())
    return;

  QString defaultlist = locateLocal("appdata", name);
  QFile f(defaultlist);

  if (truncate) f.open( IO_ReadWrite | IO_Translate | IO_Truncate);
  else f.open( IO_ReadWrite | IO_Translate | IO_Append);

  QCString tempstring;

  for (int i = 0; i < (int)listbox->count(); i++){
    tempstring = QFile::encodeName(listbox->text(i));
    tempstring += '\n';
    f.writeBlock(tempstring,tempstring.length());
  }

  f.close();

}


void PlaylistEdit::loadPlaylist(const QString &name){

  current_playlist = name;

 QString defaultlist = locate("appdata", current_playlist);
 if (defaultlist.isEmpty())
    return;

 QFile f(defaultlist);

 f.open( IO_ReadOnly | IO_Translate);

 char buffer[1024];
 QString tempstring;

 while(!f.atEnd()){
   //tempstring = tempstring.stripWhiteSpace();
   //if (!tempstring.isEmpty())
   //   listbox->insertItem(tempstring,-1);
   buffer[0] = (char) 0;
   f.readLine(buffer,511);
   //printf("Found:%s\n",buffer);
   tempstring = buffer;
   tempstring = tempstring.stripWhiteSpace();
   if (!tempstring.isEmpty()){
     //    printf("Insertring:%s\n",tempstring.data());
     listbox->insertItem(tempstring,-1);
   }
 }

 f.close();

}

//void PlaylistEdit::help(){
//
//  if(!thisapp)
//    return;
//
//  thisapp->invokeHTMLHelp("","");
//
//}
