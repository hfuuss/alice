
#ifndef DY_OFFENSIVE_HALF_SHOOT_AREA_MOVE_H
#define DY_OFFENSIVE_HALF_SHOOT_AREA_MOVE_H

#include <rcsc/player/player_agent.h>
#include<rcsc/geom/vector_2d.h>
class  offsnive_half_shoot_area_move
{
public:
     offsnive_half_shoot_area_move()
    {}

    bool execute(rcsc::PlayerAgent *agent);
    double get_dash_power(rcsc::PlayerAgent *agent);
    rcsc::Vector2D get_target(rcsc::PlayerAgent *agent,rcsc::Vector2D home_pos);


};

#endif // MIRACLE_OFFENSIVE_HALF_SHOOT_AREA_MOVE_H
