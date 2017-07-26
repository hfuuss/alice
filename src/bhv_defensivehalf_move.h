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
  #ifndef BHV_DEFENSIVEHALF_MOVE
  #define BHV_DEFENSIVEHALF_MOVE
  #include <rcsc/geom/vector_2d.h>
  #include <rcsc/player/soccer_action.h>
  #include <rcsc/player/player_object.h>
  class Bhv_DefensiveHalf_Move
	  : public rcsc::SoccerBehavior {
  private:
      const rcsc::Vector2D M_home_pos;
      bool is_marking;

  public:
      Bhv_DefensiveHalf_Move()
      { }

      bool execute( rcsc::PlayerAgent * agent );

  private:
      bool  Block_BA_DF( rcsc::PlayerAgent * agent );
      bool  Block_BA_DG(rcsc::PlayerAgent * agent );
      bool  EmergencyMove(rcsc::PlayerAgent *agent);
      bool  MarkPlan(rcsc::PlayerAgent *agent);
      bool  Mark(rcsc::PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target);
      bool  PressMark(rcsc::PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target);
      void  doNormalMove( rcsc::PlayerAgent * agent );

  };

  #endif // MOVE_6_H
