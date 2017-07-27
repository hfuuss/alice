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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_get_ball.h"

#include <rcsc/soccer_math.h>
#include <rcsc/timer.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include<rcsc/action/body_intercept.h>
#include <rcsc/action/view_synch.h>
#include "bhv_basic_tackle.h"
#include "neck_offensive_intercept_neck.h"
#include "neck_check_ball_owner.h"
#include "neck_default_intercept_neck.h"
#include "chain_action/field_analyzer.h"
#include "field_analyzer.h"
#define  USE_TEST_BLCOK_POINT	1


using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GetBall::execute( PlayerAgent * agent )
{


    const WorldModel & wm = agent->world();

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    //
    // check ball owner
    //
    if (  mate_min == 0||mate_min<opp_min|| wm.existKickableTeammate() )
    {

        return false;
    }
       
       
     if (self_min<=opp_min&&self_min<=mate_min)
	  
     {
              Body_Intercept().execute( agent );
              if (wm.ball().vel().x>0)
              agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
              else
              agent->setNeckAction(new  Neck_DefaultInterceptNeck(new Neck_TurnToBallOrScan(0)));


            return true;
     }





    const PlayerObject *fastopp = wm.interceptTable()->fastestOpponent();


    if (!fastopp)
    {
        return false;
    }





    






     Vector2D center_pos=get_block_center_pos(agent);

     double dist_thr = wm.self().playerType().kickableArea() - 0.2;
     
    






    bool save_recovery = true;



    Param param;
    simulate( wm, center_pos, M_bounding_rect, dist_thr, save_recovery,
              &param );

     if ( ! param.point_.isValid() )
     {
         return false;
     }
     

     rcsc::Vector2D ball = wm.ball().pos();
     rcsc::Vector2D me = wm.self().pos() + wm.self().vel();

     int myCycles = wm.interceptTable()->selfReachCycle();
     int tmmCycles = wm.interceptTable()->teammateReachCycle();
     int oppCycles = wm.interceptTable()->opponentReachCycle();

     rcsc::Vector2D blockPos = param.point_;
     rcsc::Vector2D opp = fastopp->pos()+fastopp->vel();
     rcsc::Vector2D goal = rcsc::Vector2D(-52.0, 0.0);




     // opponent target
     rcsc::Vector2D oppPos = opp;
     rcsc::Vector2D oppTarget = center_pos;//rcsc::Vector2D( -52.5, oppPos.y*0.95 ); // oppPos.y*0.975



     rcsc::Line2D targetLine = rcsc::Line2D( oppTarget, oppPos );

     static rcsc::Vector2D opp_static_pos = opp;


     if( me.dist( blockPos ) < 2.5 && targetLine.dist(me) <0.5 && myCycles <= 2 )
     {

        rcsc::Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
     }




     opp_static_pos = opp;


    //
    // execute action
    //
    if ( Body_GoToPoint( param.point_,
                         dist_thr,
                         ServerParam::i().maxPower(),
                         -1.0,
                         1,
                         save_recovery,
                         15.0 // dir threshold
                         ).execute( agent ) )
    {

    }
    else
    {
      


         AngleDeg body_angle = wm.ball().angleFromSelf() + 90.0;
         if ( body_angle.abs() < 90.0 )
          {
            body_angle += 180.0;
          }

          Body_TurnToAngle( body_angle ).execute( agent );

    }

    if ( wm.ball().distFromSelf() < 4.0 )
    {
        agent->setViewAction( new View_Synch() );
    }
    agent->setNeckAction( new Neck_CheckBallOwner() );

    return true;
}







bool Bhv_GetBall::DefenderGetBall(PlayerAgent* agent)
{
    rcsc::Rect2D  bounding_rect (rcsc:: Vector2D (-52.5,-34.0) ,  rcsc::Vector2D( 0,34.0));
    return Bhv_GetBall( bounding_rect ).execute( agent );
}

















