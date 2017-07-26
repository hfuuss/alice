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

#include "bhv_side_half_offensive_move.h"

#include "strategy.h"
#include "field_analyzer.h"

#include "role_side_half.h"

#include "bhv_side_half_pass_request_move.h"


#include "neck_offensive_intercept_neck.h"


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


using namespace rcsc;

namespace {

/*-------------------------------------------------------------------*/
/*!

 */
bool
PassRequestMove_checkOtherReceiver( const int /*count*/,
                                    const WorldModel & wm,
                                    const Vector2D & receive_pos )
{
    const double my_dist2 = wm.self().pos().dist2( receive_pos );

    for ( PlayerPtrCont::const_iterator t = wm.teammatesFromBall().begin(), end = wm.teammatesFromBall().end();
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
PassRequestMove_checkOpponent( const int count,
                               const WorldModel & wm,
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
                  count,
                  first_ball_vel.x, first_ball_vel.y, first_ball_speed, ball_move_angle.degree(),
                  ball_move_step );
#else
    (void)count;
#endif

    for ( PlayerPtrCont::const_iterator o = wm.opponentsFromBall().begin(), end = wm.opponentsFromBall().end();
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

            const  double control_area = ( ( *o)->goalie()
                                           && ball_pos.x > SP.theirPenaltyAreaLineX()
                                           && ball_pos.absY() < SP.penaltyAreaHalfWidth()
                                           ? ptype->reliableCatchableDist() + 0.1
                                           : ptype->kickableArea() + 0.1 );
            const double ball_dist = (*o)->pos().dist( ball_pos );
            double dash_dist = ball_dist;
            dash_dist -= control_area;
            //dash_dist -= ptype->realSpeedMax() * std::min( 5.0, (*o)->posCount() * 0.8 );

            int opponent_turn = 1;
            int opponent_dash = ptype->cyclesToReachDistance( dash_dist );

#ifdef DEBUG_PRINT_PASS_REQUEST_OPPONENT
            dlog.addText( Logger::TEAM,
                          "%d: opponent %d (%.1f %.1f) dash=%d move_dist=%.1f",
                          count,
                          (*o)->unum(), (*o)->pos().x, (*o)->pos().y,
                          opponent_dash, dash_dist );
#endif


            if ( 1 + opponent_turn + opponent_dash < step )
            {
#ifdef DEBUG_PRINT_PASS_REQUEST_OPPONENT
                dlog.addText( Logger::TEAM,
                              "%d: exist reachable opponent %d (%.1f %.1f)",
                              count,
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
int
get_teammate_reach_step( const WorldModel & wm )
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
Vector2D
PassRequestMove_getReceivePos( const WorldModel & wm,
                               int * result_self_move_step,
                               int * result_offside_reach_step,
                               int * result_ball_move_step )
{
    const ServerParam & SP = ServerParam::i();
    const Vector2D goal_pos = SP.theirTeamGoalPos();

    const int teammate_step = get_teammate_reach_step( wm );
    const Vector2D first_ball_pos = wm.ball().inertiaPoint( teammate_step );
    const Vector2D self_pos = wm.self().inertiaFinalPoint();

    int count = 0;

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

            ++count;
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
            if ( PassRequestMove_checkOtherReceiver( count, wm, receive_pos ) )
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

            // check other receivers
            if ( PassRequestMove_checkOpponent( count, wm, first_ball_pos, receive_pos,
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
bool
is_intercept_situation( const WorldModel & wm )
{
    static int s_previous_teammate_step = 1000;
    static GameTime s_previous_time( -1, 0 );

    if ( s_previous_time.stopped() == 0
         && s_previous_time.cycle() + 1 == wm.time().cycle() )
    {
        if ( s_previous_teammate_step <= 1 )
        {
            if ( ! wm.teammatesFromBall().empty() )
            {
                const PlayerObject * t =  wm.teammatesFromBall().front();
                if ( t->distFromBall()
                     < t->playerTypePtr()->kickableArea()
                     + t->distFromSelf() * 0.05
                     + wm.ball().distFromSelf() * 0.05 )
                {
                    dlog.addText( Logger::TEAM,
                                  __FILE__":(is_intercept_situation) maybe teammate kickable" );
                    s_previous_teammate_step = wm.interceptTable()->teammateReachCycle();
                    s_previous_time = wm.time();
                    return false;
                }
            }
        }
    }

    int self_step = wm.interceptTable()->selfReachStep();
    int teammate_step = wm.interceptTable()->teammateReachStep();
    int opponent_step = wm.interceptTable()->teammateReachStep();

    s_previous_teammate_step = teammate_step;
    s_previous_time = wm.time();

    if ( wm.existKickableTeammate() )
    {
        return false;
    }

    if ( self_step >= 3 )
    {
        Vector2D ball_pos = wm.ball().inertiaPoint( self_step );
        if ( ball_pos.x < 36.0
             && ( wm.existKickableOpponent()
                  || opponent_step <= self_step - 5 ) )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__":(is_intercept_situation) false: opponent has ball" );
            return false;
        }
    }

    if ( ( 2 <= teammate_step
           && self_step <= 4 )
         || ( self_step <= teammate_step + 1
              && 4 <= teammate_step ) )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
class IntentionSideHalfMove
    : public SoccerIntention {
private:
    const Vector2D M_target_point;
    GameTime M_last_time;
    int M_ball_holder_unum;
    int M_count;
    int M_offside_count;
    int M_their_ball_count;
public:

    IntentionSideHalfMove( const Vector2D & target_point,
                           const GameTime & start_time,
                           const int ball_holder_unum )
        : M_target_point( target_point ),
          M_last_time( start_time ),
          M_ball_holder_unum( ball_holder_unum ),
          M_count( 0 ),
          M_offside_count( 0 ),
          M_their_ball_count( 0 )
      { }

    bool finished( const PlayerAgent * agent );
    bool execute( PlayerAgent * agent );
};


/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionSideHalfMove::finished( const PlayerAgent * agent )
{
    //if ( ++M_count > 15 )
    if ( ++M_count > 5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) no remained step" );
        return true;
    }

    const WorldModel & wm = agent->world();

    if ( wm.time().cycle() - 1 != M_last_time.cycle() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) time mismatch" );
        return true;
    }

    if ( wm.audioMemory().passTime() == wm.time()
         && ! wm.audioMemory().pass().empty()
         && ( wm.audioMemory().pass().front().receiver_ == wm.self().unum() )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) hear pass message" );
        return true;
    }

    // if ( wm.self().pos().x > wm.offsideLineX() + 1.0 )
    // {
    //     dlog.addText( Logger::TEAM,
    //                   __FILE__":(IntentionSideHalfMove::finished) too over offside line" );
    //     return true;
    // }

    if ( wm.self().pos().x > wm.offsideLineX() )
    {
        ++M_offside_count;
        if ( M_offside_count >= 5 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__":(IntentionSideHalfMove::finished) over offside line count" );
            return true;
        }
    }

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( opp_min < mate_min )
    {
        ++M_their_ball_count;
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) their ball count = %d",
                      M_their_ball_count );
        if ( M_their_ball_count >= 3 )
        {
            return true;
        }
    }

    if ( ! wm.existKickableTeammate()
         && self_min <= mate_min )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) my ball" );
        return true;
    }

