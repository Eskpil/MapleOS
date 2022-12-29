#include <AK/DeprecatedString.h>
#include <AK/NeverDestroyed.h>
#include <AK/OwnPtr.h>
#include <AK/StringBuilder.h>
#include <AK/WeakPtr.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "GroupFile.h"

static NeverDestroyed<WeakPtr<GroupFile>> s_the;

ErrorOr<OwnPtr<GroupFile>> GroupFile::open_or_create()
{
    struct stat st {
        0
    };

    if (stat("/etc/group", &st) == -1) {
        int fd = open("/etc/group", O_RDWR | O_CREAT, 0644);

        if (fd != -1) {
            close(fd);
        }
    }

    FILE* f = fopen("/etc/group", "r+");
    if (f == nullptr) {
        return Error::from_errno(errno);
    }

    return adopt_own(*new GroupFile(f));
}

GroupFile* GroupFile::the() { return *s_the; }

GroupFile::GroupFile(FILE* file)
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
        GroupFile::Group group {};
        auto parts = line.split_view(":"sv);

        group.name = MUST(String::from_utf8(parts.at(0)));
        group.password = MUST(String::from_utf8(parts.at(1)));
        group.id = parts.at(2).to_int().value_or(0);

        // NOTE: We could probably remove the second expression here
        // but it is better to be safe than sorry.
        if (parts.size() >= 4 && parts.at(3).length() > 0) {
            for (auto user : parts.at(3).split_view(","sv)) {
                MUST(group.users.try_append(MUST(String::from_utf8(user))));
            }
        }

        MUST(m_groups.try_append(group));
    }

    fclose(m_inner);

    *s_the = *this;
}

GroupFile::~GroupFile()
{
    //    fclose(m_inner);
}

ErrorOr<bool> GroupFile::add_group(GroupFile::Group const& group)
{
    m_inner = fopen("/etc/group", "w");

    StringBuilder builder;

    builder.append(group.name.bytes_as_string_view());
    builder.append(':');

    builder.append(group.password.bytes_as_string_view());
    builder.append(':');

    auto id_string = MUST(String::formatted("{}", group.id));
    builder.append(id_string);
    builder.append(':');

    StringBuilder users_builder;

    for (size_t i = 0; group.users.size() > i; ++i) {
        auto user = group.users.at(i);
        auto is_last = group.users.size() - 1 == i;
        users_builder.append(user);
        if (!is_last)
            users_builder.append(',');
    }

    builder.append(users_builder.build());
    builder.append('\n');

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

int GroupFile::avaliable_system_id()
{
    int attempt = 100;
    for (auto const& group : groups()) {
        if (group.id >= 0 && 99 >= group.id)
            continue;
        if (group.id >= 999 && 65535 >= group.id)
            continue;
        if (group.id == attempt)
            ++attempt;
    }

    return attempt;
}