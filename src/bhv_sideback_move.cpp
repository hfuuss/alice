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
#include "bhv_sideback_move.h"


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
#include "bhv_goalie_chase_ball.h"
#include "bhv_get_ball.h"
#include "neck_default_intercept_neck.h"
#include "field_analyzer.h"
#include "df_positioning.h"
#include "mark_find_best_target.h"
#include "neck_check_ball_owner.h"
//#define  DEBUG_DEFENSIVE_MARK


using namespace rcsc;

bool Bhv_SideBack_Move::is_marking=false;

bool Bhv_SideBack_Move::execute( PlayerAgent * agent )
{




    const WorldModel & wm = agent->world();

    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const int Minmin=std::min(self_min,std::min(mate_min,opp_min) );

    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );


    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = target_point;

    int num = wm.self().unum();




    Vector2D myInterceptPos = wm.ball().inertiaPoint( self_min );



    //try intercept

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
    case Strategy::BA_DefMidField:
    case Strategy::BA_DribbleBlock:
    {

        if (Block_BA_DF(agent))
        return true;
    }

        break;
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Danger:
    {
//        if (Bhv_GoalieChaseBall::is_ball_chase_situation(agent))
//        {
//            return  Bhv_GoalieChaseBall().execute(agent);
//        }
        if (Block_BA_CB(agent))
         return true;
    }

        break;

    default:
        break;
    }



    if (MarkPlan(agent))
    {
        return true;
    }



    if ( EmergencyMove(agent))
    {
        return true;
    }
    




    if (DefensiveMark(agent))
    {

        return true;
    }



    doNormalMove(agent);
    
    
}


void Bhv_SideBack_Move::doNormalMove(PlayerAgent* agent)
{
    const WorldModel & wm = agent->world();
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    // const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = Strategy::i().getPosition( wm.self().unum() );
    homePos.x=Strategy::i().get_defense_line_x(wm,homePos);


    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();


    const Vector2D  PRE_BALL =wm.ball().inertiaPoint(std::min(opp_min,mate_min) );
    

    double dash_power = Strategy::get_defender_dash_power( wm, homePos);
    double dist_thr = std::fabs( wm.ball().pos().x - wm.self().pos().x ) * 0.1;
    

    if(  me.x > wm.ourDefenseLineX() + 2.0 && homePos.x > me.x )
    {
      homePos.x = me.x;
    }

    if ( dist_thr < 0.5 ) dist_thr = 0.5;


    if (! Body_GoToPoint( homePos, dist_thr, dash_power,
                         -1.0, // dash speed
                         1, true, 12
                         ).execute( agent ) )
    {

        // already there

        Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
        Vector2D my_final = wm.self().inertiaFinalPoint();
        AngleDeg ball_angle = ( ball_next - my_final ).th();

        AngleDeg target_angle;
        if ( ball_next.x < -30.0 )
        {
            target_angle = ball_angle + 90.0;
            if ( ball_next.x < -45.0 )
            {
                if ( target_angle.degree() < 0.0 )
                {
                    target_angle += 180.0;
                }
            }
            else
            {
                if ( target_angle.degree() > 0.0 )
                {
                    target_angle += 180.0;
                }
            }
        }
        else
        {
            target_angle = ball_angle + 90.0;
            if ( ball_next.x > my_final.x + 15.0 )
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
        }

        Body_TurnToAngle( target_angle ).execute( agent );
    }

    agent->setNeckAction( new Neck_CheckBallOwner() );

}