    // if ( M_ball_holder_unum != Unum_Unknown
    //      && wm.interceptTable()->fastestTeammate()
    //      && wm.interceptTable()->fastestTeammate()->unum() != M_ball_holder_unum )
    // {
    //     dlog.addText( Logger::TEAM,
    //                   __FILE__":(IntentionSideHalfMove::finished) ball holder changed." );
    //     return true;
    // }

    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );

    if ( home_pos.x > M_target_point.x + 5.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__":(IntentionSideHalfMove::finished) home pos x >> target x." );
        return true;
    }

    const Vector2D my_pos = wm.self().inertiaFinalPoint();
    if ( my_pos.dist2( M_target_point ) < 1.5 )
    {
        if ( Bhv_SideHalfOffensiveMove::is_marked( wm ) )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__":(IntentionSideHalfMove::finished) exist marker." );
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionSideHalfMove::execute( PlayerAgent * agent )
{
    const double dash_power = ServerParam::i().maxDashPower();

    M_last_time = agent->world().time();

    dlog.addText( Logger::TEAM,
                  __FILE__": (IntentionSideHalfMove::execute) target=(%.1f %.1f) power=%.1f",
                  M_target_point.x, M_target_point.y,
                  dash_power );

    agent->debugClient().addMessage( "SHOffMove:Intention%d", M_count );

    Bhv_SideHalfOffensiveMove::go_to_point( agent, M_target_point, dash_power );
    Bhv_SideHalfOffensiveMove::set_turn_neck( agent );

    return true;
}

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfOffensiveMove::execute( PlayerAgent * agent )
{
    if ( doIntercept( agent ) )
    {
        return false;
    }

//    //if ( doBlock( agent ) )
//    if ( doGetBall( agent ) )
//    {
//        return true;
//    }

    if ( Bhv_SideHalfPassRequestMove2013().execute( agent ) )
    {
         return true;
    }



    if ( doCrossMove( agent ) )
    {
        return true;
    }


    doNormalMove( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfOffensiveMove::doIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( is_intercept_situation( wm ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doIntercept)" );
        agent->debugClient().addMessage( "SHOffMove:Intercept" );

        Vector2D face_point( 52.5, wm.self().pos().y );
        if ( wm.self().pos().absY() < 10.0 )
        {
            face_point.y *= 0.8;
        }
        else if ( wm.self().pos().absY() < 20.0 )
        {
            face_point.y *= 0.9;
        }

        Body_Intercept( true, face_point ).execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        return true;
    }

    return false;
}





bool
Bhv_SideHalfOffensiveMove::doGetBall( PlayerAgent * agent )
{
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfOffensiveMove::doPassRequestMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int teammate_step = wm.interceptTable()->teammateReachCycle();
    const int opponent_step = wm.interceptTable()->opponentReachCycle();

    if ( teammate_step > opponent_step )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doPassRequestMove) their ball?" );
        const PlayerObject * t =  wm.teammatesFromBall().front();
        if ( t->distFromBall()
             < t->playerTypePtr()->kickableArea()
             + t->distFromSelf() * 0.05
             + wm.ball().distFromSelf() * 0.05 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (doPassRequestMove) maybe teammate kickable" );
        }
        else
        {
            return false;
        }
    }


    if ( wm.self().pos().x > wm.offsideLineX() - 0.5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doPassRequestMove) avoid offside" );
        return false;
    }

