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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_side_half_pass_request_move.h"

#include "strategy.h"
#include "field_analyzer.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/arm_point_to_point.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/world_model.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/time/timer.h>
#include <rcsc/types.h>
#include <rcsc/player/say_message_builder.h>
// #define DEBUG_PROFILE
// #define DEBUG_PRINT
// #define DEBUG_PRINT_EVAL
// #define DEBUG_PRINT_PASS_REQUEST
// #define DEBUG_PRINT_PASS_REQUEST_OPPONENT

// #define DEBUG_PAINT_REQUESTED_PASS_COURSE

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfPassRequestMove2013::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! isMoveSituation( wm ) )
    {
        return false;
    }

    int self_move_step = 1000;
    int offside_reach_step = -1;
    int ball_move_step = 1000;
    const Vector2D receive_pos = getReceivePos( wm,
                                                &self_move_step,
                                                &offside_reach_step,
                                                &ball_move_step );
    if ( ! receive_pos.isValid() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": target point not found." );
        return false;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": target=(%.1f %.1f) self_step=%d offside_step=%d ball_step=%d",
                  receive_pos.x, receive_pos.y,
                  self_move_step, offside_reach_step, ball_move_step );

    if ( doAvoidOffside( agent,
                         receive_pos,
                         offside_reach_step ) )
    {
        return true;
    }

    const double dash_power = getDashPower( wm, receive_pos );

    doGoToPoint( agent, receive_pos, dash_power );
    setTurnNeck( agent );
   // agent->setArmAction( new Arm_PointToPoint( receive_pos ) );
    agent->addSayMessage( new PassRequestMessage( receive_pos) );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfPassRequestMove2013::doGoToPoint( PlayerAgent * agent,
                                              const Vector2D & target_point,
                                              const double dash_power )
{
    const WorldModel & wm = agent->world();
    const int teammate_step = getTeammateReachStep( wm );

    const double dist_thr = std::min( 1.0, wm.ball().pos().dist( target_point ) * 0.1 + 0.5 );

    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    const Vector2D my_inertia = wm.self().inertiaPoint( teammate_step );
    const double target_dist = my_inertia.dist( target_point );

    //
    // back dash to avoid offside
    //
    if ( teammate_step <= 5
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6
         && target_dist > dist_thr
         && ( my_inertia.x > target_point.x
              || my_inertia.x > wm.offsideLineX() - 0.8 )
         && std::fabs( my_inertia.x - target_point.x ) < 3.0
         && wm.self().body().abs() < 15.0 )
    {
        double back_accel
            = std::min( target_point.x, wm.offsideLineX() )
            - wm.self().pos().x
            - wm.self().vel().x;
        double back_dash_power = back_accel / wm.self().dashRate();
        back_dash_power = wm.self().getSafetyDashPower( back_dash_power );
        back_dash_power = ServerParam::i().normalizePower( back_dash_power );

        agent->debugClient().addMessage( "SH:PassRequest:Back%.0f", back_dash_power );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doGoToPoint) Back power=%.1f", back_dash_power );

        agent->doDash( back_dash_power );
        return;
    }

    //
    // move to the target point
    //

    if ( Body_GoToPoint( target_point, dist_thr, dash_power,
                         -1.0, // dash speed
                         100, // cycle
                         true, // save recovery
                         25.0 // angle threshold
                         ).execute( agent ) )
    {
        agent->debugClient().addMessage( "SH:PassRequest:Move" );
        return;
    }

    //
    // already there
    //

    AngleDeg body_angle = 0.0;
    if ( target_dist > dist_thr )
    {
        body_angle = ( target_point - my_inertia ).th();
    }

    Body_TurnToAngle( body_angle ).execute( agent );

    agent->debugClient().addMessage( "SH:PassRequest:Turn" );
    dlog.addText( Logger::TEAM,
                  __FILE__": (doGoToPoint) turn to angle=%.1f",
                  body_angle.degree() );

}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfPassRequestMove2013::setTurnNeck( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const int teammate_step = getTeammateReachStep( wm );
    const Vector2D teammate_ball_pos = wm.ball().inertiaPoint( teammate_step );

    if ( ( wm.existKickableTeammate()
           || teammate_step <= 2 )
         && wm.self().pos().dist2( teammate_ball_pos ) < std::pow( 20.0, 2 ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (setTurnNeck) check ball holder" );
        agent->setNeckAction( new Neck_TurnToBallOrScan( -1 ) );
        return;
    }

    int opponent_step = wm.interceptTable()->opponentReachCycle();
    int count_thr = 0;
    ViewWidth view_width = agent->effector().queuedNextViewWidth();
    int see_cycle = agent->effector().queuedNextSeeCycles();

    if ( view_width.type() == ViewWidth::WIDE )
    {
        if ( opponent_step > see_cycle )
        {
            count_thr = 2;
        }
    }
    else if ( view_width.type() == ViewWidth::NORMAL )
    {
        if ( opponent_step > see_cycle )
        {
            count_thr = 1;
        }
    }

    agent->setNeckAction( new Neck_TurnToBallOrScan( count_thr ) );
    dlog.addText( Logger::TEAM,
                  __FILE__": (setTurnNeck) count_thr=%d",
                  count_thr, opponent_step, see_cycle );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfPassRequestMove2013::doAvoidOffside( PlayerAgent * agent,
                                                 const Vector2D & receive_pos,
                                                 const int offside_reach_step )
{
    const WorldModel & wm = agent->world();
    const int teammate_step = wm.interceptTable()->teammateReachCycle();

    if ( offside_reach_step < 0
         || offside_reach_step > teammate_step )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doAvoidOffside) [false] safe" );
        return false;
    }

    const Vector2D my_inertia = wm.self().inertiaPoint( teammate_step + 1 );

    if ( my_inertia.x > wm.offsideLineX() - 0.8 )
    {
        Body_TurnToPoint( receive_pos ).execute( agent );
        agent->debugClient().addMessage( "SH:PassRequest:AvoidOffside:Turn" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doAvoidOffside) turn to" );
    }
    else
    {
        double required_speed = ( wm.offsideLineX() - 0.8 - wm.self().pos().x )
                * ( 1.0 - wm.self().playerType().playerDecay() )
            / ( 1.0 - std::pow( wm.self().playerType().playerDecay(), teammate_step + 3 ) );
        double dash_power = std::min( ServerParam::i().maxDashPower(),
                                      required_speed / wm.self().dashRate() );
        agent->debugClient().addMessage( "SH:PassRequest:WaitOffside:Move%.0f", dash_power );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doPassRequestMove) adjust. x_move=%.3f required_speed=%.3f dash_power=%.1f",
                      wm.offsideLineX() - 0.8 - wm.self().pos().x,
                      required_speed,
                      dash_power );
        doGoToPoint( agent, receive_pos, dash_power );
    }

    setTurnNeck( agent );
   // agent->setArmAction( new Arm_PointToPoint( receive_pos ) );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