/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_GetBall::simulate( const WorldModel & wm,
                       const Vector2D & center_pos,
                       const Rect2D & bounding_rect,
                       const double & dist_thr,
                       const bool save_recovery,
                       Param * param )
{ const PlayerObject * opp = wm.interceptTable()->fastestOpponent();

    if ( ! opp )
    {

        return;
    }



    dlog.addText( Logger::BLOCK,
                  __FILE__":(simulate) center=(%.1f %.1f) dist_thr=%.2f",
                  center_pos.x, center_pos.y,
                  dist_thr );

    const double pitch_half_length = ServerParam::i().pitchHalfLength();
    const double pitch_half_width = ServerParam::i().pitchHalfWidth();

    const PlayerType * opp_type = opp->playerTypePtr();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const Vector2D opp_trap_pos=get_block_opponent_trap_point(wm);

    bool on_block_line = false;
    {
        const Segment2D block_segment( opp_trap_pos, center_pos );
        const Vector2D my_inertia = wm.self().inertiaFinalPoint();
        if ( block_segment.contains( my_inertia )
             && block_segment.dist( my_inertia ) < 0.5 )
        {
            on_block_line = true;

        }
    }

    const Vector2D unit_vec = ( center_pos - opp_trap_pos ).setLengthVector( 1.0 );



    Param best;

    const double min_length = 0;
    const double max_length = std::min( 30.0, opp_trap_pos.dist( center_pos ) ) ;
    double dist_step = wm.ball().distFromSelf()<2.5? 0.1:0.3;

    int count = 0;
    for ( double len = min_length;
          len < max_length;
          len += dist_step, ++count )
    {
        Vector2D target_point = opp_trap_pos + unit_vec * len;
        if ( len >= 1.0 ) dist_step = 1.0;


        //
        // region check
        //
        if ( ! bounding_rect.contains( target_point ) )
        {

            continue;
        }

        if ( target_point.absX() > pitch_half_length
             || target_point.absY() > pitch_half_width )
        {

            continue;
        }

        //
        // predict self cycle
        //
        double stamina = 0.0;
        int n_turn = 0;

        double self_dist=std::max( 0.0,
                                   wm.self().pos().dist( target_point )  );
        int self_cycle = predictSelfReachCycle( wm,
                                                target_point,
                                                dist_thr,
                                                save_recovery,
                                                &n_turn,
                                                &stamina );
        if ( self_cycle > 100 )
        {

            continue;
        }

        double opp_dist = std::max( 0.0,
                                    opp_trap_pos.dist( target_point )  );
        int opp_cycle
            = predict_opp_reach_cycle(wm,opp,target_point,0.0,1.0,1,0,0,false);

        if ( self_cycle <= opp_cycle - 1
             || ( self_cycle <= 3 && self_cycle <= opp_cycle  ) )
        {
            best.point_ = target_point;
            best.turn_ = n_turn;
            best.cycle_ = self_cycle;
            best.stamina_ = stamina;
            break;
        }
    }




    if ( best.point_.isValid() )
    {
        *param = best;

    }
    if ( ! on_block_line
         && best.turn_ > 0 )
    {
        simulateNoTurn( wm, opp_trap_pos, center_pos,
                        bounding_rect, dist_thr, save_recovery,
                        &best );
    }
    

}


 
 
 int 
 Bhv_GetBall::predict_opp_reach_cycle(const rcsc::WorldModel & wm,const rcsc::AbstractPlayerObject* player, const Vector2D& target_point, const double& dist_thr, const double& penalty_distance, const int body_count_thr, const int default_n_turn, const int wait_cycle, const bool use_back_dash)
{
  const PlayerType * ptype = player->playerTypePtr();

    const Vector2D & first_player_pos = ( player->seenPosCount() <= player->posCount()
                                          ? player->seenPos()
                                          : player->pos() );
    const Vector2D & first_player_vel = ( player->seenVelCount() <= player->velCount()
                                          ? player->seenVel()
                                          : player->vel() );
    const double first_player_speed = first_player_vel.r() * std::pow( ptype->playerDecay(), wait_cycle );
    
    double oppDribbleSpeed=0.675;
    
    
    const int opp_min=wm.interceptTable()->opponentReachCycle();

    
    

    //near opp area   try cacluate more accuratle   dy 2017
    
    const Vector2D self_pos=wm.self().pos()+wm.self().vel();
    const Vector2D ball=wm.ball().pos()+wm.ball().vel();
    
    bool  safe_block=self_pos.dist(ServerParam::i().ourTeamGoalPos())< ball.dist(ServerParam::i().ourTeamGoalPos());
    
    
    if (opp_min<=1&&wm.self().distFromBall()<6.0/*&&safe_block*/)
    {
        


        double oppDribbleSpeed=player->playerTypePtr()->realSpeedMax();
	
	
     if(wm.ball().velCount()<=1&&opp_min==0)
      {
             oppDribbleSpeed= wm.ball().vel().r()<0.618?
                 (wm.ball().vel().r()+0.618)/2: bound (0.618, wm.ball().vel().r(),player->playerTypePtr()->realSpeedMax());
      }


      if(player->velCount()<=1)
       {

         oppDribbleSpeed=player->vel().r()<0.618? (player->vel().r()+0.618)/2:bound (0.618, player->vel().r(),player->playerTypePtr()->realSpeedMax());

       }

     if (wm.ball().velCount()<=0)  oppDribbleSpeed=bound(0.1,player->vel().r(),player->playerTypePtr()->realSpeedMax());

        Vector2D inertia_pos = ptype->inertiaFinalPoint( first_player_pos, first_player_vel );
        double target_dist = inertia_pos.dist( target_point );
	
        const std::vector< double > M_dash_distance_table=player->playerTypePtr()->dashDistanceTable();
        double dash_dist=target_dist;
	
        std::vector< double >::const_iterator
        it = std::lower_bound( M_dash_distance_table.begin(),
                               M_dash_distance_table.end(),
                               dash_dist - 0.001 );

         if ( it != M_dash_distance_table.end() )
         {
             return ( static_cast< int >( std::distance( M_dash_distance_table.begin(), it) )  + 1 ); // is it necessary?
         }

        double rest_dist = dash_dist - M_dash_distance_table.back();
         int cycle = M_dash_distance_table.size();

         cycle += static_cast< int >( std::ceil( rest_dist / oppDribbleSpeed) );




         return  cycle;
    }


       const Vector2D trap_pos = get_block_opponent_trap_point(wm);
    
    
        double opp_dist = trap_pos.dist( target_point );
        int opp_cycle = opp_min  + player->playerTypePtr()->cyclesToReachDistance( opp_dist );
	
	
	return opp_cycle;

}




