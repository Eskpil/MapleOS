#pragma once

#include <AK/Array.h>
#include <AK/String.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>

class Essentials : public RefCounted<Essentials> {
public:
    ErrorOr<bool> static load();
};