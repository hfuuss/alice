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
#ifndef BHV_OFFENSIVEHALF_MOVE_H
#define BHV_OFFENSIVEHALF_MOVE_H
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_object.h>
class Bhv_OffensiveHalf_Defensive_Move
        : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;

public:
    Bhv_OffensiveHalf_Defensive_Move()
    {  }

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool doGetBall( rcsc::PlayerAgent * agent );
   bool EmergencyMove(rcsc::PlayerAgent *agent);
    void doNormalMove( rcsc::PlayerAgent * agent );
    bool  PressMark(rcsc::PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target);
    bool  MarkPlan(rcsc::PlayerAgent *agent);
    bool  Mark(rcsc::PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target);

};
#endif // MOVE_7_H
