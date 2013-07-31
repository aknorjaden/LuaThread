
#include "StdAfx.h"
#include "EVEmu_Types.h"

#include "..\common\boost\boost\thread\thread.hpp"
#include "..\common\boost\boost\thread\mutex.hpp"
#include "..\common\boost\boost\thread\locks.hpp"
#include "..\common\boost\boost\type_traits.hpp"

#pragma once
#include "LuaEnvironment.h"


#ifndef LUATHREAD_H
#define LUATHREAD_H


///////////////////////////////////////////////////////////////////////////////////////////////////
// USE Cases:
//
// ???
// ----------------------------------------------
// 1) 
// 2) 
//
//
// ???
// ------------------------------------------
// 1) 
// 2)
//
///////////////////////////////////////////////////////////////////////////////////////////////////


class LuaEnvironment;

// This class is an owner of a single Lua interpreter instance.
// It either creates one directly in the same process/thread as
// this class instance, or when 'useThreading' is set to 'true',
// it creates a new boost::thread which contains the lua interpreter
// instance.

class LuaThread
{
    public:
        LuaThread(std::string threadName, std::string scriptPath, std::string logFilePath, bool useThreading = false);
        ~LuaThread();

        LuaEnvironment * GetLuaEnv();

        // Script Management:
        int32 ExecuteScript(std::string scriptName);

        // Script Management - Threading Enabled Use Only!
        int32 KillScript();		// Only used for threaded scripts
        int32 PingScript();		// Only used for threaded scripts

        // Accessing Lua Internals:
		bool DoesLuaVariableExist(std::string varName);
        std::string GetString(std::string varName);
        double GetDouble(std::string varName);
        bool GetBool(std::string varName);
        int32 SetString(std::string varName, std::string strVal);
        int32 SetDouble(std::string varName, double doubleVal);
        int32 SetBool(std::string varName, bool boolVal);

        // Thread-initiated calls to us:
        // (DO NOT USE THESE FROM ANY CLASS OR FUNCTION OTHER THAN LuaEnvironment)
        int32 Script_ExecutionComplete(uint32 accessCode = 0);
        int32 Script_LogMessage(std::string logMessage, uint32 accessCode = 0);
        int32 Script_SetOwnedLuaEnvironment(LuaEnvironment * luaenv, uint32 accessCode = 0);

    protected:
        void _LogMessage(std::string logMessage);

        // Mutex-protected Flag Modifier Functions:
        bool _GetScriptCompleteFlag() { return m_bScriptExecutionComplete; };
		uint32 _SetScriptCompleteFlag()
		{
        	boost::mutex::scoped_lock lock(script_complete_mutex);
		    m_bScriptExecutionComplete = true;
			return 1;
		}

		uint32 _ClearScriptCompleteFlag()
		{
		    boost::mutex::scoped_lock lock(script_complete_mutex);
		    m_bScriptExecutionComplete = false;
		    return 1;
		}

        std::string m_ThreadName;
		std::string m_ScriptPath;
        std::string m_ScriptName;
        std::string m_LogFilePath;
        FILE * m_pLogFile;
        bool m_UseThreading;
        uint32 m_MyScriptAccessCode;
        bool m_bLogFileUnavailable;

        boost::shared_ptr<LuaEnvironment> m_pLuaEnvironment;
        boost::shared_ptr<boost::thread> m_pThread;

		// Thread Mutexes and Mutex-protected Flags:
		boost::mutex script_complete_mutex;
        boost::mutex script_log_output_mutex;

		// DO NOT Modify these directly, use their modifier functions even inside this class!
        // DO NOT Reference these directly either, use their Get() functions even inside this class!
		bool m_bScriptExecutionComplete;
};

#endif