/*-------------------------------------------------------------------*/
/*!

 */
int
Bhv_GetBall::predictSelfReachCycle( const WorldModel & wm,
                                    const Vector2D & target_point,
                                    const double & dist_thr,
                                    const bool save_recovery,
                                    int * turn_step,
                                    double * stamina )
{
    const ServerParam & param = ServerParam::i();
    const PlayerType & self_type = wm.self().playerType();
    const double max_moment = param.maxMoment();
    const double recover_dec_thr = param.staminaMax() * param.recoverDecThr();

    const double first_my_speed = wm.self().vel().r();

    for ( int cycle = 0; cycle < 100; ++cycle )
    {
        const Vector2D inertia_pos = wm.self().inertiaPoint( cycle );
        double target_dist = ( target_point - inertia_pos ).r();
        if ( target_dist - dist_thr > self_type.realSpeedMax() * cycle )
        {
            continue;
        }

        int n_turn = 0;
        int n_dash = 0;

        AngleDeg target_angle = ( target_point - inertia_pos ).th();
        double my_speed = first_my_speed;

        //
        // turn
        //
        double angle_diff = ( target_angle - wm.self().body() ).abs();
        double turn_margin = 180.0;
        if ( dist_thr < target_dist )
        {
            turn_margin = std::max( 15.0,
                                    AngleDeg::asin_deg( dist_thr / target_dist ) );
        }

        while ( angle_diff > turn_margin )
        {
            angle_diff -= self_type.effectiveTurn( max_moment, my_speed );
            my_speed *= self_type.playerDecay();
            ++n_turn;
        }

        StaminaModel stamina_model = wm.self().staminaModel();



        AngleDeg dash_angle = wm.self().body();
        if ( n_turn > 0 )
        {
            angle_diff = std::max( 0.0, angle_diff );
            dash_angle = target_angle;
            if ( ( target_angle - wm.self().body() ).degree() > 0.0 )
            {
                dash_angle -= angle_diff;
            }
            else
            {
                dash_angle += angle_diff;
            }

            stamina_model.simulateWaits( self_type, n_turn );
        }

        //
        // dash
        //

        Vector2D my_pos = inertia_pos;
        Vector2D vel = wm.self().vel() * std::pow( self_type.playerDecay(), n_turn );
        while ( n_turn + n_dash < cycle
                && target_dist > dist_thr )
        {
            double dash_power = std::min( param.maxDashPower(),
                                          stamina_model.stamina() + self_type.extraStamina() );
            if ( save_recovery
                 && stamina_model.stamina() - dash_power < recover_dec_thr )
            {
                dash_power = std::max( 0.0, stamina_model.stamina() - recover_dec_thr );
            }

            Vector2D accel = Vector2D::polar2vector( dash_power
                                                     * self_type.dashPowerRate()
                                                     * stamina_model.effort(),
                                                     dash_angle );
            vel += accel;
            double speed = vel.r();
            if ( speed > self_type.playerSpeedMax() )
            {
                vel *= self_type.playerSpeedMax() / speed;
            }

            my_pos += vel;
            vel *= self_type.playerDecay();

            stamina_model.simulateDash( self_type, dash_power );

            target_dist = my_pos.dist( target_point );
            ++n_dash;
        }

        if ( target_dist <= dist_thr
             || inertia_pos.dist2( target_point ) < inertia_pos.dist2( my_pos ) )
        {
            if ( turn_step )
            {
                *turn_step = n_turn;
            }

            if ( stamina )
            {
                *stamina = stamina_model.stamina();
            }

            //             dlog.addText( Logger::BLOCK,
            //                           "____ cycle=%d n_turn=%d n_dash=%d stamina=%.1f",
            //                           cycle,
            //                           n_turn, n_dash,
            //                           my_stamina );
            return n_turn + n_dash;
        }
    }

    return 1000;
}

