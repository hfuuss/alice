// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

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

#ifndef BRAIN_H
#define BRAIN_H

#include "field_evaluator.h"
#include "predict_state.h"

#include <vector>

namespace rcsc {
class AbstractPlayerObject;
class Vector2D;
}

class ActionStatePair;

class Brain
    : public FieldEvaluator {
private:

public:
    Brain()=default;
    virtual
    ~Brain()
    {}

    virtual
    double operator()( const PredictState & state,
                       const std::vector< ActionStatePair > & path );
    static const  Brain  & Instance();
    bool   read(std::string  &data_dir);
private:
    double evaluate_state( const PredictState & state );

    bool  is_bed_pass( const ActionStatePair  path );

    double decesion_tree( const ActionStatePair  path );
    double less36_decesion_tree( const ActionStatePair  path );
    double over36_decesion_tree( const ActionStatePair  path );
//    double evaluate_dribble( const PredictState & state,const ActionStatePair path );
//    double evaluate_pass( const PredictState & state,const ActionStatePair path );
//    double evaluate_hold( const PredictState & state,const ActionStatePair path );
//    double evaluate_shoot( const PredictState & state,const ActionStatePair path );


};

#endif
