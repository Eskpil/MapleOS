#include <AK/Error.h>
#include <AK/NonnullRefPtr.h>

#include <dirent.h>
#include <sys/stat.h>

#include "Targets.h"
#include "Target.h"
#include "Service.h"
#include "ExecutionOrder.h"

ErrorOr<NonnullRefPtr<Targets>> Targets::create() {
    return adopt_ref(*new Targets);
}

Targets::Targets() = default;


Targets::~Targets() = default;

ErrorOr<bool> Targets::append(NonnullRefPtr<Target> target)
{
    if (m_targets.contains(target->name())) {
        return Error::from_string_view(String::formatted("Target: {} has already been defined", target->name()).release_value());
    }

    auto name = target->name();
    m_targets.set(name, move(target));

    return true;
}

ErrorOr<NonnullRefPtr<ExecutionOrder>> Targets::create_execution_order() {
    return adopt_ref(*new ExecutionOrder(m_targets));
}

ErrorOr<bool> Targets::load_directory(LexicalPath const& path) {
    struct dirent* dp;
    DIR* dfd;

    if ((dfd = opendir(path.string().characters())) == NULL) {
        return Error::from_errno(errno);
    }

    auto orphan_services = Vector<NonnullRefPtr<Service>>();

    while ((dp = readdir(dfd)) != NULL) {
        struct stat stbuf;

        StringView filename(dp->d_name, strlen(dp->d_name));
        StringBuilder builder;

        builder.append(path.string());
        builder.append("/"sv);
        builder.append(filename);

        auto filepath = builder.build();

        if (stat(filepath.characters(), &stbuf) == -1) {
            return Error::from_errno(errno);
        }

        if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
            continue;
            // Skip directories
        } else {
            if (filename.ends_with(".target"sv, CaseSensitivity::CaseSensitive)) {
                auto target_or_error = Target::create(LexicalPath(filepath));
                if (target_or_error.is_error()) {
                    return target_or_error.release_error();
                }
                auto target = target_or_error.release_value();
                TRY(append(target));

                continue;
            }

            auto service_or_error = Service::create(LexicalPath(filepath));
            if (service_or_error.is_error()) {
                return service_or_error.release_error();
            }

            auto service = service_or_error.release_value();
            TRY(orphan_services.try_append(move(service)));
        }
    }

    for (auto service : orphan_services) {
        auto service_fulfills = service->fulfills();
        for (const auto& target : m_targets) {
            if (target.value->fulfills().contains_slow(service_fulfills)) {
                auto success_or_error = target.value->append(move(service));
                if (success_or_error.is_error()) {
                    return success_or_error.release_error();
                }
            }
        }
    }

    return true;
}

void Targets::dump() {
    for (auto const& batch : m_targets) {
        outln("{}:", batch.key);
        batch.value->dump("\t"sv);
    }


}