rcsc::Vector2D
Bhv_SideHalfPassRequestMove2013::getReceivePos( const WorldModel & wm,
                                                int * result_self_move_step,
                                                int * result_offside_reach_step,
                                                int * result_ball_move_step )
{
    const ServerParam & SP = ServerParam::i();
    const Vector2D goal_pos = SP.theirTeamGoalPos();

    const int teammate_step = getTeammateReachStep( wm );
    const Vector2D first_ball_pos = wm.ball().inertiaPoint( teammate_step );
    const Vector2D self_pos = wm.self().inertiaFinalPoint();

    Vector2D best_point = Vector2D::INVALIDATED;
    int best_self_move_step = 1000;
    int best_offside_reach_step = -1;
    int best_ball_move_step = 1000;
    double best_point_goal_dist2 = 10000000.0;

    //
    // angle loop
    //
    for ( double a = -60.0; a < 61.0; a += 8.0 )
    {
        const Vector2D unit_vec = Vector2D::from_polar( 1.0, a );

        //
        // distance loop
        //
        double dist_step = 3.0;
        for ( double d = 10.0; d < 40.0; d += dist_step, dist_step += 0.5 )
        {
            const Vector2D receive_pos = self_pos + ( unit_vec * d );
            const double goal_dist2 = receive_pos.dist2( goal_pos );

            ++M_count;
#ifdef DEBUG_PRINT_PASS_REQUEST
            dlog.addText( Logger::TEAM,
                          "%d: receive_pos=(%.1f %.1f) angle=%.0f dist=%.1f dist_step=%.1f goal_dist=%.1f",
                          count, receive_pos.x, receive_pos.y, a, d, dist_step, std::sqrt( goal_dist2 ) );
#endif

            // check range
            if ( receive_pos.x < wm.offsideLineX()
                 && receive_pos.x < SP.pitchLength() / 6.0 )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx not a through pass / not attacking third area.", count );
#endif
                continue;
            }

            // check exsiting candidate
            if ( best_point_goal_dist2 < goal_dist2 )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx already exist better candidate.", count );
#endif
                continue;
            }

            if ( receive_pos.x > SP.pitchHalfLength() - 2.0
                 || receive_pos.absY() > SP.pitchHalfWidth() - 2.0 )
            {
                // exit distance loop
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx out of bounds", count );
#endif
                break;
            }

            // check other receivers
            if ( checkOtherReceiver( wm, receive_pos ) )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx exist other receiver candidate.", count );
#endif
                continue;
            }

            //
            // estimate required time steps
            //

            int offside_reach_step = -1;
            int self_move_step = 1000;
            if ( receive_pos.x > wm.offsideLineX() )
            {
                const Segment2D self_move_line( self_pos, receive_pos );
                const Line2D offside_line( Vector2D( wm.offsideLineX(), -100.0 ),
                                           Vector2D( wm.offsideLineX(), +100.0 ) );
                const Vector2D offside_line_pos = self_move_line.intersection( offside_line );
                if ( offside_line_pos.isValid() )
                {
                    StaminaModel stamina_model = wm.self().staminaModel();
                    offside_reach_step = FieldAnalyzer::predict_self_reach_cycle( wm,
                                                                                  offside_line_pos,
                                                                                  0.1,
                                                                                  0, // wait cycle
                                                                                  true,
                                                                                  &stamina_model );
                    offside_reach_step -= 1; // need to decrease 1 step
                }
            }
            {
                const int wait_cycle = ( ( offside_reach_step >= 0
                                           && offside_reach_step <= teammate_step + 1 )
                                         ? ( teammate_step + 2 ) - offside_reach_step
                                         : 0 );
                StaminaModel stamina_model = wm.self().staminaModel();

                self_move_step = FieldAnalyzer::predict_self_reach_cycle( wm,
                                                                          receive_pos,
                                                                          1.0, // dist_thr
                                                                          wait_cycle, // wait cycle
                                                                          true, // save_recovery
                                                                          &stamina_model );
            }

            const double ball_move_dist = first_ball_pos.dist( receive_pos );
            int ball_move_step = std::max( 1, self_move_step - ( teammate_step + 1 ) );
            double first_ball_speed = SP.firstBallSpeed( ball_move_dist, ball_move_step );

            if ( first_ball_speed > SP.ballSpeedMax() )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: over the max speed %.3f",
                              count, first_ball_speed );
