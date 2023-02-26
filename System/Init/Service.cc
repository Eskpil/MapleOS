#include <AK/Forward.h>
#include <AK/Error.h>
#include <AK/JsonParser.h>
#include <AK/JsonObject.h>
#include <AK/StringBuilder.h>
#include <AK/String.h>

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "Service.h"
#include "ServiceConfig.h"
#include "GroupFile.h"
#include "PasswdFile.h"

ErrorOr<NonnullRefPtr<Service>> Service::create(LexicalPath const& filepath) {
    FILE* file = fopen(filepath.string().characters(), "r");
    if (file == nullptr) {
        return Error::from_errno(errno );
    }

    struct stat statbuf = {};
    auto fd = fileno(file);
    if (fstat(fd, &statbuf) < 0) {
        return Error::from_errno(errno);
    }

    u8 *data = (u8 *)malloc(statbuf.st_size);
    auto nread = fread(data, 1, statbuf.st_size, file);
    if (statbuf.st_size != nread) {
        return Error::from_errno(errno);
    }

    auto parser = JsonParser(StringView(data, statbuf.st_size));
    auto value = TRY(parser.parse());

    free(data);
    fclose(file);

    auto config = new ServiceConfig(value);

    auto service = adopt_ref(*new Service(move(config)));
    return service;
}

Service::Service(ServiceConfig *config) {
    m_config = config;
}

Service::~Service() {
    delete m_config;
}

ErrorOr<bool> Service::start() {
    outln("Starting service: {}", m_config->name());

    struct stat st {0};
    if (stat(bin().to_deprecated_string().characters(), &st) == -1) {
        return Error::from_string_view(MUST(String::formatted("{}: Path {} does not exist. Skipping.", name(), bin())));
    }

    int group_id = 0;
    int user_id = 0;

    bool has_group;
    for (auto const& g : GroupFile::the()->groups()) {
        if (g.name == group()) {
            group_id = g.id;
            has_group = true;
        }
    }

    if (!has_group) {
        GroupFile::Group new_group {
            .name = group(),
            .password = MUST(String::from_utf8("x"sv)),
            .id = GroupFile::the()->avaliable_system_id(),
        };
        group_id = new_group.id;
        TRY(GroupFile::the()->add_group(new_group));
    }

    bool has_user;
    for (auto const& u : PasswdFile::the()->users()) {
        if (u.username == user()) {
            has_user = true;
        }
    }

    if (!has_user) {
        PasswdFile::User new_user {
            .username = user(),
            .password = MUST(String::from_utf8("x"sv)),
            .id = PasswdFile::the()->avaliable_system_id(),
            .group_id = group_id,
            .gecos = user(),
            .home_directory = MUST(String::formatted("/var/init/{}", name())),
            .shell = MUST(String::formatted("/sbin/nologin")),
        };
        user_id = new_user.id;
        TRY(PasswdFile::the()->add_user(new_user));
    }

    pid_t pid = fork();
    if (pid < 0) {
        // fork failed
        return Error::from_errno(errno);
    } else if (pid == 0) {
        // we are the child (success)
        // TODO: Implement priority with the <sched.h> module.

        user_id = 0;
        group_id = 0;

        if (setgid(group_id) == -1) {
            return Error::from_errno(errno);
        }
        if (setuid(user_id) == -1) {
            return Error::from_errno(errno);
        }

        for (auto const& var : environment()) {
            putenv(const_cast<char *>(var.to_deprecated_string().characters()));
        }

        char *argv[arguments().size() + 2];
        argv[0] = const_cast<char*>(bin().to_deprecated_string().characters());
        for (size_t i = 0; i < arguments().size(); i++)
            argv[i + 1] = const_cast<char*>(arguments()[i].to_deprecated_string().characters());
        argv[arguments().size() + 1] = nullptr;

        if (execv(argv[0], argv) == -1) {
            outln("Failed to execv: {}: {}", bin(), strerror(errno));
            VERIFY_NOT_REACHED();
        }
    } else {
        // we are the parent (success)
    }

    return true;
}
