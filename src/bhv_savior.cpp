// -*-c++-*-

/*!
  \file bhv_savior.cpp
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_savior.h"
#include "bhv_basic_tackle.h"
#include "bhv_deflecting_tackle.h"



#include <rcsc/player/player_agent.h>
#include <rcsc/player/penalty_kick_state.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/action/bhv_neck_body_to_ball.h>
#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/view_synch.h>

#include "chain_action/field_analyzer.h"
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/math_util.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <rcsc/action/body_pass.h>
#include "neck_default_intercept_neck.h"

#if 1
# define VISUAL_DEBUG
#endif


//
// each setting switch
//

/*
 * uncomment this to use old condition for decision which positioning mode,
 * normal positioning or goal line positioning
 */
#define DO_AGGRESSIVE_POSITIONING



/*
 * do goal line positioninig instead of normal move if ball is side
 */
//#define DO_GOAL_LINE_POSITIONINIG_AT_SIDE


/*
 * uncomment this to use old condition for decision which positioning mode,
 * normal positioning, goal line positioning, goal parallel positioning
 */
//#define PENALTY_SHOOTOUT_BLOCK_IN_PENALTY_AREA
//#define PENALTY_SHOOTOUT_GOAL_PARALLEL_POSITIONING

using namespace rcsc;

namespace {
static const double EMERGENT_ADVANCE_DIR_DIFF_THRESHOLD = 3.0; //degree
static const double SHOOT_ANGLE_THRESHOLD = 10.0; // degree
static const double SHOOT_DIST_THRESHOLD = 40.0; // meter
static const int SHOOT_BLOCK_TEAMMATE_POS_COUNT_THRESHOLD = 20; // steps
}

bool
Bhv_Savior::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_Savior::execute()" );

    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

#ifdef VISUAL_DEBUG
    if ( opponentCanShootFrom( wm, wm.ball().pos(), 20 ) )
    {
        agent->debugClient().addLine( wm.ball().pos(),
                                      Vector2D( SP.ourTeamGoalLineX(),
                                                -SP.goalHalfWidth() ) );
        agent->debugClient().addLine( wm.ball().pos(),
                                      Vector2D( SP.ourTeamGoalLineX(),
                                                +SP.goalHalfWidth() ) );
    }
#endif



     return this->doPlayOnMove( agent,M_is_penalty_kick_mode);
    
}

bool
Bhv_Savior::doPlayOnMove( PlayerAgent * agent,
                          const bool is_penalty_kick_mode )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    //
    // set some parameters for thinking
    //
    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();
    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int predict_step = std::min( opponent_reach_cycle,
                                       std::min( teammate_reach_cycle,
                                                 self_reach_cycle ) );
    const Vector2D self_intercept_point = wm.ball().inertiaPoint( self_reach_cycle );
    const Vector2D ball_pos
        = FieldAnalyzer::get_field_bound_predict_ball_pos( wm, predict_step, 0.5 );



    //
    // set default neck action
    //
    setDefaultNeckAction( agent, is_penalty_kick_mode, predict_step );


    //
    // if catchable, do catch
    //
    if ( doCatchIfPossible( agent ) )
    {
        return true;
    }


    //
    // if kickable, do kick
    //
    if ( wm.self().isKickable() )
    {
      
      Body_ClearBall().execute(agent);
      return true;

    }


    agent->debugClient().addMessage( "ball dist=%f", wm.ball().distFromSelf() );


    //
    // set parameters
    //
    const Rect2D shrinked_penalty_area
        ( Vector2D( SP.ourTeamGoalLineX(),
                    - (SP.penaltyAreaHalfWidth() - 1.0) ),
          Size2D( SP.penaltyAreaLength() - 1.0,
                  (SP.penaltyAreaHalfWidth() - 1.0) * 2.0 ) );

    const bool is_shoot_ball
        = FieldAnalyzer::is_ball_moving_to_our_goal( wm.ball().pos(),
                                                     wm.ball().vel(),
                                                     1.0 );
    dlog.addText( Logger::TEAM,
                  __FILE__": is_shoot_ball = %s",
                  ( is_shoot_ball ? "true" : "false" ) );

#ifdef VISUAL_DEBUG
    if ( is_shoot_ball )
    {
        agent->debugClient().addLine( wm.self().pos() + Vector2D( -2.0, -2.0 ),
                                      wm.self().pos() + Vector2D( -2.0, +2.0 ) );
    }
#endif

    const bool is_despair_situation
        = ( is_shoot_ball && self_intercept_point.x <= SP.ourTeamGoalLineX() );



    //
    // tackle
    //
    if ( doTackleIfNecessary( agent, is_despair_situation ) )
    {
        return true;
    }


    //
    // chase ball
    //
    if ( doChaseBallIfNessary( agent,
                               is_penalty_kick_mode,
                               is_despair_situation,
                               self_reach_cycle,
                               teammate_reach_cycle,
                               opponent_reach_cycle,
                               self_intercept_point,
                               shrinked_penalty_area ) )
    {
        return true;
    }


    //
    // check ball
    //
    if ( doFindBallIfNecessary( agent, opponent_reach_cycle ) )
    {
        return true;
    }


    //
    // positioning
    //
    if ( doPositioning( agent, ball_pos,
                        is_penalty_kick_mode, is_despair_situation ) )
    {
        return true;
    }


    agent->doTurn( 0.0 );
    return true;
}

void
Bhv_Savior::setDefaultNeckAction( rcsc::PlayerAgent * agent,
                                  const bool is_penalty_kick_mode,
                                  const int predict_step )
{
    const WorldModel & wm = agent->world();

    if ( is_penalty_kick_mode )
    {
        agent->setNeckAction( new Neck_ChaseBall() );
    }
    else if ( wm.ball().pos().x > 0.0 )
    {
        agent->setNeckAction( new Neck_ScanField() );
    }
    else if ( wm.ball().seenPosCount() == 0 )
    {
        if ( predict_step >= 10 )
        {
            agent->setNeckAction( new Neck_ScanField() );
        }
        else if ( predict_step > 3 )
        {
            agent->setNeckAction( new Neck_TurnToBall() );
        }
        else
        {
            agent->setNeckAction( new Neck_ChaseBall() );
        }
    }
    else
    {
        agent->setNeckAction( new Neck_ChaseBall() );
    }
}

bool
Bhv_Savior::doPositioning( rcsc::PlayerAgent * agent,
                           const Vector2D & ball_pos,
                           const bool is_penalty_kick_mode,
                           const bool is_despair_situation )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const Vector2D & current_ball_pos = wm.ball().pos();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();



    bool dangerous;
    bool aggressive;
    bool emergent_advance;
    bool goal_line_positioning;

    const Vector2D best_position = this->getBasicPosition
        ( agent,
          is_penalty_kick_mode,
          is_despair_situation,
          &dangerous,
          &aggressive,
          &emergent_advance,
          &goal_line_positioning );


#ifdef VISUAL_DEBUG
    if ( emergent_advance )
    {
        const Vector2D hat_base_vec = ( best_position - wm.self().pos() ).normalizedVector();

        // draw arrow
        agent->debugClient().addLine( wm.self().pos() + hat_base_vec * 3.0,
                                      wm.self().pos() + hat_base_vec * 3.0
                                      + Vector2D::polar2vector
                                      ( 1.5, hat_base_vec.th() + 150.0 ) );
        agent->debugClient().addLine( wm.self().pos() + hat_base_vec * 3.0,
                                      wm.self().pos() + hat_base_vec * 3.0
                                      + Vector2D::polar2vector
                                      ( 1.5, hat_base_vec.th() - 150.0 ) );
        agent->debugClient().addLine( wm.self().pos() + hat_base_vec * 4.0,
                                      wm.self().pos() + hat_base_vec * 4.0
                                      + Vector2D::polar2vector
                                      ( 1.5, hat_base_vec.th() + 150.0 ) );
        agent->debugClient().addLine( wm.self().pos() + hat_base_vec * 4.0,
                                      wm.self().pos() + hat_base_vec * 4.0
                                      + Vector2D::polar2vector
                                      ( 1.5, hat_base_vec.th() - 150.0 ) );
    }

    if ( goal_line_positioning )
    {
        agent->debugClient().addLine( Vector2D
                                      ( SP.ourTeamGoalLineX() - 1.0,
                                        - SP.goalHalfWidth() ),
                                      Vector2D
                                      ( SP.ourTeamGoalLineX() - 1.0,
                                        + SP.goalHalfWidth() ) );
    }

    agent->debugClient().addLine( wm.self().pos(), best_position );