#if 1
    // 2012-06-21
    if ( wm.self().pos().x > 33.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doPassRequestMove) too front" );
        return false;
    }
#endif

#ifdef DEBUG_PRINT_PASS_REQUEST
    dlog.addText( Logger::TEAM,
                  __FILE__": (doPassRequestMove) start" );
#endif

    int self_move_step = 1000;
    int offside_reach_step = -1;
    int ball_move_step = 1000;
    const Vector2D receive_pos = PassRequestMove_getReceivePos( wm,
                                                                &self_move_step,
                                                                &offside_reach_step,
                                                                &ball_move_step );

    if ( ! receive_pos.isValid() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doPassRequestMove) target point not found." );
        return false;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (doPassRequestMove) pos=(%.1f %.1f) self_move_step=%d offside_step=%d teammate_step=%d ball_move_step=%d",
                  receive_pos.x, receive_pos.y,
                  self_move_step, offside_reach_step, teammate_step, ball_move_step );
#ifdef DEBUG_PAINT_REQUESTED_PASS_COURSE
    const Vector2D first_ball_pos = wm.ball().inertiaPoint( teammate_step );
    dlog.addLine( Logger::TEAM,
                  first_ball_pos, receive_pos, "#0F0" );
#endif

    if ( offside_reach_step >= 0
         && offside_reach_step <= teammate_step )
    {
        const Vector2D my_inertia = wm.self().inertiaPoint( teammate_step + 1 );
        // if ( my_inertia.x > wm.offsideLineX() - 0.5 )
        // {
        //     Vector2D avoid_offside_pos = wm.self().pos();
        //     avoid_offside_pos.x -= 1.0;
        //     agent->debugClient().addMessage( "SH:PassRequest:WaitOffside:Back" );
        //     dlog.addText( Logger::TEAM,
        //                   __FILE__": (doPassRequestMove) go to back" );
        //     Bhv_SideHalfOffensiveMove::go_to_point( agent,
        //                                             avoid_offside_pos,
        //                                             ServerParam::i().maxDashPower() );
        // } else
        if ( my_inertia.x > wm.offsideLineX() - 0.8 )
        {
            Body_TurnToPoint( receive_pos ).execute( agent );
            agent->debugClient().addMessage( "SH:PassRequest:WaitOffside:Turn" );
            dlog.addText( Logger::TEAM,
                          __FILE__": (doPassRequestMove) turn to" );
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
            Bhv_SideHalfOffensiveMove::go_to_point( agent, receive_pos, dash_power );
        }
        Bhv_SideHalfOffensiveMove::set_turn_neck( agent );
        agent->setArmAction( new Arm_PointToPoint( receive_pos ) );
        return true;
    }

    // if ( self_move_step < ball_move_step )
    // {
    //     agent->debugClient().addMessage( "SHOffMove:PassRequestWaitKick" );
    //     dlog.addText( Logger::TEAM,
    //                   __FILE__": (doPassRequestMove) wait" );
    //     Body_TurnToPoint( receive_pos ).execute( agent );
    //     Bhv_SideHalfOffensiveMove::set_turn_neck( agent );
    //     return true;
    // }

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

    dlog.addText( Logger::TEAM,
                  __FILE__": (doPassRequestMove) go to point. dash_power=%.1f", dash_power );

    agent->debugClient().addMessage( "SHOffMove:PassRequest%.0f", dash_power );


    Bhv_SideHalfOffensiveMove::go_to_point( agent, receive_pos, dash_power );
    Bhv_SideHalfOffensiveMove::set_turn_neck( agent );
    agent->setArmAction( new Arm_PointToPoint( receive_pos ) );
    return true;
}

