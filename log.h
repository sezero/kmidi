/*
 *        kmidi - midi to wav player/converter
 *
 * $Id$
 *            Copyright (C) 1997  Bernd Wuebben
 *                 wuebben@math.cornel.edu 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LOG_WIDGET_
#define _LOG_WIDGET_

#include<stdlib.h>

#include<qdialog.h>
#include<qfont.h>
#include<qstring.h>
#include<qpainter.h>
#include<qpixmap.h>
#include<qlabel.h>
#include<qmultilinedit.h>
#include<qpushbutton.h>
#include <qtimer.h>
#include <qstrlist.h>


class myMultiEdit : public QMultiLineEdit {

public:

  myMultiEdit(QWidget *parent=0, const char *name=0);
  ~myMultiEdit();

bool rowYPos(int row, int& yPos);
void newLine();
};


class LogWindow : public QWidget {

Q_OBJECT

public:
  LogWindow(QWidget *parent=0, const char *name=0);
  ~LogWindow();

  void insertStr(const QString &);
  void clear();

private slots:
    void updatewindow();

private:
      void resizeEvent(QResizeEvent *e);

private:

  bool timerset;
  QTimer *sltimer;
  QStrList *stringlist;
  void enter();
  QFrame *fline;
  myMultiEdit *text_window;


};
#endif

