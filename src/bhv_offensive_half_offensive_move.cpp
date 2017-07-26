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

#include "bhv_offensive_half_offensive_move.h"

#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>

#include <rcsc/action/body_intercept.h>

#include "bhv_basic_move.h"
#include "bhv_attacker_offensive_move.h"
#include "bhv_basic_tackle.h"
#include "neck_offensive_intercept_neck.h"

#include "strategy.h"
//#include "bhv_press.h"

#include "bhv_get_ball.h"
#include "df_positioning.h"
#include "mark_find_best_target.h"

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_OffensiveHalfOffensiveMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_OffensiveHalfOffensiveMove" );

    const WorldModel & wm = agent->world();
    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();




    //
    // intercept
    //
    if ( doIntercept( agent ) )
    {
        return true;
    }
    
    
    if (doPress(agent))
    {
      return true;
    }




    //
    // get ball
    //
    if ( doGetBall( agent ) )
    {
        return true;
    }



    if (EmergencyMove(agent))
     {
        return true;
    }


    Bhv_BasicMove( ).execute( agent );

    return true;
}


bool Bhv_OffensiveHalfOffensiveMove::EmergencyMove(PlayerAgent* agent)
{
    const WorldModel & wm = agent->world();



    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );


    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = target_point;

    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D  PRE_BALL =wm.ball().inertiaPoint(std::min(opp_min,mate_min) );
    bool  EmergencyMove=false;
    // midfielders fall back fast
    if( opp_min < mate_min && opp_min < 15 && me.x > homePos.x + 4 && wm.ourDefenseLineX() < me.x - 0.5
            && ball.x > -40.0 && std::fabs( me.absY() - homePos.absY() ) < 7.5 )
    {
        homePos.x = me.x - 10;
        homePos.y = me.y;
        double dash_power = Strategy::get_defender_dash_power(wm,homePos);
        DefensiveAction(homePos,dash_power,NULL).Formation(agent);
        return true;
    }





    return false;



}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_OffensiveHalfOffensiveMove::doIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && self_min < mate_min
         && self_min <= opp_min + 1 )
    {
     
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        return true;
    }

    return false;
    
    
    
}


bool 
Bhv_OffensiveHalfOffensiveMove::doPress(PlayerAgent* agent)
{



    return false;
    
    
    
}















bool
Bhv_OffensiveHalfOffensiveMove::doGetBall( PlayerAgent * agent )
{
     const WorldModel & wm = agent->world();


    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    const AbstractPlayerObject *fastest_opp=wm.interceptTable()->fastestOpponent();


    if (wm.existKickableTeammate() || mate_min<opp_min||self_min<opp_min)
    {

        return false;
    }
    //
    // force  intercept
    //
    if ( ! wm.existKickableTeammate()
         && self_min <= mate_min+1
         && self_min <= opp_min + 1 )
    {
        dlog.addText( Logger::ROLE,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );
#ifdef NO_USE_NECK_DEFAULT_INTERCEPT_NECK
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
#else
        if ( opp_min >= self_min + 3 )
        {
            agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        }
        else
        {
            agent->setNeckAction(  new Neck_TurnToBallOrScan()  );
        }
#endif
        return true;
    }

    const PlayerObject *nearestOpp=wm.getOpponentNearestToSelf(5,true);

  const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();




    if (mark_target==fastest_opp||self_min<mate_min)
    {

        return Bhv_GetBall().DefenderGetBall(agent);

    }






    return false;
}