namespace {

bool
CrossMove_existCloserTeammate( const WorldModel & wm,
                               const Vector2D & target_point,
                               const double self_move_dist )
{
    const PlayerObject * passer = wm.interceptTable()->fastestTeammate();
    const double self_move_dist2 = std::pow( self_move_dist, 2 );

    for ( PlayerPtrCont::const_iterator t = wm.teammatesFromSelf().begin(), end = wm.teammatesFromSelf().end();
          t != end;
          ++t )
    {
        if ( passer == *t ) continue;

        double move_dist2 = (*t)->pos().dist2( target_point );
        if ( move_dist2 < self_move_dist2 )
        {
            return true;
        }
    }

    return false;
}

bool
CrossMove_existPassCourse( const WorldModel & wm,
                           const Vector2D & first_ball_pos,
                           const double first_ball_speed,
                           const Vector2D & target_point,
                           const int ball_move_step )
{
    const Vector2D first_ball_vel = ( target_point - first_ball_pos ).setLengthVector( first_ball_speed );


    for ( AbstractPlayerCont::const_iterator o = wm.theirPlayers().begin(),
              end = wm.theirPlayers().end();
          o != end;
          ++o )
    {
        Vector2D ball_pos = first_ball_pos;
        Vector2D ball_vel = first_ball_vel;
        for ( int step = 1; step <= ball_move_step; ++step )
        {
            ball_pos += ball_vel;
            ball_vel *= ServerParam::i().ballDecay();

            Vector2D opponent_pos = (*o)->inertiaPoint( step );
            double dash_dist = opponent_pos.dist( ball_pos );
            int dash_step = (*o)->playerTypePtr()->cyclesToReachDistance( dash_dist );
            if ( dash_step + 3 <= step )
            {
                return false;
            }

        }
    }

    return true;
}

double
CrossMove_getOpponentDist( const WorldModel & wm,
                           const Vector2D & target_point,
                           const int ball_move_step )
{
    double min_dist = 100000.0;

    for ( AbstractPlayerCont::const_iterator o = wm.theirPlayers().begin(), end = wm.theirPlayers().end();
          o != end;
          ++o )
    {
        if ( (*o)->posCount() > 8 ) continue;

        double dist = (*o)->pos().dist( target_point );
        double chase_step = std::max( 0, bound( 0, (*o)->posCount() -2, 2 ) + ball_move_step - 1 );
        dist -= (*o)->playerTypePtr()->realSpeedMax() * chase_step;

        if ( dist < min_dist )
        {
            min_dist = dist;
        }
    }

    return min_dist;
}

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfOffensiveMove::doCrossMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.offsideLineX() < ServerParam::i().theirPenaltyAreaLineX()  )
    {
        dlog.addText( Logger::ROLE,
                      __FILE__": (doCrossMove) offside line < penalty line" );
        return false;
    }

    const int teammate_step = wm.interceptTable()->teammateReachCycle();
    const PlayerObject * passer = wm.interceptTable()->fastestTeammate();
    if ( ! passer )
    {
        dlog.addText( Logger::ROLE,
                      __FILE__": (doCrossMove) NULL passer" );
        return false;
    }

    const Vector2D ball_pos = wm.ball().inertiaPoint( teammate_step );

    if ( ball_pos.dist2( ServerParam::i().theirTeamGoalPos() )
         > std::pow( 35.0, 2 ) )
    {
        dlog.addText( Logger::ROLE,
                      __FILE__": (doCrossMove) ball is far from goal" );
        return false;
    }


    const double y_step = ServerParam::i().goalWidth() / 14.0;
    const double max_x = ServerParam::i().pitchHalfLength() - 5.0;

    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
    double best_home_pos_dist2 = 1000000.0;
    Vector2D best_point = Vector2D::INVALIDATED;

    int count = 0;
    for ( double x = ServerParam::i().theirPenaltyAreaLineX();
          x < max_x;
          x += 2.0 )
    {
        if ( x > wm.offsideLineX() + 0.5 )
        {
            break;
        }

        double y = -ServerParam::i().goalHalfWidth();
        for ( int iy = 0; iy <= 14; ++iy, y += y_step )
        {
            ++count;

            const Vector2D target_point( x, y );

            const double self_move_dist = wm.self().pos().dist( target_point );
            const int self_move_step = wm.self().playerType().cyclesToReachDistance( self_move_dist );
            const double ball_move_dist = ball_pos.dist( target_point );
            const double first_ball_speed
                = std::min( ServerParam::i().ballSpeedMax(),
                            ServerParam::i().firstBallSpeed( ball_move_dist,
                                                             std::max( 1, self_move_step - teammate_step - 2 ) ) );
            const int ball_move_step = ServerParam::i().ballMoveStep( first_ball_speed, ball_move_dist );

#ifdef DEBUG_PRINT
            const int total_step = teammate_step + 2 + ball_move_step;
            dlog.addText( Logger::ROLE,
                          "%d: (%.2f %.2f) total_step=%d",
                          count, target_point.x, target_point.y, total_step );
            dlog.addText( Logger::ROLE,
                          "%d: self move_dist=%.1f move_step=%d",
                          count, self_move_dist, self_move_step );
            dlog.addText( Logger::ROLE,
                          "%d: ball move_dist=%.1f move_step=%d first_speed=%.2f",
                          count, ball_move_dist, ball_move_step, first_ball_speed );
#endif

            //
            // filter
            //
            if ( first_ball_speed < 1.7 )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::ROLE,
                              "%d: skip. first_ball_speed", count );
#endif
                continue;
            }

            if ( CrossMove_existCloserTeammate( wm, target_point, self_move_dist ) )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::ROLE,
                              "%d: skip. exist other teammate", count );
