#include <AK/Error.h>
#include <AK/LexicalPath.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "Essentials.h"

ALWAYS_INLINE ErrorOr<bool> check_or_create_directory(LexicalPath path, int permissions_octal) {
    struct stat st = {0};
    if (stat(path.string().characters(), &st) == -1) {
        if (mkdir(path.string().characters(), permissions_octal) == -1) {
            return Error::from_errno(errno);
        }
    }

    return true;
}

ErrorOr<bool> Essentials::load() {
    TRY(check_or_create_directory(LexicalPath("/usr"), 0755));
    {
        TRY(check_or_create_directory(LexicalPath("/usr/bin"), 0755));
        TRY(check_or_create_directory(LexicalPath("/usr/include"), 0755));
        TRY(check_or_create_directory(LexicalPath("/usr/share"), 0755));
    };

    TRY(check_or_create_directory(LexicalPath("/etc"), 0755));
    {
        TRY(check_or_create_directory(LexicalPath("/etc/services"), 0755));
        TRY(check_or_create_directory(LexicalPath("/etc/interfaces"), 0755));
    };

    TRY(check_or_create_directory(LexicalPath("/var"), 0755));
    {
        TRY(check_or_create_directory(LexicalPath("/var/log"), 0755));
        TRY(check_or_create_directory(LexicalPath("/var/init"), 0700));
    };

    TRY(check_or_create_directory(LexicalPath("/home"), 0755));

    return true;
}