bool Bhv_SideBack_Move::EmergencyMove(PlayerAgent* agent)
{
    const WorldModel & wm = agent->world();
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    // const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = Strategy::i().getPosition( wm.self().unum() );
     homePos.x=Strategy::i().get_defense_line_x(wm,homePos);
    const int num=wm.self().unum();


    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D  PRE_BALL =wm.ball().inertiaPoint(std::min(opp_min,mate_min) );
    const double  dash_power=ServerParam::i().maxDashPower();

    bool  emergency_situation=false;

    // danger situation for defenders

        // while other defenders are behind my homepos
    if( num < 6 && opp_min < mate_min && opp_min < 8 && wm.ourDefenseLineX() <homePos.x - 0.5 &&
        ball.x < 0.0 && ball.x > -35.0 )
    {
       homePos.x = std::max( -35.0, wm.ourDefenseLineX() );
       emergency_situation=true;
    }


//       // defenders fall back fast
//       if( num < 6 && opp_min < mate_min && opp_min < 15 && me.x > homePos.x + 2.5 && wm.ourDefenseLineX() < me.x - 0.5 &&
//           ball.x < 0.0 && ball.x > -40.0 && std::fabs( me.absY() - homePos.absY() ) < 5.0 )
//       {
//           homePos.x = me.x - 10;
//           homePos.y = me.y;
//           emergency_situation=true;

//       }





    if (emergency_situation){


                double dash_power = ServerParam::i().maxDashPower();
                double dist_thr = std::fabs( wm.ball().pos().x - wm.self().pos().x ) * 0.1;

                dist_thr=dist_thr<0.5?0.5:dist_thr;


                if ( !Body_GoToPoint( homePos, dist_thr, dash_power,
                                     -1.0, // dash speed
                                     1, // 1 step
                                     true, // save recovery
                                     12.0
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



bool Bhv_SideBack_Move::MarkPlan(PlayerAgent *agent)
{

    const WorldModel &wm = agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min =wm.interceptTable()->selfExhaustReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    const  Vector2D  home =Strategy::i().getPosition(wm.self().unum());

    const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();

    if (Mark(agent,mark_target))  return true;

    return false;



}


bool Bhv_SideBack_Move::PressMark(PlayerAgent *agent, const AbstractPlayerObject *mark_target)
{

    const WorldModel &wm = agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min =wm.interceptTable()->selfExhaustReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();

   if (wm.ball().pos().x<0) return false;

   if (self_min<opp_min||mate_min<opp_min||wm.existKickableTeammate())
    {
        return false;
    }
    if (!mark_target || mark_target->posCount()>5)
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
    if (target_opp.x>5.0)  return false;


    if (wm.self().stamina()<0.5*ServerParam::i().staminaMax()) return false;

    if (homedistOpp>15.0) return false;



    const double    markPower =ServerParam::i().maxDashPower();
    DefensiveAction(target_opp,markPower,mark_target).Mark(agent);
    return true;
}

bool Bhv_SideBack_Move::Mark(PlayerAgent *agent,const rcsc::AbstractPlayerObject *mark_target)
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


    const int min_cycle=std::min(opp_min,mate_min);

    const Vector2D target_opp =mark_target->inertiaPoint(3);
    if (target_opp.x>-35.0||wm.ball().inertiaPoint(min_cycle).x>-10.0)
    {
        return false;
    }
    
 



    double min_df_x=std::min(wm.ourDefenseLineX(),wm.ball().inertiaPoint(min_cycle).x);




    const Vector2D home = Strategy::i().getPosition(wm.self().unum());
    const double homedistOpp= target_opp.dist(home);
    const int min_min=std::min( self_min, std::min(mate_min,opp_min));
    const Vector2D Pre_Ball =wm.ball().inertiaPoint(min_min);

    double home_dist_thr=20.0;
    Circle2D opp_rec(mark_target->pos(),4);
    if ( opp_rec.contains(Pre_Ball))
    {
        home_dist_thr*=2;
    }
    if (homedistOpp>home_dist_thr&&wm.ball().pos().x<-36.0)
    {
        return false;
    }


    const Vector2D  targetPoint =mark_find_best_target::getMarkTarget(agent,mark_target);


    if (!targetPoint.isValid())
    {
        return false;
    }
    const double    markPower =ServerParam::i().maxDashPower();
    DefensiveAction(targetPoint,markPower,mark_target).Mark(agent);
    return  true;
}


bool  Bhv_SideBack_Move::DefensiveMark( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

     Vector2D ball = wm.ball().pos();
     Vector2D me = wm.self().pos();

     int self_min = wm.interceptTable()->selfReachCycle();
     int mate_min = wm.interceptTable()->teammateReachCycle();
     int opp_min = wm.interceptTable()->opponentReachCycle();

     const Vector2D homePos=Strategy::i().getPosition(wm.self().unum());
     Vector2D targetPoint=homePos;

     if (self_min<opp_min||mate_min<opp_min)
     {
         return false;
     }

     const PlayerPtrCont & opps = wm.opponentsFromSelf();
     const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();


   //  if (mark_target==wm.interceptTable()->fastestOpponent()&&opp_min>2)
   //  {
   //        return Bhv_GetBall().DefenderGetBall(agent);
   //  }


      if (!mark_target||mark_target==wm.interceptTable()->fastestOpponent())  return false;


     const Vector2D opp = ( mark_target
                                              ? mark_target->inertiaFinalPoint()
                                              : Vector2D( -1000.0, 0.0 ) );
   if (wm.ourPlayer(wm.self().unum()-2)&&opp.absY()<wm.ourPlayer(wm.self().unum()-2)->pos().absY())   return false;


  Vector2D oppFhome = Vector2D(0,0);


  int num = wm.self().unum();
  float yDiff = 10.0;

  if( wm.ball().pos().x<-25.0)
       yDiff = 3.0;  // IO2012 4.5 bud









  if( homePos.x > -35.0 && opp.x >=wm.ourDefenseLineX()&&opp.x<wm.ourDefenseLineX()+3.0
      &&opp.x>-35.0)
  {

    if( (wm.self().unum() == 4 && opp.y < homePos.y + yDiff&&opp.y>homePos.y-yDiff) ||
        (wm.self().unum() == 5 && opp.y > homePos.y - yDiff&&homePos.y<homePos.y+yDiff) )
    {


        targetPoint.y = sign(opp.y)*std::max(0.0,(opp.absY()-2.0));
        targetPoint.x = opp_min>1? opp.x - 2.0:opp.x;



      if( targetPoint.x < -36.0 )  // was -37.0 before 2011
         targetPoint.x = -36.0;

     double dash_power=ServerParam::i().maxDashPower();

      DefensiveAction(homePos,dash_power,mark_target).Mark(agent);
      return true;

    }

  }


  return false;
}




bool Bhv_SideBack_Move::Block_BA_DF(PlayerAgent *agent)
{
    const WorldModel & wm = agent->world();
    Vector2D homePos = Strategy::i().getPosition( wm.self().unum() );
    homePos.x=Strategy::i().get_defense_line_x(wm,homePos);
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const AbstractPlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();
    if ( ! fastest_opp )
    {

        return false;
    }
    if (wm.existKickableTeammate() || mate_min<opp_min||self_min<opp_min)
    {

        return false;
    }

    const Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );




  const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();
  const Vector2D our_goal(-52.5,0);

  if (mark_target&&mark_target!=fastest_opp&&
          mark_target->pos().dist(our_goal)<wm.ball().pos().dist(our_goal))  return false;


    const double min_y = ( position_type == Position_Right
                           ? 0
                           : -34.0 );
    const double max_y = ( position_type == Position_Left
                           ?  0
                           : +34.0);
    
    
    
    
    Rect2D free_attack(Vector2D(fastest_opp->pos().x, (fastest_opp->pos().absY()+1.0)*sign(fastest_opp->pos().y)),
		       Vector2D(wm.ourDefenseLineX(), 34.0*sign(fastest_opp->pos().y)));
    
    
    
    
    if (wm.existOpponentIn(free_attack,5,false)&&fastest_opp->pos().absY()<20.0)
    {
      
        return false;
    }
    
    
    
    
    

    if (mark_target==fastest_opp||self_min<=mate_min)
    {

        double  max_x=0;


        Rect2D bounding_rect( Vector2D( -51.0, min_y ),
                              Vector2D( max_x, max_y ) ); // Vector2D( -15.0, max_y ) );
        if ( Bhv_GetBall( bounding_rect ).execute( agent ) )
        {
            return true;
        }

    }
    



    return false;
}

bool Bhv_SideBack_Move::Block_BA_CB(rcsc::PlayerAgent *agent)
{
    const WorldModel & wm = agent->world();

    const Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );


    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    Vector2D homePos = target_point;

    int num = wm.self().unum();


    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    if (wm.existKickableTeammate() || mate_min<opp_min||self_min<opp_min)
    {

        return false;
    }
    const   rcsc::AbstractPlayerObject * fastest_opp=wm.interceptTable()->fastestOpponent();
    const PlayerObject *nearestOpp=wm.getOpponentNearestToSelf(5,true);

   const  AbstractPlayerObject * mark_target = mark_find_best_target(agent).get_ba_dg_best_mark_opp();



    const Vector2D  our_goal(-52.5,0);


    if (
            mark_target&&mark_target!=fastest_opp&&
            mark_target->pos().dist(our_goal)< fastest_opp->pos().dist(our_goal)){

        return false;
    }




    bool  same_side= (wm.self().unum()==5&&wm.ball().pos().y>0)
            ||(wm.self().unum()==4&&wm.ball().pos().y<0);


    if (mark_target==fastest_opp||self_min<=mate_min)
    {

        return   Bhv_GetBall().DefenderGetBall(agent);
    }

    if (same_side)
    {

        return   Bhv_GetBall().DefenderGetBall(agent);
    }


    return false;
}


bool Bhv_SideBack_Move::isGolieHelperSituation(PlayerAgent *agent)
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    if (Strategy::get_ball_area(wm)!=Strategy::BA_Danger)
    {
        return  false;
    }

    if (Bhv_GoalieChaseBall().is_ball_chase_situation(agent))
    {
        return true;
    }

    return false;
}

