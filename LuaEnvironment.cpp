
#include <iostream>
#include <fstream>
#include <string>
#include "Windows.h"
#include "LuaEnvironment.h"

LuaEnvironment::LuaEnvironment(std::string threadName, std::string scriptPath, bool bThreadingEnabled)
{
    m_bThreadingEnabled = bThreadingEnabled;
    m_bTerminateThreadProcess = false;
    m_pMyLuaThread = NULL;
    m_MyScriptAccessCode = 0;
	m_SleepIntervalMilliSeconds = 1000;
    m_ThreadName = threadName;
    m_ScriptPath = scriptPath;
    m_pLua = NULL;
    m_ScriptState = STATE_IDLE;
    m_p_terminate_mutex = NULL;
    m_p_repeat_mutex = NULL;
    m_p_execute_mutex = NULL;
    m_p_stop_mutex = NULL;
    m_bThreadProcessActive = false;

    m_bTerminateThreadFlag = false;
    m_bRepeatScriptRunsFlag = false;
    m_bExecuteScriptFlag = false;
    m_bStopScriptRunsFlag = false;

    m_bInitialized = false;

    std::cout << "LuaEnvironment CONSTRUCTOR called!" << std::endl;
    return;
}

LuaEnvironment::~LuaEnvironment()
{
    std::cout << "LuaEnvironment DESTRUCTOR called!" << std::endl;
}

void LuaEnvironment::operator()(LuaThread * pMyLuaThread, std::string scriptName, uint32 accessCode)
{
	// This gets called when boost::thread is used to create a thread with a copy of this object:
    std::cout << "LuaEnvironment class functor called, starting Thread Process!" << std::endl;
    if( m_bThreadProcessActive )
    {
        std::cout << "LuaEnvironment class _ThreadProcess() already Active!  You cannot call the LuaEnvironment class functor TWICE!" << std::endl;
        return;
    }

    m_pMyLuaThread = pMyLuaThread;
    m_CurrentScriptRunning = m_ScriptPath + scriptName;
    SetScriptAccessCode(0, accessCode);

    // Send back to owning LuaThread object instance our pointer:
    m_pMyLuaThread->Script_SetOwnedLuaEnvironment(this,m_MyScriptAccessCode);

    InitializeLuaEnvironment();

    _ThreadProcess();
}

void LuaEnvironment::SetThreadOwner(LuaThread * pMyLuaThread)
{
    m_pMyLuaThread = pMyLuaThread;
}

void LuaEnvironment::SetScriptAccessCode(uint32 currentAccessCode, uint32 accessCode)
{
    if( currentAccessCode == m_MyScriptAccessCode )
        m_MyScriptAccessCode = accessCode;
}

int32 LuaEnvironment::InitializeLuaEnvironment()
{
    if( m_bInitialized )
    {
        std::cout << "LuaEnvironment::InitializeLuaEnvironment(): Environment already Initialized!  You cannot call this more than once!" << std::endl;
        return 0;
    }

    std::cout << "LuaEnvironment::InitializeLuaEnvironment(): Initializing Lua Environment object..." << std::endl;

    // Create new object instances for critical objects that CANNOT be created in the constructor in the case that
    // this class in instantiated then copied into a thread.  This function MUST either be called inside this class'
    // functor, LuaEnvironment::operator()(), or explicitly by the owner object, LuaThread.
    m_pLua = new Lua::LuaContext();
    m_bTerminateThreadFlag = false;
    m_bRepeatScriptRunsFlag = false;
    m_bExecuteScriptFlag = false;
    m_bStopScriptRunsFlag = false;
    m_bTerminateThreadProcess = false;

    m_p_terminate_mutex = new boost::mutex();
    m_p_repeat_mutex = new boost::mutex();
    m_p_execute_mutex = new boost::mutex();
    m_p_stop_mutex = new boost::mutex();

    if( m_pLua == NULL )
    {
        std::cout << "LuaEnvironment::InitializeLuaEnvironment(): ERROR: Could not successfully create LuaContext object instance." << std::endl;
        return 0;
    }

    if( m_pMyLuaThread == NULL )
    {
        std::cout << "LuaEnvironment::InitializeLuaEnvironment(): ERROR: You MUST call SetThreadOwner() to set the pointer to the LuaThread class object!" << std::endl;
        return 0;
    }

    m_bInitialized = true;

    return 1;
}

int32 LuaEnvironment::SetSleepInterval(uint32 sleepIntervalMilliSeconds)
{
	m_SleepIntervalMilliSeconds = sleepIntervalMilliSeconds;
	return 1;
}

void LuaEnvironment::KillThread()
{
    m_bTerminateThreadProcess = true;
}

int32 LuaEnvironment::ExecuteScript(std::string scriptName, uint32 accessCode)
{
    int32 check = 0;

    if( accessCode == m_MyScriptAccessCode )
    {
        m_CurrentScriptRunning = m_ScriptPath + "/";
        m_CurrentScriptRunning += scriptName;
        std::cout << "LuaEnvironment::ExecuteScript(): Attempting to Execute Script '" << scriptName << "'..." << std::endl;

        if( ((check = _CheckInitializedState()) <= 0) )
        {
            std::cout << "LuaEnvironment::ExecuteScript(): THREAD[" << m_ThreadName << "]: LuaEnvironment NOT initialized!  You MUST call InitializeLuaEnvironment() BEFORE attempting to execute a script!" << std::endl;
            return 0;
        }

        if( m_bThreadProcessActive == false )
        {
            if( !(m_bThreadingEnabled) )
                _SetExecuteScriptFlag();    // Set Execute Script flag right away when threading is disabled
                                            // because we want the script to execute immediately so that it will
                                            // return, notify LuaThread that it completed, then LuaThread will notify
                                            // LuaEnvironment to stop script execution.  This is the flow for non-threading.
            m_bTerminateThreadProcess = false;
            _ThreadProcess();
        }
        else
            std::cout << "LuaEnvironment::ExecuteScript(): ERROR: Cannot execute script, _ThreadProcess() already Active!" << std::endl;
    }
    else
        return 0;

    return 1;
}