#endif
                continue;
            }

            // if ( ! ShootSimulator::can_shoot_from( true, target_point, wm.theirPlayers(), 8 ) )
            // {
            //     dlog.addText( Logger::ROLE,
            //                   "%d: skip. no shoot course", count );
            //     continue;
            // }

            if ( ! CrossMove_existPassCourse( wm, ball_pos, first_ball_speed, target_point, ball_move_step ) )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::ROLE,
                              "%d: skip. no pass course", count );
#endif
                continue;
            }

            const double opponent_dist = CrossMove_getOpponentDist( wm, target_point, ball_move_step );

            if ( opponent_dist < 3.0 )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::ROLE,
                              "%d: skip. opponent_dist=%.1f", count, opponent_dist );
#endif
                continue;
            }

            double home_pos_dist2 = home_pos.dist2( target_point );
            if ( best_home_pos_dist2 > home_pos_dist2 )
            {
                best_home_pos_dist2 = home_pos_dist2;
                best_point = target_point;
#ifdef DEBUG_PRINT
                dlog.addText( Logger::ROLE,
                              "%d: >>>> update <<<<", count );
#endif
            }

        }
    }

    if ( ! best_point.isValid() )
    {
        dlog.addText( Logger::ROLE,
                      __FILE__"(doCrossMove) not found" );
        return false;
    }

    dlog.addText( Logger::ROLE,
                  "(doCrossMove) Best Point (%.2f %.2f)",
                  best_point.x, best_point.y );

    agent->debugClient().addMessage( "SHOffMove:CrossMove" );

    Bhv_SideHalfOffensiveMove::go_to_point( agent, best_point, ServerParam::i().maxDashPower() );
    Bhv_SideHalfOffensiveMove::set_turn_neck( agent );

    agent->setIntention( new IntentionSideHalfMove( best_point, wm.time(), passer->unum() ) );
    agent->setArmAction( new Arm_PointToPoint( best_point ) );

    return true;
}




