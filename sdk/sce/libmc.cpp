#include "libmc.h"
#include "mikupan/io/mikupan_file_c.h"
#include "mikupan/debug/mikupan_logging_c.h"
#include "typedefs.h"
#include <stddef.h>
#include <string>

int retval = 0;
int mcRunCmdNo = 0;
static std::string open_path;

static std::string CardPath(int port, const char *name)
{
    if (port == 0)
    {
        return name;
    }

    std::string path = "memory-card-2";
    if (name[0] != '/')
    {
        path += '/';
    }
    path += name;
    return path;
}

int sceMcInit()
{
    return sceMcExecFinish;
}

int sceMcSync(int mode, int *cmd, int *result)
{
    if (mcRunCmdNo == 0)
    {
        return sceMcExecIdle;
    }

    if (cmd != NULL)
    {
        *cmd = mcRunCmdNo;
    }

    mcRunCmdNo = 0;

    if (result != NULL)
    {
        *result = retval;
    }

    return sceMcExecFinish;
}

int sceMcDelete(int port, int slot, const char *name)
{
    mcRunCmdNo = SCE_MC_FUNC_DELETE;
    return sceMcExecRun;
}

int sceMcFormat(int port, int slot)
{
    mcRunCmdNo = SCE_MC_FUNC_FORMAT;
    return sceMcExecRun;
}

int sceMcMkdir(int port, int slot, const char *name)
{
    const auto path = CardPath(port, name);
    info_log("MC requested folder creation: %s", path.c_str());
    mcRunCmdNo = SCE_MC_FUNC_MKDIR;
    retval = MikuPan_CreateFolder(path.c_str()) ? sceMcResSucceed : sceMcResFailReplace;
    return sceMcExecRun;
}

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    *type = sceMcTypePS2;
    *free = 1000000;
    *format = 1;
    mcRunCmdNo = SCE_MC_FUNC_GETINFO;
    retval = sceMcResSucceed;
    return sceMcExecRun;
}

int sceMcWrite(int fd, const void *buffer, int size)
{
    info_log("MC requested file write: %s", open_path.c_str());
    mcRunCmdNo = SCE_MC_FUNC_WRITE;
    retval = MikuPan_WriteFile(open_path.c_str(), buffer, size) ? size : sceMcResFailReplace;
    return sceMcExecRun;
}

int sceMcRead(int fd, void *buffer, int size)
{
    mcRunCmdNo = SCE_MC_FUNC_READ;
    retval = MikuPan_ReadFile(open_path.c_str(), buffer, size) ? size : sceMcResFailReplace;
    return sceMcExecRun;
}

int sceMcClose(int fd)
{
    mcRunCmdNo = SCE_MC_FUNC_CLOSE;
    retval = sceMcResSucceed;
    return sceMcExecRun;
}

int sceMcOpen(int port, int slot, const char *name, int mode)
{
    open_path = CardPath(port, name);
    info_log("MC requested file: %s", open_path.c_str());
    mcRunCmdNo = SCE_MC_FUNC_OPEN;
    retval = sceMcResSucceed;
    return sceMcExecRun;
}

int sceMcGetDir(int port, int slot, const char *name, unsigned mode, int maxent,
                sceMcTblGetDir *table)
{
    if (name == NULL || *name == 0)
    {
        return sceMcErrNullStr;
    }

    const auto path = CardPath(port, name);
    info_log("MC requested directory %s", path.c_str());

    mcRunCmdNo = SCE_MC_FUNC_GETDIR;

    const int num_entries =
        MikuPan_GetListFiles(path.c_str(), (MikuPan_McTblGetDir*)table, maxent);
    retval = num_entries == 0 ? sceMcResNoEntry : num_entries;

    return sceMcExecRun;
}
