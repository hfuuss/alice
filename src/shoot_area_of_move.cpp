

#include "shoot_area_of_move.h"
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
#include<rcsc/action/neck_turn_to_low_conf_teammate.h>
#include "neck_offensive_intercept_neck.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"
#include"bhv_basic_move.h"
#include "voron_point.h"
#include <rcsc/common/server_param.h>
using namespace rcsc;

bool offsnive_half_shoot_area_move::execute(rcsc::PlayerAgent *agent)
{

    const WorldModel &wm =agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    if ( Bhv_BasicTackle().execute( agent ) )
    {
        return true;
    }


    //press  opp ball owner



    if (!wm.existKickableTeammate()&&self_min<=mate_min)
    {
        Body_Intercept().execute(agent);
        agent->setNeckAction(new Neck_OffensiveInterceptNeck());
        return true;
    }



    if(opp_min+3<mate_min)
    {
        Bhv_BasicMove().execute(agent);
        return true;
    }


    double dash_power = get_dash_power(agent);

    Vector2D home_pos=Strategy::i().getPosition(wm.self().unum());
    Vector2D target_point =get_target(agent,home_pos);
    double dist_thr=0.5;



    if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power,
                                 -1.0, // speed
                                 1, // perferd cycle
                                 true, // save recovery
                                 20.0 // dir thr
                                 ).execute( agent ) )

    {

        rcsc::Body_TurnToBall().execute(agent);
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




Vector2D  offsnive_half_shoot_area_move::get_target(PlayerAgent *agent, Vector2D home_pos)
{

    const WorldModel &wm = agent->world();

    rcsc::Vector2D target_point = home_pos;
    const VoronoiDiagram &M_Pass_Voron =voron_point::instance().passVoronoiDiagram(agent);
    VoronoiDiagram::Vector2DCont Voron=M_Pass_Voron.resultPoints();
    double best_point=-1000;
    Vector2D best_pos=target_point;
    for (VoronoiDiagram::Vector2DCont::const_iterator it =Voron.begin();it!=Voron.end();it++)
    {


        if (! (*it).isValid())
        {
            continue;
        }


        if ( (*it).absX()>52||(*it).absY()>25.0)
        {
            continue;
        }


        if (  (*it).x<30)  continue;

        //    continue;


        //same side

        //if ( (*it).y*wm.ball().inertiaPoint(mate_min).y<0)
        //{
        // continue;
        //}

        //if (!FieldAnalyzer::can_shoot_from
        //    ( false,
        //    (*it),
        //    wm.getPlayerCont( new OpponentOrUnknownPlayerPredicate( wm.ourSide() ) ),
        //    VALID_PLAYER_THRESHOLD ) )
        //{
        // continue;
        //}

        double my_point=10-(*it).dist(target_point);

        if (my_point>best_point)
        {
            best_point=my_point;
            best_pos=(*it);
        }



    }



    target_point=best_pos;





    agent->doPointto(best_pos.x,best_pos.y);

    return target_point;



}


double offsnive_half_shoot_area_move::get_dash_power(PlayerAgent *agent)

{
    const WorldModel &wm = agent->world();
    
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    // set dash power
    static bool S_recover_mode = false;
    double dash_power=ServerParam::i().maxDashPower();
    return dash_power;
}

