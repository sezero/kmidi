/****************************************************************************
** $Id$
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <sys/types.h>

#include "config.h"
#include "table.h"
#include "instrum.h"
#include "playmidi.h"
#include "ctl.h"

#include <qpainter.h>

static const int NUMCOLS = 9;
static const int NUMROWS = 17;

/*
  Constructs a Table widget.
*/

Table::Table( int width, int height, QWidget *parent, const char *name, WFlags f)
    : QTableView(parent,name,f)
{
    int n;

    setAutoUpdate(true);
    setFocusPolicy( NoFocus );
    setBackgroundMode( PaletteBase );		// set widgets background
    setNumCols( NUMCOLS );			// set number of col's in table
    setNumRows( NUMROWS );			// set number of rows in table
    setCellHeight( height/NUMROWS );			// set height of cell in pixels

    cell_width[0] = 20;
    cell_width[1] = 100;
    cell_width[2] = 27;
    cell_width[8] = 27;
    for (n=3; n<8; n++) cell_width[n] = (width - (20+100+27+27)) / 5;

    setTableFlags( Tbl_clipCellPainting );	// avoid drawing outside cell
    resize( width, height );				// set default size in pixels

    contents = new QString[ NUMROWS * NUMCOLS ];	// make room for contents

    for (n=1; n<NUMROWS; n++) contents[indexOf( n, 0 )].setNum(n);

    contents[indexOf(0,0)] = QString("C");
    contents[indexOf(0,1)] = QString("Patch");
    contents[indexOf(0,2)] = QString("Pr");
    contents[indexOf(0,3)] = QString("Exp");
    contents[indexOf(0,4)] = QString("Pan");
    contents[indexOf(0,5)] = QString("Rev");
    contents[indexOf(0,6)] = QString("Chor");
    contents[indexOf(0,7)] = QString("Vol");
    contents[indexOf(0,8)] = QString("Bank");

    for (n=0; n<16; n++) {
	c_flags[n] = 0;
	t_expression[n] = 0;
	t_panning[n] = 0;
	t_reverberation[n] = 0;
	t_chorusdepth[n] = 0;
	t_volume[n] = 0;
    }
}


/*
  Destructor: deallocates memory for contents
*/

Table::~Table()
{
    delete[] contents;				// deallocation
}

int Table::cellWidth( int col )
{
   return cell_width[col];
}

void Table::clearChannels( void )
{
    int chan;
    for (chan = 1; chan < NUMROWS; chan++) {
        contents[indexOf( chan, 1 )] = QString::null;
        contents[indexOf( chan, 2 )] = QString::null;
        contents[indexOf( chan, 3 )] = QString::null;
        contents[indexOf( chan, 4 )] = QString::null;
        contents[indexOf( chan, 5 )] = QString::null;
        contents[indexOf( chan, 6 )] = QString::null;
        contents[indexOf( chan, 7 )] = QString::null;
        contents[indexOf( chan, 8 )] = QString::null;
	t_expression[chan-1] = 0;
	c_flags[chan-1] = 0;
	t_panning[chan-1] = 0;
	t_reverberation[chan-1] = 0;
	t_chorusdepth[chan-1] = 0;
	t_volume[chan-1] = 0;
	updateCell( chan, 1 );
	updateCell( chan, 2 );
	updateCell( chan, 3 );
	updateCell( chan, 4 );
	updateCell( chan, 5 );
	updateCell( chan, 6 );
	updateCell( chan, 7 );
	updateCell( chan, 8 );
    }
}

void Table::setProgram( int chan, int val, const char *inst, int bank, int variationbank )
{
    if (chan < 0 || chan > 15) return;
    chan++;
    contents[indexOf( chan, 1 )] = QString(inst);
    contents[indexOf( chan, 2 )].setNum(val);
	updateCell( chan, 1 );
	updateCell( chan, 2 );
    if (variationbank) contents[indexOf( chan, 8 )].setNum(variationbank);
    else contents[indexOf( chan, 8 )].setNum(bank);
	updateCell( chan, 8 );
}


void Table::setExpression( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    int wid = (cell_width[3]-8)*val/128;

    contents[indexOf( chan+1, 3 )].setNum(wid);
    t_expression[chan] = wid;
	updateCell( chan+1, 3, false );
}

void Table::setPanning( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    int wid = (cell_width[4]/2 - 4)*(val - 64)/64;
    contents[indexOf( chan+1, 4 )].setNum(wid);
    t_panning[chan] = wid;
	updateCell( chan+1, 4, false );
}