/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfOffensiveMove::doNormalMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    // if ( wm.offsideLineX() - 1.0 < wm.self().pos().x
    //      && wm.self().pos().x < wm.offsideLineX() + 2.0 )
    // {
    //     target_point.y = wm.self().inertiaFinalPoint().y;
    // }

    const double dash_power = Bhv_SideHalfOffensiveMove::get_dash_power( wm, target_point );

    dlog.addText( Logger::TEAM,
                  __FILE__": (doNormalMove) target=(%.1f %.1f) power=%.1f",
                  target_point.x, target_point.y, dash_power );

    agent->debugClient().addMessage( "SHOffMove:Normal%.0f", dash_power );

    Bhv_SideHalfOffensiveMove::go_to_point( agent, target_point, dash_power );
    Bhv_SideHalfOffensiveMove::set_change_view( agent );
    Bhv_SideHalfOffensiveMove::set_turn_neck( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfOffensiveMove::go_to_point( PlayerAgent * agent,
                                        const Vector2D & target_point,
                                        const double dash_power )
{
    //
    // TODO: avoid ball holder.
    //

    const WorldModel & wm = agent->world();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    // const Vector2D mate_trap_pos = wm.ball().inertiaPoint( mate_min );

    // double dist_thr = std::fabs( wm.ball().pos().x - wm.self().pos().x ) * 0.2 + 0.25;
    // if ( dist_thr < 1.0 ) dist_thr = 1.0;
    // if ( target_point.x > wm.self().pos().x - 0.5
    //      && wm.self().pos().x < wm.offsideLineX()
    //      && std::fabs( target_point.x - wm.self().pos().x ) > 1.0 )
    // {
    //     dist_thr = std::min( 1.0, wm.ball().pos().dist( target_point ) * 0.1 + 0.5 );
    // }
    double dist_thr = std::min( 1.0, wm.ball().pos().dist( target_point ) * 0.1 + 0.5 );

    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );


    const Vector2D my_inertia = wm.self().inertiaPoint( mate_min );
    const double target_dist = my_inertia.dist( target_point );

    if ( mate_min <= 5
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

        agent->debugClient().addMessage( "SHOffMove:Back%.0f", back_dash_power );
        dlog.addText( Logger::TEAM,
                      __FILE__": Back Move. power=%.1f", back_dash_power );

        agent->doDash( back_dash_power );
        return;
    }

    if ( Body_GoToPoint( target_point, dist_thr, dash_power,
                         -1.0, // dash speed
                         100, // cycle
                         true, // save recovery
                         25.0 // angle threshold
                         ).execute( agent ) )
    {
        return;
    }

    AngleDeg body_angle = 0.0;
    if ( target_dist > dist_thr )
    {
        body_angle = ( target_point - my_inertia ).th();
    }

    Body_TurnToAngle( body_angle ).execute( agent );

    agent->debugClient().addMessage( "SHOffMove:Turn" );
    dlog.addText( Logger::TEAM,
                  __FILE__": execute. turn to angle=%.1f",
                  body_angle.degree() );
}


