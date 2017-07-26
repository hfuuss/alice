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
#include "bhv_defensivehalf_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include "neck_offensive_intercept_neck.h"
#include "bhv_get_ball.h"
#include "neck_default_intercept_neck.h"
#include "neck_default_intercept_neck.h"
#include "df_positioning.h"
#include  "field_analyzer.h"
#include "neck_check_ball_owner.h"
#include "mark_find_best_target.h"
#include "bhv_goalie_chase_ball.h"
#include "bhv_basic_move.h"
using namespace rcsc;
//#define AVOIDE_MARK

bool Bhv_DefensiveHalf_Move::execute( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();

    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = target_point;

    int num = wm.self().unum();

    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();


    if ( !wm.existKickableTeammate()&& wm.interceptTable()->selfReachCycle()<=wm.interceptTable()->teammateReachCycle()&&
         wm.interceptTable()->selfReachCycle()<=wm.interceptTable()->opponentReachCycle())
    {
          Body_Intercept().execute( agent );
          if (wm.ball().vel().x>0)
          agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
          else
          agent->setNeckAction(new  Neck_DefaultInterceptNeck(new Neck_TurnToBallOrScan(0)));


        return true;
    }








    switch ( Strategy::get_ball_area(wm)) {
      
    

    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DefMidField:

    {
        if (Block_BA_DF(agent))  return true;
    }

        break;
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Danger:
    {

        if (Block_BA_DG(agent))    return true;

    }

        break;

    default:
        break;
    }
    
    if (MarkPlan(agent))
    {
        return true;
    }

//    if (EmergencyMove(agent))
//    {
//        return true;
//    }
    
    
    
    
    doNormalMove(agent);
    
    
    
    
    
}



void Bhv_DefensiveHalf_Move::doNormalMove(PlayerAgent* agent)
{
    const WorldModel & wm = agent->world();



    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );


    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    // const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = Strategy::i().getPosition( wm.self().unum() );
    homePos.x=Strategy::i().get_defense_line_x(wm,homePos)+3.0;




//    if (homePos.x>-35.0&&homePos.x-wm.ourDefenseLineX()<4.0)
//    {
//        homePos.x=wm.ourDefenseLineX()+4.0;
//    }

//    if (homePos.x>0) homePos.x=0;

    
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D  PRE_BALL =wm.ball().inertiaPoint(std::min(opp_min,mate_min) );
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;
    double dash_power = Strategy::get_defender_dash_power( wm, homePos);
    if ( !Body_GoToPoint( homePos, dist_thr, dash_power,
                         -1.0, // dash speed
                         1 // 1 step
                         ).execute( agent ) )
    {
         AngleDeg body_angle = 0.0;
        if ( wm.ball().angleFromSelf().abs() > 80.0 )
        {
            body_angle = ( wm.ball().pos().y > wm.self().pos().y
                           ? 90.0
                           : -90.0 );
        }
        Body_TurnToAngle( body_angle ).execute( agent );

    }

    agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );


}







bool Bhv_DefensiveHalf_Move::EmergencyMove(PlayerAgent* agent)
{
    const WorldModel & wm = agent->world();
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    // const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = Strategy::i().getPosition( wm.self().unum() );
   //homePos.x=Strategy::i().get_defense_line_x(wm,homePos);
    const int num=wm.self().unum();


    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D  PRE_BALL =wm.ball().inertiaPoint(std::min(opp_min,mate_min) );
    const double  dash_power=ServerParam::i().maxDashPower();

    bool  emergency_situation=false;


//   if (mate_min>opp_min&&self_min>opp_min&&
//            std::min(wm.ourDefenseLineX(),PRE_BALL.x)
//                       <wm.self().pos().x)
//   {
//       emergency_situation=true;
//       if   ( me.x > homePos.x + 2.5  && ball.x > -40.0 && std::fabs( me.absY() - homePos.absY() ) < 5.0 )
//       {
//           homePos.x = me.x - 10;
//           homePos.y = me.y;
//       }

//   }







    if (emergency_situation){


                double dash_power = ServerParam::i().maxDashPower();
                double dist_thr = std::fabs( wm.ball().pos().x - wm.self().pos().x ) * 0.1;

                dist_thr=dist_thr<0.5?0.5:dist_thr;

                if ( !Body_GoToPoint( homePos, dist_thr, dash_power,
                                     -1.0, // dash speed
                                     1 // 1 step
                                     ).execute( agent ) )
                {
                    // already there

                    int min_step = std::min( wm.interceptTable()->teammateReachCycle(),
                                             wm.interceptTable()->opponentReachCycle() );

                    Vector2D ball_pos = wm.ball().inertiaPoint( min_step );
                    Vector2D self_pos = wm.self().inertiaFinalPoint();
                    AngleDeg ball_angle = ( ball_pos - self_pos ).th();

                    AngleDeg target_angle = ball_angle + 90.0;
                    if ( ball_pos.x < -47.0 )
                    {
                        if ( target_angle.abs() > 90.0 )
                        {
                            target_angle += 180.0;
                        }
                    }
                    else
                    {
                        if ( target_angle.abs() < 90.0 )
                        {
                            target_angle += 180.0;
                        }
                    }

                    Body_TurnToAngle( target_angle ).execute( agent );
                }

                agent->setNeckAction( new Neck_CheckBallOwner() );

                return true;





    }

    return false;


}