#endif


    double max_position_error = 0.70;
    double goal_line_positioning_y_max_position_error = 0.3;
    double dash_power = SP.maxDashPower();

    dlog.addText( Logger::TEAM,
                  __FILE__": (max_position_error) base = %f",
                  max_position_error );

    if ( aggressive )
    {
        if ( wm.ball().distFromSelf() >= 30.0 )
        {
            max_position_error
                = bound( 2.0, 2.0 + (wm.ball().distFromSelf() - 30.0) / 20.0, 3.0 );
            dlog.addText( Logger::TEAM,
                          __FILE__": (max_position_error) aggressive, ball is far %f",
                          max_position_error );
        }
    }


    const double diff = (best_position - wm.self().pos()).r();

    if ( wm.ball().distFromSelf() >= 30.0
         && wm.ball().pos().x >= -20.0
         && diff < 5.0
         && ! is_penalty_kick_mode )
    {
        dash_power = SP.maxDashPower() * 0.7;

#ifdef VISUAL_DEBUG
        agent->debugClient().addLine
            ( wm.self().pos() + Vector2D( -1.0, +3.0 ),
              wm.self().pos() + Vector2D( +1.0, +3.0 ) );

        agent->debugClient().addLine
            ( wm.self().pos() + Vector2D( -1.0, -3.0 ),
              wm.self().pos() + Vector2D( +1.0, -3.0 ) );
#endif
    }

    if ( emergent_advance )
    {
        if ( diff >= 10.0 )
        {
            // for speedy movement
            max_position_error = 1.9;
        }
        else if ( diff >= 5.0 )
        {
            // for speedy movement
            max_position_error = 1.0;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": (max_position_error) emergent_advance, diff=%f, %f",
                      diff, max_position_error );
#ifdef VISUAL_DEBUG
        agent->debugClient().addCircle( wm.self().pos(), 3.0 );
#endif
    }
    else if ( goal_line_positioning )
    {
        if ( is_penalty_kick_mode )
        {
            max_position_error = 0.7;
            goal_line_positioning_y_max_position_error = 0.5;
        }
        else
        {
            if ( std::fabs( wm.self().pos().x - best_position.x ) > 5.0 )
            {
                max_position_error = 2.0;
                goal_line_positioning_y_max_position_error = 1.5;

                dlog.addText( Logger::TEAM,
                              __FILE__": (max_position_error) goal_line_positioning %f",
                              max_position_error );
            }

            if ( ball_pos.x > -10.0
                 && current_ball_pos.x > +10.0
                 && wm.ourDefenseLineX() > -20.0
                 && std::fabs( wm.self().pos().x - best_position.x ) < 5.0 )
            {
                // safety, save stamina
                dash_power = SP.maxDashPower() * 0.5;
                max_position_error = 2.0;
                goal_line_positioning_y_max_position_error = 1.5;

                dlog.addText( Logger::TEAM,
                              __FILE__": (max_position_error) goal_line_positioning, save stamina condition1 max_position_error = %f",
                              max_position_error );
            }
            else if ( ball_pos.x > -20.0
                      && current_ball_pos.x > +20.0
                      && wm.ourDefenseLineX() > -25.0
                      && teammate_reach_cycle < opponent_reach_cycle
                      && std::fabs( wm.self().pos().x - best_position.x ) < 5.0 )
            {
                // safety, save stamina
                dash_power = SP.maxDashPower() * 0.6;
                max_position_error = 1.8;
                goal_line_positioning_y_max_position_error = 1.5;

                dlog.addText( Logger::TEAM,
                              __FILE__": (max_position_error) goal_line_positioning, save stamina condition2 max_position_error = %f",
                              max_position_error );
            }
        }
    }
    else if ( dangerous )
    {
        if ( diff >= 10.0 )
        {
            // for speedy movement
            max_position_error = 1.9;
        }
        else if ( diff >= 5.0 )
        {
            // for speedy movement
            max_position_error = 1.0;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": (max_position_error) dangerous, %f",
                      max_position_error );
    }
    else
    {
        if ( diff >= 10.0 )
        {
            // for speedy movement
            max_position_error = 1.5;
        }
        else if ( diff >= 5.0 )
        {
            // for speedy movement
            max_position_error = 1.0;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": (max_position_error) normal update, diff=%f, %f",
                      diff, max_position_error );
    }

    dash_power = wm.self().getSafetyDashPower( dash_power );

    if ( is_despair_situation )
    {
        if ( max_position_error < 1.5 )
        {
            max_position_error = 1.5;

            dlog.addText( Logger::TEAM,
                          __FILE__": Savior: is_despair_situation, "
                          "position error changed to %f",
                          max_position_error );
        }
    }


    //
    // update dash power by play mode
    //
    {
        const GameMode mode = wm.gameMode();

        if ( mode.type() == GameMode::KickIn_
             || mode.type() == GameMode::OffSide_
             || mode.type() == GameMode::CornerKick_
             || ( ( mode.type() == GameMode::GoalKick_
                    || mode.type() == GameMode::GoalieCatch_
                    || mode.type() == GameMode::BackPass_ )
                  && mode.side() == wm.ourSide() )
             || ( mode.type() == GameMode::FreeKick_ ) )
        {
            max_position_error = 5.0;

            if ( wm.ball().pos().x > ServerParam::i().ourPenaltyAreaLineX() + 10.0 )
            {
                dash_power = std::min( dash_power, wm.self().playerType().staminaIncMax() );
            }

            dlog.addText( Logger::TEAM,
                          __FILE__": Savior: updated max_position_error"
                          " and dash_power by game mode,"
                          " max_position_error = %f, dash_power = %f",
                          max_position_error, dash_power );
        }
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": Savior: positioning target = [%f, %f], "
                  "max_position_error = %f, dash_power = %f",
                  best_position.x, best_position.y,
                  max_position_error, dash_power );

    const bool force_back_dash = false;

    if ( goal_line_positioning
         && ! is_despair_situation )
    {
        if ( doGoalLinePositioning( agent ,
                                    best_position,
                                    2.0,
                                    max_position_error,
                                    goal_line_positioning_y_max_position_error,
                                    dash_power ) )
        {
            return true;
        }
    }
    else
    {
        if ( doGoToPoint( agent,
                          best_position,
                          max_position_error,
                          dash_power,
                          true,
                          force_back_dash,
                          is_despair_situation ) )
        {
            agent->debugClient().addMessage( "Savior:Positioning" );
#ifdef VISUAL_DEBUG
            agent->debugClient().setTarget( best_position );
            agent->debugClient().addCircle( best_position, max_position_error );
#endif
            dlog.addText( Logger::TEAM,
                          __FILE__ ": go to (%.2f %.2f) error=%.3f  dash_power=%.1f  force_back=%d",
                          best_position.x, best_position.y,
                          max_position_error,
                          dash_power,
                          static_cast< int >( force_back_dash ) );
            return true;
        }
    }


    //
    // emergency stop
    //
    if ( opponent_reach_cycle <= 1
         && (current_ball_pos - wm.self().pos()).r() < 10.0
         && wm.self().vel().r() >= 0.05 )
    {
        if ( Body_StopDash( true ).execute( agent ) )
        {
            agent->debugClient().addMessage( "Savior:EmergemcyStop" );
            dlog.addText( Logger::TEAM,
                          __FILE__ ": emergency stop" );
            return true;
        }
    }


    //
    // go to position with minimum error
    //
    const PlayerObject * const firstAccessOpponent
        = wm.interceptTable()->fastestOpponent();

    if ( (firstAccessOpponent
          && ( firstAccessOpponent->pos() - wm.self().pos() ).r() >= 5.0)
         && opponent_reach_cycle >= 3 )
    {
        if ( goal_line_positioning
             && ! is_despair_situation )
        {
            if ( doGoalLinePositioning( agent,
                                        best_position,
                                        2.0,
                                        0.3,
                                        0.3,
                                        dash_power ) )
            {
                return true;
            }
        }
        else
        {
           const double dist_thr = ( wm.gameMode().type() == GameMode::PlayOn
                                      ? 0.3
                                      : std::max( 0.3, wm.ball().distFromSelf() * 0.05 ) );

            if ( doGoToPoint( agent,
                              best_position,
                              dist_thr,
                              dash_power,
                              true,
                              false,
                              is_despair_situation ) )
            {
                agent->debugClient().addMessage( "Savior:TunePositioning" );
#ifdef VISUAL_DEBUG
                agent->debugClient().setTarget( best_position );
                agent->debugClient().addCircle( best_position, 0.3 );
#endif
                dlog.addText( Logger::TEAM,
                              __FILE__ ": go to position with minimum error. target=(%.2f %.2f) dash_power=%.1f",
                              best_position.x, best_position.y,
                              dash_power );
                return true;
            }
        }
    }


    //
    // stop
    //
    if ( wm.self().vel().rotatedVector( - wm.self().body() ).absX() >= 0.01 )
    {
        if ( Body_StopDash( true ).execute( agent ) )
        {
            agent->debugClient().addMessage( "Savior:Stop" );
            dlog.addText( Logger::TEAM,
                          __FILE__ ": stop. line=%d", __LINE__ );
            return true;
        }
    }


    //
    // turn body angle against ball
    //
    const Vector2D final_body_pos = wm.self().inertiaFinalPoint();

    AngleDeg target_body_angle = 0.0;

