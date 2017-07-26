// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef BODY_FORESTALL_BLOCK_H
#define BODY_FORESTALL_BLOCK_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

namespace rcsc {
class WorldModel;
}

class Body_ForestallBlock
    : public rcsc::BodyAction {
private:
    static int S_count_no_move;

    const rcsc::Vector2D M_home_position;

public:
    explicit
    Body_ForestallBlock( const rcsc::Vector2D & home_pos )
        : M_home_position( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    rcsc::Vector2D getTargetPointSimple( const rcsc::WorldModel & wm );
    rcsc::Vector2D getTargetPoint( const rcsc::WorldModel & wm );

};

#endif
