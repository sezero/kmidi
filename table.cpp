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

/*
  Constructs a Table widget.
*/

Table::Table( int width, int height, QWidget *parent, const char *name )
    : QTableView(parent,name)
{

    setAutoUpdate(true);
    setFocusPolicy( NoFocus );
    setBackgroundMode( PaletteBase );		// set widgets background
    setNumCols( 5 );			// set number of col's in table
    setNumRows( 16 );			// set number of rows in table
    setCellHeight( height/16 );			// set height of cell in pixels
    setTableFlags( //Tbl_vScrollBar |		// always vertical scroll bar
		   //Tbl_hScrollBar |		// ditto for horizontal
		   Tbl_clipCellPainting |	// avoid drawing outside cell
		   Tbl_smoothScrolling);	// easier to see the scrolling
    resize( width, height );				// set default size in pixels

    contents = new QString[ 16 * 5 ];	// make room for contents

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
   int leftover = width() - (20+30+80+80);
   switch (col) {
	case 0: return 20;
	case 1: return leftover;
	case 2: return 30;
	case 3: return 80;
	case 4: return 80;
   }
   return 0;
}

void Table::clearChannels( void )
{
    for (int chan = 0; chan < 16; chan++) {
        contents[indexOf( chan, 1 )] = QString::null;
        contents[indexOf( chan, 2 )] = QString::null;
	updateCell( chan, 1 );
	updateCell( chan, 2 );
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
}

void Table::setPanning( int chan, int val )
{
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

    if (!row) p->drawLine( 0,  1, x2,  1 );		// horiz. at top
    if (!col) p->drawLine( 1,  0,  1, y2 );		// vert. on left
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
  Handles mouse press events for the Table widget.
  The current cell marker is set to the cell the mouse is clicked in.
*/

//void Table::mousePressEvent( QMouseEvent* e )
//{
//    int oldRow = curRow;			// store previous current cell
//    int oldCol = curCol;
//    QPoint clickedPos = e->pos();		// extract pointer position
//    curRow = findRow( clickedPos.y() );		// map to row; set current cell
//    curCol = findCol( clickedPos.x() );		// map to col; set current cell
//    if ( (curRow != oldRow) 			// if current cell has moved,
//	 || (curCol != oldCol) ) {
//	updateCell( oldRow, oldCol );		// erase previous marking
//	updateCell( curRow, curCol );		// show new current cell
//    }
//}


/*
  Handles key press events for the Table widget.
  Allows moving the current cell marker around with the arrow keys
*/

//void Table::keyPressEvent( QKeyEvent* e )
//{
//    int oldRow = curRow;			// store previous current cell
//    int oldCol = curCol;
//    switch( e->key() ) {			// Look at the key code
//	case Key_Left:				// If 'left arrow'-key, 
//	    if( curCol > 0 ) {			// and cr't not in leftmost col
//		curCol--;     			// set cr't to next left column
//		int edge = leftCell();		// find left edge
//		if ( curCol < edge )		// if we have moved off  edge,
//		    setLeftCell( edge - 1 );	// scroll view to rectify
//	    }
//	    break;
//	case Key_Right:				// Correspondingly...
//	    if( curCol < numCols()-1 ) {
//		curCol++;
//		int edge = lastColVisible();
//		if ( curCol >= edge )
//		    setLeftCell( leftCell() + 1 );
//	    }
//	    break;
//	case Key_Up:
//	    if( curRow > 0 ) {
//		curRow--;
//		int edge = topCell();
//		if ( curRow < edge )
//		    setTopCell( edge - 1 );
//	    }
//	    break;
//	case Key_Down:
//	    if( curRow < numRows()-1 ) {
//		curRow++;
//		int edge = lastRowVisible();
//		if ( curRow >= edge )
//		    setTopCell( topCell() + 1 );
//	    }
//	    break;
//	default:				// If not an interesting key,
//	    e->ignore();			// we don't accept the event
//	    return;	
//    }
//    
//    if ( (curRow != oldRow) 			// if current cell has moved,
//	 || (curCol != oldCol)  ) {
//	updateCell( oldRow, oldCol );		// erase previous marking
//	updateCell( curRow, curCol );		// show new current cell
//    }
//}


/*
  Handles focus reception events for the Table widget.
  Repaint only the current cell; to avoid flickering
*/

//void Table::focusInEvent( QFocusEvent* )
//{
//    updateCell( curRow, curCol );		// draw current cell
//}    


/*
  Handles focus loss events for the Table widget.
  Repaint only the current cell; to avoid flickering
*/

//void Table::focusOutEvent( QFocusEvent* )
//{
//    updateCell( curRow, curCol );		// draw current cell
//}    


/*
  Utility function for mapping from 2D table to 1D array
*/

int Table::indexOf( int row, int col ) const
{
    return (row * numCols()) + col;
}
#include "table.moc"

