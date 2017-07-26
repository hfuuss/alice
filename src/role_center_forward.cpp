// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty ofo
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

#include "role_center_forward.h"

#include "strategy.h"

#include "bhv_basic_offensive_kick.h"
#include "bhv_basic_tackle.h"
#include "bhv_basic_move.h"


#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/view_wide.h>

#include <rcsc/formation/formation.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/math_util.h>
#include<rcsc/player/say_message_builder.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include "neck_default_intercept_neck.h"
#include "neck_offensive_intercept_neck.h"
#include "bhv_chain_action.h"
#include "field_analyzer.h"
#include<rcsc/geom/voronoi_diagram.h>
#include <rcsc/action/body_go_to_point_dodge.h>
#include "bhv_attacker_offensive_move.h"
#include "shoot_area_cf_move.h"
static const int VALID_PLAYER_THRESHOLD = 8;

using namespace rcsc;
int RoleCenterForward::S_count_shot_area_hold = 0;
const std::string RoleCenterForward::NAME( "CenterForward" );

/*-------------------------------------------------------------------*/
/*!

*/
bool
RoleCenterForward::execute( rcsc::PlayerAgent* agent )
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
RoleCenterForward::doKick( rcsc::PlayerAgent* agent )
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
RoleCenterForward::doMove( rcsc::PlayerAgent * agent )
{
    S_count_shot_area_hold = 0;

    rcsc::Vector2D home_pos = Strategy::i().getPosition( agent->world().self().unum() );
    if ( ! home_pos.isValid() ) home_pos.assign( 0.0, 0.0 );

    switch ( Strategy::get_ball_area( agent->world() ) ) {
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Danger:
       Bhv_BasicMove( ).execute( agent );
        break;
    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DefMidField:
     // Bhv_AttackerOffensiveMove( home_pos,true).execute( agent );
      //  break;
    case Strategy::BA_DribbleAttack:
    case Strategy::BA_OffMidField:

        Bhv_AttackerOffensiveMove( home_pos,true).execute( agent );
       // Bhv_CenterForwardMove().execute(agent);
        break;
    case Strategy::BA_Cross:
    case Strategy::BA_ShootChance:
      shoot_area_cf_move().execute(agent);
        break;
    default:
   
      Bhv_BasicMove().execute(agent);
        break;
    }
}
