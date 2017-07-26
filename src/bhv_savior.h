// -*-c++-*-

/*!
  \file bhv_savior.h
  \brief aggressive goalie behavior
*/

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

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

#ifndef BHV_SAVIOR_H
#define BHV_SAVIOR_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/player/abstract_player_object.h>
#include <rcsc/math_util.h>

namespace rcsc {
class PlayerAgent;
class WorldModel;
class Vector2D;
class Rect2D;
class AngleDeg;
}


class Bhv_Savior
    : public rcsc::SoccerBehavior {
  
private:
      
       bool M_is_penalty_kick_mode;
       
public:
      
Bhv_Savior(bool  is_pk_mode=false):
M_is_penalty_kick_mode(is_pk_mode)
 {}
       


public:
    static
    rcsc::AngleDeg getDirFromOurGoal( const rcsc::Vector2D & pos );

    static
    double getTackleProbability( const rcsc::Vector2D & body_relative_ball );

    static
    rcsc::Vector2D getSelfNextPosWithDash( const rcsc::WorldModel & wm,
                                           const double dash_power );

    static
    double getSelfNextTackleProbabilityWithDash( const rcsc::WorldModel & wm,
                                                 const double dash_power );

    static
    double getSelfNextTackleProbabilityWithTurn( const rcsc::WorldModel & wm );


    static
    bool opponentCanShootFrom( const rcsc::WorldModel & wm,
                               const rcsc::Vector2D & pos,
                               const long valid_teammate_threshold = 20 );

    static
    bool canCatchAtNearPost( const rcsc::WorldModel & wm,
                             const rcsc::Vector2D & pos,
                             const rcsc::Vector2D & ball_pos );

    static
    bool canBlockShootFrom( const rcsc::WorldModel & wm,
                            const rcsc::Vector2D & pos );


    static
    double
    getMaxShootAngle( const rcsc::WorldModel & wm,
                      const rcsc::AbstractPlayerCont & teammates_without_self,
                      const rcsc::Vector2D & ball_pos,
                      const rcsc::Vector2D & self_pos,
                      bool * can_shoot = static_cast< bool * >( 0 ) );



private:
    rcsc::Vector2D getBasicPosition( rcsc::PlayerAgent * agent,
                                     const bool is_penalty_kick_mode,
                                     const bool is_despair_situation,
                                     bool * dangerous,
                                     bool * aggressive_positioning,
                                     bool * emergent_advance,
                                     bool * goal_line_positioning ) const;

    rcsc::Vector2D getBasicPositionFromBall( rcsc::PlayerAgent * agent,
                                             const rcsc::Vector2D & ball_pos,
                                             double base_dist,
                                             const rcsc::Vector2D & self_pos,
                                             const bool is_penalty_kick_mode,
                                             const bool is_despair_situation,
                                             bool * dangerous,
                                             const bool emergent_advance,
                                             bool * goal_line_positioning ) const;

    rcsc::Vector2D getGoalLinePositioningTarget( rcsc::PlayerAgent * agent,
                                                 const rcsc::WorldModel & wm,
                                                 const double goal_x_dist,
                                                 const rcsc::Vector2D & ball_pos,
                                                 const bool is_despair_situation ) const;


    static
    bool isGoalLinePositioningSituationBase( const rcsc::WorldModel & wm,
                                             const rcsc::Vector2D & ball_pos,
                                             const bool is_penalty_kick_mode );



public:
    bool doPlayOnMove( rcsc::PlayerAgent * agent,
                       const bool is_penalty_kick_mode = false );

    void setDefaultNeckAction( rcsc::PlayerAgent * agent,
                               const bool is_penalty_kick_mode,
                               const int predict_step );

    bool doCatchIfPossible( rcsc::PlayerAgent * agent );

    bool doTackleIfNecessary( rcsc::PlayerAgent * agent,
                              const bool is_is_despair_situation );

    bool doChaseBallIfNessary( rcsc::PlayerAgent * agent,
                               const bool is_penalty_kick_mode,
                               const bool is_despair_situation,
                               const int self_reach_cycle,
                               const int teammate_reach_cycle,
                               const int opponent_reach_cycle,
                               const rcsc::Vector2D & self_intercept_point,
                               const rcsc::Rect2D & shrinked_penalty_area );

    bool doFindBallIfNecessary( rcsc::PlayerAgent * agent,
                                const int opponent_reach_cycle );

    bool doPositioning( rcsc::PlayerAgent * agent,
                        const rcsc::Vector2D & ball_pos,
                        const bool is_penalty_kick_mode,
                        const bool is_despair_situation );

    bool doGoToPoint( rcsc::PlayerAgent * agent,
                      const rcsc::Vector2D & target_point,
                      const double max_error = 1.0,
                      const double max_power = 100.0,
                      const bool use_back_dash = true,
                      const bool force_back_dash = false,
                      const bool emergency_mode = false );


    bool doChaseBall( rcsc::PlayerAgent * agent );

    bool doGoalLinePositioning( rcsc::PlayerAgent * agent,
                                const rcsc::Vector2D & target_position,
                                const double low_priority_x_position_error,
                                const double max_x_position_error,
                                const double max_y_position_error,
                                const double dash_power );

public:
    bool execute( rcsc::PlayerAgent * agent );
};

#endif
