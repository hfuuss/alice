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

#include "voron_point.h"
#include <rcsc/player/world_model.h>
#include<rcsc/common/server_param.h>

using namespace  rcsc;


const   rcsc::VoronoiDiagram  &voron_point::passVoronoiDiagram(PlayerAgent *agent)
{

    const rcsc::Rect2D PITCH_RECT( rcsc::Vector2D( - rcsc::ServerParam::i().pitchHalfLength(),
                                                   - rcsc::ServerParam::i().pitchHalfWidth() ),
                                   rcsc::Size2D( rcsc::ServerParam::i().pitchLength(),
                                                 rcsc::ServerParam::i().pitchWidth() ) );


    M_pass_voronoi_diagram.clear();
    const WorldModel &wm = agent->world();

    const SideID our = wm.ourSide();

    const AbstractPlayerCont::const_iterator end = wm.allPlayers().end();
    for ( AbstractPlayerCont::const_iterator p = wm.allPlayers().begin();
          p != end;
          ++p )
    {


        if ( (*p)->side() == our )
        {

        }
        else
        {
            M_pass_voronoi_diagram.addPoint( (*p)->pos() );
        }
    }


    M_pass_voronoi_diagram.setBoundingRect( PITCH_RECT );
    M_pass_voronoi_diagram.compute();


    return M_pass_voronoi_diagram;

}
const   rcsc::VoronoiDiagram  &voron_point::passVoronoiDiagram(const rcsc::WorldModel &wm)
{


    const rcsc::Rect2D PITCH_RECT( rcsc::Vector2D( - rcsc::ServerParam::i().pitchHalfLength(),
                                                   - rcsc::ServerParam::i().pitchHalfWidth() ),
                                   rcsc::Size2D( rcsc::ServerParam::i().pitchLength(),
                                                 rcsc::ServerParam::i().pitchWidth() ) );


    M_pass_voronoi_diagram.clear();

    const SideID our = wm.ourSide();

    const AbstractPlayerCont::const_iterator end = wm.allPlayers().end();
    for ( AbstractPlayerCont::const_iterator p = wm.allPlayers().begin();
          p != end;
          ++p )
    {


        if ( (*p)->side() == our )
        {

        }
        else
        {
            M_pass_voronoi_diagram.addPoint( (*p)->pos() );
        }
    }


    M_pass_voronoi_diagram.setBoundingRect( PITCH_RECT );
    M_pass_voronoi_diagram.compute();


    return M_pass_voronoi_diagram;

}
