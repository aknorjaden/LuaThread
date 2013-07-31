
#include <stdio.h>
#include <string>
#include "LuaThread.h"

LuaThread::LuaThread(std::string threadName, std::string scriptPath, std::string logFilePath, bool useThreading)
{
    m_ThreadName = threadName;
    m_ScriptPath = scriptPath;
    m_LogFilePath = logFilePath;
    m_UseThreading = useThreading;
    m_pLogFile = NULL;
    m_bLogFileUnavailable = false;

    // Create LuaEnvironment object directly in this thread only if NOT using threading:
    if( !(m_UseThreading) )
    {
        m_pLuaEnvironment = boost::shared_ptr<LuaEnvironment>(new LuaEnvironment(m_ThreadName,m_ScriptPath,false));
        assert( m_pLuaEnvironment.get() != NULL );
		m_pLuaEnvironment->SetThreadOwner(this);
    }

    // Initialize LuaEnvironment Object:
    m_MyScriptAccessCode = 0x12345678;
    m_pLuaEnvironment->SetScriptAccessCode(0,m_MyScriptAccessCode);
    m_pLuaEnvironment->SetSleepInterval(5000);

    // Open log file and print initial message:
    std::string logFile = m_LogFilePath;
    logFile += "/";
    logFile += m_ThreadName;
    logFile += ".log";
    m_pLogFile = fopen(logFile.c_str(), "w");
    if( m_pLogFile == NULL )
        m_bLogFileUnavailable = true;

    _LogMessage("-------------------->>> LUA THREAD LOGGING INITIATED <<<--------------------");
    _LogMessage(m_ThreadName);
    _LogMessage("----------------------------------------------------------------------------");
}

LuaThread::~LuaThread()
{
    if( m_UseThreading )
    {
        // Destroy the LuaEnvironment thread:

        //boost::this_thread::sleep(boost::posix_time::seconds(5));
        m_pThread->~thread();

        //delete m_pThread.get();
    }

    //m_pLuaEnvironment->~LuaEnvironment();

    if( m_pLogFile != NULL )
    {
        // Close the log file that is currently open:
        std::string logFileLine;
        _LogMessage("LuaThread: LOGGING SHUTTING DOWN");
        fputs( logFileLine.c_str(), m_pLogFile );
        fclose(m_pLogFile);
    }
}

int32 LuaThread::ExecuteScript(std::string scriptName)
{
    // Create a new thread into which the LuaEnvironment object is inserted and executed:
    if( m_UseThreading )
    {
        // 1. Create instance of LuaEnvironment
        // 2. Create boost::thread and pass in copy of LuaEnvironment instance and functor parameters
        // 3. Wait until we have a valid pointer to the thread-contained LuaEnvironment object instance
        // 4. Access thread and make call to LuaEnvironment to initialize the lua interpreter environment

        LuaEnvironment tempLuaEnv(m_ThreadName,m_ScriptPath,true);
        m_pThread = boost::shared_ptr<boost::thread>(new boost::thread(tempLuaEnv, this, scriptName, m_MyScriptAccessCode));

        if( m_pThread == NULL )
            return 0;
        
        // Wait for newly created thread to take pointer to us, call
        while( m_pLuaEnvironment == NULL );



        // tempLuaEnv will destruct leaving the current scope of the "if( m_UseThreading ) {...}" block
    }
    else
    {
        // Not using threading, so directly call ExecuteScript() with the script name:
        if (m_pLuaEnvironment->InitializeLuaEnvironment() <= 0)
            return 0;
        return m_pLuaEnvironment->ExecuteScript(scriptName,m_MyScriptAccessCode);
    }

    return 1;
}

int32 LuaThread::KillScript()
{
    return 0;
}

int32 LuaThread::PingScript()
{
    return 0;
}

bool LuaThread::DoesLuaVariableExist(std::string varName)
{
	return m_pLuaEnvironment->GetLua()->doesVariableExist(varName);
}

std::string LuaThread::GetString(std::string varName)
{
	if( DoesLuaVariableExist(varName) )
		return m_pLuaEnvironment->GetLua()->readVariable<std::string>(varName);
	else
		return "variable does not exist!";
}

double LuaThread::GetDouble(std::string varName)
{
	if( DoesLuaVariableExist(varName) )
	    return m_pLuaEnvironment->GetLua()->readVariable<double>(varName);
	else
		return -1.0;
}

bool LuaThread::GetBool(std::string varName)
{
	if( DoesLuaVariableExist(varName) )
	    return m_pLuaEnvironment->GetLua()->readVariable<bool>(varName);
	else
		return false;
}

int32 LuaThread::SetString(std::string varName, std::string strVal)
{
	if( DoesLuaVariableExist(varName) )
	{
		m_pLuaEnvironment->GetLua()->writeVariable(varName, std::string(strVal));
		return 0;
	}
	else
		return -1;
}

int32 LuaThread::SetDouble(std::string varName, double doubleVal)
{
	if( DoesLuaVariableExist(varName) )
	{
		m_pLuaEnvironment->GetLua()->writeVariable(varName, double(doubleVal));
	    return 0;
	}
	else
		return -1;
}

int32 LuaThread::SetBool(std::string varName, bool boolVal)
{
	if( DoesLuaVariableExist(varName) )
	{
		m_pLuaEnvironment->GetLua()->writeVariable(varName, bool(boolVal));
	    return 0;
	}
	else
		return -1;
}

int32 LuaThread::Script_ExecutionComplete(uint32 accessCode)
{
    if( accessCode == m_MyScriptAccessCode )
	{
        _SetScriptCompleteFlag();
        if( !m_UseThreading )
            // Threading not being used, so on script execution complete, decide whether to send Stop() command to LuaEnvironment:
            m_pLuaEnvironment->StopScriptProcess(m_MyScriptAccessCode);
	}
    else
        return -1;

    return 1;
}

int32 LuaThread::Script_LogMessage(std::string logMessage, uint32 accessCode)
{
    if( accessCode == m_MyScriptAccessCode )
    {
        _LogMessage(logMessage);
    }
    else
        return -1;

    return 1;
}

int32 LuaThread::Script_SetOwnedLuaEnvironment(LuaEnvironment * luaenv, uint32 accessCode)
{
    if( luaenv == NULL )
        return 0;

    m_pLuaEnvironment = boost::shared_ptr<LuaEnvironment>(luaenv);
    
    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Protected and Private Member Functions:

void LuaThread::_LogMessage(std::string logMessage)
{
    // print to log file using 'logMessage' string
	boost::mutex::scoped_lock lock(script_log_output_mutex);
    std::string logFileLine;
    logFileLine = "[2012-04-30] [HH:MM:SS] ";
    logFileLine += logMessage;
    logFileLine += "\n";
    fputs( logFileLine.c_str(), m_pLogFile );
    fflush( m_pLogFile );
}
