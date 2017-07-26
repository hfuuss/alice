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

#include "bhv_basic_tackle.h"

#include "tackle_generator.h"
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include "strategy.h"
#include "bhv_deflecting_tackle.h"
//#define DEBUG_PRINT

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicTackle::execute( PlayerAgent * agent )
{

    //check clear goal



    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();
    const int player_num=wm.self().unum();
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    bool use_foul = false;
    double tackle_prob = wm.self().tackleProbability();


//    if (opp_min>0&&wm.ball().inertiaPoint(self_min).x<-52.0&&
//            wm.ball().inertiaPoint(self_min).absY()<10.0&&wm.self().tackleProbability()>0)
//    {

//        return Bhv_DeflectingTackle(rcsc::EPS,true).execute(agent);
//    }


    if ( agent->config().version() >= 14.0
         && wm.self().card() == NO_CARD
         && ( wm.ball().pos().x > SP.ourPenaltyAreaLineX() + 0.5
              && wm.ball().pos().absY() > SP.penaltyAreaHalfWidth() + 0.5 )
         && tackle_prob < wm.self().foulProbability()&&player_num>8)
    {
        tackle_prob = wm.self().foulProbability();
        use_foul = true;
    }


    //2017年 01月 29日 星期日 21:29:15 CST


    if (wm.ball().pos().x<-35.0 && wm.ball().pos().absY()<15.0)
    {

        use_foul=false;
    }

    if (wm.ball().pos().x>35.0)
    {
        use_foul=false;
    }




   if (wm.self().unum()<6&&!wm.self().goalie())  M_min_probability=0.90;
   
   
   
   
   



    if ( tackle_prob < M_min_probability )
    {
//        dlog.addText( Logger::TEAM,
//                      __FILE__": failed. low tackle_prob=%.2f < %.2f",
//                      wm.self().tackleProbability(),
//                      M_min_probability );
        return false;
    }


    const Vector2D self_reach_point = wm.ball().inertiaPoint( self_min );




    //
    // check where the ball shall be gone without tackle
    //

    bool ball_will_be_in_our_goal = false;

    if ( self_reach_point.x < -SP.pitchHalfLength() )
    {
        const Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
        const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                Vector2D( -SP.pitchHalfLength(), -10.0 ) );

        const Vector2D intersect = ball_ray.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < SP.goalHalfWidth() + 1.0 )
        {
            ball_will_be_in_our_goal = true;

            dlog.addText( Logger::TEAM,
                          __FILE__": ball will be in our goal. intersect=(%.2f %.2f)",
                          intersect.x, intersect.y );
        }
    }


    bool  should_tackle_situation=false;

    const int cycle_thr= wm.lastKickerSide()==wm.ourSide() ? 3:1;

    if ( wm.existKickableOpponent()
         || ball_will_be_in_our_goal
         || ( opp_min < self_min - cycle_thr
              && opp_min < mate_min - cycle_thr) )
    {
         should_tackle_situation=true;
    }
    
    
    
    if (wm.existKickableTeammate())  should_tackle_situation=false;





    if (!should_tackle_situation)
    {
        return false;
    }



    return doTackle(agent);
}

bool