int32 LuaEnvironment::RunScriptProcess(uint32 accessCode)
{
    if( accessCode == m_MyScriptAccessCode )
    {
        _SetExecuteScriptFlag();
    }
    else
        return 0;

    return 1;
}

int32 LuaEnvironment::StopScriptProcess(uint32 accessCode)
{
    if( accessCode == m_MyScriptAccessCode )
    {
        _SetStopScriptRunsFlag();
        if( !(m_bThreadingEnabled) )
            m_bTerminateThreadProcess = true;
    }
    else
        return 0;

    return 1;
}

int32 LuaEnvironment::TerminateThread(uint32 accessCode)
{
    if( accessCode == m_MyScriptAccessCode )
    {
        _SetTerminateThreadFlag();
    }
    else
        return 0;

    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Protected and Private Member Functions:

int32 LuaEnvironment::_CheckInitializedState()
{
    // Check ALL class members critical for operation to ensure they are properly initialized:

    if( m_pMyLuaThread == NULL )
        return -1;

//    if( m_pLua == NULL )
//        return -2;

    if( m_p_stop_mutex == NULL )
        return -3;

    if( m_p_execute_mutex == NULL)
        return -4;

    if( m_p_terminate_mutex == NULL)
        return -5;

    if( m_p_repeat_mutex == NULL)
        return -6;

    // All checks PASSED, return true
    return 1;
}

void LuaEnvironment::_ThreadProcess()
{
    m_bThreadProcessActive = true;
    std::ifstream * scriptFileStream;
    scriptFileStream = new std::ifstream(m_CurrentScriptRunning.c_str(), std::ifstream::in);

    // If the file stream failed to load the script file, we have a major problem
    // so terminate the thread process:
    if( scriptFileStream->fail() )
    {
        std::cout << m_ThreadName.c_str() << " ERROR: Failed to open script " << m_CurrentScriptRunning.c_str() << std::endl;
        _SetTerminateThreadFlag();
    }

	std::cout << m_ThreadName.c_str() << " STARTING UP..." << std::endl;
    while( (!m_bTerminateThreadProcess) && (!_GetTerminateThreadFlag()) )
    {
        // Check Flags and change state accordingly:
        if( _GetExecuteScriptFlag() )
            m_ScriptState = STATE_RUN;
        if( _GetRepeatScriptRunsFlag() )
            m_ScriptState = STATE_REPEAT;
        if( _GetStopScriptRunsFlag() )
            m_ScriptState = STATE_IDLE;

        // Actions taken by State:
        switch (m_ScriptState)
        {
            case STATE_IDLE:
                std::cout << "LuaEnvironment::ThreadProcess(): Executing IDLE state" << std::endl;
                _ClearRepeatScriptRunsFlag();
                _ClearExecuteScriptFlag();
                _ClearStopScriptRunsFlag();
                break;

            case STATE_RUN:
                std::cout << "LuaEnvironment::ThreadProcess(): Executing RUN state" << std::endl;
                _ClearExecuteScriptFlag();
                std::cout << "LuaEnvironment::ThreadProcess(): EXECUTING Lua script..." << std::endl;
                m_pLua->executeCode(*scriptFileStream);		// Alternative:  m_pLua->executeCode(std::ifstream(m_CurrentScriptRunning.c_str()));
                _Owner_ScriptCompleteNotify();
                break;

            case STATE_REPEAT:
                std::cout << "LuaEnvironment::ThreadProcess(): Executing REPEAT state" << std::endl;
                std::cout << "LuaEnvironment::ThreadProcess(): EXECUTING Lua script w/ REPEAT..." << std::endl;
                m_pLua->executeCode(*scriptFileStream);		// Alternative:  m_pLua->executeCode(std::ifstream(m_CurrentScriptRunning.c_str()));
                _Owner_ScriptCompleteNotify();
                break;

            default:
                break;
        }

        // Sleep in this thread for programmed time ONLY if threading is enabled:
        if( m_bThreadingEnabled )
        {
            std::cout << "LuaEnvironment::ThreadProcess(): Thread Process going to sleep..." << std::endl;
            boost::this_thread::sleep(boost::posix_time::milliseconds(m_SleepIntervalMilliSeconds));
            std::cout << "LuaEnvironment::ThreadProcess(): Thread Process has reawakened!" << std::endl;
        }
    }
    std::cout << m_ThreadName.c_str() << " FINISHED!" << std::endl;

    m_bThreadProcessActive = false;
}

int32 LuaEnvironment::_Owner_LogMessage(std::string logMessage)
{
    return m_pMyLuaThread->Script_LogMessage(logMessage, m_MyScriptAccessCode);
}

int32 LuaEnvironment::_Owner_ScriptCompleteNotify()
{
    return m_pMyLuaThread->Script_ExecutionComplete(m_MyScriptAccessCode);
}
