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

#include "bhv_penalty_kick.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"

#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_go_to_static_ball.h"

#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_dribble2008.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_smart_kick.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/neck_scan_field.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/penalty_kick_state.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>
#include "neck_default_intercept_neck.h"
#include<rcsc/action/neck_turn_to_ball_or_scan.h>
#include "neck_check_ball_owner.h"
#include <rcsc/action/view_synch.h>
#include "bhv_strict_check_shoot.h"
#include "bhv_chain_action.h"
#include "bhv_basic_offensive_kick.h"
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include "bhv_savior.h"
using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    int num = wm.self().unum();
    
    switch ( wm.gameMode().type() ) {
    case GameMode::PenaltySetup_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), num ) )
            {
                return doKickerSetup( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyReady_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerReady( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyTaken_:
        if ( state->currentTakerSide() == agent->world().ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKicker( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalie( agent );
            }
        }
        break;
    case GameMode::PenaltyScore_:
    case GameMode::PenaltyMiss_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyOnfield_:
    case GameMode::PenaltyFoul_:
        break;
    default:
        // nothing to do.
        std::cerr << "Current playmode is NOT a Penalty Shootout???" << std::endl;
        return false;
    }


    if ( wm.self().goalie() )
    {
        return doGoalieWait( agent );
    }
    else
    {
        return doKickerWait( agent );
    }

    // never reach here
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerWait( PlayerAgent * agent )
{
#if 1
    //int myid = agent->world().self().unum() - 1;
    //Vector2D wait_pos(1, 6.5, 15.0 * myid);
    //Vector2D wait_pos(1, 5.5, 90.0 + (180.0 / 12.0) * agent->world().self().unum());
    double dist_step = ( 9.0 + 9.0 ) / 12;

    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D ball = wm.ball().pos();

    int num = wm.self().unum();

    Vector2D wait_pos( sign(ball.x)*7.5, (-9.8 + dist_step * agent->world().self().unum())*0.4 );

    if( num == 2 || num == 11 )
           wait_pos.x *= 0.6;
    else if( num == 3 || num == 10 )
           wait_pos.x *= 0.7;
    else if( num == 4 || num == 9 )
           wait_pos.x *= 0.8;
    else if( num == 5 || num == 8 )
           wait_pos.x *= 0.9;

    // already there
    if ( agent->world().self().pos().dist( wait_pos ) < 0.7 )
    {
        Bhv_NeckBodyToBall().execute( agent );
    }
    else
    {
        // no dodge
        Body_GoToPoint( wait_pos,
                        0.3,
                        ServerParam::i().maxDashPower()
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
    }
#else
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    const int taker_unum = 11 - ( ( state->ourTakerCounter() - 1 ) % 11 );
    const double circle_r = ServerParam::i().centerCircleR() - 1.0;
    const double dir_step = 360.0 / 9.0;
    //const AngleDeg base_angle = ( wm.time().cycle() % 360 ) * 4;
    const AngleDeg base_angle = wm.time().cycle() % 90;

    AngleDeg wait_angle;
    Vector2D wait_pos( 0.0, 0.0 );

    int i = 0;
    for ( int unum = 2; unum <= 11; ++unum )
    {
        if ( taker_unum == unum )
        {
            continue;
        }

        // TODO: goalie check

        if ( i >= 9 )
        {
            wait_pos.assign( 0.0, 0.0 );
            break;
        }

        if ( wm.self().unum() == unum )
        {
            wait_angle = base_angle + dir_step * i;
            if ( wm.ourSide() == RIGHT )
            {
                wait_angle += dir_step;
            }
            wait_pos = Vector2D::from_polar( circle_r, wait_angle );
            break;
        }

        ++i;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": penalty wait. count=%d pos=(%.1f %.1f) angle=%.1f",
                  i,
                  wait_pos.x, wait_pos.y,
                  wait_angle.degree() );

    Body_GoToPoint( wait_pos,
                    0.5,
                    ServerParam::i().maxDashPower()
                    ).execute( agent );
    agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerSetup( PlayerAgent * agent )
{
    const Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    const PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    AngleDeg place_angle = 0.0;

    // ball is close enoughly.
    if ( ! Bhv_GoToStaticBall( place_angle ).execute( agent ) )
    {
        Body_TurnToPoint( goal_c ).execute( agent );
        if ( opp_goalie )
        {
            agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
            agent->setNeckAction( new Neck_TurnToPoint( goal_c ) );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerReady( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    // stamina recovering...
    if ( wm.self().stamina() < ServerParam::i().staminaMax() - 10.0
         && ( wm.time().cycle() - state->time().cycle() > ServerParam::i().penReadyWait() - 3 ) )
    {
        return doKickerSetup( agent );
    }

    if ( ! wm.self().isKickable() )
    {
        return doKickerSetup( agent );
    }

    return doKicker( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKicker( PlayerAgent * agent )
{
    //
    // server does NOT allow multiple kicks
    //

    if ( ! ServerParam::i().penAllowMultKicks() )
    {
        return doOneKickShoot( agent );
    }

    //
    // server allows multiple kicks
    //

    const WorldModel & wm = agent->world();

    // get ball
    if ( ! wm.self().isKickable() )
    {
        if ( ! Body_Intercept().execute( agent ) )
        {
            Body_GoToPoint( wm.ball().pos(),
                            0.4,
                            ServerParam::i().maxDashPower()
                            ).execute( agent );
        }

        if ( wm.ball().posCount() > 0 )
        {
            agent->setNeckAction( new Neck_TurnToBall() );
        }
        else
        {
            const PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
            if ( opp_goalie )
            {
                agent->setNeckAction( new Neck_TurnToGoalieOrScan());
            }
            else
            {
                agent->setNeckAction( new Neck_ScanField() );
            }
        }

        return true;
    }

    

    // kick decision
    if ( doShoot( agent ) )
    {
        return true;
    }

    return  doDribble(agent);
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doOneKickShoot( PlayerAgent* agent )
{
    const double ball_speed = agent->world().ball().vel().r();
    // ball is moveng --> kick has taken.
    if ( ! ServerParam::i().penAllowMultKicks()
         && ball_speed > 0.3 )
    {
        return false;
    }

    // go to the ball side
    if ( ! agent->world().self().isKickable() )
    {
        Body_GoToPoint( agent->world().ball().pos(),
                        0.4,
                        ServerParam::i().maxDashPower()
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    // turn to the ball to get the maximal kick rate.
    if ( ( agent->world().ball().angleFromSelf() - agent->world().self().body() ).abs()
         > 3.0 )
    {
        Body_TurnToBall().execute( agent );
        const PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
        if ( opp_goalie )
        {
            agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
            Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
            agent->setNeckAction( new Neck_TurnToPoint( goal_c ) );
        }
        return true;
    }

    // decide shot target point
    Vector2D shoot_point = ServerParam::i().theirTeamGoalPos();

    const PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    if ( opp_goalie )
    {
        shoot_point.y = ServerParam::i().goalHalfWidth() - 1.0;
        if ( opp_goalie->pos().absY() > 0.5 )
        {
            if ( opp_goalie->pos().y > 0.0 )
            {
                shoot_point.y *= -1.0;
            }
        }
        else if ( opp_goalie->bodyCount() < 2 )
        {
            if ( opp_goalie->body().degree() > 0.0 )
            {
                shoot_point.y *= -1.0;
            }
        }
    }

    // enforce one step kick
    Body_KickOneStep( shoot_point,
                      ServerParam::i().ballSpeedMax()
                      ).execute( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doShoot( PlayerAgent * agent )
{ 
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    const PlayerObject * opp=wm.getOpponentGoalie();
    Vector2D opp_pos=opp? opp->pos():Vector2D(sign(wm.ball().pos().x)*52.5,0);


    Vector2D me = wm.self().pos();
    Vector2D ball = wm.ball().pos();
    Vector2D goal(rcsc::sign(ball.x)*52.5,0.0);


    // fix  shoot generator dist ->50

    if (Bhv_StrictCheckShoot().execute(agent))
     {
        std::cout<<"Strict Check shoot\n";
        return true;
     }
    



    return false;

}

/*-------------------------------------------------------------------*/
/*!
  dribble to the shootable point
*/
bool
Bhv_PenaltyKick::doDribble( PlayerAgent * agent )
{
    static const int CONTINUAL_COUNT = 20;
    static int S_target_continual_count = CONTINUAL_COUNT;

    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    const Vector2D goal_c = SP.theirTeamGoalPos();

    const double penalty_abs_x = ServerParam::i().theirPenaltyAreaLineX();

    const PlayerObject * opp_goalie = wm.getOpponentGoalie();
    const double goalie_max_speed = 1.0;

    const double my_abs_x = wm.self().pos().absX();

    const double goalie_dist = ( opp_goalie
                                 ? ( opp_goalie->pos().dist( wm.self().pos() )
                                     - goalie_max_speed * std::min( 5, opp_goalie->posCount() ) )
                                 : 200.0 );
    const double goalie_abs_x = ( opp_goalie
                                  ? opp_goalie->pos().absX()
                                  : 200.0 );


    /////////////////////////////////////////////////
    // dribble parametors

    const double base_target_abs_y = ServerParam::i().goalHalfWidth() + 4.0;
    Vector2D drib_target = goal_c;
    double drib_power = ServerParam::i().maxDashPower();
    int drib_dashes = 6;



    Vector2D ball = wm.ball().pos();
    Vector2D myTarget = Vector2D(sign(wm.ball().pos().x)*44.0, 0);

  
    if (ball.absX()<25)
        myTarget = Vector2D(sign(ball.x)*34.0, sign(wm.ball().pos().y)*20);




    Body_Dribble( myTarget,
                  1.0,
                  ServerParam::i().maxDashPower(),
                  3,
                  true
                  ).execute( agent );

    if ( opp_goalie )
    {
        agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
    }
    else
    {
        agent->setNeckAction( new Neck_ScanField() );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieWait( PlayerAgent* agent )
{
#if 0
    Vector2D wait_pos( - ServerParam::i().pitchHalfLength() - 2.0, -25.0 );

    if ( agent->world().self().pos().absX()
         > ServerParam::i().pitchHalfLength()
         && wait_pos.y * agent->world().self().pos().y < 0.0 )
    {
        wait_pos.y *= -1.0;
    }

    double dash_power = ServerParam::i().maxDashPower();

    if ( agent->world().self().stamina()
         < ServerParam::i().staminaMax() * 0.8 )
    {
        dash_power *= 0.2;
    }
    else
    {
        dash_power *= ( 0.2 + ( ( agent->world().self().stamina()
                                  / ServerParam::i().staminaMax() ) - 0.8 ) / 0.2 * 0.8 );
    }

    Vector2D face_point( wait_pos.x * 0.5, 0.0 );
    if ( agent->world().self().pos().dist2( wait_pos ) < 1.0 )
    {
        Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        Body_GoToPoint( wait_pos,
                        0.5,
                        dash_power
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToPoint( face_point ) );
    }
#else
    Body_TurnToBall().execute( agent );
    agent->setNeckAction( new Neck_TurnToBall() );
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieSetup( PlayerAgent * agent )
{
    Vector2D move_point( ServerParam::i().ourTeamGoalLineX() + ServerParam::i().penMaxGoalieDistX() - 0.1,
                         0.0 );

    if ( Body_GoToPoint( move_point,
                         0.5,
                         ServerParam::i().maxDashPower()
                         ).execute( agent ) )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    // already there
    if ( std::fabs( agent->world().self().body().abs() ) > 2.0 )
    {
        Vector2D face_point( 0.0, 0.0 );
        Body_TurnToPoint( face_point ).execute( agent );
    }

    agent->setNeckAction( new Neck_TurnToBall() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalie( PlayerAgent* agent )
{
  
  return  Bhv_Savior(true).execute(agent);
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieBasicMove( PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    Rect2D our_penalty( Vector2D( -SP.pitchHalfLength(),
                                  -SP.penaltyAreaHalfWidth() + 1.0 ),
                        Size2D( SP.penaltyAreaLength() - 1.0,
                                SP.penaltyAreaWidth() - 2.0 ) );

    dlog.addText( Logger::TEAM,
                  __FILE__": goalieBasicMove. " );

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();




    Vector2D my_pos = wm.self().pos();
    Vector2D ball_pos;
    if ( wm.existKickableOpponent() )
    {
        ball_pos = wm.opponentsFromBall().front()->pos();
        ball_pos += wm.opponentsFromBall().front()->vel();
    }
    else
    {
        ball_pos = inertia_n_step_point( wm.ball().pos(),
                                         wm.ball().vel(),
                                         3,
                                         SP.ballDecay() );
    }

    const Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
    const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), ServerParam::i().goalHalfWidth() ),
                            Vector2D( -SP.pitchHalfLength(), -ServerParam::i().goalHalfWidth() ) );

    const Vector2D intersect = ball_ray.intersection( goal_line );

    Vector2D shoot_target=Vector2D(sign(wm.ball().pos().x)*52.5,0.0);



    if (wm.ball().pos().absY()>7.0)
    {
        shoot_target.y=sign(wm.ball().pos().y)*ServerParam::i().goalHalfWidth();
    }


    if (intersect.isValid())
    {
        shoot_target=intersect;
    }


    Vector2D opp_trap_pos=wm.ball().inertiaPoint(opp_min);
    Vector2D self_trap_pos = wm.ball().inertiaPoint(self_min);
    Vector2D move_pos=self_trap_pos;
    move_pos.x=std::min(opp_trap_pos.x,self_trap_pos.x)-4.0;

    Line2D shoot_line(opp_trap_pos,shoot_target);

    move_pos.y=shoot_line.getY(move_pos.x);


    if ( ! Body_GoToPoint( move_pos,
                           1.0,
                           SP.maxDashPower()
                           ).execute( agent ) )
    {
        // already there

        Body_TurnToBall().execute(agent);
    }

    agent->setNeckAction( new Neck_TurnToBallOrScan() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  ball_pos & my_pos is set to self localization oriented.
  if ( onfiled_side != our_side ), these coordinates must be reversed.
*/
Vector2D
Bhv_PenaltyKick::getGoalieMovePos( const Vector2D & ball_pos,
                                   const Vector2D & my_pos )
{
    const ServerParam & SP = ServerParam::i();
    const double min_x = -SP.pitchHalfLength() + SP.catchAreaLength()*0.9;

    if ( ball_pos.x < -49.0 )
    {
        if ( ball_pos.absY() < SP.goalHalfWidth() )
        {
            return Vector2D( min_x, ball_pos.y );
        }
        else
        {
            return Vector2D( min_x,
                             sign( ball_pos.y ) * SP.goalHalfWidth() );
        }
    }

    Vector2D goal_l( -SP.pitchHalfLength(), -SP.goalHalfWidth() );
    Vector2D goal_r( -SP.pitchHalfLength(), +SP.goalHalfWidth() );

    AngleDeg ball2post_angle_l = ( goal_l - ball_pos ).th();
    AngleDeg ball2post_angle_r = ( goal_r - ball_pos ).th();

    // NOTE: post_angle_r < post_angle_l
    AngleDeg line_dir = AngleDeg::bisect( ball2post_angle_r,
                                          ball2post_angle_l );

    Line2D line_mid( ball_pos, line_dir );
    Line2D goal_line( goal_l, goal_r );

    Vector2D intersection = goal_line.intersection( line_mid );
    if ( intersection.isValid() )
    {
        Line2D line_l( ball_pos, goal_l );
        Line2D line_r( ball_pos, goal_r );

        AngleDeg alpha = AngleDeg::atan2_deg( SP.goalHalfWidth(),
                                              SP.penaltyAreaLength() - 2.5 );
        double dist_from_goal
            = ( ( line_l.dist( intersection ) + line_r.dist( intersection ) ) * 0.5 )
            / alpha.sin();

        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. intersection=(%.1f, %.1f) dist_from_goal=%.1f",
                      intersection.x, intersection.y, dist_from_goal );
        if ( dist_from_goal <= SP.goalHalfWidth() )
        {
            dist_from_goal = SP.goalHalfWidth();
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. outer of goal. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        if ( ( ball_pos - intersection ).r() + 1.5 < dist_from_goal )
        {
            dist_from_goal = ( ball_pos - intersection ).r() + 1.5;
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. near than ball. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        AngleDeg position_error = line_dir - ( intersection - my_pos ).th();

        const double danger_angle = 21.0;
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move position_error_angle=%.1f",
                      position_error.degree() );
        if ( position_error.abs() > danger_angle )
        {
            dist_from_goal *= ( ( 1.0 - ((position_error.abs() - danger_angle)
                                         / (180.0 - danger_angle)) )
                                * 0.5 );
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. error is big. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        Vector2D result = intersection;
        Vector2D add_vec = ball_pos - intersection;
        add_vec.setLength( dist_from_goal );
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. intersection=(%.1f, %.1f) add_vec=(%.1f, %.1f)%.2f",
                      intersection.x, intersection.y,
                      add_vec.x, add_vec.y,
                      add_vec.r() );
        result += add_vec;
        if ( result.x < min_x )
        {
            result.x = min_x;
        }
        return result;
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. shot line has no intersection with goal line" );

        if ( ball_pos.x > 0.0 )
        {
            return Vector2D(min_x , goal_l.y);
        }
        else if ( ball_pos.x < 0.0 )
        {
            return Vector2D(min_x , goal_r.y);
        }
        else
        {
            return Vector2D(min_x , 0.0);
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieSlideChase( PlayerAgent* agent )
{
    const WorldModel & wm = agent->world();

    if ( std::fabs( 90.0 - wm.self().body().abs() ) > 2.0 )
    {
        Vector2D face_point( wm.self().pos().x, 100.0);
        if ( wm.self().body().degree() < 0.0 )
        {
            face_point.y = -100.0;
        }
        Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
    Line2D ball_line( ball_ray.origin(), ball_ray.dir() );
    Line2D my_line( wm.self().pos(), wm.self().body() );

    Vector2D intersection = my_line.intersection( ball_line );
    if ( ! intersection.isValid()
         || ! ball_ray.inRightDir( intersection ) )
    {
        Body_Intercept( false ).execute( agent ); // goalie mode
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    if ( wm.self().pos().dist( intersection )
         < ServerParam::i().catchAreaLength() * 0.7 )
    {
        Body_StopDash( false ).execute( agent ); // not save recovery
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    AngleDeg angle = ( intersection - wm.self().pos() ).th();
    double dash_power = ServerParam::i().maxDashPower();
    if ( ( angle - wm.self().body() ).abs() > 90.0 )
    {
        dash_power = ServerParam::i().minDashPower();
    }
    agent->doDash( dash_power );
    agent->setNeckAction( new Neck_TurnToBall() );
    return true;
}
