//
// process_utils.h
//
// Start another program without waiting for it (e.g. the editor opening the
// project launcher, or the launcher opening the editor).
//

#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include <vector>

#include "core/utils/sstring.h"

struct SProcessUtils
{
    // Starts `executable` with `args` in `workingDirectory` (empty = inherit)
    // and returns immediately. Arguments are quoted as needed. Returns false
    // if the executable does not exist or could not be started.
    static bool launchDetached(const SString& executable,
                               const std::vector<SString>& args = {},
                               const SString& workingDirectory = {});
};

#endif // PROCESS_UTILS_H
