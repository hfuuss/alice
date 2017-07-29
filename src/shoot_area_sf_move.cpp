
#include "shoot_area_sf_move.h"
#include "field_analyzer.h"
#include <rcsc/geom/voronoi_diagram.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include<rcsc/action/body_intercept.h>
#include<rcsc/action/body_go_to_point.h>
#include<rcsc/action/body_turn_to_angle.h>
#include<rcsc/action/neck_turn_to_ball_or_scan.h>
#include<rcsc/action/neck_turn_to_ball.h>
#include<rcsc/action/neck_turn_to_goalie_or_scan.h>
#include<rcsc/action/body_turn_to_ball.h>
#include <boost/iterator/iterator_concepts.hpp>
#include "neck_offensive_intercept_neck.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"
#include"bhv_basic_move.h"
#include "voron_point.h"
#include "shoot_area_cf_move.h"
#include <rcsc/common/server_param.h>
#include <rcsc/action/arm_point_to_point.h>
using namespace  rcsc;
static int  VALID_PLAYER_THRESHOLD=8;

bool shoot_area_sf_move::execute(PlayerAgent* agent)
{

    const WorldModel &wm =agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    if ( Bhv_BasicTackle().execute( agent ) )
    {
        return true;
    }

    if (wm.interceptTable()->fastestTeammate() && wm.interceptTable()->fastestTeammate()->unum()==11
            &&wm.ball().pos().absY()>5.0)
    {
        return  shoot_area_cf_move().execute(agent);
    }

    //press  opp ball owner


    if (!wm.existKickableTeammate()&&self_min<=mate_min)
    {
        Body_Intercept().execute(agent);
        agent->setNeckAction(new Neck_OffensiveInterceptNeck());
        return true;
    }


    if (throughPassShootHelper(agent))
    {
        return true;
    }



    if (opp_min+3<mate_min)
    {
        Bhv_BasicMove().execute(agent);
        return true;
    }


    Vector2D home_pos=Strategy::i().getPosition(wm.self().unum());
    Vector2D target_point =get_target(agent,home_pos);
    double dash_power = rcsc::ServerParam::i().maxPower();
    double dist_thr = wm.ball().distFromSelf()*0.1;
    
    if (dist_thr<1) dist_thr=0.5;


    if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power,
                                 -1.0, // speed
                                 1, // perferd cycle
                                 true, // save recovery
                                 20.0 // dir thr
                                 ).execute( agent ) )

    {

        rcsc::Body_TurnToBall().execute(agent);
    }

   



    agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
    agent->setArmAction( new Arm_PointToPoint( target_point ) );


    return true;


}





double shoot_area_sf_move::get_dash_power(PlayerAgent* agent)
{
    return ServerParam::i().maxDashPower();
}

