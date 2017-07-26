// -*-c++-*-

/*!
  \file neck_default_intercept_neck.cpp
  \brief default neck action to use with intercept action
*/

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA, Hidehisa AKIYAMA

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

#include "neck_default_intercept_neck.h"

#include "action_chain_graph.h"
#include "action_chain_holder.h"
#include "cooperative_action.h"

#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/view_wide.h>
#include <rcsc/action/view_normal.h>
#include <rcsc/action/view_synch.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

*/
Neck_DefaultInterceptNeck::~Neck_DefaultInterceptNeck()
{
    if ( M_default_view_act )
    {
        delete M_default_view_act;
        M_default_view_act = static_cast< ViewAction * >( 0 );
    }

    if ( M_default_neck_act )
    {
        delete M_default_neck_act;
        M_default_neck_act = static_cast< NeckAction * >( 0 );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_DefaultInterceptNeck::execute( PlayerAgent * agent )
{
    if ( doTurnToReceiver( agent ) )
    {
        return true;
    }

    dlog.addText( Logger::TEAM,
                        __FILE__": Neck_DefaultInterceptNeck. default action" );
    agent->debugClient().addMessage( "InterceptNeck" );

    if ( M_default_view_act )
    {
        M_default_view_act->execute( agent );
        agent->debugClient().addMessage( "IN:DefaultView" );
    }

    if ( M_default_neck_act )
    {
        agent->debugClient().addMessage( "IN:DefaultNeck" );
        M_default_neck_act->execute( agent );
    }
    else
    {
        agent->debugClient().addMessage( "IN:BallOrScan" );
        Neck_TurnToBallOrScan( -1 ).execute( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_DefaultInterceptNeck::doTurnToReceiver( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.interceptTable()->selfReachCycle() >= 2 )
    {
        dlog.addText( Logger::TEAM,
                            __FILE__": (doTurnToReceiver) self reach cycle >= 2 " );
        return false;
    }


    const CooperativeAction & first_action = M_chain_graph.getFirstAction();

    if ( first_action.category() != CooperativeAction::Pass )
    {
        dlog.addText( Logger::TEAM,
                            __FILE__": (doTurnToReceiver) not a pass type action" );
        return false;
    }

    const AbstractPlayerObject * receiver = wm.ourPlayer( first_action.targetPlayerUnum() );

    if ( ! receiver )
    {
        dlog.addText( Logger::TEAM,
                            __FILE__": (doTurnToReceiver) NULL receiver" );
        return false;

    }

    if ( receiver->posCount() <= 0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doTurnToReceiver) receiver[%d] pos_count=%d already seen",
                      receiver->unum(),
                      receiver->posCount() );
        return false;
    }


    View_Synch().execute( agent );

    Vector2D face_point = receiver->pos();
    face_point += receiver->vel();

    if ( receiver->distFromBall() > 5.0
         && receiver->pos().x < 35.0 )
    {
        face_point.x += 3.0;
    }

    const Vector2D next_pos = agent->effector().queuedNextMyPos();
    const AngleDeg next_body = agent->effector().queuedNextMyBody();
    const double next_view_half_width = agent->effector().queuedNextViewWidth().width() * 0.5;

    AngleDeg rel_angle = ( face_point - next_pos ).th() - next_body;

    if ( rel_angle.abs() > ServerParam::i().maxNeckAngle() + next_view_half_width - 5.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (duTurnToReceiver) receiver[%d] cannot face to face_point=(%.2f %.2f)",
                      receiver->unum(),
                      face_point.x, face_point.y );
        return false;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (doTurnToReceiver) receiver[%d] face_point=(%.2f %.2f)",
                  receiver->unum(),
                  face_point.x, face_point.y );

    agent->debugClient().setTarget( receiver->unum() );
    agent->debugClient().addMessage( "InterceptNeck:toReceiver%d",
                                     receiver->unum() );
    agent->debugClient().addLine( wm.self().pos(), face_point );
    agent->debugClient().addCircle( face_point, 3.0 );

    Neck_TurnToPoint( face_point ).execute( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
NeckAction *
Neck_DefaultInterceptNeck::clone() const
{
    return new Neck_DefaultInterceptNeck( M_chain_graph,
                                          ( M_default_view_act
                                            ? M_default_view_act->clone()
                                            : static_cast< ViewAction * >( 0 ) ),
                                          ( M_default_neck_act
                                            ? M_default_neck_act->clone()
                                            : static_cast< NeckAction * >( 0 ) ) );
}




/*-------------------------------------------------------------------*/
/*!

*/
Neck_ChaseBall::Neck_ChaseBall()
{
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_ChaseBall::execute( rcsc::PlayerAgent * agent )
{
    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__ ": Neck_ChaseBall" );

    const rcsc::ServerParam & param = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D next_ball_pos = agent->effector().queuedNextBallPos();
    const rcsc::Vector2D next_self_pos = agent->effector().queuedNextMyPos();
    const rcsc::AngleDeg next_self_body = agent->effector().queuedNextMyBody();
    const double narrow_half_width = rcsc::ViewWidth::width( rcsc::ViewWidth::NARROW ) / 2.0;
    const double next_view_half_width = agent->effector().queuedNextViewWidth().width() / 2.0;

    const rcsc::AngleDeg angle_diff = ( next_ball_pos - next_self_pos ).th() - next_self_body;

    const rcsc::SeeState & see_state = agent->seeState();
    const int see_cycles = see_state.cyclesTillNextSee() + 1;
    const bool can_see_next_cycle_narrow = ( angle_diff.abs()
                                             < param.maxNeckAngle()
                                               + narrow_half_width );
    const bool can_see_next_cycle = ( angle_diff.abs()
                                      < param.maxNeckAngle()
                                        + next_view_half_width );

    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__": see_cycles = %d",
                        see_cycles );
    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__": can_see_next_cycle = %s",
                        (can_see_next_cycle ? "true" : "false") );
    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__": can_see_next_cycle_narrow = %s",
                        (can_see_next_cycle_narrow ? "true" : "false") );

    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int opponent_reach_cycle = wm.interceptTable()->opponentReachCycle();
    int predict_step = std::min( opponent_reach_cycle,
                                 teammate_reach_cycle );

    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__": predict_step = %d", predict_step );

    if ( can_see_next_cycle )
    {
        if ( see_cycles != 1
             && can_see_next_cycle_narrow )
        {
            rcsc::dlog.addText( rcsc::Logger::ACTION,
                                __FILE__": set view to synch" );

            rcsc::View_Synch().execute( agent );
        }
        else
        {
            rcsc::dlog.addText( rcsc::Logger::ACTION,
                                __FILE__": view untouched" );
        }
    }
    else
    {
        if ( see_cycles >= 2
             && can_see_next_cycle_narrow )
        {
            rcsc::dlog.addText( rcsc::Logger::ACTION,
                                __FILE__": set view to narrow" );

            rcsc::View_Synch().execute( agent );
        }
        else
        {
            rcsc::dlog.addText( rcsc::Logger::ACTION,
                                __FILE__": set view to normal" );

            rcsc::View_Normal().execute( agent );
        }
    }

    double target_angle = rcsc::bound( - param.maxNeckAngle(),
                                       angle_diff.degree(),
                                       + param.maxNeckAngle() );

    agent->doTurnNeck( target_angle - wm.self().neck() );

    return true;
}

rcsc::NeckAction *
Neck_ChaseBall::clone() const
{
    return new Neck_ChaseBall();
}
