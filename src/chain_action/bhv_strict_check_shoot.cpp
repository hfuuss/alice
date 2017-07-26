// -*-c++-*-

/*!
  \file bhv_strict_check_shoot.cpp
  \brief strict checked shoot behavior using ShootGenerator
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_strict_check_shoot.h"

#include "shoot_generator.h"

#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/action/body_tackle_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include<rcsc/common/server_param.h>
#include<rcsc/player/intercept_table.h>
#include  "field_analyzer.h"
using namespace rcsc;



bool
Bhv_StrictCheckShoot::execute( PlayerAgent * agent )
{  const WorldModel & wm = agent->world();

    if ( ! wm.self().isKickable() )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " not ball kickable!"
                  << std::endl;
        dlog.addText( Logger::ACTION,
                      __FILE__":  not kickable" );
        return false;
    }
    const ShootGenerator::Container & cont = ShootGenerator::instance().courses( wm );

       // update
       if ( cont.empty() )
       {
           dlog.addText( Logger::SHOOT,
                         __FILE__": no shoot course" );
           return false;
       }

       ShootGenerator::Container::const_iterator best_shoot
           = std::min_element( cont.begin(),
                               cont.end(),
                               ShootGenerator::ScoreCmp() );

       if ( best_shoot == cont.end() )
       {
           dlog.addText( Logger::SHOOT,
                         __FILE__": no best shoot" );
           return false;
       }


       const int self_min=wm.interceptTable()->selfReachCycle();
       const int opp_min=wm.interceptTable()->opponentReachCycle();


#if 1
    if ( best_shoot->kick_step_ > 1
         && doHoldTurn( agent, best_shoot->target_point_ ) )
    {
        return true;
    }
#endif

    // it is necessary to evaluate shoot courses

    agent->debugClient().addMessage( "Shoot" );
    agent->debugClient().setTarget( best_shoot->target_point_ );

    Vector2D one_step_vel
        = KickTable::calc_max_velocity( ( best_shoot->target_point_ - wm.ball().pos() ).th(),
                                        wm.self().kickRate(),
                                        wm.ball().vel() );
    double one_step_speed = one_step_vel.r();

    dlog.addText( Logger::SHOOT,
                  __FILE__": shoot[%d] target=(%.2f, %.2f) first_speed=%f one_kick_max_speed=%f",
                  best_shoot->index_,
                  best_shoot->target_point_.x,
                  best_shoot->target_point_.y,
                  best_shoot->first_ball_speed_,
                  one_step_speed );

    double  opp_dist_ball = wm.getDistOpponentNearestToBall(5,true);

    if ( one_step_speed > best_shoot->first_ball_speed_ /** 0.99 */&&opp_dist_ball<3.5)
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__": force 1 step. one_step_speed=%f best_speed=%f",
                      one_step_speed, best_shoot->first_ball_speed_ );
        if ( Body_KickOneStep( best_shoot->target_point_,
                               best_shoot->first_ball_speed_ ).execute( agent ) )
            // if ( Body_SmartKick( best_shoot->target_point_,
            //                      one_step_speed,
            //                      one_step_speed * 0.99 - 0.0001,
            //                      1 ).execute( agent ) )
        {
#ifdef DEBUG_PRINT
            paint_shoot_course( wm.ball().pos(), one_step_vel );
#endif
            agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
            agent->debugClient().addMessage( "Force1Step" );
            return true;
        }
    }

    dlog.addText( Logger::SHOOT,
                  __FILE__": try smart kick" );

    if ( Body_SmartKick( best_shoot->target_point_,
                         /*best_shoot->first_ball_speed_*/ServerParam::i().ballSpeedMax(),
                         best_shoot->first_ball_speed_ /** 0.99*/,
                         3 ).execute( agent ) )
    {
#ifdef DEBUG_PRINT
        paint_shoot_course( wm.ball().pos(), best_shoot->first_ball_vel_ );
#endif
        if ( ! doTurnNeckToShootPoint( agent, best_shoot->target_point_ ) )
        {
            agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
        }
        return true;
    }

    dlog.addText( Logger::SHOOT,
                  __FILE__": failed" );
    return false;
}



