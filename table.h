/****************************************************************************
** $Id$
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef TABLE_H
#define TABLE_H

#include <qtableview.h>


class Table : public QTableView
{
    Q_OBJECT
public:
    Table( int width, int height, QWidget* parent=0, const char* name=0, WFlags f=0 );
    ~Table();
    
    const char* cellContent( int row, int col ) const;
    void setCellContent( int row, int col, const char* );

    void clearChannels( void );
    void setProgram( int chan, int val, const char *inst, int bank, int variationbank );
    void setExpression( int chan, int val );
    void setPanning( int chan, int val );
    void setReverberation( int chan, int val );
    void setChorusDepth( int chan, int val );
    void setVolume( int chan, int val );
    int c_flags[16];

protected:
    void paintCell( QPainter*, int row, int col );
    int cellWidth( int col );
 
private:
    int indexOf( int row, int col ) const;
    QString* contents;
    int curRow;
    int curCol;
    int t_expression[16];
    int t_panning[16];
    int t_reverberation[16];
    int t_chorusdepth[16];
    int t_volume[16];
    int cell_width[9];

};

#endif // TABLE_H