/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfOffensiveMove::set_change_view( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const AngleDeg ball_angle
        = agent->effector().queuedNextAngleFromBody( agent->effector().queuedNextBallPos() );
    const Vector2D offside_line_center( wm.offsideLineX(), 0.0 );
    const AngleDeg center_angle
        = agent->effector().queuedNextAngleFromBody( agent->effector().queuedNextBallPos() );

    const double angle_diff = ( ball_angle - center_angle ).abs();

    dlog.addText( Logger::TEAM,
                  __FILE__": (set_change_view) ball_angle=%.1f offside_line=%.1f center_angle=%.1f",
                  ball_angle.degree(), wm.offsideLineX(), center_angle.degree() );


    if ( angle_diff > ViewWidth::width( ViewWidth::NORMAL ) * 0.5 - 3.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (set_change_view) wide" );
        agent->setViewAction( new View_Wide() );
    }
    else if ( angle_diff > ViewWidth::width( ViewWidth::NARROW ) * 0.5 - 3.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (set_change_view) normal" );
        agent->setViewAction( new View_Normal() );
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (set_change_view) default" );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SideHalfOffensiveMove::set_turn_neck( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const Vector2D mate_trap_pos = wm.ball().inertiaPoint( mate_min );

    if ( ( wm.existKickableTeammate()
           || mate_min <= 2 )
         && wm.self().pos().dist( mate_trap_pos ) < 20.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (set_turn_neck) turn neck to our ball holder." );
        agent->setNeckAction( new Neck_TurnToBallOrScan( -1 ) );
        return;
    }

    int opp_min = wm.interceptTable()->opponentReachCycle();
    int count_thr = 0;
    ViewWidth view_width = agent->effector().queuedNextViewWidth();
    int see_cycle = agent->effector().queuedNextSeeCycles();

    if ( view_width.type() == ViewWidth::WIDE )
    {
        if ( opp_min > see_cycle )
        {
            count_thr = 2;
        }
    }
    else if ( view_width.type() == ViewWidth::NORMAL )
    {
        if ( opp_min > see_cycle )
        {
            count_thr = 1;
        }
    }

    agent->setNeckAction( new Neck_TurnToBallOrScan( count_thr ) );
    dlog.addText( Logger::TEAM,
                  __FILE__": (set_turn_neck) ball or scan. opp_min=%d see_cycle=%d count_thr=%d",
                  opp_min, see_cycle, count_thr );
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_SideHalfOffensiveMove::get_dash_power( const WorldModel & wm,
                                           const Vector2D & target_point )
{
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D receive_pos = wm.ball().inertiaPoint( mate_min );

    if ( target_point.x > wm.self().pos().x
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.7
         && mate_min <= 8
         && ( mate_min <= opp_min + 3
              || wm.existKickableTeammate() )
         && std::fabs( receive_pos.y - target_point.y ) < 25.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) chance. fast move" );
        return ServerParam::i().maxDashPower();
    }

    if  ( wm.self().pos().x > wm.offsideLineX()
          && ( wm.existKickableTeammate()
               || mate_min <= opp_min + 2 )
          && target_point.x < receive_pos.x + 30.0
          && wm.self().pos().dist( receive_pos ) < 30.0
          && wm.self().pos().dist( target_point ) < 20.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) offside max power" );

        return ServerParam::i().maxDashPower();
    }

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) empty capacity. max power" );

        return ServerParam::i().maxDashPower();
    }

    // 2016-06-29


    //------------------------------------------------------
    // decide dash power
    static bool s_recover_mode = false;

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) no stamina capacity. never recover mode" );
        s_recover_mode = false;
    }
    else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.4 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) change to recover mode." );
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.7 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) came back from recover mode" );
        s_recover_mode = false;
    }

    const double my_inc
        = wm.self().playerType().staminaIncMax()
        * wm.self().recovery();

    // dash power
    if ( s_recover_mode )
    {
        // Magic Number.
        // recommended one cycle's stamina recover value
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) recover mode" );
        //return std::max( 0.0, my_inc - 30.0 );
        return 1.0;
    }

    if ( ! wm.opponentsFromSelf().empty()
         && wm.opponentsFromSelf().front()->distFromSelf() < 2.0
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.7
         && mate_min <= 8
         && ( mate_min <= opp_min + 3
              || wm.existKickableTeammate() )
         )
    {
        // opponent is very close
        // full power
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) exist near opponent. full power" );
        return ServerParam::i().maxDashPower();
    }

    if  ( wm.ball().pos().x < wm.self().pos().x
          && wm.self().pos().x < wm.offsideLineX() - 0.5 )
    {
        // ball is back
        // not offside
        dlog.addText( Logger::TEAM,
                      __FILE__": (getDashPower) ball is back and not offside." );
        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
        {
            return std::min( std::max( 5.0, my_inc - 40.0 ),
                             ServerParam::i().maxDashPower() );
            //return 0.1;
        }
        else
        {
            return std::min( my_inc * 1.1,
                             ServerParam::i().maxDashPower() );
        }
    }

    if ( wm.ball().pos().x > wm.self().pos().x + 3.0 )
    {
        // ball is front
        if ( opp_min <= mate_min - 3 )
        {
            if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.6 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (getDashPower) ball is front. recover" );
                // return std::min( std::max( 0.1, my_inc - 30.0 ),
                //                  ServerParam::i().maxDashPower() );
                return 1.0;
            }
            else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (getDashPower) ball is front. keep" );
                //return std::min( my_inc, ServerParam::i().maxDashPower() );
                return my_inc * 0.9;
            }
            else
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (getDashPower) ball is front. max" );
                return ServerParam::i().maxDashPower();
            }
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (getDashPower) ball is front full powerr" );
            return ServerParam::i().maxDashPower();
        }
    }


    dlog.addText( Logger::TEAM,
                  __FILE__": (getDashPower) normal mode." );
    if ( target_point.x > wm.self().pos().x + 2.0
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6 )
    {
        return ServerParam::i().maxDashPower();
    }
    else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
    {
        return std::min( my_inc * 0.9,
                         ServerParam::i().maxDashPower() );
    }
    else
    {
        return std::min( my_inc * 1.5,
                         ServerParam::i().maxDashPower() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SideHalfOffensiveMove::is_marked( const WorldModel & wm )
{
    //
    // TODO: compare with previous analysis.
    // if same player is detected within marker poisition continuously in several times,
    // that player would be the marker.
    //

    static GameTime s_update_time( 0, 0 );
    static bool s_result = false;
    if ( s_update_time == wm.time() )
    {
        return s_result;
    }
    s_update_time = wm.time();

    const double mark_dist_thr2 = std::pow( 2.0, 2 );
    const double mark_line_dist_thr2 = std::pow( 4.0, 2 );
    // const double mark_x_thr = 10.0;
    // const double mark_y_thr = 3.0;

    const Vector2D my_pos = wm.self().inertiaFinalPoint();
    const Vector2D ball_pos = wm.ball().inertiaPoint( wm.interceptTable()->teammateReachCycle() );

    const Sector2D front_space_sector( Vector2D( my_pos.x - 5.0, my_pos.y ),
                                       4.0, 20.0,
                                       -15.0, 15.0 );

    dlog.addSector( Logger::TEAM,
                    front_space_sector, "#F00" );

    for ( PlayerPtrCont::const_iterator p = wm.opponentsFromSelf().begin(),
              end = wm.opponentsFromSelf().end();
          p != end;
          ++p )
    {
        if ( (*p)->distFromSelf() > 15.0 ) break;
        if ( (*p)->ghostCount() >= 5 ) continue;

        const Vector2D opos = (*p)->pos() + (*p)->vel();

        double d2 = opos.dist2( my_pos );
        if ( d2 < mark_dist_thr2 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_marked) found (1) marker %d (%.1f %.1f)",
                          (*p)->unum(), (*p)->pos().x, (*p)->pos().y );
            s_result = true;
            return true;
        }

        // opponent exists on my front space
        if ( ! (*p)->goalie()
             && my_pos.x > wm.offsideLineX() - 10.0
             && front_space_sector.contains( opos )
             // && std::fabs( opos.y - my_pos.y ) < mark_y_thr
             // && my_pos.x < opos.x
             // && opos.x < my_pos.x + mark_x_thr
             )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_marked) found (2) marker %d (%.1f %.1f)",
                          (*p)->unum(), (*p)->pos().x, (*p)->pos().y );
            s_result = true;
            return true;
        }

        // opponent exist on pass line
        if ( d2 < mark_line_dist_thr2
             && opos.x > my_pos.x - 1.0
             && ( opos.y - my_pos.y ) * ( ball_pos.y - my_pos.y ) > 0.0 )
        {
            AngleDeg ball_to_self = ( my_pos - ball_pos ).th();
            AngleDeg ball_to_opp = ( (*p)->pos() - ball_pos ).th();

            if ( ( ball_to_self - ball_to_opp ).abs() < 10.0 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (is_marked) found (3) marker %d (%.1f %.1f)",
                              (*p)->unum(), (*p)->pos().x, (*p)->pos().y );
                s_result = true;
                return true;
            }
        }
    }

    s_result = false;
    return false;
}

