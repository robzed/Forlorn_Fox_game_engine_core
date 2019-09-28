//
//  MouseTarget.h
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

#ifndef __MouseTarget__
#define __MouseTarget__

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "LuaBridge.h"

// ---------------------------------------------------------------------------
class MyPoint
{
public:
    MyPoint(double x1, double y1) : x(x1), y(y1) {}
    MyPoint() :x(0), y(0) {}
    // http://courses.cms.caltech.edu/cs11/material/cpp/donnie/cpp-ops.html
    // http://stackoverflow.com/questions/4421706/operator-overloading
    MyPoint& operator=(const MyPoint&rhs)
    {
        // this will work even for self assignment
        x=rhs.x;
        y=rhs.y;
        return *this;
    }
    MyPoint operator-(const MyPoint& p)
    {
        MyPoint result(*this);
        // this will work even for self
        result.x -= p.x;
        result.y -= p.y;
        return result;
    }
    double x;
    double y;
};

// ---------------------------------------------------------------------------

class MouseTargetBaseType;

#include "GameApplication.h"

class MouseTargetBaseType
{
public:
    MouseTargetBaseType(GameApplication* application,
                        luabridge::LuaRef function_in,
                        luabridge::LuaRef object_arg1_in,
                        luabridge::LuaRef arg_last_in);
    virtual ~MouseTargetBaseType();
    virtual void run(double x, double y, bool button_down, bool drag);
    virtual bool inside(double x, double y) = 0;
    virtual void care_about_drag_button_down();
    virtual void care_about_drag_button_up();
    virtual int get_shape(lua_State *L) = 0;
private:
    luabridge::LuaRef function;
    luabridge::LuaRef object_arg1;
    luabridge::LuaRef arg_last;
    bool report_drag_down;
    bool report_drag_up;
    GameApplication* app;
};


class MouseTargetRectangle : public MouseTargetBaseType
{
public:
    MouseTargetRectangle(GameApplication* application,
                         double x1, double y1, double x2, double y2,
                         luabridge::LuaRef function,
                         luabridge::LuaRef object_arg1,
                         luabridge::LuaRef arg_last);
    void define(double x1, double y1, double x2, double y2);
    bool inside(double x, double y);
    virtual int get_shape(lua_State *L);
private:
    double _x1, _y1;
    double _x2, _y2;
};


class MouseTargetTriangle : public MouseTargetBaseType
{
public:
    MouseTargetTriangle(GameApplication* application,
                        luabridge::LuaRef function,
                        luabridge::LuaRef object_arg1,
                        luabridge::LuaRef arg_last);
    void define(double x1, double y1, double x2, double y2, double x3, double y3);
    bool inside(double x, double y);
    virtual int get_shape(lua_State *L);
private:
    MyPoint A;
    MyPoint B;
    MyPoint C;
    MyPoint v0;
    MyPoint v1;
    double dot00;
    double dot01;
    double dot11;
    double invDenom;
};


class MouseTargetCircle : public MouseTargetBaseType
{
public:
    MouseTargetCircle(GameApplication* application,
                      double x, double y, double radius,
                      luabridge::LuaRef function,
                      luabridge::LuaRef object_arg1,
                      luabridge::LuaRef arg_last);
    void define(double x, double y, double radius);
    bool inside(double x, double y);
    virtual int get_shape(lua_State *L);
private:
    double _x, _y;
    double _radius_squared;
    double _radius; // stored just for get_type
};


#endif /* defined(__MouseTarget__) */
