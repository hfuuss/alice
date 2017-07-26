


#include "rec_pass_shoot_move.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/view_wide.h>


#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

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
#include "shoot_area_cf_move.h"
#include "neck_offensive_intercept_neck.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"
#include"bhv_basic_move.h"
#include <fstream>
#include "Evaluation.h"


bool dy_rec_pass_shoot_move::execute(rcsc::PlayerAgent *agent)
{
    //    const WorldModel &wm = agent->world();
    //    const int self_min=wm.interceptTable()->selfReachCycle();
    //    const int opp_min=wm.interceptTable()->opponentReachCycle();
    //    const int mate_min=wm.interceptTable()->teammateReachCycle();

    //    if (!(wm.ball().pos().x>35&&wm.self().unum()>6) )
    //    {
    //        return false;

    //    }

    //    if (opp_min+2<mate_min)
    //    {
    //        return false;
    //    }


    //   Vector2D target=rcsc::Vector2D::INVALIDATED;

    //   if (search_route(agent,target))
    //   {

    //       double dash_power=ServerParam::i().maxDashPower();

    //       double dist_th5=0.5;

    //       if (!Body_GoToPoint(target,dist_th5,dash_power).execute(agent))
    //       {
    //           Body_TurnToBall().execute(agent);
    //       }

    //       if (wm.ball().distFromSelf()>18)
    //           agent->setNeckAction(new Neck_TurnToBallOrScan());
    //       else
    //           agent->setNeckAction(new Neck_TurnToBall());

    //     std::cout<<"FINED"<<"\n";

    //       return true;


    //   }















    return false;


}



bool dy_rec_pass_shoot_move::search_route(rcsc::PlayerAgent *agent,rcsc::Vector2D &target)
{

    const WorldModel &wm = agent->world();

    double x_search_thr=wm.offsideLineX()-wm.self().pos().x;

    double y_search_thr=4.5;
    Vector2D homePos=Strategy::i().getPosition(wm.self().unum());

    double homePos_dist_self_pos=wm.self().pos().dist(homePos);


    //while homePos dist is small

    std::vector<Vector2D>shoot_Points;
    std::vector<Vector2D>Can_rec_Points;

    if (homePos_dist_self_pos<7.0)

    {

        for (double x=-5.0;x<x_search_thr;x+=1.0)
        {
            for(double y=-5.0;y<5.0;y+=1.0)
            {
                Vector2D target(wm.self().pos().x+x,wm.self().pos().y+y);

                if (FieldAnalyzer::can_shoot_from(false,target,wm.theirPlayers(),8))
                {
                    shoot_Points.push_back(target);
                }

            }


        }




        for (std::vector<Vector2D>::const_iterator it = shoot_Points.begin();it!=shoot_Points.end();it++)
        {

            if (!can_rec_ball(agent,(*it)) ) continue;

            Can_rec_Points.push_back((*it));
        }







        //evaluate

        double best_score=-1000;
        Vector2D best_pos=rcsc::Vector2D::INVALIDATED;


        for (std::vector<Vector2D>::const_iterator it =Can_rec_Points.begin();it!=Can_rec_Points.end();it++)
        {

            double score=evaluation_point(agent,(*it));

            if (score>best_score)
            {
                best_score=score;
                best_pos=(*it);
            }



        }


        if (best_pos.isValid())
        {
            target=best_pos;

            return true;
        }
        else
        {

            return false;
        }



    }










    return false;

}




bool dy_rec_pass_shoot_move::can_rec_ball(rcsc::PlayerAgent *agent, Vector2D target)
{


    const WorldModel &wm =agent->world();
    Vector2D  ball =wm.ball().pos();
    Segment2D pass_line=Segment2D(ball,target);

    bool can_rec=true;


    //search opp

    for (rcsc::AbstractPlayerCont::const_iterator it =wm.theirPlayers().begin();it!=wm.theirPlayers().end();it++)\
    {
        if ( (*it)==NULL)  continue;

        if ( (*it)->posCount()>5) continue;

        if ( pass_line.contains((*it)->pos()) )
        {
            can_rec=false;
            break;
        }


    }


    return can_rec;



}


double dy_rec_pass_shoot_move::evaluation_point(PlayerAgent *agent, Vector2D target)
{

    const WorldModel &wm = agent->world();
    Vector2D self=wm.self().pos();
    Vector2D home =Strategy::i().getPosition(wm.self().unum());

    double d1=target.dist(self);
    double d2=target.dist(home);

    double d3=0;

    wm.getOpponentNearestTo(target,3,&d3);

    double w1=0.8;

    double w2=0.4;




    double point=10-(w1*d1+w2*d2);
    point+=Evaluation::instance().EvaluatePosition(target,true)*10;

    point+=bound(0.0,d3,4.0)*2.5;




    return point;


}