#ifdef PENALTY_SHOOTOUT_BLOCK_IN_PENALTY_AREA
    if ( ((ball_pos - final_body_pos).r() <= 12.0
#else
    if ( ((ball_pos - final_body_pos).r() <= 5.0
#endif
          && (wm.existKickableOpponent() || wm.existKickableTeammate()))
         || goal_line_positioning )
    {
        if ( goal_line_positioning )
        {
            target_body_angle = sign( final_body_pos.y ) * 90.0;
        }
        else
        {
            AngleDeg diff_body_angle = 0.0;

            if ( final_body_pos.y > 0.0 )
            {
                diff_body_angle = + 90.0;
            }
            else
            {
                diff_body_angle = - 90.0;
            }

            if ( final_body_pos.x <= -45.0 )
            {
                diff_body_angle *= -1.0;
            }

            target_body_angle = (ball_pos - final_body_pos).th()
                + diff_body_angle;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__ ": target body angle = %.2f."
                      " final_body_pos = (%.2f %.2f)",
                      target_body_angle.degree(),
                      final_body_pos.x, final_body_pos.y );
    }

#if 0
    if ( ( ! is_penalty_kick_mode
           || ( target_body_angle - wm.self().body() ).abs() > 10.0 ) )
#else
    if ( ( target_body_angle - wm.self().body() ).abs() > 10.0 )
#endif
    {
        if ( Body_TurnToAngle( target_body_angle ).execute( agent ) )
        {
            agent->debugClient().addMessage( "Savior:TurnTo%.0f",
                                             target_body_angle.degree() );
            dlog.addText( Logger::TEAM,
                          __FILE__ ": turn to angle %.1f",
                          target_body_angle.degree() );
            return true;
        }
    }


#if ! defined( PENALTY_SHOOTOUT_BLOCK_IN_PENALTY_AREA ) && ! defined( PENALTY_SHOOTOUT_GOAL_PARALLEL_POSITIONING )
    if ( is_penalty_kick_mode )
    {
        double dash_dir;

        if ( wm.self().body().degree() > 0.0 )
        {
            dash_dir = -90.0;
        }
        else
        {
            dash_dir = +90.0;
        }

        agent->doDash( SP.maxPower(), dash_dir );
        agent->debugClient().addMessage( "Savior:SideChase" );
        dlog.addText( Logger::TEAM,
                      __FILE__ ": side chase" );
        return true;
    }
#endif


    if ( Body_TurnToAngle( target_body_angle ).execute( agent ) )
    {
        agent->debugClient().addMessage( "Savior:AdjustTurnTo%.0f",
                                         target_body_angle.degree() );
        dlog.addText( Logger::TEAM,
                      __FILE__ ": adjust turn to angle %.1f",
                      target_body_angle.degree() );
        return true;
    }

    return false;
}


bool
Bhv_Savior::doCatchIfPossible( rcsc::PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const double MAX_SELF_BALL_ERROR = 0.5; // meter


    ///////////////////////////////////////////////
    // check if catchabale
    Rect2D our_penalty( Vector2D( -SP.pitchHalfLength(),
                                  -SP.penaltyAreaHalfWidth() + 1.0 ),
                        Size2D( SP.penaltyAreaLength() - 1.0,
                                SP.penaltyAreaWidth() - 2.0 ) );
    
    
    if ( wm.ball().distFromSelf() < SP.catchableArea() - 0.05
         && our_penalty.contains( wm.ball().pos() ) )
    {
        
        return agent->doCatch();
    }

    return false;
}

bool
Bhv_Savior::doTackleIfNecessary( rcsc::PlayerAgent * agent,
                                 const bool is_despair_situation )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    double tackle_prob_threshold = 0.8;

    if ( is_despair_situation )
    {
#ifdef VISUAL_DEBUG
        agent->debugClient().addLine( Vector2D
                                      ( SP.ourTeamGoalLineX() - 2.0,
                                        - SP.goalHalfWidth() ),
                                      Vector2D
                                      ( SP.ourTeamGoalLineX() - 2.0,
                                        + SP.goalHalfWidth() ) );
#endif

        tackle_prob_threshold = EPS;

        const Vector2D ball_next = wm.ball().inertiaPoint( 1 );
        const bool next_step_goal = ( ball_next.x
                                      < SP.ourTeamGoalLineX() + SP.ballRand() );

        if ( ! next_step_goal )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": next step, ball is in goal,"
                          " ball = [%f, %f], ball rand = %f = %f",
                          ball_next.x, ball_next.y,
                          SP.ballRand() );

            const double next_tackle_prob_forward
                = getSelfNextTackleProbabilityWithDash( wm, + SP.maxDashPower() );
            const double next_tackle_prob_backword
                = getSelfNextTackleProbabilityWithDash( wm, SP.minDashPower() );

            dlog.addText( Logger::TEAM,
                          __FILE__": next_tackle_prob_forward = %f",
                          next_tackle_prob_forward );
            dlog.addText( Logger::TEAM,
                          __FILE__": next_tackle_prob_backword = %f",
                          next_tackle_prob_backword );

            if ( next_tackle_prob_forward > wm.self().tackleProbability() )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": dash forward expect next tackle" );

                agent->doDash( SP.maxDashPower() );

                return true;
            }
            else if ( next_tackle_prob_backword > wm.self().tackleProbability() )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": dash backward expect next tackle" );
                agent->doDash( SP.minDashPower() );

                return true;
            }
        }

        if ( ! next_step_goal )
        {
            const double next_tackle_prob_with_turn
                = getSelfNextTackleProbabilityWithTurn( wm );

            dlog.addText( Logger::TEAM,
                          __FILE__": next_tackle_prob_with_turn = %f",
                          next_tackle_prob_with_turn );
            if ( next_tackle_prob_with_turn > wm.self().tackleProbability() )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": turn to next ball position for tackling" );
                Body_TurnToPoint( ball_next, 1 ).execute( agent );
                return true;
            }
        }
    }

    if ( is_despair_situation )
    {
        if ( Bhv_DeflectingTackle( 0.5, true ).execute( agent ) )
        {
            agent->debugClient().addMessage( "Savior:DeflectingTackle(%f)",
                                             wm.self().tackleProbability() );
            return true;
        }
    }

    if ( Bhv_BasicTackle( 0.85, 160.0 ).execute( agent ) )
    {
        agent->debugClient().addMessage( "Savior:Tackle(%f)",
                                         wm.self().tackleProbability() );
        return true;
    }

    return false;
}

bool
Bhv_Savior::doChaseBallIfNessary( PlayerAgent * agent,
                                  const bool is_penalty_kick_mode,
                                  const bool is_despair_situation,
                                  const int self_reach_cycle,
                                  const int teammate_reach_cycle,
                                  const int opponent_reach_cycle,
                                  const Vector2D & self_intercept_point,
                                  const Rect2D & shrinked_penalty_area )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    if ( is_despair_situation )
    {
#ifdef VISUAL_DEBUG
        agent->debugClient().addLine( wm.self().pos()
                                      + Vector2D( +2.0, -2.0 ),
                                      wm.self().pos()
                                      + Vector2D( +2.0, +2.0 ) );
#endif

        agent->debugClient().addMessage( "Savior:DespairChase" );
        if ( this->doChaseBall( agent ) )
        {
            return true;
        }
    }


    if ( wm.gameMode().type() == GameMode::PlayOn
         && ! wm.existKickableTeammate() )
    {
        if ( self_reach_cycle + 1 < opponent_reach_cycle
             && ( ( self_reach_cycle + 5 <= teammate_reach_cycle
                    || self_intercept_point.absY() < SP.penaltyAreaHalfWidth() - 1.0 )
                  && shrinked_penalty_area.contains( self_intercept_point ) )
             && ( self_reach_cycle <= teammate_reach_cycle
                  || ( shrinked_penalty_area.contains( self_intercept_point )
                       && self_reach_cycle <= teammate_reach_cycle + 3 ) )
             )
        {
            agent->debugClient().addMessage( "Savior:Chase" );

            if ( this->doChaseBall( agent ) )
            {
                return true;
            }
        }

#if 1
        // 2009-12-13 akiyama
        if ( self_reach_cycle + 3 < opponent_reach_cycle
             && self_reach_cycle + 2 <= teammate_reach_cycle
             && self_intercept_point.x > SP.ourPenaltyAreaLineX()
             && self_intercept_point.absY() < SP.penaltyAreaHalfWidth() - 1.0 )
        {
            agent->debugClient().addMessage( "Savior:ChaseAggressive" );

            if ( this->doChaseBall( agent ) )
            {
                return true;
            }
        }
#endif
    }


    if ( is_penalty_kick_mode
         && ( self_reach_cycle + 1 < opponent_reach_cycle
#ifdef PENALTY_SHOOTOUT_BLOCK_IN_PENALTY_AREA
              && ( shrinked_penalty_area.contains( self_intercept_point )
                   || self_intercept_point.x <= SP.ourTeamGoalLineX() )
#endif
              ) )
    {
        agent->debugClient().addMessage( "Savior:ChasePenaltyKickMode" );

        if ( Body_Intercept().execute( agent ) )
        {
            agent->setNeckAction( new Neck_ChaseBall() );

            return true;
        }
    }

    return false;
}

bool
Bhv_Savior::doFindBallIfNecessary( PlayerAgent * agent,
                                   const int opponent_reach_cycle )
{
    const WorldModel & wm = agent->world();

    long ball_premise_accuracy = 2;
    bool brief_ball_check = false;

    if ( wm.self().pos().x >= -30.0 )
    {
        ball_premise_accuracy = 6;
        brief_ball_check = true;
    }
    else if ( opponent_reach_cycle >= 3 )
    {
        ball_premise_accuracy = 3;
        brief_ball_check = true;
    }

    if ( wm.ball().seenPosCount() > ball_premise_accuracy
         || ( brief_ball_check
              && wm.ball().posCount() > ball_premise_accuracy ) )
    {
        agent->debugClient().addMessage( "Savior:FindBall" );
        dlog.addText( Logger::TEAM,
                      __FILE__ ": find ball" );
        return Bhv_NeckBodyToBall().execute( agent );
    }

    return false;
}

bool
Bhv_Savior::doGoToPoint( PlayerAgent * agent,
                         const rcsc::Vector2D & target_point,
                         double max_error,
                         double max_power,
                         bool use_back_dash,
                         bool force_back_dash,
                         bool emergency_mode )
{
    const WorldModel & wm = agent->world();
    const GameMode & mode = wm.gameMode();

    if ( mode.type() == GameMode::BeforeKickOff 
         || mode.type() == GameMode::AfterGoal_ )
    {
        if ( wm.self().pos().dist( target_point ) < max_error )
        {
            return false;
        }

        agent->doMove( target_point.x, target_point.y );
        return true;
    }



    return Body_GoToPoint( target_point,
                           max_error,
                           max_power ).execute( agent );

}



bool
Bhv_Savior::doGoalLinePositioning( PlayerAgent * agent,
                                   const Vector2D & target_position,
                                   const double low_priority_x_position_error,
                                   const double max_x_position_error,
                                   const double max_y_position_error,
                                   const double dash_power )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const double dist = target_position.dist( wm.self().pos() );
    const double x_diff = std::fabs( target_position.x - wm.self().pos().x );
    const double y_diff = std::fabs( target_position.y - wm.self().pos().y );

    if ( x_diff < max_x_position_error
         && y_diff < max_y_position_error )
    {
        return false;
    }

    Vector2D p = target_position;
    const bool use_back_dash = ( agent->world().ball().pos().x <= -20.0 );

    const bool x_near = ( x_diff < max_x_position_error );
    const bool y_near = ( y_diff < max_y_position_error );


#ifdef VISUAL_DEBUG
    agent->debugClient().addRectangle( Rect2D::from_center
                                       ( target_position,
                                         max_x_position_error * 2.0,
                                         max_y_position_error * 2.0 ) );