void Table::setReverberation( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    int wid = (cell_width[5]-8)*val/128;
    contents[indexOf( chan+1, 5 )].setNum(wid);
    t_reverberation[chan] = wid;
	updateCell( chan+1, 5, false );
}

void Table::setChorusDepth( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    int wid = (cell_width[6]-8)*val/128;
    contents[indexOf( chan+1, 6 )].setNum(wid);
    t_chorusdepth[chan] = wid;
	updateCell( chan+1, 6, false );
}

void Table::setVolume( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    int wid = (cell_width[7]-8)*val/128;
    contents[indexOf( chan+1, 7 )].setNum(wid);
    t_volume[chan] = wid;
	updateCell( chan+1, 7, false );
}

/*
  Return content of cell
*/

QString Table::cellContent( int row, int col ) const
{
    return contents[indexOf( row, col )];	// contents array lookup
}


/*
  Set content of cell
*/

void Table::setCellContent( int row, int col, const char* c )
{
    contents[indexOf( row, col )] = c;		// contents lookup and assign
    updateCell( row, col );			// show new content
}


/*
  Handles cell painting for the Table widget.
*/

void Table::paintCell( QPainter* p, int row, int col )
{
    int w = cellWidth( col );			// width of cell in pixels
    int h = cellHeight( row );			// height of cell in pixels
    int x2 = w - 1;
    int y2 = h - 1;
    int chan = row-1;

    /*
      Draw our part of cell frame.
    */
    p->drawLine( x2, 0, x2, y2 );		// draw vertical line on right
    p->drawLine( 0, y2, x2, y2 );		// draw horiz. line at bottom

    if (!row) p->drawLine( 0,  0, x2,  0 );		// horiz. at top
    if (!col) p->drawLine( 0,  0,  0, y2 );		// vert. on left

    /*
      Draw cell content (text)
    */

    if (row==0 || col==0 || col==2 || col==8)
        p->drawText( 0, 0, w, h, AlignCenter, contents[indexOf(row,col)] );

    else if (col==1)
	p->drawText( 3, 0, w, h, AlignLeft, contents[indexOf(row,col)] );

    else if (col==3) {
	int wid = t_expression[chan];
	if (wid < w-8) p->fillRect(4+wid,4,w-8-wid,h-8, backgroundColor());
	if (wid > 0) {
	    if (c_flags[chan] & FLAG_PERCUSSION)
		p->fillRect(4,4,wid,h-8, QColor("yellow"));
	    else p->fillRect(4,4,wid,h-8, QColor("orange"));
	}
    }
    else if (col==4) {
	int wid = t_panning[chan];
	if (wid > 0) {
	    p->fillRect(4,4,(w-8)/2,h-8, backgroundColor());
	    if (wid < w/2-4) p->fillRect(w/2+wid,4,w/2-4-wid,h-8, backgroundColor());
	    p->fillRect(w/2,4,wid,h-8, QColor("green"));
	}
	else if (wid < 0) {
	    p->fillRect(w/2,4,(w-8)/2,h-8, backgroundColor());
	    if (-wid < w/2-4) p->fillRect(4,4,w/2-4+wid,h-8, backgroundColor());
	    p->fillRect(w/2 + wid,4,-wid,h-8, QColor("blue"));
	}
	else p->fillRect(4,4,w-8,h-8, backgroundColor());
    }
    else if (col==5) {
	int wid = t_reverberation[chan];
	if (wid < w-8) p->fillRect(4+wid,4,w-8-wid,h-8, backgroundColor());
	if (wid > 0) {
	    p->fillRect(4,4,wid,h-8, QColor("SlateBlue2"));
	}
    }
    else if (col==6) {
	int wid = t_chorusdepth[chan];
	if (wid < w-8) p->fillRect(4+wid,4,w-8-wid,h-8, backgroundColor());
	if (wid > 0) {
	    p->fillRect(4,4,wid,h-8, QColor("coral2"));
	}
    }
    else if (col==7) {
	int wid = t_volume[chan];
	if (wid < w-8) p->fillRect(4+wid,4,w-8-wid,h-8, backgroundColor());
	if (wid > 0) {
	    if (c_flags[chan] & FLAG_PERCUSSION)
		p->fillRect(4,4,wid,h-8, QColor("yellow"));
	    else p->fillRect(4,4,wid,h-8, QColor("white"));
	}
    }
}

/*
  Utility function for mapping from 2D table to 1D array
*/

int Table::indexOf( int row, int col ) const
{
    return (row * numCols()) + col;
}
#include "table.moc"

