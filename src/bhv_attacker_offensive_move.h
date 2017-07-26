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

#ifndef TOKYOTECH_BHV_ATTACKER_OFFENSIVE_MOVE_H
#define TOKYOTECH_BHV_ATTACKER_OFFENSIVE_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_AttackerOffensiveMove
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;
    const bool M_forward_player;
public:
    Bhv_AttackerOffensiveMove( const rcsc::Vector2D & home_pos,
                               const bool forward_player )
        : M_home_pos( home_pos )
        , M_forward_player( forward_player )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool throughPassShootHelper(rcsc::PlayerAgent *agent);
    bool doPassRequestMove( rcsc::PlayerAgent * agent );
    bool CF_Voron_through_Pass(rcsc::PlayerAgent *agent,rcsc::Vector2D & target_point);
    bool doForestall( rcsc::PlayerAgent * agent );
    bool doIntercept( rcsc::PlayerAgent * agent );
    bool doAvoidMarker( rcsc::PlayerAgent * agent );


    double getDashPower( const rcsc::PlayerAgent * agent,
                         const rcsc::Vector2D & target_point );


};

#endif
