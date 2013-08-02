// LuaThread.cpp : Defines the entry point for the console application.
//

#include "LuaThread.h"
#include "stdafx.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

int main(int argc, char * argv[])
{
    std::cout << std::endl << "Creating LuaThread object..." << std::endl;

    LuaThread myLuaThread_A("THREAD [NPC] [1-GBBP] [Belt VIII-II] [Guristas Fleet]", "./evemu/scripts", "./evemu/log/threads", true, true);
    LuaThread myLuaThread_B("THREAD [NPC] [Lonetrek] [Belt V-II] [Sansha Fleet]", "./evemu/scripts", "./evemu/log/threads", true, true);

    // TODO:
    // 1) Make a call to myLuaThread to execute the test lua script
    myLuaThread_A.ExecuteScript("test.lua");
    myLuaThread_B.ExecuteScript("testB.lua");
    
	myLuaThread_A.ResumeScript();
	myLuaThread_B.ResumeScript();

	Sleep(10000);

    // 2) Make a call to myLuaThread's GetXXXX() functions that will get variables created and filled with data inside the script
    double width = 0.0, height = 0.0;

	while( !myLuaThread_A.HasScriptExecutedOnce() );
	width = myLuaThread_A.GetDouble("width");
	height = myLuaThread_A.GetDouble("height");

	std::cout << std::endl << "myLuaThread_A | width = " << width << ", height = " << height << std::endl;

	while( !myLuaThread_B.HasScriptExecutedOnce() );
	width = myLuaThread_B.GetDouble("width");
	height = myLuaThread_B.GetDouble("height");

	std::cout << std::endl << "myLuaThread_B | width = " << width << ", height = " << height << std::endl;

    std::cout << std::endl << "press the ENTER key to exit...";
    std::cin.get();

	myLuaThread_A.KillScript();
	myLuaThread_B.KillScript();

	return 0;
}
