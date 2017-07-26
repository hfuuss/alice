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

#include "role_side_forward.h"

#include "strategy.h"

#include "bhv_basic_offensive_kick.h"


#include "bhv_basic_tackle.h"

#include "bhv_basic_move.h"



#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/view_wide.h>

#include <rcsc/formation/formation.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/math_util.h>
#include <rcsc/action/body_intercept.h>
#include "neck_offensive_intercept_neck.h"
#include "bhv_chain_action.h"
#include "field_analyzer.h"
#include<rcsc/geom/voronoi_diagram.h>
#include <rcsc/action/body_go_to_point_dodge.h>
#include "bhv_attacker_offensive_move.h"
#include "shoot_area_sf_move.h"
static const int VALID_PLAYER_THRESHOLD = 8;

const std::string RoleSideForward::NAME( "SideForward" );
using namespace rcsc;
/*-------------------------------------------------------------------*/
/*!

*/
bool
RoleSideForward::execute( rcsc::PlayerAgent * agent )
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
RoleSideForward::doKick( rcsc::PlayerAgent * agent )
{
  if (Bhv_ChainAction().execute(agent))
  {
    return;
  }
  Bhv_BasicOffensiveKick().execute(agent);
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleSideForward::doMove( rcsc::PlayerAgent * agent )
{
   
  
   const Vector2D home=Strategy::i().getPosition(agent->world().self().unum());

    switch ( Strategy::get_ball_area( agent->world() ) ) {
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Danger:
        Bhv_BasicMove( ).execute( agent );
        break;
    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DefMidField:
    case Strategy::BA_DribbleAttack:
    case Strategy::BA_OffMidField:
       // Bhv_SideHalfOffensiveMove().execute(agent);
      Bhv_AttackerOffensiveMove(home,true).execute(agent);
        break;
    case Strategy::BA_Cross: // x>39, y>17
    case Strategy::BA_ShootChance: // x>39, y<17
      shoot_area_sf_move().execute(agent);
     //  Bhv_SideHalfOffensiveMove().execute(agent);

        break;
    default:
      
        Bhv_BasicMove( ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        break;
    }
}
