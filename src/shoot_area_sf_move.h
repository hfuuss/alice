

#ifndef DY_SHOOT_AREA_SF_MOVE_H
#define DY_SHOOT_AREA_SF_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
using namespace rcsc;


class shoot_area_sf_move
{
public:
    shoot_area_sf_move()
    {}
    bool execute(rcsc::PlayerAgent *agent);
    bool throughPassShootHelper(rcsc::PlayerAgent *agent);
    bool rec_pass_shoot_helper(rcsc::PlayerAgent *agent);//2016年 05月 03日 星期二 17:32:28 CST
    double get_dash_power(rcsc::PlayerAgent *agent);
    Vector2D get_target(rcsc::PlayerAgent *agent,rcsc::Vector2D home_pos);

};


#endif 