#endif

    dlog.addText( Logger::TEAM,
                  __FILE__": (doGoalLinePositioning): "
                  "max_position_error = %f, "
                  "enlarged_max_position_error = %f, "
                  "dist = %f, "
                  "x_diff = %f, y_diff = %f, "
                  "x_near = %s, y_near = %s",
                  max_x_position_error,
                  max_y_position_error,
                  dist,
                  x_diff, y_diff,
                  ( x_near ? "true" : "false" ),
                  ( y_near ? "true" : "false" ) );

    if ( x_diff > low_priority_x_position_error )
    {
        if ( doGoToPoint( agent,
                          p,
                          std::min( max_x_position_error,
                                    max_y_position_error ),
                          dash_power,
                          use_back_dash,
                          false ) )
        {
            agent->debugClient().addMessage( "Savior:GoalLinePositioning:normal" );
#ifdef VISUAL_DEBUG
            agent->debugClient().setTarget( p );
#endif
            dlog.addText( Logger::TEAM,
                          __FILE__ ": go to (%.2f %.2f) error=%.3f dash_power=%.1f",
                          p.x, p.y,
                          std::min( max_x_position_error,
                                    max_y_position_error ),
                          dash_power );

            return true;
        }
    }
    else if ( ! y_near )
    {
        p.assign( wm.self().pos().x, p.y );

        if ( doGoToPoint( agent,
                          p,
                          std::min( max_x_position_error,
                                    max_y_position_error ),
                          dash_power,
                          use_back_dash,
                          false ) )
        {
            agent->debugClient().addMessage( "Savior:GoalLinePositioning:AdjustY" );
#ifdef VISUAL_DEBUG
            agent->debugClient().setTarget( p );
#endif
            dlog.addText( Logger::TEAM,
                          __FILE__ ": go to (%.2f %.2f) error=%.3f dash_power=%.1f",
                          p.x, p.y, low_priority_x_position_error, dash_power );
            return true;
        }
    }
    else
    {
        const double dir_target = (wm.self().body().degree() > 0.0
                                   ? +90.0
                                   : -90.0 );

        if ( std::fabs( wm.self().body().degree() - dir_target ) > 2.0 )
        {
            if ( Body_TurnToAngle( dir_target ).execute( agent ) )
            {
                return true;
            }
        }


        double side_dash_dir = ( wm.self().body().degree() > 0.0
                                 ? +90.0
                                 : -90.0 );
        if ( wm.self().pos().x < target_position.x )
        {
            side_dash_dir *= -1.0;
        }

        const double side_dash_rate = wm.self().dashRate() * SP.dashDirRate( side_dash_dir );

        double side_dash_power = x_diff / std::max( side_dash_rate, EPS );
        side_dash_power = std::min( side_dash_power, dash_power );

        agent->doDash( side_dash_power, side_dash_dir );
        agent->debugClient().addMessage( "Savior:GoalLinePositioning:SideDash" );
        dlog.addText( Logger::TEAM,
                      __FILE__ ": goal line posisitioning side dash" );

        return true;
    }

    return false;
}



Vector2D
Bhv_Savior::getBasicPosition( PlayerAgent * agent,
                              const bool is_penalty_kick_mode,
                              const bool is_despair_situation,
                              bool * dangerous,
                              bool * aggressive,
                              bool * emergent_advance,
                              bool * goal_line_positioning ) const
{
    *dangerous = false;
    *aggressive = false;
    *emergent_advance = false;
    *goal_line_positioning = false;

    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();
    const int predict_step = std::min( opponent_reach_cycle,
                                       std::min( teammate_reach_cycle,
                                                 self_reach_cycle ) );

    const Vector2D self_pos = wm.self().pos();
    const Vector2D ball_pos
        = FieldAnalyzer::get_field_bound_predict_ball_pos( wm, predict_step, 0.5 );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) ball predict pos = (%f,%f)",
                  ball_pos.x, ball_pos.y );

    const Rect2D penalty_area( Vector2D( SP.ourTeamGoalLineX(),
                                   - SP.penaltyAreaHalfWidth() ),
                               Size2D( SP.penaltyAreaLength(),
                                       SP.penaltyAreaWidth() ) );

    const Rect2D conservative_area( Vector2D( SP.ourTeamGoalLineX(),
                                              - SP.pitchHalfWidth() ),
                                    Size2D( SP.penaltyAreaLength() + 15.0,
                                            SP.pitchWidth() ) );

    const Vector2D goal_pos = SP.ourTeamGoalPos();

    const size_t num_teammates_in_conservative_area
        = wm.getPlayerCont( new AndPlayerPredicate
                            ( new TeammatePlayerPredicate( wm ),
                              new ContainsPlayerPredicate< Rect2D >
                              ( conservative_area ) ) ).size();

    const bool ball_is_in_conservative_area
        = conservative_area.contains( wm.ball().pos() );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) number of teammates in conservative area = %d",
                  (int)num_teammates_in_conservative_area );
    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) ball in area = %d",
                  (int)(conservative_area.contains( wm.ball().pos() )) );


    double base_dist = 14.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) basic base dist = %f",
                  base_dist );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) ourDefenseLineX = %f",
                  wm.ourDefenseLineX() );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getBasicPosition) ball dir from goal = %f",
                  getDirFromOurGoal( ball_pos ).degree() );

    *goal_line_positioning = isGoalLinePositioningSituationBase( wm, ball_pos, is_penalty_kick_mode );

    // if ball is not in penalty area and teammates are in penalty area
    // (e.g. our clear ball succeded), do back positioning
    if ( ! ball_is_in_conservative_area
         && num_teammates_in_conservative_area >= 2 )
    {
        if ( ( goal_pos - ball_pos ).r() > 20.0 )
        {
            base_dist = std::min( base_dist,
                                  5.0 + ( ( goal_pos - ball_pos ).r() - 20.0 ) * 0.1 );
        }
        else
        {
            base_dist = 5.0;
        }
    }

    // aggressive positioning
    const double additional_forward_positioning_max = 14.0;

    const double emergent_advance_dist = (goal_pos - ball_pos).r() - 3.0;

    const AbstractPlayerObject * const fastestOpponent = wm.interceptTable()->fastestOpponent();
    // if opponent player will be have the ball at backward of our
    // defense line, do 1-on-1 aggressive defense
    if ( true
         && ball_pos.x >= SP.ourPenaltyAreaLineX() - 5.0
         && ! *goal_line_positioning
         && opponent_reach_cycle < teammate_reach_cycle
         && ball_pos.x <= wm.ourDefenseLineX() + 5.0
         && (getDirFromOurGoal( ball_pos )
             - getDirFromOurGoal( wm.self().inertiaPoint(1) )).abs()
            < EMERGENT_ADVANCE_DIR_DIFF_THRESHOLD
         && opponentCanShootFrom( wm, ball_pos, 20 )
         && ! is_penalty_kick_mode
         && fastestOpponent
         && wm.getPlayerCont
         ( new AndPlayerPredicate
           ( new TeammatePlayerPredicate( wm ),
             new XCoordinateBackwardPlayerPredicate
             ( fastestOpponent->pos().x ),
             new YCoordinatePlusPlayerPredicate
             ( fastestOpponent->pos().y - 1.0 ),
             new YCoordinateMinusPlayerPredicate
             ( fastestOpponent->pos().y + 1.0 ) ) ).empty()
         && getDirFromOurGoal( ball_pos ).abs() < 20.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getBasicPosition) emergent advance mode" );
        *emergent_advance = true;

        agent->debugClient().addMessage( "EmergentAdvance" );
    }


    if ( is_penalty_kick_mode )
    {
#if defined( PENALTY_SHOOTOUT_BLOCK_IN_PENALTY_AREA )
        base_dist = 15.3;
#else
        base_dist = (goal_pos - ball_pos).r() - 4.5;

        if ( (wm.self().pos() - goal_pos).r() > base_dist )
        {
            base_dist = (wm.self().pos() - goal_pos).r();
        }
#endif

        dlog.addText( Logger::TEAM,
                      __FILE__": (getBasicPosition) penalty kick mode, "
                      "new base dist = %f", base_dist );
    }
    else if ( *emergent_advance )
    {
        base_dist = emergent_advance_dist;
    }
    else if ( wm.ourDefenseLineX() >= - additional_forward_positioning_max )
    {
        static const double AGGRESSIVE_POSITIONING_STAMINA_RATE_THRESHOLD = 0.6;

        if ( wm.self().stamina()
             >= SP.staminaMax() * AGGRESSIVE_POSITIONING_STAMINA_RATE_THRESHOLD )
        {
            base_dist += std::min( wm.ourDefenseLineX(), 0.0 )
                + additional_forward_positioning_max;

            dlog.addText( Logger::TEAM,
                          __FILE__ ": (getBasicPosition) aggressive positioning,"
                          " base_dist = %f",
                          base_dist );
            agent->debugClient().addMessage( "AggressivePositioniong" );

            *aggressive = true;
        }
    }
    else
    {
        *aggressive = false;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__ ": (getBasicPosition) base dist = %f", base_dist );

    *dangerous = false;

    return getBasicPositionFromBall( agent,
                                     ball_pos,
                                     base_dist,
                                     wm.self().pos(),
                                     is_penalty_kick_mode,
                                     is_despair_situation,
                                     dangerous,
                                     *emergent_advance,
                                     goal_line_positioning );
}


