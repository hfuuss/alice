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

#ifndef BHV_SIDE_HALF_OFFENSIVE_MOVE_H
#define BHV_SIDE_HALF_OFFENSIVE_MOVE_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

namespace rcsc {
class WorldModel;
}

class Bhv_SideHalfOffensiveMove
    : public rcsc::SoccerBehavior {
public:

    Bhv_SideHalfOffensiveMove()
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool doBlock( rcsc::PlayerAgent * agent );
    bool doIntercept( rcsc::PlayerAgent * agent );
    bool doGetBall( rcsc::PlayerAgent * agent );
    bool doPassRequestMove( rcsc::PlayerAgent * agent );
    bool doCrossMove( rcsc::PlayerAgent * agent );
    bool doFreeMove( rcsc::PlayerAgent * agent );
    bool doAvoidMark( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getFreeMoveTargetVoronoi( const rcsc::WorldModel & wm );
    rcsc::Vector2D getFreeMoveTargetGrid( const rcsc::WorldModel & wm );
    void doNormalMove( rcsc::PlayerAgent * agent );

public:

    static
    void go_to_point( rcsc::PlayerAgent * agent,
                      const rcsc::Vector2D & target_point,
                      const double dash_power );
    static
    void set_change_view( rcsc::PlayerAgent * agent );
    static
    void set_turn_neck( rcsc::PlayerAgent * agent );

    static
    double get_dash_power( const rcsc::WorldModel & wm,
                           const rcsc::Vector2D & target_point );

    static
    bool is_marked( const rcsc::WorldModel & wm );

    static
    bool is_triangle_member( const rcsc::WorldModel & wm );

    static
    double evaluate_free_move_point( const int count,
                                     const rcsc::WorldModel & wm,
                                     const rcsc::Vector2D & home_pos,
                                     const rcsc::Vector2D & ball_pos,
                                     const rcsc::Vector2D & move_pos );

};

#endif