Vector2D shoot_area_sf_move::get_target(PlayerAgent* agent,rcsc::Vector2D home_pos)
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D target_point = home_pos;

    int mate_min = wm.interceptTable()->teammateReachCycle();
    rcsc::Vector2D ball_pos = ( mate_min < 10
                                ? wm.ball().inertiaPoint( mate_min )
                                : wm.ball().pos() );
    bool opposite_side = false;
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );

    if ( ( ball_pos.y > 0.0
           && position_type == Position_Left )
         || ( ball_pos.y <= 0.0
              && position_type == Position_Right )
         || ( ball_pos.y * home_pos.y < 0.0
              && position_type == Position_Center )
         )
    {
        opposite_side = true;
    }

    if ( opposite_side
         && ball_pos.x > 40.0 // very chance
         && 6.0 < ball_pos.absY()
         && ball_pos.absY() < 15.0 )
    {
        rcsc::Circle2D goal_area( rcsc::Vector2D( 47.0, 0.0 ),
                                  4.0 );//7.0 to big

        if ( ! wm.existTeammateIn( goal_area, 10, true ) )
        {

            target_point.x = 47.0;

            if ( wm.self().pos().x > wm.offsideLineX() - 0.5
                 && target_point.x > wm.offsideLineX() - 0.5 )
            {
                target_point.x = wm.offsideLineX() - 0.5;
            }

            if ( ball_pos.absY() < 9.0 )
            {
                target_point.y = 0.0;
            }
            else
            {
                target_point.y = ( ball_pos.y > 0.0
                                   ? 1.0 : -1.0 );
            }


            const VoronoiDiagram &M_Pass_Voron =voron_point().i().instance().passVoronoiDiagram(agent);
            VoronoiDiagram::Vector2DCont Voron=M_Pass_Voron.resultPoints();
            double  min_dis=1000;
            Vector2D best_pos=target_point;
            for (VoronoiDiagram::Vector2DCont::const_iterator it =Voron.begin();it!=Voron.end();it++)
            {


                if (! (*it).isValid())
                {
                    continue;
                }

                //2016年 04月 06日 星期三 10:01:12 CST

                if ((*it).x>wm.offsideLineX())
                {
                    continue;
                }



                if( (*it).absX()>52||(*it).absY()>34)
                {
                    continue;
                }

                if ( !goal_area.contains((*it)))
                {
                    continue;
                }

                double dist=(*it).dist(target_point);

                if (dist<min_dis)
                {
                    min_dis=dist;
                    best_pos=(*it);
                }


            }
            
            target_point=best_pos;
            //	   if (target_point.x>wm.offsideLineX())
            //	  {
            //		  target_point.x=wm.offsideLineX()-0.5;
            //	  }
            return target_point;

        }

    }

    // consider near opponent
    const VoronoiDiagram &M_Pass_Voron =voron_point::instance().passVoronoiDiagram(agent);
    VoronoiDiagram::Vector2DCont Voron=M_Pass_Voron.resultPoints();
    double  min_dis=1000;
    Vector2D best_pos=target_point;
    for (VoronoiDiagram::Vector2DCont::const_iterator it =Voron.begin();it!=Voron.end();it++)
    {


        if (! (*it).isValid())
        {
            continue;
        }



        if ( (*it).absX()>52||(*it).absY()>34)
        {
            continue;
        }

        // 2016年 04月 06日 星期三 10:01:12 CST  Deng Yong

        if ( (*it).x>wm.offsideLineX())
        {
            continue;
        }

        //		  //! field anlyzer has a bug
        //	      if (!FieldAnalyzer::can_shoot_from
        //               ( false,
        //               (*it),
        //               wm.getPlayerCont( new OpponentOrUnknownPlayerPredicate( wm.ourSide() ) ),
        //               VALID_PLAYER_THRESHOLD ) )
        //	      {
        //	        continue;
        //	      }


        double dist=(*it).dist(target_point);

        if (dist<min_dis)
        {
            min_dis=dist;
            best_pos=(*it);
        }


    }

    target_point=best_pos;
    //      if (target_point.x>wm.offsideLineX())
    //	  {
    //		  target_point.x=wm.offsideLineX()-0.5;
    //	  }

    return target_point;
}


bool shoot_area_sf_move::throughPassShootHelper(PlayerAgent *agent)
{

    const WorldModel &wm =agent->world();
    const int num =wm.self().unum();
    bool through_pass=false;
    Vector2D home_pos=Strategy::i().getPosition(wm.self().unum());
    Vector2D target_point=home_pos;
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min =wm.interceptTable()->opponentReachCycle();
    const int self_min=wm.interceptTable()->selfReachCycle();
   



    bool   fast_dribble_team=mate_min<opp_min&&wm.ball().vel().x>0.99&&wm.ball().pos().x>wm.self().pos().x;



    if (!fast_dribble_team)  return false;



        double buff=wm.offsideLineX()-wm.self().pos().x;

        target_point.x=std::max(wm.offsideLineX()+buff,wm.ball().inertiaPoint(mate_min).x+buff);

        target_point.y=fabs(target_point.y-wm.self().pos().y)<6.0? wm.self().pos().y:target_point.y;

        double opp_dist_target=1000;
        const PlayerObject *opp_marker= wm.getOpponentNearestTo(target_point,5,&opp_dist_target);

        if (wm.self().pos().x>home_pos.x)
        {
            return false;
        }



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


bool shoot_area_sf_move::rec_pass_shoot_helper(rcsc::PlayerAgent *agent)
{
    const WorldModel &wm = agent->world();
    if (  wm.ourPlayer(11) &&wm.ourPlayer(11)->pos().absY()<4.0)
    {
        return false;
    }

    if ( wm.interceptTable()->fastestTeammate()->unum()==11)
    {






    }




















}