rcsc::Vector2D Bhv_GetBall::get_block_center_pos( rcsc::PlayerAgent * agent )
{
  const WorldModel&wm = agent->world();
  const int opp_min=wm.interceptTable()->opponentReachCycle();
  Vector2D opp_trap_pos = wm.ball().inertiaPoint(opp_min);


    const PlayerObject * opponent = wm.interceptTable()->fastestOpponent();
    if ( opponent
         && opponent->bodyCount() <= 1
         && opponent->isKickable()
         && -44.0 < opp_trap_pos.x
         && opp_trap_pos.x < -25.0 )
    {
        const Segment2D goal_segment( Vector2D( -ServerParam::i().pitchHalfLength(),
                                                -ServerParam::i().goalHalfWidth() ),
                                      Vector2D( -ServerParam::i().pitchHalfLength(),
                                                +ServerParam::i().goalHalfWidth() ) );
        const Segment2D opponent_move( opp_trap_pos,
                                       opp_trap_pos + Vector2D::from_polar( 500.0, opponent->body() ) );
        Vector2D intersection = goal_segment.intersection( opponent_move, false );
        if ( intersection.isValid() )
        {

            return intersection;
        }
    }



    //
    // searth the best point
    //
    Vector2D center_pos( -44.0, 0.0 );
    if ( opp_trap_pos.x < -38.0
         && opp_trap_pos.absY() < 7.0 )
    {
        center_pos.x = -52.5;
        // 2009-06-17
        center_pos.y = -2.0 * sign( wm.self().pos().y );

    }
    else if ( opp_trap_pos.x < -38.0
              && opp_trap_pos.absY() < 9.0 )
    {
        center_pos.x = std::min( -49.0, opp_trap_pos.x - 0.2 );
        //center_pos.y = opp_trap_pos.y * 0.6;
        Vector2D goal_pos( - ServerParam::i().pitchHalfLength(),
                           ServerParam::i().goalHalfWidth() * 0.5 );
        if ( opp_trap_pos.y > 0.0 )
        {
            goal_pos.y *= -1.0;
        }

        Line2D opp_line( opp_trap_pos, goal_pos );
        center_pos.y = opp_line.getY( center_pos.x );
        if ( center_pos.y == Line2D::ERROR_VALUE )
        {
            center_pos.y = opp_trap_pos.y * 0.6;
        }

    }
    else if ( opp_trap_pos.x < -38.0
              && opp_trap_pos.absY() < 12.0 )
    {
        //center_pos.x = -50.0;
        center_pos.x = std::min( -46.5, opp_trap_pos.x - 0.2 );
        //center_pos.y = 2.5 * sign( opp_trap_pos.y );
        //center_pos.y = 6.5 * sign( opp_trap_pos.y );
        //center_pos.y = opp_trap_pos.y * 0.8;
        center_pos.y = 0.0;

    }
    else if ( opp_trap_pos.x < -30.0
              && 2.0 < opp_trap_pos.absY()
              && opp_trap_pos.absY() < 8.0 )
    {
        center_pos.x = -50.0;
        center_pos.y = opp_trap_pos.y * 0.9;

    }
    else if ( opp_trap_pos.absY() > 25.0 )
    {
        center_pos.x = -44.0;
        if ( opp_trap_pos.x < -36.0 )
        {
            center_pos.y = 5.0 * sign( opp_trap_pos.y );

        }
        else
        {
            center_pos.y = 20.0 * sign( opp_trap_pos.y );

        }
    }
    else if ( opp_trap_pos.absY() > 20.0 )
    {
        center_pos.x = -44.0;
        if ( opp_trap_pos.x > -18.0 )
        {
            center_pos.y = 10.0 * sign( opp_trap_pos.y );
        }
        else
        {
            center_pos.y = 5.0 * sign( opp_trap_pos.y );
        }

    }
    else if ( opp_trap_pos.absY() > 15.0 )
    {
        center_pos.x = -44.0;
        if ( opp_trap_pos.x > -18.0 )
        {
            center_pos.y = 10.0 * sign( opp_trap_pos.y );
        }
        else
        {
            center_pos.y = 5.0 * sign( opp_trap_pos.y );
        }

    }


    return center_pos;
}


