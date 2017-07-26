// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
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
#ifndef DY_SHOOT_AREA_CF_MOVE_H
#define DY_SHOOT_AREA_CF_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/world_model.h>
using namespace rcsc;


class shoot_area_cf_move
{
public:
    shoot_area_cf_move()
    {}
    bool execute(rcsc::PlayerAgent *agent);
    double evaluateTargetPoint( const int count,
                                const rcsc::WorldModel & wm,
                                const rcsc::Vector2D & home_pos,
                                const rcsc::Vector2D & ball_pos,
                                const rcsc::Vector2D & move_pos );
    public:
    static
    int opponents_in_cross_cone( const rcsc::WorldModel & wm,
                                 const rcsc::Vector2D & ball_pos,
                                 const rcsc::Vector2D & move_pos );
	
    bool throughPassShootHelper(rcsc::PlayerAgent *agent);


};


#endif 