#endif
                first_ball_speed = SP.ballSpeedMax();
                ball_move_step = SP.ballMoveStep( first_ball_speed, ball_move_dist );
            }

#ifdef DEBUG_PRINT_PASS_REQUEST
            dlog.addText( Logger::TEAM,
                          "%d: self_move_step=%d ball_move_step=%d teammate_step=%d first_ball_speed=%.2f",
                          count, self_move_step, ball_move_step, teammate_step, first_ball_speed );
#endif
            if ( ball_move_step < 1 )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx illegal ball move step %d", count, ball_move_step );
#endif
                continue;
            }
#ifdef DEBUG_PRINT_PASS_REQUEST
            char msg[8];
            snprintf( msg, 8, "%d", count );
            dlog.addMessage( Logger::TEAM,
                             receive_pos, msg );
            dlog.addRect( Logger::TEAM,
                          receive_pos.x - 0.1, receive_pos.y - 0.1, 0.2, 0.2,
                          "#F00" );
#endif

            // check opponent
            if ( checkOpponent( wm, first_ball_pos, receive_pos,
                                                first_ball_speed,  ball_move_step ) )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST
                dlog.addText( Logger::TEAM,
                              "%d: xxx opponent will intercept the ball.", count );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PASS_REQUEST
            dlog.addText( Logger::TEAM,
                          "%d: OK receive=(%.1f %.1f)", count,
                          receive_pos.x, receive_pos.y );
            dlog.addMessage( Logger::TEAM,
                             receive_pos, msg );
            dlog.addRect( Logger::TEAM,
                          receive_pos.x - 0.1, receive_pos.y - 0.1, 0.2, 0.2,
                          "#00F", true );
