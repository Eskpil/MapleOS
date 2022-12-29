#include <AK/Error.h>
#include <AK/NonnullRefPtr.h>
#include <AK/LexicalPath.h>
#include <AK/JsonParser.h>

#include <sys/stat.h>

#include "Target.h"
#include "TargetConfig.h"

ErrorOr<NonnullRefPtr<Target>> Target::create(LexicalPath const& filepath) {
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

    auto config = new TargetConfig(value);

    auto target = adopt_ref(*new Target(move(config)));
    return target;
}

Target::Target(TargetConfig *config) {
    m_config = config;
}

Target::~Target() {
    delete m_config;
}

ErrorOr<bool> Target::append(NonnullRefPtr<Service> service) {
    if (m_services.contains(service->name())) {
        return Error::from_string_view(String::formatted("Service: {} has already been defined for target: {}", service->name(), name()).release_value());
    }

    auto fulfills = service->fulfills();
    m_services.set(fulfills, move(service));

    return true;
}

void Target::dump(StringView indent) {
    for (auto const& service : m_services) {
        outln("{}Name = {}", indent, service.value->name());
        outln("{}Bin = {}", indent, service.value->bin());
        outln("{}User = {}", indent, service.value->user());
        outln("{}Group = {}", indent, service.value->group());
        outln("{}Fulfills = {}", indent, service.value->fulfills());
        outln("");
    }
}
