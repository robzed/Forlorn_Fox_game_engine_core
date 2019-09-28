//
//  MouseTarget.cpp
//  Forlorn Fox
//
//  Created by Rob Probin on 15/03/2015.
/*
 * ------------------------------------------------------------------------------
 * Copyright (c) 2015 Rob Probin and Tony Park
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 * -----------------------------------------------------------------------------
 * (This is the zlib License)
 *
*/

#include "MouseTarget.h"
#include "Utilities.h"


double dot(MyPoint&p1, MyPoint&p2)
{
    return (p1.x * p2.x) + (p1.y * p2.y);
}


MouseTargetBaseType::MouseTargetBaseType(GameApplication* application,
                                         luabridge::LuaRef function_in,
                                         luabridge::LuaRef object_arg1_in,
                                         luabridge::LuaRef arg_last_in)
//: type(0)
: function(function_in)
, object_arg1(object_arg1_in)
, arg_last(arg_last_in)
, report_drag_down(false)
, report_drag_up(false)
, app(application)
{
    function.force_to_main_state();
}

MouseTargetBaseType::~MouseTargetBaseType()
{
    // make sure it's removed if it get's destroyed
    app->remove_mouse_target(this);
}

void MouseTargetBaseType::run(double x, double y, bool button_down, bool drag)
{
    if(drag)
    {
        if(button_down and not report_drag_down)
        {
            return;
        }
        if(not button_down and not report_drag_up)
        {
            return;
        }
    }
    try {
        function(object_arg1, x, y, button_down, drag, arg_last);
    }
    catch (luabridge::LuaException const& e) {
        Utilities::fatalError(std::string("MouseTargetBaseType::run failed with ") + e.what ());
    }
}

void MouseTargetBaseType::care_about_drag_button_down()
{
    report_drag_down = true;
}

void MouseTargetBaseType::care_about_drag_button_up()
{
    report_drag_up = true;
}


