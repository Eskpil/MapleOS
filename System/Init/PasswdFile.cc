#include <AK/DeprecatedString.h>
#include <AK/NeverDestroyed.h>
#include <AK/StringBuilder.h>
#include <AK/WeakPtr.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "PasswdFile.h"

static NeverDestroyed<WeakPtr<PasswdFile>> s_the;

ErrorOr<OwnPtr<PasswdFile>> PasswdFile::open_or_create()
{
    struct stat st {
        0
    };

    if (stat("/etc/passwd", &st) == -1) {
        int fd = open("/etc/passwd", O_RDWR | O_CREAT, 0644);

        if (fd != -1) {
            close(fd);
        }
    }

    FILE* f = fopen("/etc/passwd", "r");
    if (f == nullptr) {
        return Error::from_errno(errno);
    }

    return adopt_own(*new PasswdFile(f));
}

PasswdFile* PasswdFile::the()
{
    return *s_the;
}

PasswdFile::PasswdFile(FILE* file)
{
    VERIFY(!*s_the);
    m_inner = file;

    struct stat statbuf = {};
    auto fd = fileno(file);
    if (fstat(fd, &statbuf) < 0) {
        outln("{}", Error::from_errno(errno));
        return;
    }

    u8* data = (u8*)malloc(statbuf.st_size);
    auto nread = fread(data, 1, statbuf.st_size, file);
    if (statbuf.st_size != nread) {
        outln("{}", Error::from_errno(errno));
        return;
    }

    auto contents = StringView(data, statbuf.st_size);

    for (auto line : contents.lines()) {
        PasswdFile::User user;
        auto parts = line.split_view(":"sv);

        user.username = MUST(String::from_utf8(parts.at(0)));
        user.password = MUST(String::from_utf8(parts.at(1)));
        user.id = parts.at(2).to_int().value_or(0);
        user.group_id = parts.at(3).to_int().value_or(0);
        user.gecos = MUST(String::from_utf8(parts.at(4)));
        user.home_directory = MUST(String::from_utf8(parts.at(5)));
        user.shell = MUST(String::from_utf8(parts.at(6)));

        MUST(m_users.try_append(user));
    }

    fclose(m_inner);
    *s_the = *this;
}

ErrorOr<bool> PasswdFile::add_user(PasswdFile::User const& user)
{
    m_inner = fopen("/etc/passwd", "w");

    StringBuilder builder;

    builder.append(user.username.bytes_as_string_view());
    builder.append(':');

    builder.append(user.password.bytes_as_string_view());
    builder.append(':');

    auto id_string = MUST(String::formatted("{}", user.id));
    builder.append(id_string);
    builder.append(':');

    auto group_id_string = MUST(String::formatted("{}", user.group_id));
    builder.append(group_id_string);
    builder.append(':');

    builder.append(user.gecos);
    builder.append(':');

    builder.append(user.home_directory);
    builder.append(':');

    builder.append(user.shell);
    builder.append('\n');

    outln("Adding user: {}", builder.build());

    if (fseek(m_inner, 0L, SEEK_END) == -1) {
        return Error::from_errno(errno);
    }

    if (fprintf(m_inner, "%s", builder.build().characters()) == -1) {
        return Error::from_errno(errno);
    }

    if (fflush(m_inner) == -1) {
        return Error::from_errno(errno);
    }

    fclose(m_inner);

    return true;
}

int PasswdFile::avaliable_system_id()
{
    int attempt = 100;
    for (auto const& user : users()) {
        if (user.id >= 0 && 99 >= user.id)
            continue;
        if (user.id >= 999 && 65535 >= user.id)
            continue;
        if (user.id == attempt)
            ++attempt;
    }

    return attempt;
}

PasswdFile::~PasswdFile()
{
    // fclose(m_inner);
}