Vector2D
Bhv_Savior::getBasicPositionFromBall( PlayerAgent * agent,
                                      const Vector2D & ball_pos,
                                      const double base_dist,
                                      const Vector2D & self_pos,
                                      const bool is_penalty_kick_mode,
                                      const bool is_despair_situation,
                                      bool * dangerous,
                                      const bool emergent_advance,
                                      bool * goal_line_positioning ) const
{
    double GOAL_LINE_POSITIONINIG_GOAL_X_DIST = 1.5;

#ifdef PENALTY_SHOOTOUT_GOAL_PARALLEL_POSITIONING
    if ( is_penalty_kick_mode )
    {
        GOAL_LINE_POSITIONINIG_GOAL_X_DIST = 15.8;
    }
#endif


    static const double MINIMUM_GOAL_X_DIST = 1.0;

    const WorldModel & wm = agent->world();
    const ServerParam & SP =  ServerParam::i();

    const double goal_line_x = SP.ourTeamGoalLineX();
    const double goal_half_width = SP.goalHalfWidth();

    const double alpha = std::atan2( goal_half_width, base_dist );

    const double min_x = ( goal_line_x + MINIMUM_GOAL_X_DIST );

    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const Vector2D goal_right_post( goal_center.x, -goal_half_width );

    const Line2D line_1( ball_pos, goal_left_post );
    const Line2D line_2( ball_pos, goal_right_post );

    const AngleDeg line_dir = AngleDeg::normalize_angle
        ( AngleDeg::normalize_angle
          ( ( goal_left_post - ball_pos ).th().degree()
            + ( goal_right_post - ball_pos ).th().degree() )
          / 2.0 );

    const Line2D line_m( ball_pos, line_dir );

    const Line2D goal_line( goal_left_post, goal_right_post );

    if ( ! emergent_advance
         && *goal_line_positioning )
    {
        return this->getGoalLinePositioningTarget
            ( agent, wm, GOAL_LINE_POSITIONINIG_GOAL_X_DIST,
              ball_pos, is_despair_situation );
    }


    const Vector2D p = goal_line.intersection( line_m );

    if ( ! p.isValid() )
    {
        if ( ball_pos.x > 0.0 )
        {
            return Vector2D( min_x, goal_left_post.y );
        }
        else if ( ball_pos.x < 0.0 )
        {
            return Vector2D( min_x, goal_right_post.y );
        }
        else
        {
            return Vector2D( min_x, goal_center.y );
        }
    }

    // angle -> dist
    double dist_from_goal = (line_1.dist(p) + line_2.dist(p)) / 2.0 / std::sin(alpha);

    if ( dist_from_goal <= goal_half_width )
    {
        dist_from_goal = goal_half_width;
    }

    if ( (ball_pos - p).r() + 1.5 < dist_from_goal )
    {
        dist_from_goal = (ball_pos - p).r() + 1.5;
    }

    const AngleDeg current_position_line_dir = (self_pos - p).th();

    const AngleDeg position_error = line_dir - current_position_line_dir;

    const double danger_angle = 21.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": position angle error = %f",
                  position_error.abs() );

    agent->debugClient().addMessage( "angleError = %f", position_error.abs() );

    if ( ! is_penalty_kick_mode
         && position_error.abs() > danger_angle
         && (ball_pos - goal_center).r() < 50.0
         && (p - self_pos).r() > 5.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": position angle error too big,"
                      " dangerous" );

        dist_from_goal *= (1.0 - (position_error.abs() - danger_angle)
                           / (180.0 - danger_angle) ) / 3.0;

        if ( dist_from_goal < goal_half_width - 1.0 )
        {
            dist_from_goal = goal_half_width - 1.0;
        }

        *dangerous = true;

#if defined( DO_AGGRESSIVE_POSITIONING )
        if ( self_pos.x < -45.0
             && ( ball_pos - self_pos ).r() < 20.0 )
        {
            dlog.addText( Logger::TEAM,
                          "change to goal line positioning" );

            *goal_line_positioning = true;

            return this->getGoalLinePositioningTarget
                ( agent, wm, GOAL_LINE_POSITIONINIG_GOAL_X_DIST,
                  ball_pos, is_despair_situation );
        }
#endif
    }
    else if ( position_error.abs() > 5.0
              && (p - self_pos).r() > 5.0 )
    {
        const double current_position_dist = (p - self_pos).r();

        if ( dist_from_goal < current_position_dist )
        {
            dist_from_goal = current_position_dist;
        }

        dist_from_goal = (p - self_pos).r();

        dlog.addText( Logger::TEAM,
                      __FILE__": position angle error big,"
                      " positioning has changed to projection point, "
                      "new dist_from_goal = %f", dist_from_goal );

        *dangerous = true;

#ifdef DO_GOAL_LINE_POSITIONINIG_AT_SIDE
        if ( self_pos.x < -45.0
             && ( ball_pos - self_pos ).r() < 20.0 )
        {
            dlog.addText( Logger::TEAM,
                          "change to goal line positioning" );

            *goal_line_positioning = true;

            return this->getGoalLinePositioningTarget
                ( agent, wm, GOAL_LINE_POSITIONINIG_GOAL_X_DIST,
                  ball_pos, is_despair_situation );
        }
#endif

        {
            const Vector2D r = p + ( ball_pos - p ).setLengthVector( dist_from_goal );

#ifdef VISUAL_DEBUG
            agent->debugClient().addLine( p, r );
            agent->debugClient().addLine( self_pos, r );
#endif
        }
    }
    else if ( ! is_penalty_kick_mode
              && position_error.abs() > 10.0 )
    {
        dist_from_goal = std::min( dist_from_goal, 14.0 );

        dlog.addText( Logger::TEAM,
                      __FILE__": position angle error is big,"
                      "new dist_from_goal = %f", dist_from_goal );
    }


    Vector2D result = p + ( ball_pos - p ).setLengthVector( dist_from_goal );

    if ( result.x < min_x )
    {
        result.assign( min_x, result.y );
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": positioning target = [%f,%f]",
                  result.x, result.y );

    return result;
}




Vector2D
Bhv_Savior::getGoalLinePositioningTarget( PlayerAgent * agent,
                                          const WorldModel & wm,
                                          const double goal_x_dist,
                                          const Vector2D & ball_pos,
                                          const bool is_despair_situation ) const
{
    const ServerParam & SP = ServerParam::i();

    const double goal_line_x = SP.ourTeamGoalLineX();

    const double goal_half_width = SP.goalHalfWidth();
    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const Vector2D goal_right_post( goal_center.x, -goal_half_width );

    const AngleDeg line_dir = getDirFromOurGoal( ball_pos );
    const Line2D line_m( ball_pos, line_dir );
    const Line2D goal_line( goal_left_post, goal_right_post );
    const Line2D target_line( goal_left_post + Vector2D( goal_x_dist, 0.0 ),
                              goal_right_post + Vector2D( goal_x_dist, 0.0 ) );

    if ( is_despair_situation )
    {
        const double target_x = std::max( wm.self().inertiaPoint( 1 ).x,
                                          goal_line_x );

        const Line2D positioniong_line( Vector2D( target_x, -1.0 ),
                                        Vector2D( target_x, +1.0 ) );

        if ( wm.ball().vel().r() < EPS )
        {
            return wm.ball().pos();
        }

        const Line2D ball_line( ball_pos, wm.ball().vel().th() );

        const Vector2D c = positioniong_line.intersection( ball_line );

        if ( c.isValid() )
        {
            return c;
        }
    }


    //
    // set base point to ball line and positioning line
    //
    Vector2D p = target_line.intersection( line_m );

    if ( ! p.isValid() )
    {
        double target_y;

        if ( ball_pos.absY() > SP.goalHalfWidth() )
        {
            target_y = sign( ball_pos.y )
                * std::min( ball_pos.absY(),
                            SP.goalHalfWidth() + 2.0 );
        }
        else
        {
            target_y = ball_pos.y * 0.8;
        }

        p = Vector2D( goal_line_x + goal_x_dist, target_y );
    }


    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int predict_step = std::min( opponent_reach_cycle, teammate_reach_cycle );

    const Vector2D predict_ball_pos = wm.ball().inertiaPoint( predict_step );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getGoalLinePositioningTarget) predict_step = %d, "
                  "predict_ball_pos = [%f, %f]",
                  predict_step, predict_ball_pos.x, predict_ball_pos.y );

    const bool can_shoot_from_predict_ball_pos = opponentCanShootFrom( wm, predict_ball_pos );

    dlog.addText( Logger::TEAM,
                  __FILE__": (getGoalLinePositioningTarget) can_shoot_from_predict_ball_pos = %s",
                  ( can_shoot_from_predict_ball_pos? "true": "false" ) );


#if 1
    //
    // shift to middle of goal if catchable
    //
    const double ball_dist = FieldAnalyzer::get_dist_from_our_near_goal_post( predict_ball_pos );
    const int ball_step = static_cast< int >( std::ceil( ball_dist / SP.ballSpeedMax() ) + EPS );

    const double s = sign( p.y );
    const double org_p_y_abs = std::fabs( p.y );

    for( double new_y_abs = 0.0; new_y_abs < org_p_y_abs; new_y_abs += 0.5 )
    {
        const Vector2D new_p( p.x, s * new_y_abs );

        const double self_dist
            = FieldAnalyzer::get_dist_from_our_near_goal_post( Vector2D( p.x, s * new_y_abs ) );
        const int self_step = wm.self().playerTypePtr()->cyclesToReachDistance( self_dist );

        dlog.addText( Logger::TEAM,
                      __FILE__": check back shift y_abs = %f, self_step = %d, ball_step = %d",
                      new_y_abs, self_step, ball_step );

        if ( self_step * 1.05 + 4 < ball_step )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": shift back to [%d, %d]",
                          p.x, p.y );

# ifdef VISUAL_DEBUG
            agent->debugClient().addCircle( p, 0.5 );

            agent->debugClient().addLine( new_p + Vector2D( -1.5, -1.5 ),
                                          new_p + Vector2D( +1.5, +1.5 ) );
            agent->debugClient().addLine( new_p + Vector2D( -1.5, +1.5 ),
                                          new_p + Vector2D( +1.5, -1.5 ) );

            agent->debugClient().addLine( p, new_p );

            if ( can_shoot_from_predict_ball_pos )
            {
                agent->debugClient().addLine( Vector2D( SP.ourTeamGoalLineX() - 5.0,
                                                        rcsc::sign( wm.ball().pos().y )
                                                        * SP.goalHalfWidth() ),
                                              Vector2D( SP.ourTeamGoalLineX() + 5.0,
                                                        rcsc::sign( wm.ball().pos().y )
                                                        * SP.goalHalfWidth() ) );
            }
# endif
            p = new_p;
            break;
        }
    }

    if ( p.absY() > SP.goalHalfWidth() + 2.0 )
    {
        p.assign( p.x, sign( p.y ) * ( SP.goalHalfWidth() + 2.0 ) );
    }

    return p;