#include<iostream>
void check_LuaRef(luabridge::LuaRef f, std::string where)
{
    lua_State* state = f.state();
    
    lua_rawgeti(state,  LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State *mL = lua_tothread(state,-1);
    lua_pop(state, 1);
    
    if(mL != state)
    {
        std::cout << "Tried to register function not on the main LuaState Lua-thread" << std::endl;
        std::cout << where << std::endl;
        lua_Debug ar;

        int level = 0;
        //
        while(true)
        {
            int greater = lua_getstack(state, level, &ar);
            if(greater) break;
            lua_getinfo(state, "nSl", &ar);
            int line = ar.currentline;
            std::cout << line << ar.short_src << std::endl;
            level ++;
        }
        Utilities::fatalError(std::string("Register function on non-main LuaState"));
    }
}

MouseTargetRectangle::MouseTargetRectangle(GameApplication* application,
                                           double x1, double y1, double x2, double y2,
                                           luabridge::LuaRef function,
                                           luabridge::LuaRef object_arg1,
                                           luabridge::LuaRef arg_last)
: MouseTargetBaseType(application, function, object_arg1, arg_last)
//, _x1(x1), _y1(y1)
//, _x2(x2), _y2(y2)
{
    define(x1, y1, x2, y2);
    
    //check_LuaRef(function, "function in MouseTargetRectangle::MouseTargetRectangle");
    //check_LuaRef(object_arg1, "object_arg1 in MouseTargetRectangle::MouseTargetRectangle");
    //check_LuaRef(arg_last, "arg_last in MouseTargetRectangle::MouseTargetRectangle");

}

void MouseTargetRectangle::define(double x1, double y1, double x2, double y2)
{
    _x1 = x1; _y1 = y1;
    _x2 = x2; _y2 = y2;
    // make sure _x1 and _y1 is the smallest value
    if(_x1 > _x2)
    {
        double temp = _x1;
        _x1 = _x2;
        _x2 = temp;
    }
    if(_y1 > _y2)
    {
        double temp = _y1;
        _y1 = _y2;
        _y2 = temp;
    }
}

bool MouseTargetRectangle::inside(double x, double y)
{
    if( x < _x1 or x > _x2)
    {
        return false;
    }
    // must be within
    if( y < _y1 or y > _y2)
    {
        return false;
    }
    return true;
}

int MouseTargetRectangle::get_shape(lua_State *L)
{
    lua_pushstring(L, "Rectangle");
    lua_pushnumber(L, _x1);
    lua_pushnumber(L, _y1);
    lua_pushnumber(L, _x2);
    lua_pushnumber(L, _y2);
    return 5;
}

// ---------------------------------------------------------------------------


MouseTargetTriangle::MouseTargetTriangle(GameApplication* application,
                                         luabridge::LuaRef function,
                                         luabridge::LuaRef object_arg1,
                                         luabridge::LuaRef arg_last)
: MouseTargetBaseType(application, function, object_arg1, arg_last)
//, _x1(-1), _y1(-1)
//, _x2(-1), _y2(-1)
//, _x3(-1), _y3(-1)
, A(0, 0)
, B(0, 0)
, C(0, 0)
// we don't need to init these, but we do anyway
, dot00(0)
, dot01(0)
, dot11(0)
, invDenom(0)
{
}

void MouseTargetTriangle::define(double x1, double y1, double x2, double y2, double x3, double y3)
{
    A = MyPoint(x1, y1);
    B = MyPoint(x2, y2);
    C = MyPoint(x3, y3);
    //_x1 = x1; _y1 = y1;
    //_x2 = x2; _y2 = y2;
    //_x3 = x3; _y3 = y3;
    
    // precalculate everything we can
    v0 = C - A;
    v1 = B - A;
    dot00 = dot(v0, v0);
    dot01 = dot(v0, v1);
    dot11 = dot(v1, v1);
    invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
}

bool MouseTargetTriangle::inside(double x, double y)
{
    // http://stackoverflow.com/questions/2049582/how-to-determine-a-point-in-a-triangle
    // http://www.blackpawn.com/texts/pointinpoly/default.html
    // http://mathforum.org/library/drmath/view/54505.html
    // http://www.gamedev.net/topic/295943-is-this-a-better-point-in-triangle-test-2d/
    // http://faraday.physics.utoronto.ca/PVB/Harrison/Flash/Vectors/DotProduct/DotProduct.html
    // http://www.mathsisfun.com/algebra/vectors-dot-product.html
    
    MyPoint P(x, y);
    
    // Compute vectors
    MyPoint v2 = P - A;
    
    // Compute dot products
    double dot02 = dot(v0, v2);
    double dot12 = dot(v1, v2);
    
    // Compute barycentric coordinates
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v < 1);
}

int MouseTargetTriangle::get_shape(lua_State *L)
{
    lua_pushstring(L, "Triangle");
    lua_pushnumber(L, A.x);
    lua_pushnumber(L, A.y);
    lua_pushnumber(L, B.x);
    lua_pushnumber(L, B.y);
    lua_pushnumber(L, C.x);
    lua_pushnumber(L, C.y);
    return 7;
}


// ---------------------------------------------------------------------------

MouseTargetCircle::MouseTargetCircle(GameApplication* application,
                                     double x, double y, double radius,
                                     luabridge::LuaRef function,
                                     luabridge::LuaRef object_arg1,
                                     luabridge::LuaRef arg_last)
: MouseTargetBaseType(application, function, object_arg1, arg_last)
, _x(x), _y(y)
, _radius_squared(radius*radius)
, _radius(radius)
{
}

void MouseTargetCircle::define(double x, double y, double radius)
{
    _x = x; _y = y;
    _radius_squared = radius*radius;
}

bool MouseTargetCircle::inside(double x, double y)
{
    // offset
    x -= _x;
    y -= _y;
    
    double sum_squares = (x * x) + (y * y);
    return sum_squares <= _radius_squared;
}

int MouseTargetCircle::get_shape(lua_State *L)
{
    lua_pushstring(L, "Circle");
    lua_pushnumber(L, _x);
    lua_pushnumber(L, _y);
    lua_pushnumber(L, _radius);
    return 4;
}

