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
#include <qkeycode.h>
#include <qprinter.h>

#include "config.h"
#include "instrum.h"
#include "playmidi.h"
#include "ctl.h"

static const int NUMCOLS = 7;
static const int NUMROWS = 16;

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
    for (n=0; n<16; n++) contents[indexOf( n, 0 )].setNum(n+1);

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
   int leftover = width() - (20+100+27+27+27+40);
   switch (col) {
	case 0: return 20;
	case 1: return 100;
	case 2: return 27;
	case 3: return 27;
	case 4: return 27;
	case 5: return 40;
	case 6: return leftover;
   }
   return 0;
}

void Table::clearChannels( void )
{
    for (int chan = 0; chan < 16; chan++) {
        contents[indexOf( chan, 1 )] = QString::null;
        contents[indexOf( chan, 2 )] = QString::null;
        contents[indexOf( chan, 3 )] = QString::null;
        contents[indexOf( chan, 4 )] = QString::null;
	updateCell( chan, 1 );
	updateCell( chan, 2 );
	updateCell( chan, 3 );
	updateCell( chan, 4 );
    }
}

void Table::setProgram( int chan, int val, const char *inst )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan, 1 )] = QString(inst);
    contents[indexOf( chan, 2 )].setNum(val);
	updateCell( chan, 1 );
	updateCell( chan, 2 );
}

void Table::setVolume( int chan, int val )
{
}

void Table::setExpression( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan, 3 )].setNum(val);
	updateCell( chan, 3 );
}

void Table::setPanning( int chan, int val )
{
    if (chan < 0 || chan > 15) return;
    contents[indexOf( chan, 4 )].setNum(val);
	updateCell( chan, 4 );
}

void Table::setPitchbend( int chan, int val )
{
}

void Table::setSustain( int chan, int val )
{
}


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

    /*
      Draw our part of cell frame.
    */
    p->drawLine( x2, 0, x2, y2 );		// draw vertical line on right
    p->drawLine( 0, y2, x2, y2 );		// draw horiz. line at bottom

    //if (!row) p->drawLine( 0,  1, x2,  1 );		// horiz. at top
    //if (!col) p->drawLine( 1,  0,  1, y2 );		// vert. on left
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
    if (col==1) p->drawText( 3, 0, w, h, AlignLeft, contents[indexOf(row,col)] );
    else p->drawText( 0, 0, w, h, AlignCenter, contents[indexOf(row,col)] );
}

/*
  Utility function for mapping from 2D table to 1D array
*/

int Table::indexOf( int row, int col ) const
{
    return (row * numCols()) + col;
}
#include "table.moc"