#elif 0
# if 0
    //
    // adjust point to minimize shoot angle
    //
    const double GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH = 4.0;
    const double GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE = 0.2;

    const AbstractPlayerCont teammates_without_self
        = wm.getPlayerCont( new TeammatePlayerPredicate( wm ) );

    bool current_can_shoot_from;
    const double current_free_angle = getMaxShootAngle( wm,
                                                        teammates_without_self,
                                                        predict_ball_pos,
                                                        p,
                                                        &current_can_shoot_from );
    double minus_min_free_angle = current_free_angle;
    Vector2D minus_best_point = p;

    for( double new_y = p.y - GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE ;
         new_y > p.y - GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH - EPS
             && new_y > - ( SP.goalHalfWidth() + 2.0 ) ;
         new_y -= GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        if ( minus_min_free_angle > free_angle )
        {
            minus_min_free_angle = free_angle;
            minus_best_point = new_p;
        }
        else
        {
            break;
        }
    }

    double plus_min_free_angle = current_free_angle;
    Vector2D plus_best_point = p;
    for( double new_y = p.y + GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE;
         new_y < p.y + GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH + EPS
             && new_y < + ( SP.goalHalfWidth() + 2.0 ) ;
         new_y += GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        if ( plus_min_free_angle > free_angle )
        {
            plus_min_free_angle = free_angle;
            plus_best_point = new_p;
        }
        else
        {
            break;
        }
    }

    Vector2D best_point;
    if ( minus_min_free_angle == plus_min_free_angle )
    {
        best_point = p;
    }
    else if ( minus_min_free_angle < plus_min_free_angle )
    {
        best_point = minus_best_point;
    }
    else
    {
        best_point = plus_best_point;
    }
# else
    //
    // adjust point to minimize shoot angle and move center if possible
    //
    const double GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH = 4.0;
    const double GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE = 0.2;

    class EvaluationItems
    {
    private:
        Vector2D M_pos;
        bool M_can_catch_at_post;
        double M_shoot_free_angle;
        bool M_can_shoot;

    public:
        EvaluationItems( const Vector2D & pos,
                         bool can_catch_at_post,
                         double shoot_free_angle, bool can_shoot )
            : M_pos( pos ),
              M_can_catch_at_post( can_catch_at_post ),
              M_shoot_free_angle( shoot_free_angle ),
              M_can_shoot( can_shoot )
        {
        }

        const Vector2D & getPos() const
        {
            return M_pos;
        }

        bool getCanCatchAtPost() const
        {
            return M_can_catch_at_post;
        }

        double getShootFreeAngle() const
        {
            return M_shoot_free_angle;
        }

        bool getCanShootFrom() const
        {
            return M_can_shoot;
        }

        bool less( const EvaluationItems & e ) const
        {
            if ( M_can_catch_at_post != e.M_can_catch_at_post )
            {
                return e.M_can_shoot;
            }
            else if ( M_can_shoot != e.M_can_shoot )
            {
                return M_can_shoot;
            }
            else
            {
                if ( M_can_shoot )
                {
                    return ( M_shoot_free_angle > e.M_shoot_free_angle );
                }
                else
                {
                    return ( M_pos.absY() > e.M_pos.absY() );
                }
            }
        }
    };


    const AbstractPlayerCont teammates_without_self
        = wm.getPlayerCont( new TeammatePlayerPredicate( wm ) );

    bool current_can_shoot_from;
    const double current_free_angle = getMaxShootAngle( wm,
                                                        teammates_without_self,
                                                        predict_ball_pos,
                                                        p,
                                                        &current_can_shoot_from );
    const EvaluationItems current_ev( p,
                                      canCatchAtNearPost( wm,
                                                          p,
                                                          predict_ball_pos ),
                                      current_free_angle,
                                      current_can_shoot_from );

    EvaluationItems minus_ev = current_ev;
    for( double new_y = p.y - GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE ;
         new_y > p.y - GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH - EPS
             && new_y > - ( SP.goalHalfWidth() + 2.0 ) ;
         new_y -= GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        EvaluationItems new_ev( new_p,
                                canCatchAtNearPost( wm,
                                                    new_p,
                                                    predict_ball_pos ),
                                free_angle,
                                can_shoot_from );

        if ( minus_ev.less( new_ev ) )
        {
            minus_ev = new_ev;
        }
        else
        {
            break;
        }
    }

    EvaluationItems plus_ev = current_ev;
    for( double new_y = p.y + GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE;
         new_y < p.y + GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH + EPS
             && new_y < + ( SP.goalHalfWidth() + 2.0 ) ;
         new_y += GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        EvaluationItems new_ev( new_p,
                                canCatchAtNearPost( wm,
                                                    new_p,
                                                    predict_ball_pos ),
                                free_angle,
                                can_shoot_from );

        if ( plus_ev.less( new_ev ) )
        {
            plus_ev = new_ev;
        }
        else
        {
            break;
        }
    }

    EvaluationItems best_ev = current_ev;
    {
        if ( minus_ev.less( plus_ev ) )
        {
            best_ev = plus_ev;
        }
        else if ( plus_ev.less( minus_ev ) )
        {
            best_ev = minus_ev;
        }
        else
        {
            best_ev = current_ev;
        }
    }

    const Vector2D best_point = best_ev.getPos();

    dlog.addText( Logger::TEAM,
                  __FILE__": (getGoalLinePositioningTarget)"
                  " result = [%.2f, %.2f],"
                  " can_shoot from = %s, shoot free angle = %.2f",
                  best_ev.getPos().x, best_ev.getPos().y,
                  ( best_ev.getCanShootFrom() ? "true" : "false" ),
                  best_ev.getShootFreeAngle() );
#endif

# ifdef VISUAL_DEBUG
    if ( best_point.dist( p ) >= EPS )
    {
        agent->debugClient().addCircle( p, 0.5 );

        agent->debugClient().addLine( best_point + Vector2D( -1.5, -1.5 ),
                                      best_point + Vector2D( +1.5, +1.5 ) );
        agent->debugClient().addLine( best_point + Vector2D( -1.5, +1.5 ),
                                      best_point + Vector2D( +1.5, -1.5 ) );

        agent->debugClient().addLine( p, best_point );

        agent->debugClient().addLine( Vector2D( SP.ourTeamGoalLineX() - 5.0,
                                                rcsc::sign( wm.ball().pos().y )
                                                * SP.goalHalfWidth() ),
                                      Vector2D( SP.ourTeamGoalLineX() + 5.0,
                                                rcsc::sign( wm.ball().pos().y )
                                                * SP.goalHalfWidth() ) );
    }
# endif

    return best_point;
#else
    //
    // shift to middle of goal if catchable
    //
    const Vector2D org_p = p;

    const double ball_dist = FieldAnalyzer::get_dist_from_our_near_goal_post( predict_ball_pos );
    const int ball_step = static_cast< int >( std::ceil( ball_dist / SP.ballSpeedMax() ) + EPS );

    const double s = sign( p.y );
    const double org_p_y_abs = std::fabs( p.y );

    for( double new_y_abs = 0.0; new_y_abs < org_p_y_abs; new_y_abs += 0.5 )
    {
        const Vector2D new_p( p.x, s * new_y_abs );

        const double self_dist
            = FieldAnalyzer::get_dist_from_our_near_goal_post( Vector2D( p.x, s * new_y_abs ) );
        const int self_step = wm.self().playerTypePtr()->cyclesToReachDistance( self_dist );

        dlog.addText( Logger::TEAM,
                      __FILE__": check back shift y_abs = %f, self_step = %d, ball_step = %d",
                      new_y_abs, self_step, ball_step );

        if ( self_step * 1.05 + 4 < ball_step )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": shift back to [%d, %d]",
                          p.x, p.y );

            p = new_p;
            break;
        }
    }


    if ( p.absY() > SP.goalHalfWidth() + 2.0 )
    {
        p.assign( p.x, sign( p.y ) * ( SP.goalHalfWidth() + 2.0 ) );
    }

    if ( org_p.dist( p ) > EPS )
    {
# ifdef VISUAL_DEBUG
            agent->debugClient().addCircle( p, 0.5 );

            agent->debugClient().addLine( p + Vector2D( -1.5, -1.5 ),
                                          p + Vector2D( +1.5, +1.5 ) );
            agent->debugClient().addLine( p + Vector2D( -1.5, +1.5 ),
                                          p + Vector2D( +1.5, -1.5 ) );

            agent->debugClient().addLine( org_p, p );

            if ( can_shoot_from_predict_ball_pos )
            {
                agent->debugClient().addLine( Vector2D( SP.ourTeamGoalLineX() - 5.0,
                                                        rcsc::sign( wm.ball().pos().y )
                                                        * SP.goalHalfWidth() ),
                                              Vector2D( SP.ourTeamGoalLineX() + 5.0,
                                                        rcsc::sign( wm.ball().pos().y )
                                                        * SP.goalHalfWidth() ) );
            }
# endif

        return p;
    }


    //
    // adjust point to minimize shoot angle
    //
    const double GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH = 1.5;
    const double GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE = 0.1;

    const AbstractPlayerCont teammates_without_self
        = wm.getPlayerCont( new TeammatePlayerPredicate( wm ) );

    bool current_can_shoot_from;
    const double current_free_angle = getMaxShootAngle( wm,
                                                        teammates_without_self,
                                                        predict_ball_pos,
                                                        p,
                                                        &current_can_shoot_from );
    double minus_min_free_angle = current_free_angle;
    Vector2D minus_best_point = p;

    for( double new_y = p.y - GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE ;
         new_y > p.y - GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH - EPS
             && new_y > - ( SP.goalHalfWidth() + 2.0 ) ;
         new_y -= GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        if ( minus_min_free_angle > free_angle )
        {
            minus_min_free_angle = free_angle;
            minus_best_point = new_p;
        }
        else
        {
            break;
        }
    }

    double plus_min_free_angle = current_free_angle;
    Vector2D plus_best_point = p;
    for( double new_y = p.y + GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE;
         new_y < p.y + GOAL_LINE_POSITIONINIG_ADJUST_HALF_WIDTH + EPS
             && new_y < + ( SP.goalHalfWidth() + 2.0 ) ;
         new_y += GOAL_LINE_POSITIONINIG_ADJUST_STEP_SIZE )
    {
        const Vector2D new_p( p.x, new_y );

        bool can_shoot_from;
        const double free_angle = getMaxShootAngle( wm,
                                                    teammates_without_self,
                                                    predict_ball_pos,
                                                    new_p,
                                                    &can_shoot_from );
        if ( plus_min_free_angle > free_angle )
        {
            plus_min_free_angle = free_angle;
            plus_best_point = new_p;
        }
        else
        {
            break;
        }
    }

    Vector2D best_point;
    if ( minus_min_free_angle == plus_min_free_angle )
    {
        best_point = p;
    }
    else if ( minus_min_free_angle < plus_min_free_angle )
    {
        best_point = minus_best_point;
    }
    else
    {
        best_point = plus_best_point;
    }

    return best_point;
