
#include <stdio.h>
#include <string>
#include "EVEmu_Types.h"
#include "luawrapper\LuaContext.h"
#include "..\common\boost\boost\thread\thread.hpp"
#include "..\common\boost\boost\thread\mutex.hpp"
#include "..\common\boost\boost\thread\locks.hpp"
#include "..\common\boost\boost\type_traits.hpp"

#pragma once
#include "LuaThread.h"

#ifndef LUAENVIRONMENT_H
#define LUAENVIRONMENT_H

///////////////////////////////////////////////////////////////////////////////////////////////////
// USE Cases:
//
// LuaEnvironment existing in LuaThread's thread:
// ----------------------------------------------
// 1) First, create pointer and object to a new LuaEnvironment via new LuaEnvironment(...)
// 2) Call LuaEnvironment::SetThreadOwner() passing in the 'this' pointer of the owner object (LuaThread)
// 3) Call LuaEnvironment::SetScriptAccessCode(0,x) where 'x' is your customized Access Code,
//    like a password, ensuring only the owner object will have control over this LuaEnvironment object instance.
// 4) Call LuaEnvironment::SetSleepInterval() passing in the number of milliseconds that the LuaEnvironment
//    object instance's _ThreadProcess() function's loop will go inactive (sleep) between each attempt to
//    execute the assigned lua script.
// 5) Call LuaEnvironment::InitializeLuaEnvironment() to initialize critical objects that cannot be initialized
//    during the LuaEnvironment class constructor.
// 6) You may now make the call to LuaEnvironment::ExecuteScript() passing in the scriptName and the new Access code
// *** Alternatively, you may use the "functor" call, the LuaEnvironment::operator()() function, passing in the
//     'this' pointer for the owning LuaThread, the script name, and the access code, but this function is meant
//     to be used ONLY where LuaEnvironment is used inside its OWN thread.  Where no threading is required,
//     use ExecuteScript() instead.
//
//
// LuaEnvironment existing in its OWN thread:
// ------------------------------------------
// 1) First, create pointer and object to a new LuaEnvironment via a thread mechanism, such as boost::thread like this:
//    boost::thread * pThread = new boost::thread(tempLuaEnv, this, scriptName, scriptAccessCode);
//        where 'tempLuaEnv' is an already created LuaEnvironment object created like this:  LuaEnvironment tempLuaEnv(threadName,scriptPath);
//              'this' is the pointer to the owning LuaThread object instance
// 2) 
//
///////////////////////////////////////////////////////////////////////////////////////////////////


class LuaThread;

class LuaEnvironment
{
    public:
        LuaEnvironment(std::string threadName, std::string scriptPath, bool bThreadingEnabled);
        ~LuaEnvironment();

		void operator()(LuaThread * pMyLuaThread, std::string scriptName, uint32 accessCode);

		// Thread Management:
        void SetThreadOwner(LuaThread * pMyLuaThread);
        void SetScriptAccessCode(uint32 currentAccessCode, uint32 accessCode = 0xFFFFFFFF);
        int32 InitializeLuaEnvironment();
		int32 SetSleepInterval(uint32 sleepIntervalMilliSeconds);
		void KillThread();

		// Thread Operations:
        int32 ExecuteScript(std::string scriptName, uint32 accessCode = 0);
        int32 RunScriptProcess(uint32 accessCode = 0);
        int32 StopScriptProcess(uint32 accessCode = 0);
        int32 TerminateThread(uint32 accessCode = 0);

        // Ideally access to the m_pLua member would be protected or private, however,
        // since it is only exposed to the layer above (LuaThread) and no further, its
        // exposure is contained whilest not having to entirely and needlssly replicating
        // the entire interface in this class.
        // Perhaps this will be done at some other time, but for now, it is exposed as public.
        Lua::LuaContext * GetLua() { return m_pLua; }

protected:
        int32 _CheckInitializedState();
		void _ThreadProcess();

        // Mutex-protected Flag Modifier Functions:
        bool _GetTerminateThreadFlag() { return m_bTerminateThreadFlag; };
		uint32 _SetTerminateThreadFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_terminate_mutex);
			m_bTerminateThreadFlag = true;
			return 1;
		}

		uint32 _ClearTerminateThreadFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_terminate_mutex);
			m_bTerminateThreadFlag = false;
			return 1;
		}

        bool _GetRepeatScriptRunsFlag() { return m_bRepeatScriptRunsFlag; };
		uint32 _SetRepeatScriptRunsFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_repeat_mutex);
			m_bRepeatScriptRunsFlag = true;
			return 1;
		}

		uint32 _ClearRepeatScriptRunsFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_repeat_mutex);
			m_bRepeatScriptRunsFlag = false;
			return 1;
		}

        bool _GetExecuteScriptFlag() { return m_bExecuteScriptFlag; };
		uint32 _SetExecuteScriptFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_execute_mutex);
			m_bExecuteScriptFlag = true;
			return 1;
		}

		uint32 _ClearExecuteScriptFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_execute_mutex);
			m_bExecuteScriptFlag = false;
			return 1;
		}

        bool _GetStopScriptRunsFlag() { return m_bStopScriptRunsFlag; };
		uint32 _SetStopScriptRunsFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_stop_mutex);
			m_bStopScriptRunsFlag = true;
			return 1;
		}

		uint32 _ClearStopScriptRunsFlag()
		{
			boost::mutex::scoped_lock lock(*m_p_stop_mutex);
			m_bStopScriptRunsFlag = false;
			return 1;
		}

        // Remote Methods - Accessed via pointer to LuaThread object:
        int32 _Owner_LogMessage(std::string logMessage);
        int32 _Owner_ScriptCompleteNotify();

        enum ScriptStates
        {
            STATE_IDLE,
            STATE_RUN,
            STATE_REPEAT
        };

        bool m_bThreadingEnabled;
        ScriptStates m_ScriptState;
        bool m_bTerminateThreadProcess;
        LuaThread * m_pMyLuaThread;
        uint32 m_MyScriptAccessCode;
        std::string m_ThreadName;
        std::string m_ScriptPath;
        std::string m_CurrentScriptRunning;
		uint32 m_SleepIntervalMilliSeconds;
        bool m_bThreadProcessActive;
        Lua::LuaContext * m_pLua;

		// Thread Mutexes and Mutex-protected Flags:
		boost::mutex * m_p_terminate_mutex;
		boost::mutex * m_p_repeat_mutex;
		boost::mutex * m_p_execute_mutex;
		boost::mutex * m_p_stop_mutex;

		// DO NOT Modify these directly, use their modifier functions even inside this class!
        // DO NOT Reference these directly either, use their Get() functions even inside this class!
		bool m_bTerminateThreadFlag;
		bool m_bRepeatScriptRunsFlag;
		bool m_bExecuteScriptFlag;
		bool m_bStopScriptRunsFlag;

    private:
        bool m_bInitialized;
};

#endif
