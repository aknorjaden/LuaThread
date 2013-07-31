// LuaThread.cpp : Defines the entry point for the console application.
//

#include "LuaThread.h"
#include "stdafx.h"
#include <iostream>


int main(int argc, char * argv[])
{
    std::cout << std::endl << "Creating LuaThread object..." << std::endl;

    LuaThread myLuaThread("THREAD [NPC] [1-GBBP] [Belt VIII-II] [Guristas Fleet]", "./evemu/scripts", "./evemu/log/threads", false);

    // TODO:
    // 1) Make a call to myLuaThread to execute the test lua script
    myLuaThread.ExecuteScript("test.lua");
    
    // 2) Make a call to myLuaThread's GetXXXX() functions that will get variables created and filled with data inside the script
    double width = 0.0, height = 0.0;

	width = myLuaThread.GetDouble("width");
	height = myLuaThread.GetDouble("height");

	std::cout << std::endl << "width = " << width << ", height = " << height << std::endl;

    std::cout << std::endl << "press the ENTER key to exit...";
    std::cin.get();

	return 0;
}
