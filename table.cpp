/****************************************************************************
** $Id$
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "table.h"
#include <qpainter.h>
//#include <qkeycode.h>
//#include <qprinter.h>

#include "config.h"
#include "instrum.h"
#include "playmidi.h"
#include "ctl.h"

static const int NUMCOLS = 9;
static const int NUMROWS = 17;

/*
  Constructs a Table widget.
*/

Table::Table( int width, int height, QWidget *parent, const char *name )
    : QTableView(parent,name)
{

    setAutoUpdate(true);
    setFocusPolicy( NoFocus );
    setBackgroundMode( PaletteBase );		// set widgets background
    setNumCols( NUMCOLS );			// set number of col's in table
    setNumRows( NUMROWS );			// set number of rows in table
    setCellHeight( height/NUMROWS );			// set height of cell in pixels
    setTableFlags( Tbl_clipCellPainting );	// avoid drawing outside cell
    resize( width, height );				// set default size in pixels

    contents = new QString[ NUMROWS * NUMCOLS ];	// make room for contents

    int n;
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
	t_panning[n] = 64;
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
   int leftover = width() - (20+100+27);
   switch (col) {
	case 0: return 20;
	case 1: return 100;
	case 2: return 27;
	default: return leftover/6;
   }
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
	t_panning[chan-1] = 64;
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
    contents[indexOf( chan+1, 3 )].setNum(val);
    t_expression[chan] = val;
	updateCell( chan+1, 3 );
}

void Table::setPanning( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan+1, 4 )].setNum(val);
    t_panning[chan] = val;
	updateCell( chan+1, 4 );
}

void Table::setReverberation( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan+1, 5 )].setNum(val);
    t_reverberation[chan] = val;
	updateCell( chan+1, 5 );
}

void Table::setChorusDepth( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan+1, 6 )].setNum(val);
    t_chorusdepth[chan] = val;
	updateCell( chan+1, 6 );
}

void Table::setVolume( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan+1, 7 )].setNum(val);
    t_volume[chan] = val;
	updateCell( chan+1, 7 );
}

//void Table::setPitchbend( int chan, int val )
//{
//}

//void Table::setSustain( int chan, int val )
//{
//}


/*
  Return content of cell
*/

const char* Table::cellContent( int row, int col ) const
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
      Draw extra frame inside if this is the current cell.
    */
    //p->drawRect( 0, 0, x2, y2 );	// draw rect. along cell edges
#if 0
    if ( (row == curRow) && (col == curCol) ) {	// if we are on current cell,
	if ( hasFocus() ) {
	    p->drawRect( 0, 0, x2, y2 );	// draw rect. along cell edges
	}
	else {					// we don't have focus, so
	    p->setPen( DotLine );		// use dashed line to
	    p->drawRect( 0, 0, x2, y2 );	// draw rect. along cell edges
	    p->setPen( SolidLine );		// restore to normal
	}
    }
#endif

    /*
      Draw cell content (text)
    */

    if (row==0 || col==0 || col==2 || col==8)
        p->drawText( 0, 0, w, h, AlignCenter, contents[indexOf(row,col)] );

    else if (col==1)
	p->drawText( 3, 0, w, h, AlignLeft, contents[indexOf(row,col)] );

    else if (col==3) {
	int wid = (w-8)*t_expression[chan]/128;
	if (wid > 0) {
	    if (c_flags[chan] & FLAG_PERCUSSION)
		p->fillRect(4,4,wid,h-8, QColor("yellow"));
	    else p->fillRect(4,4,wid,h-8, QColor("orange"));
	}
    }
    else if (col==4) {
	int wid, pan = t_panning[chan];
	if (pan > 64) {
	    wid = (w/2 - 4)*(pan - 64)/64;
	    if (wid > 0) p->fillRect(w/2,4,wid,h-8, QColor("green"));
	}
	else if (pan < 64) {
	    wid = (w/2 - 4)*(64 - pan)/64;
	    if (wid > 0) p->fillRect(w/2 - wid,4,wid,h-8, QColor("blue"));
	}
    }
    else if (col==5) {
	int wid = (w-8)*t_reverberation[chan]/128;
	if (wid > 0) {
	    p->fillRect(4,4,wid,h-8, QColor("RoyalBlue"));
	}
    }
    else if (col==6) {
	int wid = (w-8)*t_chorusdepth[chan]/128;
	if (wid > 0) {
	    p->fillRect(4,4,wid,h-8, QColor("coral2"));
	}
    }
    else if (col==7) {
	int wid = (w-8)*t_volume[chan]/128;
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