#endif
            best_point = receive_pos;
            best_self_move_step = self_move_step;
            best_offside_reach_step = offside_reach_step;
            best_ball_move_step = ball_move_step;
            best_point_goal_dist2 = goal_dist2;
        }
    }

    if ( best_point.isValid() )
    {
        *result_self_move_step = best_self_move_step;
        *result_offside_reach_step = best_offside_reach_step;
        *result_ball_move_step = best_ball_move_step;
#ifdef DEBUG_PRINT_PASS_REQUEST
        dlog.addText( Logger::TEAM,
                      __FILE__":(getReceivePos) target=(%.1f %.1f) self_step=%d offside_step=%d ball_step=%d",
                      best_point.x, best_point.y,
                      best_self_move_step, best_offside_reach_step, best_ball_move_step );
        dlog.addRect( Logger::TEAM,
                      best_point.x - 0.1, best_point.y - 0.1, 0.2, 0.2,
                      "#0FF", true );
#endif
    }

    return best_point;
}


/*-------------------------------------------------------------------*/
/*!

 */
int
Bhv_SideHalfPassRequestMove2013::getTeammateReachStep( const WorldModel & wm )
{
    const PlayerObject * t = wm.getTeammateNearestToBall( 5 );
    if ( t
         && t->distFromBall() < ( t->playerTypePtr()->kickableArea()
                                  + t->distFromSelf() * 0.05
                                  + wm.ball().distFromSelf() * 0.05 ) )
    {
        return 0;
    }

    return wm.interceptTable()->teammateReachStep();
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfPassRequestMove2013::isMoveSituation( const WorldModel & wm )
{
    if ( wm.self().pos().x > 33.0 )
    {
        dlog.addText( Logger::TEAM | Logger::ROLE,
                      __FILE__":(isMoveSituation) [false] too front" );
        return false;
    }

    if ( wm.self().pos().x > wm.offsideLineX() - 0.75 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(isMoveSituation) [false] in offside area" );
        return false;
    }

    if ( wm.teammatesFromBall().empty() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (isMoveSituation) [false] no teammate" );
        return false;
    }

    const int teammate_step = wm.interceptTable()->teammateReachCycle();
    const int opponent_step = wm.interceptTable()->opponentReachCycle();

    if ( teammate_step <= opponent_step )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(isMoveSituation) [true] our ball?" );
        return true;
    }

    const PlayerObject * t =  wm.teammatesFromBall().front();
    if ( t->distFromBall() < ( t->playerTypePtr()->kickableArea()
                               + t->distFromSelf() * 0.05
                               + wm.ball().distFromSelf() * 0.05 ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (isMoveSituation) [true] maybe teammate kickable" );
        return true;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (isMoveSituation) [false] their ball " );
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfPassRequestMove2013::checkOtherReceiver( const WorldModel & wm,
                                                     const Vector2D & receive_pos )
{
    const double my_dist2 = wm.self().pos().dist2( receive_pos );

    for ( PlayerPtrCont::const_iterator t = wm.teammatesFromBall().begin(),
              end = wm.teammatesFromBall().end();
          t != end;
          ++t )
    {
        if ( (*t)->goalie() ) continue;
        if ( (*t)->posCount() > 10 ) continue;

        if ( (*t)->pos().dist2( receive_pos ) < my_dist2 )
        {
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfPassRequestMove2013::checkOpponent( const WorldModel & wm,
                                                const Vector2D & first_ball_pos,
                                                const Vector2D & receive_pos,
                                                const double first_ball_speed,
                                                const int ball_move_step )
{
    const ServerParam & SP = ServerParam::i();

    const AngleDeg ball_move_angle = ( receive_pos - first_ball_pos ).th();
    const Vector2D first_ball_vel = Vector2D::from_polar( first_ball_speed, ball_move_angle );
#ifdef DEBUG_PRINT_PASS_REQUEST
    dlog.addText( Logger::TEAM,
                  "%d: ball_vel=(%.3f %.3f) speed=%.3f angle=%.1f ball_step=%d",
                  M_count,
                  first_ball_vel.x, first_ball_vel.y,
                  first_ball_speed, ball_move_angle.degree(),
                  ball_move_step );
#endif

    for ( PlayerPtrCont::const_iterator o = wm.opponentsFromBall().begin(),
              end = wm.opponentsFromBall().end();
          o != end;
          ++o )
    {
        const PlayerType * ptype = (*o)->playerTypePtr();

        Vector2D ball_pos = first_ball_pos;
        Vector2D ball_vel = first_ball_vel;

        for ( int step = 1; step <= ball_move_step; ++step )
        {
            ball_pos += ball_vel;
            ball_vel *= SP.ballDecay();

            const  double control_area = ( (*o)->goalie()
                                           && ball_pos.x > SP.theirPenaltyAreaLineX()
                                           && ball_pos.absY() < SP.penaltyAreaHalfWidth()
                                           ? ptype->reliableCatchableDist() + 0.1
                                           : ptype->kickableArea() + 0.1 );
            const double ball_dist = (*o)->pos().dist( ball_pos );
            double dash_dist = ball_dist;
            dash_dist -= control_area;

            int opponent_turn = 1;
            int opponent_dash = ptype->cyclesToReachDistance( dash_dist );

#ifdef DEBUG_PRINT_PASS_REQUEST_OPPONENT
            dlog.addText( Logger::TEAM,
                          "%d: opponent %d (%.1f %.1f) dash=%d move_dist=%.1f",
                          M_count,
                          (*o)->unum(), (*o)->pos().x, (*o)->pos().y,
                          opponent_dash, dash_dist );
#endif


            if ( 1 + opponent_turn + opponent_dash < step )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST_OPPONENT
                dlog.addText( Logger::TEAM,
                              "%d: exist reachable opponent %d (%.1f %.1f)",
                              M_count,
                              (*o)->unum(), (*o)->pos().x, (*o)->pos().y );
#endif
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_SideHalfPassRequestMove2013::getDashPower( const WorldModel & wm,
                                               const Vector2D & receive_pos )
{
    const int teammate_step = getTeammateReachStep( wm );

    double dash_power = ServerParam::i().maxDashPower();
    if ( wm.self().pos().x > wm.offsideLineX() - 1.5 )
    {
        dash_power *= 0.8;
    }

    if ( receive_pos.x > wm.offsideLineX()
         && ( teammate_step <= 0
              || wm.existKickableTeammate() ) )
    {
        AngleDeg target_angle = ( receive_pos - wm.self().inertiaFinalPoint() ).th();
        if ( ( target_angle - wm.self().body() ).abs() < 25.0 )
        {
            dash_power = 0.0;

            const Vector2D accel_vec = Vector2D::from_polar( 1.0, wm.self().body() );
            for ( double p = ServerParam::i().maxDashPower(); p > 0.0; p -= 10.0 )
            {
                Vector2D self_vel = wm.self().vel() + accel_vec * ( p * wm.self().dashRate() );
                Vector2D self_next = wm.self().pos() + self_vel;

                if ( self_next.x < wm.offsideLineX() - 0.5 )
                {
                    dash_power = p;
                    break;
                }
            }
        }
    }

    return dash_power;
}
