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

#ifndef BHV_SIDE_HALF_PASS_REQUEST_MOVE_2013_H
#define BHV_SIDE_HALF_PASS_REQUEST_MOVE_2013_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

namespace rcsc {
class WorldModel;
}

class Bhv_SideHalfPassRequestMove2013
    : public rcsc::SoccerBehavior {
private:
    int M_count;

public:

    Bhv_SideHalfPassRequestMove2013()
        : M_count( 0 )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    void doGoToPoint( rcsc::PlayerAgent * agent,
                      const rcsc::Vector2D & receive_pos,
                      const double dash_power );
    void setTurnNeck( rcsc::PlayerAgent * agent );
    bool doAvoidOffside( rcsc::PlayerAgent * agent,
                         const rcsc::Vector2D & receive_pos,
                         const int offside_reach_step );

    rcsc::Vector2D getReceivePos( const rcsc::WorldModel & wm,
                                  int * result_self_move_step,
                                  int * result_offside_reach_step,
                                  int * result_ball_move_step );
    double getDashPower( const rcsc::WorldModel & wm,
                         const rcsc::Vector2D & receive_pos );

    int getTeammateReachStep( const rcsc::WorldModel & wm );

    bool isMoveSituation( const rcsc::WorldModel & wm );

    bool checkOtherReceiver( const rcsc::WorldModel & wm,
                             const rcsc::Vector2D & receive_pos );
    bool checkOpponent( const rcsc::WorldModel & wm,
                        const rcsc::Vector2D & first_ball_pos,
                        const rcsc::Vector2D & receive_pos,
                        const double first_ball_speed,
                        const int ball_move_step );

};

#endif
