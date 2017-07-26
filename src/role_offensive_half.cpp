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

#include "role_offensive_half.h"

#include "strategy.h"

#include "bhv_basic_offensive_kick.h"

#include "bhv_basic_tackle.h"




#include "bhv_basic_move.h"
#include "bhv_attacker_offensive_move.h"
#include "body_forestall_block.h"


#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_scan_field.h>

#include <rcsc/formation/formation.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/player_config.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/action/body_intercept.h>
#include<rcsc/action/body_clear_ball.h>
#include "bhv_chain_action.h"
#include "bhv_offensive_half_offensive_move.h"
#include "shoot_area_of_move.h"
#include "bhv_offensivehalf_defensive_move.h"
using namespace rcsc;
const std::string RoleOffensiveHalf::NAME( "OffensiveHalf" );

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg( &RoleOffensiveHalf::create,
                                                       RoleOffensiveHalf::NAME );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool 
RoleOffensiveHalf::execute( rcsc::PlayerAgent * agent )
{
    //////////////////////////////////////////////////////////////
    // play_on play
    bool kickable = agent->world().self().isKickable();
    if ( agent->world().existKickableTeammate()
         && agent->world().teammatesFromBall().front()->distFromBall()
         < agent->world().ball().distFromSelf() )
    {
        kickable = false;
    }

    if ( kickable )
    {
        // kick
        doKick( agent );
    }
    else
    {
        // positioning
        doMove( agent );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleOffensiveHalf::doKick( rcsc::PlayerAgent * agent )
{

    const WorldModel &wm =agent->world();
    

 
  
    if ( Bhv_ChainAction().execute( agent ) )
    {

        return;
    }

    Bhv_BasicOffensiveKick().execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleOffensiveHalf::doMove( rcsc::PlayerAgent * agent )
{

    const WorldModel &wm = agent->world();
    int num=wm.self().unum();
    
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    



    switch ( Strategy::get_ball_area(wm) ) {
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Danger:
    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DefMidField:
      
     if (wm.existKickableOpponent()||opp_min<mate_min&&self_min<opp_min)
     {
         Bhv_OffensiveHalf_Defensive_Move().execute(agent);
     }
     else 
     {
       
         Bhv_OffensiveHalfOffensiveMove().execute(agent);
     }
       
        break;
    case Strategy::BA_DribbleAttack:
    case Strategy::BA_OffMidField:
      // doOffensiveMove( agent, home_pos );
      Bhv_OffensiveHalfOffensiveMove().execute(agent);
   //   Bhv_BasicMove().execute(agent);
        break;
    case Strategy::BA_Cross:
    case Strategy::BA_ShootChance:
        //doShootAreaMove( agent, home_pos );
        offsnive_half_shoot_area_move().execute(agent);
        break;
    default:
      
        Bhv_BasicMove( ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        break;
    }
}