Bhv_BasicTackle::clearGoal(PlayerAgent *agent)
{
    const rcsc::WorldModel & wm = agent->world();

       if ( wm.self().tackleProbability() <= 0.0 )
       {
           return false;
       }

       const rcsc::ServerParam & param = rcsc::ServerParam::i();

       const int self_min = wm.interceptTable()->selfReachCycle();

       const rcsc::Vector2D self_trap_pos = wm.ball().inertiaPoint( self_min );
       if ( self_trap_pos.x > - param.pitchHalfLength() + 0.5 )
       {
           return false;
       }

       //
       // cannot intercept the ball in the field
       //

       rcsc::dlog.addText( rcsc::Logger::TEAM,
                           __FILE__": my trap pos(%.1f %.1f) < pitch.x",
                           self_trap_pos.x, self_trap_pos.y );

       const rcsc::Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
       const rcsc::Line2D goal_line( rcsc::Vector2D( - param.pitchHalfLength(), 10.0 ),
                                     rcsc::Vector2D( - param.pitchHalfLength(), -10.0 ) );
       const rcsc::Vector2D intersect =  ball_ray.intersection( goal_line );
       if ( ! intersect.isValid()
            || intersect.absY() > param.goalHalfWidth() + 0.5 )
       {
           return false;
       }

       //
       // ball is moving to our goal
       //

       rcsc::dlog.addText( rcsc::Logger::TEAM,
                           __FILE__": ball is moving to our goal" );


       if ( agent->config().version() < 12.0 )
       {
           double tackle_power = ( wm.self().body().abs() > 90.0
                                   ? param.maxTacklePower()
                                   : - param.maxBackTacklePower() );

           rcsc::dlog.addText( rcsc::Logger::TEAM,
                               __FILE__": clear goal" );
           agent->debugClient().addMessage( "tackleClearOld%.0f", tackle_power );
           agent->doTackle( tackle_power );
           agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
           return true;
       }

       //
       // search best angle
       //

       const rcsc::Line2D line_c( rcsc::Vector2D( -param.pitchHalfLength(), 0.0 ),
                                  rcsc::Vector2D( 0.0, 0.0 ) );
       const rcsc::Line2D line_l( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                                  rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );
       const rcsc::Line2D line_r( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                                  rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );

       const rcsc::AngleDeg ball_rel_angle
           = wm.ball().angleFromSelf() - wm.self().body();
       const double tackle_rate
           = ( param.tacklePowerRate()
               * ( 1.0 - 0.5 * ( ball_rel_angle.abs() / 180.0 ) ) );

       rcsc::AngleDeg best_angle = 0.0;
       double max_speed = -1.0;

       for ( double a = -180.0; a < 180.0; a += 10.0 )
       {
           rcsc::AngleDeg target_rel_angle = a - wm.self().body().degree();

           double eff_power = param.maxBackTacklePower()
               + ( ( param.maxTacklePower() - param.maxBackTacklePower() )
                   * ( 1.0 - target_rel_angle.abs() / 180.0 ) );
           eff_power *= tackle_rate;

           rcsc::Vector2D vel = wm.ball().vel()
               + rcsc::Vector2D::polar2vector( eff_power, rcsc::AngleDeg( a ) );
           rcsc::AngleDeg vel_angle = vel.th();

           if ( vel_angle.abs() > 80.0 )
           {
               rcsc::dlog.addText( rcsc::Logger::TEAM,
                                   __FILE__": clearGoal() angle=%.1f. vel_angle=%.1f is dangerouns",
                                   a,
                                   vel_angle.degree() );
               continue;
           }


           double speed = vel.r();

           int n_intersects = 0;
           if ( ball_ray.intersection( line_c ).isValid() ) ++n_intersects;
           if ( ball_ray.intersection( line_l ).isValid() ) ++n_intersects;
           if ( ball_ray.intersection( line_r ).isValid() ) ++n_intersects;

           if ( n_intersects == 3 )
           {
               rcsc::dlog.addText( rcsc::Logger::TEAM,
                                   "__ angle=%.1f vel=(%.1f %.1f)"
                                   " 3 intersects with v_lines. angle is dangerous.",
                                   a, vel.x, vel.y );
               speed -= 2.0;
           }
           else if ( n_intersects == 2
                     && wm.ball().pos().absY() > 3.0 )
           {
               rcsc::dlog.addText( rcsc::Logger::TEAM,
                                   "__ executeV12() angle=%.1f vel=(%.1f %.1f)"
                                   " 2 intersects with v_lines. angle is dangerous.",
                                   a, vel.x, vel.y );
               speed -= 2.0;
           }

           if ( speed > max_speed )
           {
               max_speed = speed;
               best_angle = target_rel_angle + wm.self().body();
               rcsc::dlog.addText( rcsc::Logger::TEAM,
                                   __FILE__": clearGoal() update. angle=%.1f vel_angle=%.1f speed=%.2f",
                                   a,
                                   vel_angle.degree(),
                                   speed );
           }
           else
           {
               rcsc::dlog.addText( rcsc::Logger::TEAM,
                                   __FILE__": clearGoal() no_update. angle=%.1f vel_angle=%.1f speed=%.2f",
                                   a,
                                   vel_angle.degree(),
                                   speed );
           }
       }

       //
       // never accelerate the ball
       //

       if ( max_speed < 1.0 )
       {
           rcsc::dlog.addText( rcsc::Logger::TEAM,
                               __FILE__": failed clearGoal. max_speed=%.3f", max_speed );
           return false;
       }

       rcsc::dlog.addText( rcsc::Logger::TEAM,
                           __FILE__": clear goal" );
       agent->debugClient().addMessage( "tackleClear%.0f", best_angle.degree() );
       agent->doTackle( ( best_angle - wm.self().body() ).degree() );
       agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );

       return true;
   }


/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicTackle::doTackle( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const TackleGenerator:: Result & result
        = TackleGenerator::instance().bestResult( wm );

    Vector2D ball_next = wm.ball().pos() + result.ball_vel_;


    double tackle_dir = ( result.tackle_angle_ - wm.self().body() ).degree();

    agent->doTackle( tackle_dir);
    agent->setNeckAction( new Neck_TurnToPoint( ball_next ) );

    return true;
}
