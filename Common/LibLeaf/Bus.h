#pragma once

#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>

namespace Leaf {
class Bus : public RefCounted<Bus> {
};
}