#endif
}

bool
Bhv_Savior::canCatchAtNearPost( const WorldModel & wm,
                                const Vector2D & pos,
                                const Vector2D & ball_pos )
{
    const ServerParam & SP = ServerParam::i();

    //
    // shift to middle of goal if catchable
    //
    const double ball_dist = FieldAnalyzer::get_dist_from_our_near_goal_post( ball_pos );
    const int ball_step = static_cast< int >( std::ceil( ball_dist / SP.ballSpeedMax() ) + EPS );

    const double self_dist
        = FieldAnalyzer::get_dist_from_our_near_goal_post( pos );
    const int self_step = wm.self().playerTypePtr()->cyclesToReachDistance( self_dist );

    if ( self_step * 1.05 + 4 < ball_step )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": can catch at near post [%d, %d]",
                      pos.x, pos.y );
        return true;
    }

    return false;
}

double
Bhv_Savior::getMaxShootAngle( const WorldModel & wm,
                              const AbstractPlayerCont & teammates_without_self,
                              const Vector2D & ball_pos,
                              const Vector2D & self_pos,
                              bool * can_shoot )
{
    AbstractPlayerCont our_team_players = teammates_without_self;
    const PredictPlayerObject predict_self( wm.self(), self_pos );
    our_team_players.push_back( &predict_self );

    double free_angle;

    const bool can_shoot_from
        = FieldAnalyzer::opponent_can_shoot_from( ball_pos,
                                                  our_team_players,
                                                  SHOOT_BLOCK_TEAMMATE_POS_COUNT_THRESHOLD,
                                                  SHOOT_DIST_THRESHOLD,
                                                  SHOOT_ANGLE_THRESHOLD,
                                                  -1.0,
                                                  &free_angle,
                                                  true );
    if ( can_shoot != static_cast< bool * >( 0 ) )
    {
        *can_shoot = can_shoot_from;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (getMaxShootAngle) check free angle, pos = [%.2f, %.2f],"
                  " can_shoot = %s, free angle = %.2f",
                  self_pos.x, self_pos.y, ( can_shoot_from ? "true" : "false" ),
                  free_angle );

        return free_angle;
}



/*-------------------------------------------------------------------*/
AngleDeg
Bhv_Savior::getDirFromOurGoal( const Vector2D & pos )
{
    const ServerParam & SP = ServerParam::i();

    const double goal_half_width = SP.goalHalfWidth();

    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const Vector2D goal_right_post( goal_center.x, -goal_half_width );

    return AngleDeg( ( (pos - goal_left_post).th().degree()
                       + (pos - goal_right_post).th().degree() ) / 2.0 );
}

double
Bhv_Savior::getTackleProbability( const Vector2D & body_relative_ball )
{
    const ServerParam & SP = ServerParam::i();

    double tackle_length;

    if ( body_relative_ball.x > 0.0 )
    {
        if ( SP.tackleDist() > EPS )
        {
            tackle_length = SP.tackleDist();
        }
        else
        {
            return 0.0;
        }
    }
    else
    {
        if ( SP.tackleBackDist() > EPS )
        {
            tackle_length = SP.tackleBackDist();
        }
        else
        {
            return 0.0;
        }
    }

    if ( SP.tackleWidth() < EPS )
    {
        return 0.0;
    }


    double prob = 1.0;

    // vertical dist penalty
    prob -= std::pow( body_relative_ball.absX() / tackle_length,
                      SP.tackleExponent() );

    // horizontal dist penalty
    prob -= std::pow( body_relative_ball.absY() / SP.tackleWidth(),
                      SP.tackleExponent() );

    // don't allow negative value by calculation error.
    return std::max( 0.0, prob );
}

/*-------------------------------------------------------------------*/
Vector2D
Bhv_Savior::getSelfNextPosWithDash( const WorldModel & wm,
                                    const double dash_power )
{
    return wm.self().inertiaPoint( 1 )
        + Vector2D::polar2vector
        ( dash_power * wm.self().playerType().dashPowerRate(),
          wm.self().body() );
}

double
Bhv_Savior::getSelfNextTackleProbabilityWithDash( const WorldModel & wm,
                                                  const double dash_power )
{
    return getTackleProbability( ( wm.ball().inertiaPoint( 1 )
                                   - getSelfNextPosWithDash( wm, dash_power ) )
                                 .rotatedVector( - wm.self().body() ) );
}

double
Bhv_Savior::getSelfNextTackleProbabilityWithTurn( const WorldModel & wm )
{
    const ServerParam & SP = ServerParam::i();

    const Vector2D self_next = wm.self().pos() + wm.self().vel();
    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
    const Vector2D ball_rel = ball_next - self_next;
    const double ball_dist = ball_rel.r();

    if ( ball_dist > SP.tackleDist()
         && ball_dist > SP.tackleWidth() )
    {
        return 0.0;
    }

    double max_turn = wm.self().playerType().effectiveTurn( SP.maxMoment(),
                                                            wm.self().vel().r() );
    AngleDeg ball_angle = ball_rel.th() - wm.self().body();
    ball_angle = std::max( 0.0, ball_angle.abs() - max_turn );

    return getTackleProbability( Vector2D::polar2vector( ball_dist, ball_angle ) );
}


/*-------------------------------------------------------------------*/
bool
Bhv_Savior::opponentCanShootFrom( const WorldModel & wm,
                                  const Vector2D & pos,
                                  const long valid_teammate_threshold )
{
    return FieldAnalyzer::opponent_can_shoot_from
           ( pos,
             wm.getPlayerCont( new TeammatePlayerPredicate( wm ) ),
             valid_teammate_threshold,
             SHOOT_DIST_THRESHOLD,
             SHOOT_ANGLE_THRESHOLD );
}

bool
Bhv_Savior::canBlockShootFrom( const WorldModel & wm,
                               const Vector2D & pos )
{
    bool result = ! FieldAnalyzer::opponent_can_shoot_from
                    ( pos,
                      wm.getPlayerCont( new TeammatePlayerPredicate( wm ) ),
                      SHOOT_BLOCK_TEAMMATE_POS_COUNT_THRESHOLD,
                      SHOOT_DIST_THRESHOLD,
                      SHOOT_ANGLE_THRESHOLD,
                      -1.0 );

    dlog.addText( Logger::TEAM,
                  __FILE__": canBlockShootFrom "
                  "pos = [%f, %f], result = %s",
                  pos.x, pos.y, (result? "true":"false") );

    return result;
}

#if 0
double
Bhv_Savior::getFreeAngleFromPos( const WorldModel & wm,
                                 const long valid_teammate_threshold,
                                 const Vector2D & pos )
{
    const ServerParam & SP = ServerParam::i();

    const double goal_half_width = SP.goalHalfWidth();
    const double field_half_length = SP.pitchHalfLength();

    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left( goal_center.x, +goal_half_width );
    const Vector2D goal_right( goal_center.x, -goal_half_width );

    const Vector2D goal_center_left( field_half_length,
                                     (+ goal_half_width - 1.5) / 2.0 );

    const Vector2D goal_center_right( field_half_length,
                                      (- goal_half_width + 1.5) / 2.0 );


    const double center_angle = getMinimumFreeAngle( wm,
                                                     goal_center,
                                                     valid_teammate_threshold,
                                                     pos );

    const double left_angle   = getMinimumFreeAngle( wm,
                                                     goal_left,
                                                     valid_teammate_threshold,
                                                     pos );

    const double right_angle  = getMinimumFreeAngle( wm,
                                                     goal_right,
                                                     valid_teammate_threshold,
                                                     pos );

    const double center_left_angle  = getMinimumFreeAngle
        ( wm,
          goal_center_left,
          valid_teammate_threshold,
          pos );

    const double center_right_angle = getMinimumFreeAngle
        ( wm,
          goal_center_right,
          valid_teammate_threshold,
          pos );

    return std::max( center_angle,
                     std::max( left_angle,
                               std::max( right_angle,
                                         std::max( center_left_angle,
                                                   center_right_angle ) ) ) );
}


double
Bhv_Savior::getMinimumFreeAngle( const WorldModel & wm,
                                 const Vector2D & goal,
                                 const long valid_teammate_threshold,
                                 const Vector2D & pos )
{
    const ServerParam & SP = ServerParam::i();

    const AngleDeg test_dir = (goal - pos).th();

    double shoot_course_cone = +360.0;

    const AbstractPlayerCont team_set
        = wm.getPlayerCont( new AndPlayerPredicate
                            ( new TeammatePlayerPredicate( wm ),
                              new CoordinateAccuratePlayerPredicate
                              ( valid_teammate_threshold ) ) );

    const AbstractPlayerCont::const_iterator t_end = team_set.end();
    for ( AbstractPlayerCont::const_iterator t = team_set.begin();
          t != t_end;
          ++t )
    {
        double controllable_dist;

        if ( (*t)->goalie() )
        {
            controllable_dist = SP.catchAreaLength();
        }
        else
        {
            controllable_dist = (*t)->playerTypePtr()->kickableArea();
        }

        const Vector2D relative_player = (*t)->pos() - pos;

        const double hide_angle_radian = ( std::asin
                                           ( std::min( controllable_dist
                                                       / relative_player.r(),
                                                       1.0 ) ) );
        const double angle_diff = std::max( (relative_player.th() - test_dir).abs()
                                            - hide_angle_radian / M_PI * 180,
                                            0.0 );

        if ( shoot_course_cone > angle_diff )
        {
            shoot_course_cone = angle_diff;
        }
    }

    return shoot_course_cone;
}
#endif