bool  Bhv_DefensiveHalf_Move::Block_BA_DF(PlayerAgent *agent)
{
    //
    // tackle
    //



    const WorldModel & wm = agent->world();


    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    const AbstractPlayerObject *fastest_opp=wm.interceptTable()->fastestOpponent();

     if (!fastest_opp)
     {
         return false;
     }

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
   


    const Vector2D  our_goal(-52.5,0);





        if ( mark_target && mark_target!=fastest_opp && mark_target->pos().dist(our_goal) < fastest_opp->pos().dist(our_goal) )
        {

            return false;
        }



    if (mark_target==fastest_opp||self_min<=mate_min+3)
    {

        return Bhv_GetBall().DefenderGetBall(agent);

    }



    return false;
}



bool Bhv_DefensiveHalf_Move::Block_BA_DG(PlayerAgent *agent)
{
    const WorldModel&wm = agent->world();

    const int self_step = wm.interceptTable()->selfReachStep();
    const int teammate_step = wm.interceptTable()->teammateReachStep();
    const int opponent_step = wm.interceptTable()->opponentReachStep();

    if ( wm.existKickableTeammate()
         || opponent_step > teammate_step
         || opponent_step > self_step )
    {

        return false;
    }

    const PlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();
    if ( ! fastest_opp )
    {

        return false;
    }

    const Vector2D opponent_ball_pos = wm.ball().inertiaPoint( opponent_step );
    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );


    const PlayerObject *nearestOpp=wm.getOpponentNearestToSelf(5,true);

  const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();
   
  const Vector2D our_goal(-52.5,0);

  if (
          mark_target&&mark_target!=fastest_opp&&
          mark_target->pos().dist(our_goal)< fastest_opp->pos().dist(our_goal)){

      return false;
  }



  if (mark_target==fastest_opp||self_step<=teammate_step)
  {

      return Bhv_GetBall().DefenderGetBall(agent);

  }


  if (FieldAnalyzer::sameside(wm.ball().pos(),home_pos)&&!mark_target&&wm.ball().pos().absY()<20)

  {

      return Bhv_GetBall().DefenderGetBall(agent);

  }

  return false;


}



bool Bhv_DefensiveHalf_Move::MarkPlan(PlayerAgent *agent)
{

    const WorldModel &wm = agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min =wm.interceptTable()->selfExhaustReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();

    const  Vector2D  home =Strategy::i().getPosition(wm.self().unum());
  const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();
   


    if (PressMark(agent,mark_target)) return true;

    if (Mark(agent,mark_target))  return true;

    return false;



}


bool Bhv_DefensiveHalf_Move::PressMark(PlayerAgent *agent, const AbstractPlayerObject *mark_target)
{
    const WorldModel &wm = agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min =wm.interceptTable()->selfExhaustReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
   if (self_min<opp_min||mate_min<opp_min||wm.existKickableTeammate())
    {
        return false;
    }
    if (!mark_target || mark_target->posCount()>5||wm.self().stamina()<ServerParam::i().staminaMax()*0.5)
    {
        return false;
    }

    const int min_cycle=std::min(opp_min,mate_min);


    const Vector2D target_opp =mark_target->inertiaPoint(3);

    const Vector2D home = Strategy::i().getPosition(wm.self().unum());
    const double homedistOpp= target_opp.dist(home);
    const int min_min=std::min( self_min, std::min(mate_min,opp_min));
    const Vector2D Pre_Ball =wm.ball().inertiaPoint(min_min);


    if (target_opp.x<wm.ourDefenseLineX()) return false;



    if (homedistOpp>10.0) return false;



    const double    markPower =ServerParam::i().maxDashPower();
    DefensiveAction(target_opp,markPower,mark_target).Mark(agent);
    return true;
}

bool Bhv_DefensiveHalf_Move::Mark(PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target)
{
    const WorldModel &wm = agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min =wm.interceptTable()->selfExhaustReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();


    if (self_min+2<opp_min||mate_min+2<opp_min||wm.existKickableTeammate())
    {
        return false;
    }
    if (!mark_target || mark_target->posCount()>3||mark_target==wm.interceptTable()->fastestOpponent())
    {
        return false;
    }

    if (mark_target==wm.interceptTable()->fastestOpponent())   return false;


    const int min_cycle=std::min(opp_min,mate_min);


    const Vector2D target_opp =mark_target->inertiaPoint(3);
    if (target_opp.x>-30.0||wm.ball().inertiaPoint(min_cycle).x>-10.0)
    {
        return false;
    }
    
 



    double min_df_x=std::min(wm.ourDefenseLineX(),wm.ball().inertiaPoint(min_cycle).x);



    if (wm.ourDefenseLineX()>-37.0&&wm.ball().pos().x>-35.0)
    {
        return false;
    }



    const Vector2D home = Strategy::i().getPosition(wm.self().unum());
    const double homedistOpp= target_opp.dist(home);
    const int min_min=std::min( self_min, std::min(mate_min,opp_min));
    const Vector2D Pre_Ball =wm.ball().inertiaPoint(min_min);

//    double home_dist_thr=20.0;
//    Circle2D opp_rec(mark_target->pos(),4);
//    if ( opp_rec.contains(Pre_Ball))
//    {
//        home_dist_thr*=2;
//    }
//    if (homedistOpp>home_dist_thr&&wm.ball().pos().x<-36.0)
//    {
//        return false;
//    }


    const Vector2D  targetPoint =mark_find_best_target::getMarkTarget(agent,mark_target);

    if (!targetPoint.isValid())
    {
        return false;
    }
    const double    markPower =ServerParam::i().maxDashPower();
    DefensiveAction(targetPoint,markPower,mark_target).Mark(agent);
    return  true;
}