/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_StrictCheckShoot::doHoldTurn( PlayerAgent * agent,
                       const Vector2D & shoot_point )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();
    const PlayerType & ptype = wm.self().playerType();

    const AbstractPlayerObject * goalie = wm.getOpponentGoalie();
    if ( ! goalie )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) false. no goalie" );
        return false;
    }

    if ( goalie->seenPosCount() <= 1 )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) false. goalie count %d", goalie->posCount() );
        return false;
    }

    AngleDeg target_angle = ( shoot_point - wm.self().pos() ).th();
    AngleDeg relative_angle = target_angle - wm.self().body();
    if ( relative_angle.abs() < SP.maxNeckAngle() + 20.0 )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) false. only turn neck" );
        return false;
    }

    const PlayerObject * nearest_opponent = wm.getOpponentNearestToSelf( 5, true );
    if ( nearest_opponent
         && nearest_opponent->distFromSelf() < 3.0 )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) false. opponent exists" );
        return false;
    }

    const Vector2D self_next = wm.self().pos() + wm.self().vel();
    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

    if ( self_next.dist2( ball_next ) < std::pow( ptype.kickableArea() - 0.3, 2 ) )
    {
        agent->debugClient().addMessage( "Shoot:Turn" );
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) true. do turn" );
        agent->doTurn( relative_angle );
        agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
        return true;
    }

    const Vector2D kick_accel = self_next - ball_next;
    const double kick_accel_len = kick_accel.r();

    if ( kick_accel_len - ptype.playerSize()*0.5 < SP.maxPower() * wm.self().kickRate() )
    {
        agent->debugClient().addMessage( "Shoot:Collide" );
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) true. do collision kick" );
        double kick_power = std::min( kick_accel_len / wm.self().kickRate(), SP.maxPower() );
        agent->doKick( kick_power, kick_accel.th() - wm.self().body() );
        agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
        return true;
    }

    double ball_speed = wm.ball().vel().r();
    double stop_ball_speed = std::max( 0.0, ball_speed - SP.maxPower() * wm.self().kickRate() );
    Vector2D stop_ball_vel = wm.ball().vel();
    if ( ball_speed > 1.0e-3 )
    {
        stop_ball_vel *= stop_ball_speed / ball_speed;
    }

    Vector2D ball_next2 = wm.ball().pos() + stop_ball_vel * ( 1.0 + SP.ballDecay() );
    Vector2D self_next2 = self_next + wm.self().vel() * ptype.playerDecay();

    if ( self_next2.dist2( ball_next2 ) > std::pow( ptype.kickableArea() - 0.3, 2 ) )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__":(doHoldTurn) false. cannot keep the ball" );
        return false;
    }

    agent->debugClient().addMessage( "Shoot:StopBall" );
    dlog.addText( Logger::SHOOT,
                  __FILE__":(doHoldTurn) false. stop the ball" );

    double kick_power = std::min( ball_speed / wm.self().kickRate(), SP.maxPower() );
    AngleDeg kick_dir = ( wm.ball().vel().th() + 180.0 ) - wm.self().body();
    agent->doKick( kick_power, kick_dir );
    agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_StrictCheckShoot::doTurnNeckToShootPoint( PlayerAgent * agent,
                                   const Vector2D & shoot_point )
{
    const WorldModel & wm = agent->world();
    Vector2D target_point = shoot_point;

    const AbstractPlayerObject * goalie = wm.getOpponentGoalie();
    if ( goalie )
    {
        if ( goalie->posCount() > 1 )
        {
            return false;
        }

        if ( goalie->posCount() > 0 )
        {
            AngleDeg target_angle = agent->effector().queuedNextAngleFromBody( target_point );
            AngleDeg goalie_angle = agent->effector().queuedNextAngleFromBody( goalie->pos() );

            if ( ( target_angle - goalie_angle ).abs() < agent->effector().queuedNextViewWidth().width() )
            {
                target_point += goalie->pos();
                target_point *= 0.5;
            }
        }
    }

    const double angle_buf = 10.0; // Magic Number

    if ( agent->effector().queuedNextCanSeeWithTurnNeck( target_point, angle_buf ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": look the mid point(%.2f %.2f)",
                      target_point.x, target_point.y );

        agent->debugClient().addMessage( "Shoot:NeckToMid" );
        agent->setNeckAction( new Neck_TurnToPoint( target_point ) );
        return true;
    }

    if ( agent->effector().queuedNextCanSeeWithTurnNeck( shoot_point, angle_buf ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": look the shoot point(%.2f %.2f)",
                      shoot_point.x, target_point.y );

        agent->debugClient().addMessage( "Shoot:NeckToShot" );
        agent->setNeckAction( new Neck_TurnToPoint( shoot_point ) );
        return true;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": cannot look the mid point or target point" );

    return false;
}