/*-------------------------------------------------------------------*/
bool
Bhv_Savior::doChaseBall( PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();

    const int self_reach_cycle = agent->world().interceptTable()->selfReachCycle();
    const Vector2D intercept_point = agent->world().ball().inertiaPoint( self_reach_cycle );

    dlog.addText( Logger::TEAM,
                  __FILE__": (doChaseBall): intercept point = [%f, %f]",
                  intercept_point.x, intercept_point.y );

    if ( intercept_point.x > SP.ourTeamGoalLineX() - 1.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doChaseBall): normal intercept" );

        if ( Body_Intercept().execute( agent ) )
        {
            agent->setNeckAction( new Neck_ChaseBall() );

            return true;
        }
    }


    //
    // go to goal line
    //
    const Vector2D predict_ball_pos
        = FieldAnalyzer::get_field_bound_predict_ball_pos( agent->world(),
                                                           self_reach_cycle,
                                                           1.0 );

    const double target_error = std::max( SP.catchableArea(),
                                          SP.tackleDist() );

    dlog.addText( Logger::TEAM,
                  __FILE__": (doChaseBall): "
                  "goal line intercept point = [%f, %f]",
                  predict_ball_pos.x, predict_ball_pos.y );

    if ( doGoToPoint( agent,
                      predict_ball_pos,
                      target_error,
                      SP.maxPower(),
                      true,
                      true,
                      true ) )
    {
        agent->setNeckAction( new Neck_ChaseBall() );

        return true;
    }

    return false;
}


bool
Bhv_Savior::isGoalLinePositioningSituationBase( const WorldModel & wm,
                                                const Vector2D & ball_pos,
                                                const bool is_penalty_kick_mode )
{
    const double SIDE_ANGLE_DEGREE_THRESHOLD = 50.0;

    if ( is_penalty_kick_mode )
    {
#ifdef PENALTY_SHOOTOUT_GOAL_PARALLEL_POSITIONING
        const ServerParam & SP = ServerParam::i();

        if ( wm.self().pos().x > SP.ourPenaltyAreaLineX() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (isGoalLinePositioningSituationBase): "
                          "penalty kick mode parallel positioning,"
                          " over penalty area x, return false" );
            return false;
        }
        else if ( ball_pos.absY() < SP.goalHalfWidth() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (isGoalLinePositioningSituationBase): "
                          "penalty kick mode parallel positioning, return true" );
            return true;
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (isGoalLinePositioningSituationBase): "
                          "penalty kick mode parallel positioning,"
                          " side of goal, return false" );
            return false;
        }
#else
        dlog.addText( Logger::TEAM,
                      __FILE__": (isGoalLinePositioningSituationBase): "
                      "penalty kick mode, return false" );
        return false;
#endif
    }


    if ( wm.ourDefenseLineX() >= -15.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (isGoalLinePositioningSituationBase): "
                      "defense line(%f) too forward, "
                      "no need to goal line positioning",
                      wm.ourDefenseLineX() );
        return false;
    }


    const AngleDeg ball_dir = getDirFromOurGoal( ball_pos );
    const bool is_side_angle = ( ball_dir.abs() > SIDE_ANGLE_DEGREE_THRESHOLD );
    dlog.addText( Logger::TEAM,
                  __FILE__": (isGoalLinePositioningSituationBase) "
                  "ball direction from goal = %f, is_side_angle = %s",
                  ball_dir.degree(), ( is_side_angle? "true": "false" ) );

    if ( is_side_angle )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (isGoalLinePositioningSituationBase) "
                      "side angle, not goal line positioning" );
        return false;
    }

    if ( wm.ourDefenseLineX() <= -15.0
         && ball_pos.x < 5.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (isGoalLinePositioningSituationBase): "
                      "defense line(%f) too back, "
                      "return true",
                      wm.ourDefenseLineX() );
        return true;
    }


    dlog.addText( Logger::TEAM,
                  __FILE__": (isGoalLinePositioningSituationBase) "
                  "return false" );
    return false;
}

#if 0
bool
Bhv_Savior::isGoalLinePositioningSituation( const WorldModel & wm,
                                            const Vector2D & ball_pos,
                                            const bool is_penalty_kick_mode )
{
    const ServerParam & SP =  ServerParam::i();

    const Vector2D self_pos = wm.self().pos();

    const double goal_half_width = SP.goalHalfWidth();

    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const Vector2D goal_right_post( goal_center.x, -goal_half_width );

    const Line2D line_1( ball_pos, goal_left_post );
    const Line2D line_2( ball_pos, goal_right_post );

    const AngleDeg line_dir = AngleDeg::normalize_angle
        ( AngleDeg::normalize_angle
          ( (goal_left_post - ball_pos).th().degree()
            + (goal_right_post - ball_pos).th().degree() )
          / 2.0 );

    const Line2D line_m( ball_pos, line_dir );

    const Line2D goal_line( goal_left_post, goal_right_post );

    const Vector2D p = goal_line.intersection( line_m );

    if ( isGoalLinePositioningSituationBase( wm, ball_pos, is_penalty_kick_mode ) )
    {
        return true;
    }

    if ( p.isValid() )
    {
        const AngleDeg current_position_line_dir = (self_pos - p).th();

        const AngleDeg position_error = line_dir - current_position_line_dir;

        const double danger_angle = 21.0;

        if ( ! is_penalty_kick_mode
             && position_error.abs() > danger_angle
             && (ball_pos - goal_center).r() < 50.0
             && (p - self_pos).r() > 5.0 )
        {
#if defined( DO_AGGRESSIVE_POSITIONING )
            if ( self_pos.x < -45.0
                 && ( ball_pos - self_pos ).r() < 20.0 )
            {
                return true;
            }
#endif
        }
        else if ( position_error.abs() > 5.0
                  && (p - self_pos).r() > 5.0 )
        {
#if defined( DO_AGGRESSIVE_POSITIONING )
            if ( self_pos.x < -45.0
                 && ( ball_pos - self_pos ).r() < 20.0 )
            {
                return true;
            }
#endif
        }
    }

    return false;
}



bool
Bhv_Savior::isEmergentAdvanceSituation( const rcsc::WorldModel & wm,
                                        const rcsc::Vector2D & ball_pos,
                                        const int teammate_reach_cycle,
                                        const int opponent_reach_cycle,
                                        const bool is_penalty_kick_mode )
{
    const ServerParam & SP =  ServerParam::i();
    const AbstractPlayerObject * const fastestOpponent = wm.interceptTable()->fastestOpponent();

    // if opponent player will be have the ball at backward of our
    // defense line, do 1-on-1 aggressive defense
    return ( ball_pos.x >= SP.ourPenaltyAreaLineX() + 5.0
             && opponent_reach_cycle < teammate_reach_cycle
             && ball_pos.x <= wm.ourDefenseLineX() + 5.0
             && (getDirFromOurGoal( ball_pos )
                 - getDirFromOurGoal( wm.self().inertiaPoint(1) )).abs()
             < EMERGENT_ADVANCE_DIR_DIFF_THRESHOLD
             && opponentCanShootFrom( wm, ball_pos, 20 )
             && ! is_penalty_kick_mode
             && fastestOpponent
             && wm.getPlayerCont
             ( new AndPlayerPredicate
               ( new TeammatePlayerPredicate( wm ),
                 new XCoordinateBackwardPlayerPredicate
                 ( fastestOpponent->pos().x ),
                 new YCoordinatePlusPlayerPredicate
                 ( fastestOpponent->pos().y - 1.0 ),
                 new YCoordinateMinusPlayerPredicate
                 ( fastestOpponent->pos().y + 1.0 ) ) ).empty()
             && getDirFromOurGoal( ball_pos ).abs() < 20.0 );
}

bool
Bhv_Savior::isAggressivePositioningSituation( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();
    const int predict_step = std::min( opponent_reach_cycle,
                                       std::min( teammate_reach_cycle,
                                                 self_reach_cycle ) );

    const Vector2D ball_pos
        = FieldAnalyzer::get_field_bound_predict_ball_pos( wm, predict_step, 0.5 );


    // aggressive positioning
    const double additional_forward_positioning_max = 14.0;

    if ( wm.ourDefenseLineX() >= - additional_forward_positioning_max )
    {
        static const double AGGRESSIVE_POSITIONING_STAMINA_RATE_THRESHOLD = 0.6;

        if ( wm.self().stamina()
             >= SP.staminaMax() * AGGRESSIVE_POSITIONING_STAMINA_RATE_THRESHOLD )
        {
            return true;
        }
    }

    return false;
}


bool
Bhv_Savior::isDangerousSituation( PlayerAgent * agent,
                                  const bool is_penalty_kick_mode,
                                  const bool emergent_advance,
                                  const bool goal_line_positioning )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();
    const int predict_step = std::min( opponent_reach_cycle,
                                       std::min( teammate_reach_cycle,
                                                 self_reach_cycle ) );

    const Vector2D ball_pos
        = FieldAnalyzer::get_field_bound_predict_ball_pos( wm, predict_step, 0.5 );

    const Vector2D & self_pos = wm.self().pos();
    const double goal_half_width = SP.goalHalfWidth();

    const Vector2D goal_center = SP.ourTeamGoalPos();
    const Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const Vector2D goal_right_post( goal_center.x, -goal_half_width );

    const Line2D line_1( ball_pos, goal_left_post );
    const Line2D line_2( ball_pos, goal_right_post );

    const AngleDeg line_dir = AngleDeg::normalize_angle
        ( AngleDeg::normalize_angle
          ( (goal_left_post - ball_pos).th().degree()
            + (goal_right_post - ball_pos).th().degree() )
          / 2.0 );

    const Line2D line_m( ball_pos, line_dir );

    const Line2D goal_line( goal_left_post, goal_right_post );

    if ( ! emergent_advance
         && goal_line_positioning )
    {
        return false;
    }

    const Vector2D p = goal_line.intersection( line_m );

    if ( ! p.isValid() )
    {
        return false;
    }


    const AngleDeg current_position_line_dir = (self_pos - p).th();

    const AngleDeg position_error = line_dir - current_position_line_dir;

    const double danger_angle = 21.0;

    if ( ! is_penalty_kick_mode
         && position_error.abs() > danger_angle
         && (ball_pos - goal_center).r() < 50.0
         && (p - self_pos).r() > 5.0 )
    {
        return true;
    }
    else if ( position_error.abs() > 5.0
              && (p - self_pos).r() > 5.0 )
    {
        return true;
    }

    return false;
}
#endif
