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

#include "role_goalie.h"

#include "bhv_basic_tackle.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_free_kick.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_pass.h>
#include<rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/world_model.h>
#include<rcsc/player/player_intercept.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include<rcsc/action/view_synch.h>
#include <rcsc/geom/rect_2d.h>

using namespace rcsc;

const std::string RoleGoalie::NAME( "Goalie" );

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg( &RoleGoalie::create,
                                                       RoleGoalie::name() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
RoleGoalie::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    static const
        Rect2D our_penalty( Vector2D( -ServerParam::i().pitchHalfLength(),
                                                  -ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                                  Size2D( ServerParam::i().penaltyAreaLength() - 1.0,
                                                ServerParam::i().penaltyAreaWidth() - 2.0 ) );

    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const Vector2D nearest_opp_pos = ( nearest_opp
                                             ? nearest_opp->pos()
                                             : Vector2D( -1000.0, 0.0 ) );

    const rcsc::PlayerType * player_type = wm.self().playerTypePtr();
    float kickableArea = player_type->kickableArea();

    static bool kicking = false;

    //////////////////////////////////////////////////////////////
    // play_on play

    static bool isDanger = false;
    static int dangerCycle = 0;

    rcsc::Vector2D ball = wm.ball().pos();
//     rcsc::Vector2D prevVel = (wm.,ball().vel() * 100.0) / 94.0;
    rcsc::Vector2D prevBall = ball + wm.ball().rposPrev();
    rcsc::Vector2D nextBall = ball + wm.ball().vel();


     if( wm.lastKickerSide() == wm.ourSide() )
     {
      isDanger = true;
      dangerCycle = wm.time().cycle();
     }

     


    const ServerParam &SP=ServerParam::i();
    const double MAX_SELF_BALL_ERROR = 0.5;
     const bool catchable_situation = ( wm.self().goalie()
                                        && wm.lastKickerSide() != wm.ourSide() // not back pass
                                        && ( wm.gameMode().type() == GameMode::PlayOn
                                             || wm.gameMode().type() == GameMode::PenaltyTaken_ )
                                        && wm.time().cycle() >= agent->effector().getCatchTime().cycle() + SP.catchBanCycle()
                                        && ( wm.ball().pos().x < ( SP.ourPenaltyAreaLineX()
                                                                   + SP.ballSize() * 2
                                                                   - MAX_SELF_BALL_ERROR )
                                             && wm.ball().pos().absY()  < ( SP.penaltyAreaHalfWidth()
                                                                            + SP.ballSize() * 2
                                                                            - MAX_SELF_BALL_ERROR ) ) );

     if ( catchable_situation
          && wm.self().catchProbability() > 0.99)
     {

         agent->setNeckAction( new Neck_TurnToBall() );
         agent->setViewAction( new View_Synch() );
         return agent->doCatch();
     }
    else if ( wm.self().isKickable() )
    {
        kicking = true;
        isDanger = false;
        doKick( agent );
    }
    else
    {
        kicking = false;
        doMove( agent );
    }



    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleGoalie::doKick( PlayerAgent * agent )
{
   const WorldModel & wm = agent->world();
   const int self_min=wm.interceptTable()->selfReachCycle();
   const int opp_min=wm.interceptTable()->opponentReachCycle();

   //dy 2017年 01月 30日 星期一 14:59:11 CST

   if (opp_min<=self_min+4)
   {
       Body_ClearBall().execute( agent );
       agent->setNeckAction( new Neck_ScanField() );
       return;
   }


   const PlayerPtrCont & opps = wm.opponentsFromSelf();
   const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
   const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
   const Vector2D nearest_opp_pos = ( nearest_opp
                                             ? nearest_opp->pos()
                                             : Vector2D( -1000.0, 0.0 ) );

   Vector2D dribbleTarget = Vector2D(0,sign(wm.ball().pos().y)* 20.0 );

   Vector2D nearestTmmPos = wm.teammatesFromSelf().front()->pos();


   if( wm.existKickableTeammate() || (wm.teammatesFromSelf().front()->distFromBall() < 1.3 && !wm.teammatesFromBall().empty()) )
   {
       if( rcsc::Body_KickOneStep( rcsc::Vector2D(-40.0,rcsc::sign(wm.self().pos().y)*35.0), 3.0 ).execute( agent ) )
          agent->setNeckAction( new Neck_ScanField() );
   }
   else if( nearestTmmPos.dist(wm.ball().pos()) < 2.5 )
   {
     Body_ClearBall().execute( agent );
     agent->setNeckAction( new Neck_ScanField() );
   }
   else if( nearest_opp_dist > 9.0 && wm.self().stamina() > ServerParam::i().staminaMax() * 0.62 )
   {
     Body_Dribble( dribbleTarget, 1.0, ServerParam::i().maxDashPower(), 2
                         ).execute( agent );
     agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
   }
   else if ( Body_Pass().execute( agent ) )
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
   else
   {
     Body_ClearBall().execute( agent );
     agent->setNeckAction( new rcsc::Neck_ScanField() );
   }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleGoalie::doMove( PlayerAgent * agent )
{
    if ( Bhv_BasicTackle().execute( agent ) )
    {
        return ;
    }

    else if ( Bhv_GoalieChaseBall::is_ball_chase_situation( agent ) )
    {
        Bhv_GoalieChaseBall().execute( agent );
    }
    else
    {
        Bhv_GoalieBasicMove().execute( agent );
    }
}
