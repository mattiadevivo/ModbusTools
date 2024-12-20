/*
    Modbus Tools

    Created: 2023
    Author: Serhii Marchuk, https://github.com/serhmarch

    Copyright (C) 2023  Serhii Marchuk

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "server.h"

#include <iostream>

#include <QDir>
#include <QFileInfoList>
#include <QEventLoop>

#include <project/server_project.h>
#include <project/server_builder.h>
#include <project/server_port.h>
#include <project/server_deviceref.h>

#include <gui/server_ui.h>

#include <runtime/server_runtime.h>

mbServer::Strings::Strings() : mbCore::Strings(),
    GUID(QStringLiteral("bcde38bb-2402-4b3f-9ddb-3abfd0986852")), // generated by https://www.guidgenerator.com/online-guid-generator.aspx
    settings_application(QStringLiteral("Server")),
    default_server(settings_application),
    settings_scriptEnable (QStringLiteral("Script.Enable")),
    settings_scriptManual (QStringLiteral("Script.Manual")),
    settings_scriptDefault(QStringLiteral("Script.Default"))
{
}

const mbServer::Strings &mbServer::Strings::instance()
{
    static const Strings s;
    return s;
}

QStringList mbServer::findPythonExecutables()
{
    QStringList pythonPaths;

#if defined(Q_OS_WIN)
    // Typical locations for Python on Windows
    QStringList commonLocations =
    {
        "C:/",
        "C:/Program Files/",
        "C:/Program Files (x86)/",
        "C:/Users/" + mb::currentUser() + "/AppData/Local/Programs/"
    };

    Q_FOREACH (const QString &location, commonLocations)
    {
        QDir dir(location);
        if (dir.exists())
        {
            // List all directories or files starting with "Python"
            QFileInfoList entries = dir.entryInfoList(QStringList() << "Python*", QDir::Dirs | QDir::Files);
            Q_FOREACH (const QFileInfo &entry, entries)
            {
                if (entry.isDir())
                {
                    // Look for Python executables in subdirectories
                    QDir subDir(entry.absoluteFilePath());
                    QFileInfoList pythonFiles = subDir.entryInfoList(QStringList() << "python*.exe", QDir::Files);
                    Q_FOREACH (const QFileInfo &pythonFile, pythonFiles)
                    {
                        if (pythonFile.isExecutable())
                        {
                            pythonPaths.append(pythonFile.absoluteFilePath());
                        }
                    }
                }
                else if (entry.isExecutable() && entry.fileName().startsWith("python"))
                {
                    pythonPaths.append(entry.absoluteFilePath());
                }
            }
        }
    }

#elif defined(Q_OS_LINUX)
    // Typical locations for Python on Linux
    QStringList commonLocations =
    {
        "/usr/bin",
        "/usr/local/bin",
        "/bin",
        "/opt/bin"
    };

    Q_FOREACH (const QString &location, commonLocations)
    {
        QDir dir(location);
        if (dir.exists())
        {
            // List all executables starting with "python" (case-sensitive)
            QFileInfoList pythonFiles = dir.entryInfoList(QStringList() << "python*", QDir::Files);
            Q_FOREACH (const QFileInfo &pythonFile, pythonFiles)
            {
                if (pythonFile.isExecutable())
                {
                    pythonPaths.append(pythonFile.absoluteFilePath());
                }
            }
        }
    }
#endif

    // Ensure no duplicates
    pythonPaths.removeDuplicates();
    return pythonPaths;}

mbServer::mbServer() :
    mbCore(Strings::instance().settings_application)
{
    //Strings s = Strings::instance();
    m_scriptEnable = true;
    m_autoDetectedExec = findPythonExecutables();
}

mbServer::~mbServer()
{
}

MBSETTINGS mbServer::cachedSettings() const
{
    const Strings &s = Strings::instance();
    MBSETTINGS r = mbCore::cachedSettings();
    r[s.settings_scriptEnable ] = scriptEnable();
    r[s.settings_scriptManual ] = scriptManualExecutables();
    r[s.settings_scriptDefault] = scriptDefaultExecutable();
    return r;
}

void mbServer::setCachedSettings(const MBSETTINGS &settings)
{
    const Strings &s = Strings();
    mbCore::setCachedSettings(settings);

    MBSETTINGS::const_iterator it;
    MBSETTINGS::const_iterator end = settings.end();

    it = settings.find(s.settings_scriptEnable ); if (it != end) setScriptEnable           (it.value().toBool      ());
    it = settings.find(s.settings_scriptManual ); if (it != end) scriptSetManualExecutables(it.value().toStringList());
    it = settings.find(s.settings_scriptDefault); if (it != end) scriptSetDefaultExecutable(it.value().toString    ());
}

QString mbServer::scriptDefaultExecutable() const
{
    if (m_defaultExec.isEmpty())
    {
        if (m_manualExec.count())
            m_defaultExec = m_manualExec.first();
        else if (m_autoDetectedExec.count())
            m_defaultExec = m_autoDetectedExec.first();
    }
    return m_defaultExec;
}

void mbServer::scriptSetDefaultExecutable(const QString exec)
{
    if (!m_autoDetectedExec.contains(exec) && !m_manualExec.contains(exec))
        scriptAddExecutable(exec);
    m_defaultExec = exec;
}

QString mbServer::createGUID()
{
    return Strings::instance().GUID;
}

mbCoreUi *mbServer::createUi()
{
    return new mbServerUi(this);
}

mbCoreProject *mbServer::createProject()
{
    mbServerProject *p = new mbServerProject;
    mbServerDevice *d = new mbServerDevice;
    p->deviceAdd(d);

    mbServerDeviceRef *ref = new mbServerDeviceRef(d);
    mbServerPort *port = new mbServerPort;
    port->deviceAdd(ref);
    p->portAdd(port);
    return p;
}

mbCoreBuilder *mbServer::createBuilder()
{
    return new mbServerBuilder;
}

mbCoreRuntime *mbServer::createRuntime()
{
    return new mbServerRuntime(this);
}