Vector2D  Bhv_GetBall::get_block_opponent_trap_point(const WorldModel &wm)
{
   int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );
   const PlayerObject * opp = wm.interceptTable()->fastestOpponent();


    return opp_trap_pos;
}



/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_GetBall::simulateNoTurn( const WorldModel & wm,
                             const Vector2D & opponent_trap_pos,
                             const Vector2D & center_pos,
                             const Rect2D & bounding_rect,
                             const double & dist_thr,
                             const bool save_recovery,
                             Param * param )
{
    const PlayerObject * opponent = wm.interceptTable()->fastestOpponent();

    if ( ! opponent )
    {
        dlog.addText( Logger::BLOCK,
                      __FILE__":(simulate) no fastest opponent" );
        return;
    }

    const Segment2D block_segment( opponent_trap_pos, center_pos );
    const Line2D my_move_line( wm.self().pos(), wm.self().body() );

    const Vector2D intersect_point = block_segment.intersection( my_move_line );
    if ( ! intersect_point.isValid()
         || ! bounding_rect.contains( intersect_point )
         || ( ( intersect_point - wm.self().pos() ).th() - wm.self().body() ).abs() > 5.0 )
    {
        dlog.addText( Logger::BLOCK,
                      __FILE__": (simulateNoTurn) no intersection" );
        return;
    }

    {
        //const Vector2D ball_pos = wm.ball().inertiaPoint( wm.interceptTable()->opponentReachCycle() );

        if ( param->point_.isValid() // already solution exists
             && param->point_.dist2( intersect_point ) > std::pow( 3.0, 2 )
             //|| my_move_line.dist( ball_pos ) > wm.self().playerType().kickableArea() + 0.3 )
             )
        {
            dlog.addText( Logger::BLOCK,
                          __FILE__": (simulateNoTurn) intersection too far" );
            return;
        }
    }

    const PlayerType * opponent_type = opponent->playerTypePtr();
    const double opponent_dist = std::max( 0.0,
                                           opponent_trap_pos.dist( intersect_point )
                                           - opponent_type->kickableArea() * 0.8 );
    const int opponent_step
        = predict_opp_reach_cycle(wm,opponent,center_pos,0.0,1.0,1,0,0,false);




    const int max_step = std::min( param->cycle_ , opponent_step );


    //
    //
    //
    for ( int n_dash = 1; n_dash <= max_step; ++n_dash )
    {
        const Vector2D inertia_pos = wm.self().inertiaPoint( n_dash );
        const double target_dist = inertia_pos.dist( intersect_point );

        const int dash_step = wm.self().playerType().cyclesToReachDistance( target_dist - dist_thr*0.5 );

        if ( dash_step <= n_dash )
        {
            StaminaModel stamina = wm.self().staminaModel();
            stamina.simulateDashes( wm.self().playerType(),
                                    n_dash,
                                    ServerParam::i().maxDashPower() );
            if ( save_recovery
                 && stamina.recovery() < ServerParam::i().recoverInit() )
            {
                dlog.addText( Logger::BLOCK,
                              __FILE__": (simulateNoTurn) dash=%d recover decay",
                              n_dash );
            }
            else
            {


                param->point_ = intersect_point;
                param->turn_ = 0;
                param->cycle_ = n_dash;
                param->stamina_ = stamina.stamina();
            }

            break;
        }
    }
}
