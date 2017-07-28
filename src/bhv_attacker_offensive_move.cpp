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

#include "bhv_attacker_offensive_move.h"

#include "body_forestall_block.h"
#include "bhv_basic_tackle.h"
#include "neck_offensive_intercept_neck.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/body_go_to_point.h>
#include <boost/concept_check.hpp>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include "neck_default_intercept_neck.h"

#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/voronoi_diagram.h>
#include "field_analyzer.h"
#include "strategy.h"
#include "voron_point.h"
using namespace rcsc;

/*-------------------------------------------------------------------*/

class IntentionBreakAway
    : public SoccerIntention {
private:
    const Vector2D M_target_point;
    int M_step;
    GameTime M_last_execute_time;

public:

    IntentionBreakAway( const Vector2D & target_point,
                        const int step,
                        const GameTime & start_time )
        : M_target_point( target_point )
        , M_step( step )
        , M_last_execute_time( start_time )
      { }

    bool finished( const PlayerAgent * agent );

    bool execute( PlayerAgent * agent );

};

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionBreakAway::finished( const PlayerAgent * agent )
{
    if ( M_step == 0 )
    {
     
        return true;
    }

    const WorldModel & wm = agent->world();

    if ( wm.audioMemory().passTime() == agent->world().time()
         && ! wm.audioMemory().pass().empty()
         && ( wm.audioMemory().pass().front().receiver_ == wm.self().unum() )
         )
    {
     
        return true;
    }

    if ( M_last_execute_time.cycle() + 1 != wm.time().cycle() )
    {
        
        return true;
    }

    if ( wm.audioMemory().passTime() == wm.time() )
    {
      
        return false;
    }

    if ( wm.ball().pos().x > M_target_point.x + 3.0 )
    {
        return false;
    }

    //const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( mate_min > 3 )
    {
       
        return true;
    }

    if ( opp_min < mate_min )
    {
      
        return true;
    }

    const Vector2D mate_trap_pos = wm.ball().inertiaPoint( mate_min );

    const double max_x = std::max( mate_trap_pos.x, wm.offsideLineX() );

    if ( wm.self().pos().x > max_x )
    {
       

        return true;
    }

    if ( std::fabs( M_target_point.y -  mate_trap_pos.y ) > 15.0 )
    {
      
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionBreakAway::execute( PlayerAgent * agent )
{
    --M_step;
    M_last_execute_time = agent->world().time();

    double dist_thr = 1.0;


    const int self_min = agent->world().interceptTable()->selfReachCycle();
    const int mate_min = agent->world().interceptTable()->teammateReachCycle();

    if ( self_min <= mate_min + 1 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intention execute. intercept" );
        Vector2D face_point( 52.5, agent->world().self().pos().y * 0.9 );
        Body_Intercept( true , face_point ).execute( agent );
#if 0
        if ( self_min == 4 )
        {
            agent->setViewAction( new View_Wide() );
        }
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
#else
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
#endif
        return true;
    }




    if ( ! Body_GoToPoint( M_target_point, dist_thr,
                           ServerParam::i().maxPower(),
                           100, // cycle
                           false, // back
                           true, // stamina save
                           20.0 // angle threshold
                           ).execute( agent ) )
    {
        AngleDeg body_angle( 0.0 );
        Body_TurnToAngle( body_angle ).execute( agent );
    }

    if ( agent->world().ball().posCount() <= 1 )
    {
        agent->setNeckAction( new Neck_ScanField() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    if ( M_step <=1 )
    {
        agent->doPointtoOff();
    }

    agent->addSayMessage( new PassRequestMessage( M_target_point ) );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_AttackerOffensiveMove::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();



    //
    // tackle
    //
    if ( Bhv_BasicTackle().execute( agent ) )
    {
        return true;
    }


    //------------------------------------------------------
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    //press  opp ball owner

//    if (((opp_min<mate_min&&opp_min<self_min)||wm.existKickableOpponent())&&Bhv_Press().execute(agent))
//   {

//         return true;

//   }
    //
    // forestall
    //


    //
    // intercept
    //
    if ( doIntercept( agent ) )
    {
        return false;
    }

    //press

    if ( doForestall( agent ) )
    {
        return true;
    }



    if (throughPassShootHelper(agent))
    {
        return true;
    }

    //
    // avoid marker
    //

    //------------------------------------------------------

    const Vector2D mate_trap_pos = wm.ball().inertiaPoint( mate_min );

    Vector2D target_point = M_home_pos;

    const double max_x = std::max( wm.offsideLineX(),
                                   mate_trap_pos.x );
    dlog.addText( Logger::TEAM,
                  __FILE__": max x = %.1f",
                  max_x );


    if ( wm.self().pos().x > max_x
         && wm.self().pos().x < 42.0
         //&& std::fabs( wm.self().pos().y - M_home_pos.y ) < 10.0
         )
    {
        target_point.y = wm.self().pos().y;
    }

//    // 2008-07-17 akiyama
//    if ( M_forward_player
//         && target_point.x < max_x - 1.0
//         && Strategy::i().opponentDefenseStrategy() == ManMark_Strategy )
//    {
//        target_point.x = std::min( max_x - 1.0, M_home_pos.x + 15.0 );
//    }

    if ( std::fabs( mate_trap_pos.y - M_home_pos.y ) < 15.0
         || mate_trap_pos.x > max_x - 5.0 )
    {
        if ( target_point.x > max_x - 1.5 )
        {
            target_point.x = std::min( M_home_pos.x, max_x - 1.5 );
        }
    }
    else
    {
        if ( target_point.x > max_x - 3.0 )
        {
            target_point.x = std::min( M_home_pos.x, max_x - 3.0 );
        }
    }

    // 2008-04-23 akiyama
    if ( mate_min >= 3
         && wm.self().pos().dist2( target_point ) < 5.0*5.0 )
    {
        double opp_dist = 1000.0;
        const PlayerObject * opp = wm.getOpponentNearestTo( target_point,
                                                            10,
                                                            &opp_dist );
        if ( opp
             && opp_dist < 4.0
             && std::fabs( opp->pos().y - target_point.y ) < 3.0 )
        {
            double new_y = ( target_point.y > opp->pos().y
                             ? opp->pos().y + 3.0
                             : opp->pos().y - 3.0 );
       
            target_point.y = new_y;
        }
    }

    // 2008-04-28 akiyama
    if ( mate_min < 3
         && std::fabs( wm.self().pos().y - M_home_pos.y ) < 3.0 )
    {
        double new_y = wm.self().pos().y * 0.9 + M_home_pos.y * 0.1;;

        target_point.y = new_y;
    }

    // 2008-07-18 akiyama
//     if ( M_forward_player
//          && wm.self().unum() == 11
//          && target_point.x < 36.0
//          && Strategy::i().opponentDefenseStrategy() == ManMark_Strategy )
//     {
//         target_point.x = std::min( 36.0, target_point.x + 15.0 );
//     }

    bool breakaway = false;
    bool intentional = false;

    if ( wm.existKickableTeammate()
         || mate_min <= 5
         || mate_min <= opp_min + 1 )
    {
        if ( wm.self().pos().x > max_x )
        {
            if ( std::fabs( mate_trap_pos.y - target_point.y ) < 4.0 )
            {
                double abs_y = wm.ball().pos().absY();
                bool outer = ( wm.self().pos().absY() > abs_y );
                if ( abs_y  > 25.0 ) target_point.y = ( outer ? 30.0 : 20.0 );
                else if ( abs_y > 20.0 ) target_point.y = ( outer ? 25.0 : 15.0 );
                else if ( abs_y > 15.0 ) target_point.y = ( outer ? 20.0 : 10.0 );
                else if ( abs_y > 10.0 ) target_point.y = ( outer ? 15.0 : 5.0 );
                else if ( abs_y > 5.0 ) target_point.y = ( outer ? 10.0 : 0.0 );
                else target_point.y = ( outer ? 5.0 : -5.0 );

                if ( wm.self().pos().y < 0.0 )
                {
                    target_point.y *= -1.0;
                }

            
            }
        }
        else if ( M_forward_player
                  //&& wm.audioMemory().dribbleTime() != wm.time()
                  && mate_min <=2&&mate_min<=opp_min
                  && mate_trap_pos.x > wm.offsideLineX() - 10.0
                  && wm.self().stamina() > ServerParam::i().staminaMax() * 0.8
                  && 5.0 < wm.self().pos().x && wm.self().pos().x < 27.0
                  && wm.self().pos().x > max_x - 7.0
                  && wm.self().pos().x < max_x - 1.0
                  && wm.self().pos().x < mate_trap_pos.x + 30//20
                  && wm.self().pos().dist( mate_trap_pos ) < 20.0
                  && std::fabs( M_home_pos.y - wm.self().pos().y ) < 15.0
                  )
        {

                target_point.x = std::min( wm.self().pos().x + 20.0, 50.0 );
                target_point.y = wm.self().pos().y * 0.8 + mate_trap_pos.y * 0.2;
                if ( target_point.absY() > 8.0
                     && target_point.absY() > M_home_pos.absY() )
                {
                    target_point.y = M_home_pos.y;
                }




                intentional = true;




            breakaway = true;
        }
    }

    const double dash_power = ( breakaway
                                ? ServerParam::i().maxPower()
                                : getDashPower( agent, target_point ) );

    if ( dash_power < 1.0 )
    {
     
        AngleDeg face_angle = wm.ball().angleFromSelf() + 90.0;
        if ( face_angle.abs() > 90.0 ) face_angle += 180.0;
        Body_TurnToAngle( face_angle ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
        return true;
    }



    if (wm.self().unum()==11&&CF_Voron_through_Pass(agent,target_point))
    {

    }



    double dist_thr = std::fabs( wm.ball().pos().x - wm.self().pos().x ) * 0.2 + 0.25;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;
    if ( target_point.x > wm.self().pos().x - 0.5
         && wm.self().pos().x < wm.offsideLineX()
         && std::fabs( target_point.x - wm.self().pos().x ) > 1.0 )
    {
        dist_thr = std::min( 1.0, wm.ball().pos().dist( target_point ) * 0.1 + 0.5 );
    }

;
    Vector2D my_inertia = wm.self().inertiaPoint( mate_min );

    if ( mate_min <= 5
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6
         && my_inertia.dist( target_point ) > dist_thr
         && ( my_inertia.x > target_point.x
              || my_inertia.x > wm.offsideLineX() )
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
;
        agent->doDash( back_dash_power );
    }
    else if ( Body_GoToPoint( target_point, dist_thr, dash_power,
                              100, // cycle
                              false, // back
                              true, // stamina save
                              25.0 // angle threshold
                              ).execute( agent ) )
    {
        if ( intentional )
        {
	  
            agent->addSayMessage( new PassRequestMessage( target_point ) );

            agent->setIntention( new IntentionBreakAway( target_point,
                                                         10,
                                                         wm.time() ) );
            agent->setArmAction( new Arm_PointToPoint( target_point ) );
        }
    }
    else if ( wm.self().pos().x > wm.offsideLineX() - 0.1
              && Body_GoToPoint( Vector2D( wm.offsideLineX() - 0.5, wm.self().pos().y ),
                                 0.3, // small dist threshold
                                 dash_power,
                                 1, // cycle
                                 false, // back
                                 true, // stamina save
                                 25.0 // angle threshold
                                 ).execute( agent ) )
    {
      ;
    }
    else
    {
        AngleDeg body_angle( 0.0 );
        Body_TurnToAngle( body_angle ).execute( agent );
  
    }

    //     if ( agent->world().ball().posCount() <= 1 )
    //     {
    //         agent->setNeckAction( new Neck_ScanField() );
    //     }
    //     else
    {
        //int min_step = std::min( self_min, opp_min );
        int min_step = opp_min;
        int count_thr = 0;
        ViewWidth view_width = agent->effector().queuedNextViewWidth();
        int see_cycle = agent->effector().queuedNextSeeCycles();
        if ( view_width.type() == ViewWidth::WIDE )
        {
            if ( min_step > see_cycle )
            {
                count_thr = 2;
            }
        }
        else if ( view_width.type() == ViewWidth::NORMAL )
        {
            if ( min_step > see_cycle )
            {
                count_thr = 1;
            }
        }


        agent->setNeckAction( new Neck_TurnToBallOrScan( count_thr ) );

    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_AttackerOffensiveMove::doForestall( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();


    if (wm.existKickableTeammate() || mate_min<opp_min)  return false;


    if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.6
         && opp_min < 3
         && opp_min < mate_min - 2
         && ( opp_min < self_min - 2
              || opp_min == 0
              || ( opp_min == 1 && self_min > 1 ) )
         && wm.ball().pos().dist( M_home_pos ) < 10.0
         && wm.ball().distFromSelf() < 15.0 )
    {
       

        if ( Body_ForestallBlock( M_home_pos ).execute( agent ) )
        {
            agent->setNeckAction( new Neck_TurnToBall() );
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_AttackerOffensiveMove::doIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if (!wm.existKickableTeammate()&&self_min<mate_min&&self_min<=opp_min)
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doIntercept) performed" );
        agent->debugClient().addMessage( "Attack:Intercept" );
        Vector2D face_point( 52.5, wm.self().pos().y );
        if ( wm.self().pos().absY() < 10.0 )
        {
            face_point.y *= 0.8;
        }
        else if ( wm.self().pos().absY() < 20.0 )
        {
            face_point.y *= 0.9;
        }

        Body_Intercept( true , face_point ).execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        return true;
    }
    
    

    return false;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_AttackerOffensiveMove::doAvoidMarker( PlayerAgent * agent )
{

  return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_AttackerOffensiveMove::getDashPower( const PlayerAgent * agent,
                                         const Vector2D & target_point )
{
    const WorldModel & wm = agent->world();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D receive_pos = wm.ball().inertiaPoint( mate_min );
    if (mate_min<opp_min&&wm.ball().pos().x>wm.theirDefenseLineX())
    {
      return ServerParam::i().maxDashPower(); //dy
    }

    if ( target_point.x > wm.self().pos().x
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.7
         && mate_min <= 8
         && ( mate_min <= opp_min + 3
              || wm.existKickableTeammate() )
         && std::fabs( receive_pos.y - target_point.y ) < 25.0 )
    {
       
        return ServerParam::i().maxPower();
    }

    if  ( wm.self().pos().x > wm.offsideLineX()
          && ( wm.existKickableTeammate()
               || mate_min <= opp_min + 2 )
          && target_point.x < receive_pos.x + 30.0
          && wm.self().pos().dist( receive_pos ) < 30.0
          && wm.self().pos().dist( target_point ) < 20.0 )
    {

        return ServerParam::i().maxPower();
    }

    //------------------------------------------------------
    // decide dash power
    static bool s_recover_mode = false;
    if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.4 )
    {
       
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.7 )
    {
       
        s_recover_mode = false;
    }

    const double my_inc
        = wm.self().playerType().staminaIncMax()
        * wm.self().recovery();

    // dash power
    if ( s_recover_mode )
    {
//        // Magic Number.
//        // recommended one cycle's stamina recover value
//        dlog.addText( Logger::TEAM,
//                      __FILE__": getDashPower. recover mode" );
        return std::max( 0.0, my_inc - 30.0 );
    }

    if ( ! wm.opponentsFromSelf().empty()
         && wm.opponentsFromSelf().front()->distFromSelf() < 2.0
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.7
         && mate_min <= 8
         && ( mate_min <= opp_min + 3
              || wm.existKickableTeammate() )
         )
    {
        // opponent is very
        return ServerParam::i().maxPower();
    }

    if  ( wm.ball().pos().x < wm.self().pos().x
          && wm.self().pos().x < wm.offsideLineX() )
    {
        // ball is back
        // not offside
        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
        {
            return std::min( std::max( 5.0, my_inc - 30.0 ),
                             ServerParam::i().maxPower() );
        }
        else
        {
            return std::min( my_inc * 1.1,
                             ServerParam::i().maxPower() );
        }
    }

    if ( wm.ball().pos().x > wm.self().pos().x + 3.0 )
    {
        // ball is front
        if ( opp_min <= mate_min - 3 )
        {
            if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.6 )
            {

                return std::min( std::max( 0.1, my_inc - 30.0 ),
                                 ServerParam::i().maxPower() );
            }
            else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
            {

                return std::min( my_inc, ServerParam::i().maxPower() );
            }
            else
            {
                return ServerParam::i().maxPower();
            }
        }
        else
        {
            return ServerParam::i().maxPower();
        }
    }


    if ( target_point.x > wm.self().pos().x + 2.0
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6 )
    {
        return ServerParam::i().maxPower();
    }
    else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.8 )
    {
        return std::min( my_inc * 0.9,
                         ServerParam::i().maxPower() );
    }
    else
    {
        return std::min( my_inc * 1.5,
                         ServerParam::i().maxPower() );
    }
}



/*--------------------------------------------------------------------------------------------------------------------
 * 
 * 
 *
 * author: dy
 * 
 * date:2016  3 27
 * 
 * 
 * 
 *-----------------------------------------------------------------------------------------------------------------------*/


bool Bhv_AttackerOffensiveMove::throughPassShootHelper(PlayerAgent* agent)
{
  


    const WorldModel &wm =agent->world();
    const int num =wm.self().unum();
    bool through_pass=false;
    Vector2D home_pos=Strategy::i().getPosition(wm.self().unum());
    Vector2D target_point=home_pos;
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min =wm.interceptTable()->opponentReachCycle();
    const int self_min=wm.interceptTable()->selfReachCycle();
   



    bool   fast_dribble_team=mate_min<opp_min&&wm.ball().vel().x>0.7&&wm.ball().pos().x>wm.self().pos().x;



    if (!fast_dribble_team)  return false;



        double buff=wm.offsideLineX()-wm.self().pos().x;

        target_point.x=std::max(wm.offsideLineX()+buff,wm.ball().inertiaPoint(mate_min).x+buff);

        target_point.y=fabs(target_point.y-wm.self().pos().y)<5.0? wm.self().pos().y:target_point.y;

        double opp_dist_target=1000;
        const PlayerObject *opp_marker= wm.getOpponentNearestTo(target_point,5,&opp_dist_target);

//         if (wm.self().pos().x>home_pos.x)
//         {
//             return false;
//         }
// 


           double dist_thr=wm.ball().distFromSelf()*0.1;
           dist_thr=dist_thr<0.5? 0.5:dist_thr;

           if (!Body_GoToPoint(target_point,dist_thr,ServerParam::i().maxDashPower()).execute(agent))
           {
               Body_TurnToBall().execute(agent);
           }

           if (wm.ball().distFromSelf()>18)
           {
               agent->setNeckAction(new Neck_TurnToBallOrScan());

           }

           else
           {
               agent->setNeckAction(new Neck_TurnToBall());
           }



    return true;




  }


bool Bhv_AttackerOffensiveMove::CF_Voron_through_Pass(PlayerAgent *agent, Vector2D &target_point)
{
    const  VoronoiDiagramOriginal &voronoi=voron_point::instance().passVoronoiDiagram(agent);
    VoronoiDiagram::Vector2DCont V=voronoi.resultPoints();
    const WorldModel &wm = agent->world();
    Vector2D homePos=Strategy::i().getPosition(wm.self().unum());

    if (wm.self().unum()!=11)
    {
        return false;
    }


    bool ask_pass=target_point.x>wm.theirDefenseLineX()?true:false;


    std::vector<Vector2D> tp;

    for (VoronoiDiagram::Vector2DCont::const_iterator it=V.begin();it!=V.end();it++)
    {
        if ( !(*it).isValid()) continue;
        if ( (*it).absX()>52||(*it).absY()>34)  continue;
        if (ask_pass&&(*it).x<wm.theirDefenseLineX()) continue;
        if (!ask_pass&&(*it).x<wm.theirDefenseLineX()-1.0)  continue;
        tp.push_back((*it));
    }


    //check through pass

    if (tp.size())
    {
        return false;
    }





    std::vector<Vector2D> CF_tp;

    for (std::vector<Vector2D>::const_iterator it =tp.begin();it!=tp.end();it++)
    {



        CF_tp.push_back((*it));

    }


    Vector2D bestPos=rcsc::Vector2D::INVALIDATED;
    double best_score=-1000;


    for (std::vector<Vector2D>::const_iterator it = CF_tp.begin();it!=CF_tp.end();it++)
    {

        double s=(*it).dist(ServerParam::i().theirTeamGoalPos());

        if (s<best_score)
        {
            best_score=s;
            bestPos=(*it);
        }

    }


    if (bestPos.isValid())
    {
        target_point=bestPos;
        agent->doPointto(target_point.x,target_point.y);
        return true;
    }






    return false;

}


